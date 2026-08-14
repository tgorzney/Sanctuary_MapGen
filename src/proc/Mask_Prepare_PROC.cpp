// Mask_Prepare_PROC.cpp — flattens PARAMS into the MaskStratumConfiguration records both
// backends consume: degrees -> the pinned gradient unit (the only tan() calls in the stage,
// on the host, once per run), feather widths -> reciprocals, and every stratum's stored art
// packed into ONE buffer so CPU and GPU sample byte-identical input.
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

void ConfigureSlopeGate(MaskStratumConfiguration& configuration, const Params::StratumMask& stratumMask,
                        const MaskConstants& constants) {
    float lowDegrees  = stratumMask.minimumSlopeDegrees;
    float highDegrees = stratumMask.maximumSlopeDegrees;
    if (highDegrees < lowDegrees) { const float swap = lowDegrees; lowDegrees = highDegrees; highDegrees = swap; }
    configuration.slopeGradientLow  = SlopeDegreesToGradient(lowDegrees, constants);
    configuration.slopeGradientHigh = SlopeDegreesToGradient(highDegrees, constants);

    const float featherLowGradient = configuration.slopeGradientLow
                                   - SlopeDegreesToGradient(lowDegrees - stratumMask.slopeFeatherDegreesLow, constants);
    const float featherHighGradient = SlopeDegreesToGradient(highDegrees + stratumMask.slopeFeatherDegreesHigh, constants)
                                    - configuration.slopeGradientHigh;
    configuration.inverseFeatherLow  = featherLowGradient  > 0.0f ? 1.0f / featherLowGradient  : 0.0f;
    configuration.inverseFeatherHigh = featherHighGradient > 0.0f ? 1.0f / featherHighGradient : 0.0f;

    configuration.bSmoothstepEnabled = stratumMask.bUseSmoothstep ? 1 : 0;
    configuration.bInvertEnabled     = stratumMask.bInvertSlopeGate ? 1 : 0;
    float strength = stratumMask.bSlopeGateEnabled ? stratumMask.slopeGateStrength : 0.0f;
    if (strength < 0.0f) strength = 0.0f;
    if (strength > 1.0f) strength = 1.0f;
    configuration.gateStrength = strength;
}

// Appends this stratum's art to the packed buffer. A mode of StaticOverride with no usable art
// degrades to Disabled — replacing a mask with nothing would silently erase the stratum.
void ConfigureStoredMask(MaskStratumConfiguration& configuration, const Params::StratumMask& stratumMask,
                         std::vector<float>& packedValues, int vertexSize) {
    const float remapRange = stratumMask.maskRemapMaximum - stratumMask.maskRemapMinimum;
    configuration.remapMinimum      = stratumMask.maskRemapMinimum;
    configuration.inverseRemapRange = remapRange > 0.0f ? 1.0f / remapRange : 0.0f;
    if (!stratumMask.HasStoredMask()) { configuration.mergeMode = kMergeModeDisabled; return; }

    configuration.mergeMode        = static_cast<int>(stratumMask.importedMaskMode);
    configuration.storedMaskOffset = static_cast<int>(packedValues.size());
    configuration.storedMaskWidth  = stratumMask.importedMaskWidth;
    configuration.storedMaskHeight = stratumMask.importedMaskHeight;
    const float vertexSpan = static_cast<float>(vertexSize > 1 ? vertexSize - 1 : 1);
    configuration.storedSampleScaleX = static_cast<float>(stratumMask.importedMaskWidth - 1) / vertexSpan;
    configuration.storedSampleScaleY = static_cast<float>(stratumMask.importedMaskHeight - 1) / vertexSpan;
    const std::size_t used = static_cast<std::size_t>(stratumMask.importedMaskWidth) * stratumMask.importedMaskHeight;
    packedValues.insert(packedValues.end(), stratumMask.importedMaskData.begin(),
                        stratumMask.importedMaskData.begin() + static_cast<std::ptrdiff_t>(used));
}

} // namespace

void MaskStage::PrepareRun() {
    const int vertexSize = geometry.VertexSize();
    stratumConfigurations.assign(Data::MapFields::stratumCount, MaskStratumConfiguration{});
    packedStoredMaskValues.clear();
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum) {
        MaskStratumConfiguration& configuration = stratumConfigurations[stratum];
        ConfigureSharedFields(configuration, constants, geometry.terrainMaxHeight);
        if (static_cast<std::size_t>(stratum) >= stratumMasks.size()) {
            configuration.inverseRemapRange = 1.0f;   // untouched stratum: pass the mask through
            continue;
        }
        const Params::StratumMask& stratumMask = stratumMasks[stratum];
        ConfigureSlopeGate(configuration, stratumMask, constants);
        ConfigureStoredMask(configuration, stratumMask, packedStoredMaskValues, vertexSize);
    }
    if (packedStoredMaskValues.empty()) packedStoredMaskValues.push_back(0.0f);   // never a null binding
}

} // namespace Proc
} // namespace SanmapGen
