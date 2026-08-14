// Mask_Slope_PROC.h — CPU half of the slope math: the slope field and the slope gate.
// Layer: PROC. Paired one-for-one with Mask_Slope_PROC.glsl — same expressions, same order,
// so the only CPU/GPU difference is float evaluation, never the algorithm (Constitution §4,
// DISPATCH_INTERFACE_SPEC §1). Header-only: the GPU twin is the .glsl, so there is no second
// .cpp to keep in step.
//
// PINNED SLOPE UNIT: gradient magnitude, rise/run (= tan of the terrain angle), dimensionless.
//   slope = |grad(height * terrainMaxHeight)| over a cellSize grid
// The heightfield is the normalized 0..1 field the noise/blend stage wrote; `heightScale` is
// Geometry.terrainMaxHeight READ FROM THE MAP (never the old hardcoded 128). Degrees are the
// designer-facing unit only (StratumMask_PARAMS) and are converted to gradient — via tan() on
// the host, once per run — before either backend ever sees them, so no tan() runs per cell and
// the two backends cannot disagree about the unit.
#pragma once
#include <cmath>
#include "Mask_Kernel_PROC.h"

namespace SanmapGen {
namespace Proc {

inline float ClampToUnitInterval(float value) {
    return value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
}

// Finite-difference gradient magnitude at one vertex. Interior cells use the central
// difference (span 2), edge cells the one-sided difference (span 1); the matching reciprocal
// is precomputed, never divided in the loop (Constitution §3).
inline float SlopeGradientMagnitude(const float* heightValues, int x, int y, int vertexSize,
                                    const MaskStratumConfiguration& configuration) {
    const int lowX  = x > 0 ? x - 1 : 0;
    const int highX = x + 1 < vertexSize ? x + 1 : vertexSize - 1;
    const int lowY  = y > 0 ? y - 1 : 0;
    const int highY = y + 1 < vertexSize ? y + 1 : vertexSize - 1;
    const float inverseSpanX = (highX - lowX) == 2 ? configuration.inverseDoubleSpan
                                                   : configuration.inverseSingleSpan;
    const float inverseSpanY = (highY - lowY) == 2 ? configuration.inverseDoubleSpan
                                                   : configuration.inverseSingleSpan;
    const float gradientX = (heightValues[y * vertexSize + highX] - heightValues[y * vertexSize + lowX])
                          * configuration.heightScale * inverseSpanX;
    const float gradientY = (heightValues[highY * vertexSize + x] - heightValues[lowY * vertexSize + x])
                          * configuration.heightScale * inverseSpanY;
    return std::sqrt(gradientX * gradientX + gradientY * gradientY);
}

// Hermite ease with tweakable coefficients (defaults 3/2 = the classic smoothstep, §8).
inline float SmoothstepWeight(float rampPosition, float shoulder, float scale) {
    return rampPosition * rampPosition * (shoulder - scale * rampPosition);
}

// Feathered window edges. An inverse feather of 0 marks a hard edge (the degenerate-range
// sentinel this codebase already uses for levelsRangeReciprocal).
inline float RisingEdgeWeight(float value, float edge, float inverseFeather) {
    if (inverseFeather <= 0.0f) return value >= edge ? 1.0f : 0.0f;
    return ClampToUnitInterval(1.0f - (edge - value) * inverseFeather);
}
inline float FallingEdgeWeight(float value, float edge, float inverseFeather) {
    if (inverseFeather <= 0.0f) return value <= edge ? 1.0f : 0.0f;
    return ClampToUnitInterval(1.0f - (value - edge) * inverseFeather);
}

// The gate weight this stratum's procedural mask is multiplied by. Smoothstep mode feathers
// both edges; hard-clamp mode is the binary window (and ignores the feather widths) — the two
// are visibly different by construction, never a silent variation of the same curve.
inline float SlopeGateWeight(float slopeGradient, const MaskStratumConfiguration& configuration) {
    if (configuration.gateStrength <= 0.0f) return 1.0f;
    float weight;
    if (configuration.bSmoothstepEnabled != 0) {
        const float rising = SmoothstepWeight(
            RisingEdgeWeight(slopeGradient, configuration.slopeGradientLow, configuration.inverseFeatherLow),
            configuration.smoothstepShoulder, configuration.smoothstepScale);
        const float falling = SmoothstepWeight(
            FallingEdgeWeight(slopeGradient, configuration.slopeGradientHigh, configuration.inverseFeatherHigh),
            configuration.smoothstepShoulder, configuration.smoothstepScale);
        weight = rising * falling;
    } else {
        weight = (slopeGradient >= configuration.slopeGradientLow
               && slopeGradient <= configuration.slopeGradientHigh) ? 1.0f : 0.0f;
    }
    if (configuration.bInvertEnabled != 0) weight = 1.0f - weight;
    return 1.0f + (weight - 1.0f) * configuration.gateStrength;
}

} // namespace Proc
} // namespace SanmapGen
