#version 430 core
// PreviewComposite_Color_UI.glsl — the composite's per-pixel COLOR math, and nothing else.
// GPU twin of PreviewComposite_Color_UI.h, expression for expression: the same clamp form (the
// !(value > 0.0) test traps NaN at zero on both sides), the same blend, the same RGBA8 packing.
// Pure value math — it declares no buffer and samples no field, which is what keeps every
// re-derivation of a simulated quantity out of the preview (ARCH §3.2).
// One compilation unit of the PreviewComposite program (linked, never #included); the unit that
// declares main() is PreviewComposite_UI.glsl. Blend-mode numbers arrive as #defines built from
// the C++ enum, so the two sides cannot drift.

float clampUnit(float value) {
    if (!(value > 0.0)) return 0.0;
    return value > 1.0 ? 1.0 : value;
}

// Maps a field value onto the ramp's normalized 0..1 (ARCH §8.2: the consumer owns its domain).
// The reciprocal is precomputed in the layer record — never a divide per pixel.
float normalizeToDomain(float value, float domainMinimum, float domainRangeReciprocal) {
    return clampUnit((value - domainMinimum) * domainRangeReciprocal);
}

// Subtract..HardLight (STEP200) mirror PreviewComposite_Color_UI.h's CombineChannel expression for
// expression — the #defines arrive from PreviewComposite_GpuProgram_UI.cpp's BuildEnumDefinitions,
// numerically identical to the C++ enum, so the two sides cannot drift.
float combineChannel(float destination, float source, int blendMode) {
    if (blendMode == PREVIEW_BLEND_ADD)      return destination + source;
    if (blendMode == PREVIEW_BLEND_MULTIPLY) return destination * source;
    if (blendMode == PREVIEW_BLEND_MAXIMUM)  return destination > source ? destination : source;
    if (blendMode == PREVIEW_BLEND_MINIMUM)  return destination < source ? destination : source;
    if (blendMode == PREVIEW_BLEND_SUBTRACT) return destination - source;
    // Bounded to at most 1.0 (STEP200 fix) — mirrors PreviewComposite_Color_UI.h's CombineChannel
    // exactly: an unbounded division would amplify ordinary sub-1/255 Cpu/Gpu float noise into a
    // multi-byte divergence once it survives the opacity lerp.
    if (blendMode == PREVIEW_BLEND_DIVIDE) {
        if (source <= 0.0) return 1.0;
        float divided = destination / source;
        return divided < 1.0 ? divided : 1.0;
    }
    if (blendMode == PREVIEW_BLEND_OVERLAY)
        return destination <= 0.5 ? 2.0 * destination * source
                                   : 1.0 - 2.0 * (1.0 - destination) * (1.0 - source);
    if (blendMode == PREVIEW_BLEND_SCREEN)
        return 1.0 - (1.0 - destination) * (1.0 - source);
    if (blendMode == PREVIEW_BLEND_SOFT_LIGHT)
        return source <= 0.5 ? 2.0 * destination * source + destination * destination * (1.0 - 2.0 * source)
                              : 2.0 * destination * (1.0 - source) + sqrt(destination) * (2.0 * source - 1.0);
    if (blendMode == PREVIEW_BLEND_HARD_LIGHT)
        return source <= 0.5 ? 2.0 * destination * source
                              : 1.0 - 2.0 * (1.0 - destination) * (1.0 - source);
    return source;                                    // PREVIEW_BLEND_ALPHA / REPLACE
}

// Blends one layer over the image. `amount` is the layer opacity times the layer color's own
// alpha (its coverage). Color channels only: the composite image is opaque, so the image alpha
// carries through from the clear color and no layer can punch a hole in it.
vec4 blendPreviewColor(vec4 destination, vec4 source, int blendMode, float amount) {
    vec4 result = destination;
    if (blendMode == PREVIEW_BLEND_REPLACE) {
        result.r = clampUnit(source.r);
        result.g = clampUnit(source.g);
        result.b = clampUnit(source.b);
        return result;
    }
    float blendAmount = clampUnit(amount);
    float combinedRed   = combineChannel(destination.r, source.r, blendMode);
    float combinedGreen = combineChannel(destination.g, source.g, blendMode);
    float combinedBlue  = combineChannel(destination.b, source.b, blendMode);
    result.r = clampUnit(destination.r + (combinedRed - destination.r) * blendAmount);
    result.g = clampUnit(destination.g + (combinedGreen - destination.g) * blendAmount);
    result.b = clampUnit(destination.b + (combinedBlue - destination.b) * blendAmount);
    return result;
}

// Water depth in game units, normalized across the deep-water window. Negative where the BAKED
// surface is above the water level, which the caller reads as "no water here". It reads a baked
// height and a settings threshold — it simulates nothing.
float normalizedWaterDepth(float normalizedHeight, float terrainMaxHeight, float waterLevelMaximum,
                           float deepWaterDepthMinimum, float deepWaterDepthRangeReciprocal) {
    float depth = waterLevelMaximum - normalizedHeight * terrainMaxHeight;
    if (!(depth > 0.0)) return -1.0;
    return clampUnit((depth - deepWaterDepthMinimum) * deepWaterDepthRangeReciprocal);
}

// The image is RGBA8, one packed texel per pixel (the SYS seam carries buffers, not GL images),
// so these bytes are exactly what a GL_RGBA8 upload wants.
uint packByte(float value) { return uint(clampUnit(value) * 255.0 + 0.5); }

uint packRgba8(vec4 color) {
    return packByte(color.r) | (packByte(color.g) << 8) | (packByte(color.b) << 16)
         | (packByte(color.a) << 24);
}

// `packedTexel`, not `packed`: the latter is a reserved GLSL keyword.
vec4 unpackRgba8(uint packedTexel) {
    return vec4(float(packedTexel & 0xFFu),         float((packedTexel >> 8) & 0xFFu),
                float((packedTexel >> 16) & 0xFFu), float((packedTexel >> 24) & 0xFFu)) * (1.0 / 255.0);
}
