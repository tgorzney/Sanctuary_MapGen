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
void ConfigureSharedFields(MaskStratumConfiguration& configuration, const MaskConstants& constants,
                           float terrainMaxHeight) {
    const float cellSize = constants.cellSize > 0.0f ? constants.cellSize : 1.0f;
    configuration.heightScale        = terrainMaxHeight;
    configuration.inverseSingleSpan  = 1.0f / cellSize;
    configuration.inverseDoubleSpan  = 1.0f / (constants.centralDifferenceSpan * cellSize);
    configuration.smoothstepShoulder = constants.smoothstepShoulder;
    configuration.smoothstepScale    = constants.smoothstepScale;
    configuration.maskMinimum        = constants.maskMinimum;
    configuration.maskMaximum        = constants.maskMaximum;
}

void ConfigureSlopeGate(MaskStratumConfiguration& configuration, const Params::Stratum& stratum,
                        const MaskConstants& constants) {
    float lowDegrees  = stratum.minimumSlopeDegrees;
    float highDegrees = stratum.maximumSlopeDegrees;
    if (highDegrees < lowDegrees) { const float swap = lowDegrees; lowDegrees = highDegrees; highDegrees = swap; }
    configuration.slopeGradientLow  = SlopeDegreesToGradient(lowDegrees, constants);
    configuration.slopeGradientHigh = SlopeDegreesToGradient(highDegrees, constants);

    const float featherLowGradient = configuration.slopeGradientLow
                                   - SlopeDegreesToGradient(lowDegrees - stratum.slopeFeatherDegreesLow, constants);
    const float featherHighGradient = SlopeDegreesToGradient(highDegrees + stratum.slopeFeatherDegreesHigh, constants)
                                    - configuration.slopeGradientHigh;
    configuration.inverseFeatherLow  = featherLowGradient  > 0.0f ? 1.0f / featherLowGradient  : 0.0f;
    configuration.inverseFeatherHigh = featherHighGradient > 0.0f ? 1.0f / featherHighGradient : 0.0f;

    configuration.bSmoothstepEnabled = stratum.bUseSmoothstep ? 1 : 0;
    configuration.bInvertEnabled     = stratum.bInvertSlopeGate ? 1 : 0;
    float strength = stratum.bSlopeGateEnabled ? stratum.slopeGateStrength : 0.0f;
    if (strength < 0.0f) strength = 0.0f;
    if (strength > 1.0f) strength = 1.0f;
    configuration.gateStrength = strength;
}

// Appends this stratum's loaded art to the packed buffer. A mode of StaticOverride with no
// usable art degrades to Disabled — replacing a weight with nothing would erase the stratum.
void ConfigureStoredArt(MaskStratumConfiguration& configuration, const Params::Stratum& stratum,
                        const Data::StratumArt* art, std::vector<float>& packedValues, int vertexSize) {
    const float remapRange = stratum.maskRemapMaximum - stratum.maskRemapMinimum;
    configuration.remapMinimum      = stratum.maskRemapMinimum;
    configuration.inverseRemapRange = remapRange > 0.0f ? 1.0f / remapRange : 0.0f;
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
        ConfigureSharedFields(configuration, constants, geometry.terrainMaxHeight);
        if (static_cast<std::size_t>(index) >= strata.size()) {
            configuration.inverseRemapRange = 1.0f;   // unconfigured stratum: pass the weight through
            continue;
        }
        const Data::StratumArt* art = static_cast<std::size_t>(index) < stratumArt.size()
                                    ? &stratumArt[index] : nullptr;
        ConfigureSlopeGate(configuration, strata[index], constants);
        ConfigureStoredArt(configuration, strata[index], art, packedStoredMaskValues, vertexSize);
    }
    if (packedStoredMaskValues.empty()) packedStoredMaskValues.push_back(0.0f);   // never a null binding
}

} // namespace Proc
} // namespace SanmapGen
