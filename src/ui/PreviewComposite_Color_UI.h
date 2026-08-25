// PreviewComposite_Color_UI.h — the composite's per-pixel COLOR math, and nothing else.
// Layer: UI. Pure functions of already-sampled values: no DATA, no GL, no field sampling and
// no derivative of any field (that is what keeps the shadow-sim out, ARCH §3.2). This header
// is the Cpu twin of PreviewComposite_Color_UI.glsl — one expression per operation, so the
// preview cannot silently diverge from itself the way the legacy shader diverged from the bake.
#pragma once
#include <cmath>
#include "GradientLut_UI.h"          // kLookupChannelCount: the table layout M4-2 bakes
#include "PreviewComposite_Kernel_UI.h"
#include "PreviewComposite_Settings_UI.h"

namespace SanmapGen {
namespace Ui {

struct PreviewColor {
    float red = 0.0f, green = 0.0f, blue = 0.0f, alpha = 0.0f;
};

inline float ClampUnit(float value) {
    if (!(value > 0.0f)) return 0.0f;          // the !( ) form also traps NaN at zero
    return value > 1.0f ? 1.0f : value;
}

// Maps a field value onto the ramp's normalized 0..1 (ARCH §8.2: the consumer owns its
// domain). The reciprocal is precomputed in the layer record — never a divide per pixel.
inline float NormalizeToDomain(float value, float domainMinimum, float domainRangeReciprocal) {
    return ClampUnit((value - domainMinimum) * domainRangeReciprocal);
}

// Samples a table baked by Ui::BakeGradientLut. That bake is endpoint-INCLUSIVE, so the
// sample position is `normalized * (entryCount - 1)` — no texel-center offset.
inline PreviewColor SampleGradientLookupTable(const float* tableBase, int entryCount,
                                              float normalizedPosition) {
    PreviewColor sampled;
    if (tableBase == nullptr || entryCount <= 0) return sampled;
    const float position = ClampUnit(normalizedPosition) * static_cast<float>(entryCount - 1);
    int lowEntry = static_cast<int>(position);
    if (lowEntry > entryCount - 1) lowEntry = entryCount - 1;
    const int highEntry = lowEntry < entryCount - 1 ? lowEntry + 1 : lowEntry;
    const float fraction = position - static_cast<float>(lowEntry);
    const float* const low = tableBase + static_cast<int>(kLookupChannelCount) * lowEntry;
    const float* const high = tableBase + static_cast<int>(kLookupChannelCount) * highEntry;
    sampled.red   = low[0] + (high[0] - low[0]) * fraction;
    sampled.green = low[1] + (high[1] - low[1]) * fraction;
    sampled.blue  = low[2] + (high[2] - low[2]) * fraction;
    sampled.alpha = low[3] + (high[3] - low[3]) * fraction;
    return sampled;
}

// The one channel combine, per preview blend mode. Subtract..HardLight (STEP200) mirror
// PreviewComposite_Color_UI.glsl's combineChannel expression for expression — CPU/GPU parity is
// only "same expressions, same order", never a shared implementation across the two languages.
inline float CombineChannel(float destination, float source, PreviewBlendMode blendMode) {
    switch (blendMode) {
        case PreviewBlendMode::Add:      return destination + source;
        case PreviewBlendMode::Multiply: return destination * source;
        case PreviewBlendMode::Maximum:  return destination > source ? destination : source;
        case PreviewBlendMode::Minimum:  return destination < source ? destination : source;
        case PreviewBlendMode::Subtract: return destination - source;
        // Bounded to at most 1.0, not left to blow out unboundedly as `source` -> 0 (the standard
        // compositing definition of Divide, and a real fix, not just a numerical band-aid): an
        // UNBOUNDED division amplifies any ordinary sub-1/255 Cpu/Gpu float noise in `source`
        // (already tolerated everywhere else) into a wildly different large magnitude on each
        // backend, which then survives the opacity lerp as a multi-byte divergence. Clamping here
        // means both backends saturate to the same 1.0 the instant `source` gets small, instead of
        // disagreeing about exactly how large "very large" is.
        case PreviewBlendMode::Divide: {
            if (source <= 0.0f) return 1.0f;
            const float divided = destination / source;
            return divided < 1.0f ? divided : 1.0f;
        }
        case PreviewBlendMode::Overlay:
            return destination <= 0.5f ? 2.0f * destination * source
                                        : 1.0f - 2.0f * (1.0f - destination) * (1.0f - source);
        case PreviewBlendMode::Screen:
            return 1.0f - (1.0f - destination) * (1.0f - source);
        case PreviewBlendMode::SoftLight:
            return source <= 0.5f
                ? 2.0f * destination * source + destination * destination * (1.0f - 2.0f * source)
                : 2.0f * destination * (1.0f - source) + std::sqrt(destination) * (2.0f * source - 1.0f);
        case PreviewBlendMode::HardLight:
            return source <= 0.5f ? 2.0f * destination * source
                                   : 1.0f - 2.0f * (1.0f - destination) * (1.0f - source);
        default:                         return source;              // AlphaBlend / Replace
    }
}

// Blends one layer over the image. `amount` is the layer opacity times the layer color's own
// alpha (its coverage). Color channels only: the composite image is opaque, so the image alpha
// is carried through from the clear color and no layer can punch a hole in it.
inline PreviewColor BlendPreviewColor(PreviewColor destination, PreviewColor source,
                                      PreviewBlendMode blendMode, float amount) {
    PreviewColor result = destination;
    if (blendMode == PreviewBlendMode::Replace) {
        result.red = ClampUnit(source.red);
        result.green = ClampUnit(source.green);
        result.blue = ClampUnit(source.blue);
        return result;
    }
    const float blendAmount = ClampUnit(amount);
    const float combinedRed   = CombineChannel(destination.red, source.red, blendMode);
    const float combinedGreen = CombineChannel(destination.green, source.green, blendMode);
    const float combinedBlue  = CombineChannel(destination.blue, source.blue, blendMode);
    result.red   = ClampUnit(destination.red + (combinedRed - destination.red) * blendAmount);
    result.green = ClampUnit(destination.green + (combinedGreen - destination.green) * blendAmount);
    result.blue  = ClampUnit(destination.blue + (combinedBlue - destination.blue) * blendAmount);
    return result;
}

// The stratum splat: the BAKED surface weights times each stratum's preview tint. The weights
// are consumed VERBATIM — the one remap already happened in the Mask stage (ARCH §7.2.5), and
// re-applying it here is precisely the "preview truth != bake truth" defect.
// Where the total weight is at/below the epsilon the splat paints nothing (alpha 0) and the
// layers underneath show through; it does not fall back to a base stratum the way the bake does.
inline PreviewColor SplatSurfaceStrata(const float* stratumWeights,
                                       const PreviewStratumConfiguration* stratumConfigurations,
                                       int stratumCount, int bNormalizeWeights, float weightEpsilon) {
    PreviewColor splat;
    float weightTotal = 0.0f;
    for (int stratum = 0; stratum < stratumCount; ++stratum) {
        if (stratumConfigurations[stratum].bEnabled == 0) continue;
        const float weight = stratumWeights[stratum];
        if (!(weight > 0.0f)) continue;
        splat.red   += weight * stratumConfigurations[stratum].previewColorRed;
        splat.green += weight * stratumConfigurations[stratum].previewColorGreen;
        splat.blue  += weight * stratumConfigurations[stratum].previewColorBlue;
        weightTotal += weight;
    }
    if (weightTotal <= weightEpsilon) return PreviewColor();
    if (bNormalizeWeights != 0) {
        const float totalReciprocal = 1.0f / weightTotal;
        splat.red *= totalReciprocal; splat.green *= totalReciprocal; splat.blue *= totalReciprocal;
    }
    splat.alpha = ClampUnit(weightTotal);
    return splat;
}

// Water depth in game units, normalized across the deep-water window. Negative where the
// baked surface is above the water level, which the caller reads as "no water here".
// This reads the BAKED height and a settings threshold — it simulates nothing.
inline float NormalizedWaterDepth(float normalizedHeight,
                                  const PreviewCompositeConfiguration& configuration) {
    const float heightInGameUnits = normalizedHeight * configuration.terrainMaxHeight;
    const float depth = configuration.waterLevelMaximum - heightInGameUnits;
    if (!(depth > 0.0f)) return -1.0f;
    return ClampUnit((depth - configuration.deepWaterDepthMinimum)
                     * configuration.deepWaterDepthRangeReciprocal);
}

// The image is RGBA8, one packed texel per pixel (the SYS seam carries buffers, not images).
inline unsigned int PackRgba8(PreviewColor color) {
    const unsigned int red   = static_cast<unsigned int>(ClampUnit(color.red) * 255.0f + 0.5f);
    const unsigned int green = static_cast<unsigned int>(ClampUnit(color.green) * 255.0f + 0.5f);
    const unsigned int blue  = static_cast<unsigned int>(ClampUnit(color.blue) * 255.0f + 0.5f);
    const unsigned int alpha = static_cast<unsigned int>(ClampUnit(color.alpha) * 255.0f + 0.5f);
    return red | (green << 8) | (blue << 16) | (alpha << 24);
}

inline PreviewColor UnpackRgba8(unsigned int packedTexel) {
    PreviewColor color;
    color.red   = static_cast<float>(packedTexel & 0xFFu) * (1.0f / 255.0f);
    color.green = static_cast<float>((packedTexel >> 8) & 0xFFu) * (1.0f / 255.0f);
    color.blue  = static_cast<float>((packedTexel >> 16) & 0xFFu) * (1.0f / 255.0f);
    color.alpha = static_cast<float>((packedTexel >> 24) & 0xFFu) * (1.0f / 255.0f);
    return color;
}

} // namespace Ui
} // namespace SanmapGen
