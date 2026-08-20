#version 430 core
// NoiseBlend_Shape_PROC.glsl — GPU twin of NoiseBlend_Shape_PROC.h plus the occlusion
// weighting of HeightOcclusion_MATH.h: the height-blend combine, the opacity mix, and the
// per-layer mask coverage. Same expressions in the same order as the CPU header, so the two
// backends differ only in float evaluation (Constitution §4).
// The density/levels reshape stays in the kernel main because it reads the whole layer
// configuration record, and GLSL cannot share a struct declaration across compilation units.
// One compilation unit of the NoiseBlend program (linked, never #included).

// Params::HeightBlendMode as int — the values arrive as #defines built from the C++ enum.
float combineHeight(float baseHeight, float layerValue, int blendMode) {
    if (blendMode == HEIGHT_BLEND_SUBTRACT) return baseHeight - layerValue;
    if (blendMode == HEIGHT_BLEND_MULTIPLY) return baseHeight * layerValue;
    if (blendMode == HEIGHT_BLEND_OVERLAY)
        return baseHeight < 0.5 ? 2.0 * baseHeight * layerValue
                                : 1.0 - 2.0 * (1.0 - baseHeight) * (1.0 - layerValue);
    if (blendMode == HEIGHT_BLEND_MAXIMUM) return max(baseHeight, layerValue);
    if (blendMode == HEIGHT_BLEND_MINIMUM) return min(baseHeight, layerValue);
    return baseHeight + layerValue;
}

float applyLayerToHeight(float baseHeight, float layerValue, int blendMode, float opacity,
                         float heightMinimum, float heightMaximum) {
    float combined = combineHeight(baseHeight, layerValue, blendMode);
    float blended = baseHeight + (combined - baseHeight) * opacity;
    return clamp(blended, heightMinimum, heightMaximum);
}

// MASKING_SPEC top-down occlusion: thickness * contrast, hard-clamped to the swap-guarded
// window (the C++ side already ordered it), times opacity.
float occlusionAlpha(float thickness, float contrast, float windowLow, float windowHigh, float opacity) {
    return clamp(thickness * contrast, windowLow, windowHigh) * opacity;
}
