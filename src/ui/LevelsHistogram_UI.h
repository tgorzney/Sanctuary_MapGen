// LevelsHistogram_UI.h — the input histogram behind the Levels control. Layer: UI.
// Accuracy class: Visual (it is a picture of a distribution, nothing reads it back).
// Split out of Levels_UI.h under the ARCH §1.5 ceilings: this file owns the bucket array, that
// one owns the transfer function.
//
// The buckets are CALLER-OWNED and accumulated on demand, never per frame: a 4096x4096 layer is
// 16.7M samples, so a tab fills the buckets once when the field it describes changes (the same
// frame it trips bNeedsPreviewRender) and hands the widget a read-only view every frame after.
#pragma once

namespace SanmapGen {
namespace Ui {

// What the draw path reads: normalized 0..1 bar heights the caller owns. A null or empty view
// draws an empty frame rather than nothing, so the control keeps its layout (Constitution §6).
struct LevelsHistogramView {
    const float* bucketWeights = nullptr;
    int          bucketCount   = 0;
};

inline bool LevelsHistogramViewIsDrawable(const LevelsHistogramView& view) {
    return view.bucketWeights != nullptr && view.bucketCount > 0;
}

// The bucket a 0..1 sample falls in. Values outside the range fold into the end buckets rather
// than being dropped, so the histogram always sums to the sample count.
inline int LevelsHistogramBucketIndex(float sampleValue, int bucketCount) {
    if (bucketCount <= 0) return 0;
    if (!(sampleValue == sampleValue)) return 0;                       // NaN
    if (sampleValue <= 0.0f) return 0;
    if (sampleValue >= 1.0f) return bucketCount - 1;
    const int bucketIndex = static_cast<int>(sampleValue * static_cast<float>(bucketCount));
    return bucketIndex < bucketCount ? bucketIndex : bucketCount - 1;
}

// Adds `sampleCount` samples into `bucketCounts`, reading every `sampleStride`-th value — the
// stride is how a full-resolution field is summarized cheaply (stride 16 over 16.7M samples is
// ~1M reads). Accumulates rather than clears, so several fields can share one histogram.
inline void AccumulateLevelsHistogram(const float* samples, int sampleCount, int sampleStride,
                                      float* bucketCounts, int bucketCount) {
    if (samples == nullptr || bucketCounts == nullptr || bucketCount <= 0) return;
    const int stride = sampleStride > 0 ? sampleStride : 1;
    for (int sampleIndex = 0; sampleIndex < sampleCount; sampleIndex += stride)
        bucketCounts[LevelsHistogramBucketIndex(samples[sampleIndex], bucketCount)] += 1.0f;
}

// The tallest bucket — the scale the bars are drawn against.
inline float LargestHistogramBucket(const float* bucketCounts, int bucketCount) {
    if (bucketCounts == nullptr || bucketCount <= 0) return 0.0f;
    float largestCount = bucketCounts[0];
    for (int bucketIndex = 1; bucketIndex < bucketCount; ++bucketIndex)
        if (bucketCounts[bucketIndex] > largestCount) largestCount = bucketCounts[bucketIndex];
    return largestCount;
}

// Scales the buckets so the tallest becomes 1. Divides ONCE and multiplies the reciprocal across
// the array (Constitution §3); an all-zero histogram is left alone instead of dividing by zero.
inline void NormalizeLevelsHistogram(float* bucketCounts, int bucketCount) {
    const float largestCount = LargestHistogramBucket(bucketCounts, bucketCount);
    if (!(largestCount > 0.0f)) return;
    const float reciprocalOfLargestCount = 1.0f / largestCount;
    for (int bucketIndex = 0; bucketIndex < bucketCount; ++bucketIndex)
        bucketCounts[bucketIndex] *= reciprocalOfLargestCount;
}

} // namespace Ui
} // namespace SanmapGen
