// Mask_Prepare_PROC.cpp — flattens PARAMS + the loaded art into the MaskStratumConfiguration
// records both backends consume: degrees -> the pinned gradient unit (the only tan() calls in
// the stage, on the host, once per run), feather widths -> reciprocals, and every stratum's
// stored art packed into ONE buffer so CPU and GPU sample byte-identical input.
#include "Mask_PROC.h"
#include "Mask_Merge_PROC.h"
#include <cmath>

namespace SanmapGen {
namespace Proc {
namespace {

float SlopeDegreesToGradient(float degrees, const MaskConstants& constants) {
    if (degrees < 0.0f) degrees = 0.0f;
    if (degrees > constants.maximumSlopeDegreesLimit) degrees = constants.maximumSlopeDegreesLimit;
    return std::tan(degrees * constants.degreesToRadians);
}

// The stage-wide values every stratum needs. They are copied per record because the SYS seam
// exposes int uniforms only, so the shader reads all floats out of this block.
// The gradient's run is `Geometry::worldUnitsPerCell` — the ONE owner of cell world-size
// (ARCH §7.1, M5-0d) — so the baked slope is rise-per-world-unit on exactly the scale
// Placement emits its positions at. A non-positive setting degrades to 1 rather than dividing
// by zero (Constitution §6: validate, default, carry on).
void ConfigureSharedFields(MaskStratumConfiguration& configuration, const MaskConstants& constants,
                           float terrainMaxHeight, float worldUnitsPerCell) {
    const float cellWorldSize = worldUnitsPerCell > 0.0f ? worldUnitsPerCell : 1.0f;
    configuration.heightScale        = terrainMaxHeight;
    configuration.inverseSingleSpan  = 1.0f / cellWorldSize;
    configuration.inverseDoubleSpan  = 1.0f / (constants.centralDifferenceSpan * cellWorldSize);
    configuration.smoothstepShoulder = constants.smoothstepShoulder;
    configuration.smoothstepScale    = constants.smoothstepScale;
    configuration.maskMinimum        = constants.maskMinimum;
    configuration.maskMaximum        = constants.maskMaximum;
}

// The 7 raw gate values, read from whichever record `stratum.bSlopeUseGlobal` selects
// (MASKING_SPEC §1.7 — a config SOURCE decision at flattening time, never a kernel change).
// `Params::Stratum` and `Params::SlopeDefaults` share the same 7 field names by construction, so
// this is the only place that source selection happens.
struct SlopeGateSourceValues {
    bool  bSlopeGateEnabled;
    float minimumSlopeDegrees;
    float maximumSlopeDegrees;
    float slopeFeatherDegreesLow;
    float slopeFeatherDegreesHigh;
    bool  bUseSmoothstep;
    bool  bInvertSlopeGate;
    float slopeGateStrength;
};

SlopeGateSourceValues ResolveSlopeGateSource(const Params::Stratum& stratum,
                                             const Params::SlopeDefaults& slopeDefaults) {
    if (stratum.bSlopeUseGlobal)
        return { slopeDefaults.bSlopeGateEnabled, slopeDefaults.minimumSlopeDegrees,
                 slopeDefaults.maximumSlopeDegrees, slopeDefaults.slopeFeatherDegreesLow,
                 slopeDefaults.slopeFeatherDegreesHigh, slopeDefaults.bUseSmoothstep,
                 slopeDefaults.bInvertSlopeGate, slopeDefaults.slopeGateStrength };
    return { stratum.bSlopeGateEnabled, stratum.minimumSlopeDegrees, stratum.maximumSlopeDegrees,
             stratum.slopeFeatherDegreesLow, stratum.slopeFeatherDegreesHigh, stratum.bUseSmoothstep,
             stratum.bInvertSlopeGate, stratum.slopeGateStrength };
}

void ConfigureSlopeGate(MaskStratumConfiguration& configuration, const Params::Stratum& stratum,
                        const Params::SlopeDefaults& slopeDefaults, const MaskConstants& constants) {
    const SlopeGateSourceValues source = ResolveSlopeGateSource(stratum, slopeDefaults);
    float lowDegrees  = source.minimumSlopeDegrees;
    float highDegrees = source.maximumSlopeDegrees;
    if (highDegrees < lowDegrees) { const float swap = lowDegrees; lowDegrees = highDegrees; highDegrees = swap; }
    configuration.slopeGradientLow  = SlopeDegreesToGradient(lowDegrees, constants);
    configuration.slopeGradientHigh = SlopeDegreesToGradient(highDegrees, constants);

    const float featherLowGradient = configuration.slopeGradientLow
                                   - SlopeDegreesToGradient(lowDegrees - source.slopeFeatherDegreesLow, constants);
    const float featherHighGradient = SlopeDegreesToGradient(highDegrees + source.slopeFeatherDegreesHigh, constants)
                                    - configuration.slopeGradientHigh;
    configuration.inverseFeatherLow  = featherLowGradient  > 0.0f ? 1.0f / featherLowGradient  : 0.0f;
    configuration.inverseFeatherHigh = featherHighGradient > 0.0f ? 1.0f / featherHighGradient : 0.0f;

    configuration.bSmoothstepEnabled = source.bUseSmoothstep ? 1 : 0;
    configuration.bInvertEnabled     = source.bInvertSlopeGate ? 1 : 0;
    float strength = source.bSlopeGateEnabled ? source.slopeGateStrength : 0.0f;
    if (strength < 0.0f) strength = 0.0f;
    if (strength > 1.0f) strength = 1.0f;
    configuration.gateStrength = strength;
}

// Appends this stratum's loaded art to the packed buffer. A mode of StaticOverride with no
// usable art degrades to Disabled — replacing a weight with nothing would erase the stratum.
// `maskRemapMinimum`/`maskRemapMaximum` are per-stratum material/appearance pass-through data
// (Generator Expert ruling) — NOT a Mask-stage input — so this function never reads them.
void ConfigureStoredArt(MaskStratumConfiguration& configuration, const Params::Stratum& stratum,
                        const Data::StratumArt* art, std::vector<float>& packedValues, int vertexSize) {
    if (stratum.importedMaskMode == Params::ImportedMaskMode::Disabled) return;
    if (art == nullptr || !art->HasImportedMask()) return;   // stays kMergeModeDisabled

    configuration.mergeMode        = static_cast<int>(stratum.importedMaskMode);
    configuration.storedMaskOffset = static_cast<int>(packedValues.size());
    configuration.storedMaskWidth  = art->importedMask.Width();
    configuration.storedMaskHeight = art->importedMask.Height();
    const float vertexSpan = static_cast<float>(vertexSize > 1 ? vertexSize - 1 : 1);
    configuration.storedSampleScaleX = static_cast<float>(configuration.storedMaskWidth - 1) / vertexSpan;
    configuration.storedSampleScaleY = static_cast<float>(configuration.storedMaskHeight - 1) / vertexSpan;
    const float* values = art->importedMask.Data();
    packedValues.insert(packedValues.end(), values, values + art->importedMask.CellCount());
}

} // namespace

void MaskStage::PrepareRun() {
    const int vertexSize = geometry.VertexSize();
    stratumConfigurations.assign(Data::MapFields::stratumCount, MaskStratumConfiguration{});
    packedStoredMaskValues.clear();
    for (int index = 0; index < Data::MapFields::stratumCount; ++index) {
        MaskStratumConfiguration& configuration = stratumConfigurations[index];
        ConfigureSharedFields(configuration, constants, geometry.terrainMaxHeight,
                              geometry.worldUnitsPerCell);
        if (static_cast<std::size_t>(index) >= strata.size()) continue;   // unconfigured: stays defaulted
        const Data::StratumArt* art = static_cast<std::size_t>(index) < stratumArt.size()
                                    ? &stratumArt[index] : nullptr;
        ConfigureSlopeGate(configuration, strata[index], slopeDefaults, constants);
        ConfigureStoredArt(configuration, strata[index], art, packedStoredMaskValues, vertexSize);
    }
    if (packedStoredMaskValues.empty()) packedStoredMaskValues.push_back(0.0f);   // never a null binding

    // The stage sizes its OWN output field (§3.4.1) so a caller that sized MapFields before this
    // field existed still gets a valid slope bake instead of an out-of-range write.
    if (mapFields.IsSized() && mapFields.slope.Width() != mapFields.VertexSize())
        mapFields.slope.Resize(mapFields.VertexSize(), mapFields.VertexSize(), 0.0f);
}

} // namespace Proc
} // namespace SanmapGen
