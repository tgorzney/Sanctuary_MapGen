// Mask_Merge_PROC.h — CPU half of the stored-mask merge: the ONE resampler, the three
// ImportedMaskMode merges, and the per-stratum output remap. Layer: PROC. Paired one-for-one
// with Mask_Merge_PROC.glsl — same expressions, same order (Constitution §4).
//
// Resampling is BILINEAR everywhere (MASKING_SPEC "Resample inconsistency": import used
// nearest-neighbour while the GUI resize of the same data used bilinear — unified here).
// Merge modes are MASKING_SPEC verbatim: Disabled keeps the procedural mask, ProceduralStart
// is additive (procedural + stored, clamped), StaticOverride replaces with the stored art and
// therefore is NOT slope-gated — it is locked to what the artist shipped.
#pragma once
#include "Mask_Kernel_PROC.h"

namespace SanmapGen {
namespace Proc {

// Params::ImportedMaskMode as int — the values the shader gets as #defines.
enum : int { kMergeModeDisabled = 0, kMergeModeProceduralStart = 1, kMergeModeStaticOverride = 2 };

inline float ClampToMaskRange(float value, const MaskStratumConfiguration& configuration) {
    if (value < configuration.maskMinimum) return configuration.maskMinimum;
    if (value > configuration.maskMaximum) return configuration.maskMaximum;
    return value;
}

// Bilinear sample of this stratum's stored art at a vertex of the generated grid. The art is
// stretched over the whole map, so the vertex index scales by the precomputed sample scale.
inline float SampleStoredMaskBilinear(const float* storedValues,
                                      const MaskStratumConfiguration& configuration, int x, int y) {
    if (configuration.storedMaskWidth <= 0 || configuration.storedMaskHeight <= 0) return 0.0f;
    const float maximumX = static_cast<float>(configuration.storedMaskWidth - 1);
    const float maximumY = static_cast<float>(configuration.storedMaskHeight - 1);
    float sampleX = static_cast<float>(x) * configuration.storedSampleScaleX;
    float sampleY = static_cast<float>(y) * configuration.storedSampleScaleY;
    sampleX = sampleX < 0.0f ? 0.0f : (sampleX > maximumX ? maximumX : sampleX);
    sampleY = sampleY < 0.0f ? 0.0f : (sampleY > maximumY ? maximumY : sampleY);
    const int lowX  = static_cast<int>(sampleX);
    const int lowY  = static_cast<int>(sampleY);
    const int highX = lowX + 1 < configuration.storedMaskWidth ? lowX + 1 : lowX;
    const int highY = lowY + 1 < configuration.storedMaskHeight ? lowY + 1 : lowY;
    const float fractionX = sampleX - static_cast<float>(lowX);
    const float fractionY = sampleY - static_cast<float>(lowY);
    const int base = configuration.storedMaskOffset;
    const int lowRow  = base + lowY * configuration.storedMaskWidth;
    const int highRow = base + highY * configuration.storedMaskWidth;
    const float top    = storedValues[lowRow + lowX]  + (storedValues[lowRow + highX]  - storedValues[lowRow + lowX])  * fractionX;
    const float bottom = storedValues[highRow + lowX] + (storedValues[highRow + highX] - storedValues[highRow + lowX]) * fractionX;
    return top + (bottom - top) * fractionY;
}

// The per-stratum merge. `proceduralWeight` is the slope-gated procedural mask.
inline float MergeStoredMask(float proceduralWeight, float storedWeight,
                             const MaskStratumConfiguration& configuration) {
    if (configuration.mergeMode == kMergeModeProceduralStart)
        return ClampToMaskRange(proceduralWeight + storedWeight, configuration);
    if (configuration.mergeMode == kMergeModeStaticOverride)
        return ClampToMaskRange(storedWeight, configuration);
    return ClampToMaskRange(proceduralWeight, configuration);
}

// Final per-stratum remap (identity by default). A degenerate window is the same
// zero-reciprocal sentinel the rest of the kernels use.
inline float RemapMaskValue(float value, const MaskStratumConfiguration& configuration) {
    if (configuration.inverseRemapRange <= 0.0f)
        return value >= configuration.remapMinimum ? configuration.maskMaximum : configuration.maskMinimum;
    return ClampToMaskRange((value - configuration.remapMinimum) * configuration.inverseRemapRange, configuration);
}

} // namespace Proc
} // namespace SanmapGen
