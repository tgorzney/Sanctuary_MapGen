// MapExporter_SampleQuantize_IO.h — the two normalized-sample quantizers, header-only inline
// functions (STEP32). Export-write-scoped: the format's 16-bit heightmap sample and the 8-bit
// mask/visualization channel, both clamped rather than wrapped on out-of-range input.
#pragma once

namespace SanmapGen {
namespace Io {

// 0..1 -> the format's 16-bit heightmap sample. Out-of-range input is clamped, never wrapped.
inline unsigned short QuantizeNormalizedHeightSample(float normalizedHeight) {
    if (!(normalizedHeight > 0.0f)) return 0u;                       // also catches NaN
    if (normalizedHeight >= 1.0f) return 65535u;
    return static_cast<unsigned short>(normalizedHeight * 65535.0f + 0.5f);
}

// 0..1 -> one 8-bit mask/visualization channel. Same clamping contract.
inline unsigned char QuantizeNormalizedWeightSample(float normalizedWeight) {
    if (!(normalizedWeight > 0.0f)) return 0u;
    if (normalizedWeight >= 1.0f) return 255u;
    return static_cast<unsigned char>(normalizedWeight * 255.0f + 0.5f);
}

} // namespace Io
} // namespace SanmapGen
