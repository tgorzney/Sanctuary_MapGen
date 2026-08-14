// NoiseBlend_Shape_PROC.h — CPU half of the shaping math: density/levels reshape and the
// height-blend combine. Layer: PROC. Paired one-for-one with NoiseBlend_Shape_PROC.glsl —
// same expressions, same order, so the only CPU/GPU difference is float evaluation, never
// the algorithm (Constitution §4, DISPATCH_INTERFACE_SPEC §1). Header-only: the GPU twin is
// the .glsl, so there is no second .cpp to keep in step.
#pragma once
#include <cmath>
#include "NoiseBlend_Kernel_PROC.h"

namespace SanmapGen {
namespace Proc {

inline float ClampUnitRange(float value) {
    return value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
}

// Density shaping then Photoshop-style Levels — NOISE_BLEND_SPEC's post-noise reshape.
inline float ReshapeLayerValue(float rawValue, const LayerKernelConfiguration& configuration) {
    float shaped = rawValue * configuration.landDensityMultiplier;
    const float originalShaped = shaped;
    if (configuration.mountainDensity > 0.0f) {
        const float mountain = configuration.mountainDensity;
        const float smoothed = shaped * shaped * (3.0f - 2.0f * shaped);
        shaped = shaped * (1.0f - mountain) + smoothed * mountain;
        if (shaped > 0.5f) shaped += (shaped - 0.5f) * mountain;
        shaped = ClampUnitRange(shaped);
    }
    if (configuration.plateauDensity > 0.0f)
        shaped = std::floor(shaped * configuration.terraceHeightReciprocal) * configuration.terraceHeight;
    if (configuration.rampDensity > 0.0f)
        shaped = shaped * (1.0f - configuration.rampDensity) + originalShaped * configuration.rampDensity;

    if (configuration.levelsRangeReciprocal > 0.0f)
        shaped = ClampUnitRange((shaped - configuration.levelsShadows) * configuration.levelsRangeReciprocal);
    else
        shaped = shaped >= configuration.levelsShadows ? 1.0f : 0.0f;
    if (configuration.levelsMidtones != 1.0f && configuration.levelsMidtones > 0.0f)
        shaped = std::pow(shaped, configuration.levelsMidtones);
    shaped = configuration.levelsOutputBlack
           + shaped * (configuration.levelsOutputWhite - configuration.levelsOutputBlack);
    return ClampUnitRange(shaped);
}

// The geometry blend of one layer onto the height accumulated below it. `blendMode` is
// Params::HeightBlendMode as int (Add, Subtract, Multiply, Overlay, Maximum, Minimum).
inline float CombineHeight(float baseHeight, float layerValue, int blendMode) {
    switch (blendMode) {
        case 1:  return baseHeight - layerValue;
        case 2:  return baseHeight * layerValue;
        case 3:  return baseHeight < 0.5f ? 2.0f * baseHeight * layerValue
                                          : 1.0f - 2.0f * (1.0f - baseHeight) * (1.0f - layerValue);
        case 4:  return baseHeight > layerValue ? baseHeight : layerValue;
        case 5:  return baseHeight < layerValue ? baseHeight : layerValue;
        default: return baseHeight + layerValue;
    }
}

// Opacity is the mix between "layer absent" and "layer fully combined", then the running
// height is clamped to the stage's height window. Returns the new accumulated height.
inline float ApplyLayerToHeight(float baseHeight, float layerValue,
                                const LayerKernelConfiguration& configuration) {
    const float combined = CombineHeight(baseHeight, layerValue, configuration.blendMode);
    float blended = baseHeight + (combined - baseHeight) * configuration.opacity;
    if (blended < configuration.heightMinimum) blended = configuration.heightMinimum;
    if (blended > configuration.heightMaximum) blended = configuration.heightMaximum;
    return blended;
}

} // namespace Proc
} // namespace SanmapGen
