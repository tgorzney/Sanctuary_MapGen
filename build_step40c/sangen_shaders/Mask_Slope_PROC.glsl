#version 430 core
// Mask_Slope_PROC.glsl — GPU twin of the gate half of Mask_Slope_PROC.h: the feathered window
// edges, the tweakable smoothstep, invert, and the strength mix. Same expressions in the same
// order as the CPU header, so the two backends differ only in float evaluation (Constitution
// §4). The gradient itself stays in Mask_PROC.glsl because it reads the height buffer, and a
// buffer cannot be shared across GLSL compilation units.
// One compilation unit of the Mask program (linked, never #included). Scalar arguments only —
// a struct declaration cannot cross a unit boundary either.

float clampToUnitInterval(float value) { return clamp(value, 0.0, 1.0); }

// Hermite ease with tweakable coefficients (defaults 3/2 = the classic smoothstep, §8).
float smoothstepWeight(float rampPosition, float shoulder, float scale) {
    return rampPosition * rampPosition * (shoulder - scale * rampPosition);
}

// An inverse feather of 0 marks a hard (unfeathered) edge — the same zero-reciprocal sentinel
// the rest of the kernels use for a degenerate range.
float risingEdgeWeight(float value, float edge, float inverseFeather) {
    if (inverseFeather <= 0.0) return value >= edge ? 1.0 : 0.0;
    return clampToUnitInterval(1.0 - (edge - value) * inverseFeather);
}

float fallingEdgeWeight(float value, float edge, float inverseFeather) {
    if (inverseFeather <= 0.0) return value <= edge ? 1.0 : 0.0;
    return clampToUnitInterval(1.0 - (value - edge) * inverseFeather);
}

// The weight this stratum's procedural mask is multiplied by. Smoothstep mode feathers both
// edges; hard-clamp mode is the binary window and ignores the feather widths.
float slopeGateWeight(float slopeGradient, float gradientLow, float gradientHigh, float inverseFeatherLow,
                      float inverseFeatherHigh, int bSmoothstepEnabled, int bInvertEnabled,
                      float gateStrength, float smoothstepShoulder, float smoothstepScale) {
    if (gateStrength <= 0.0) return 1.0;
    float weight;
    if (bSmoothstepEnabled != 0) {
        float rising = smoothstepWeight(risingEdgeWeight(slopeGradient, gradientLow, inverseFeatherLow),
                                        smoothstepShoulder, smoothstepScale);
        float falling = smoothstepWeight(fallingEdgeWeight(slopeGradient, gradientHigh, inverseFeatherHigh),
                                         smoothstepShoulder, smoothstepScale);
        weight = rising * falling;
    } else {
        weight = (slopeGradient >= gradientLow && slopeGradient <= gradientHigh) ? 1.0 : 0.0;
    }
    if (bInvertEnabled != 0) weight = 1.0 - weight;
    return 1.0 + (weight - 1.0) * gateStrength;
}
