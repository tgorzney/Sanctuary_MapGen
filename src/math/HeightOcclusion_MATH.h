// HeightOcclusion_MATH.h — top-down occlusion weighting for the MaterialProportions field.
// Layer: MATH — pure, stateless, plain floats only (no PARAMS/DATA/GPU dependency), so
// every stage that writes `MaterialProportions` shares ONE copy of this math: NoiseBlend_PROC
// today (ARCH §7.2 makes it the field's single writer). Realizes MASKING_SPEC Part 2 "Height
// mask (top-down occlusion)" verbatim: alpha = thickness * contrast, hard-clamped to the
// swap-guarded [minimum, maximum] window, times opacity; the contribution is capped by the
// visibility still left above. Hard clamp only — no smoothstep, no feather, no invert.
// Accuracy class: Accurate (identical expression on CPU and GPU; see the GLSL twin
// `occlusionAlpha` in NoiseBlend_Shape_PROC.glsl).
#pragma once

namespace SanmapGen {
namespace Math {

// Orders the height-blend window and guarantees a non-empty one. `separationEpsilon` is the
// caller's tweakable (MASKING_SPEC's +0.001) — never hardcoded here.
inline void OrderOcclusionWindow(float windowFirst, float windowSecond, float separationEpsilon,
                                 float& outputLow, float& outputHigh) {
    outputLow  = windowFirst < windowSecond ? windowFirst : windowSecond;
    outputHigh = windowFirst < windowSecond ? windowSecond : windowFirst;
    if (outputHigh <= outputLow) outputHigh = outputLow + separationEpsilon;
}

// Coverage this layer claims at one cell, before it is capped by the remaining visibility.
inline float OcclusionAlpha(float thickness, float contrast, float windowLow, float windowHigh,
                            float opacity) {
    float alpha = thickness * contrast;
    if (alpha < windowLow)  alpha = windowLow;
    if (alpha > windowHigh) alpha = windowHigh;
    return alpha * opacity;
}

// The part of `alpha` that actually reaches the mask: whatever visibility is still unclaimed.
inline float OcclusionContribution(float alpha, float remainingVisibility) {
    if (alpha < 0.0f) return 0.0f;
    return alpha < remainingVisibility ? alpha : remainingVisibility;
}

} // namespace Math
} // namespace SanmapGen
