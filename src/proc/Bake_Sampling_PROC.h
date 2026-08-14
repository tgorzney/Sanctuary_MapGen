// Bake_Sampling_PROC.h — the bake stage's scalar sampling math, CPU side.
// Layer: PROC. Mask remap, RGBA8 pack/unpack, and the tiled wrapped bilinear albedo fetch —
// the three expressions the GPU twin repeats in Bake_Sampling_PROC.glsl. Kept in one small
// header so the pair can be diffed side by side (ARCH §1.4) and so both the compositor and
// the parity test share ONE definition (Constitution §4: one math source, two backends).
#pragma once
#include "Bake_Kernel_PROC.h"
#include <cmath>

namespace SanmapGen {
namespace Proc {

constexpr float bakeByteScale           = 255.0f;
constexpr float bakeByteScaleReciprocal = 1.0f / 255.0f;

// Remapped visibility: maskRemapMin/Max stretched onto 0..1 (MASKING_SPEC consumption).
// A degenerate window (max <= min) collapses to a hard threshold at the minimum.
inline float RemapMaskWeight(float rawWeight, const StratumKernelConfiguration& configuration) {
    if (configuration.maskRemapRangeReciprocal <= 0.0f)
        return rawWeight >= configuration.maskRemapMinimum ? 1.0f : 0.0f;
    const float remapped = (rawWeight - configuration.maskRemapMinimum)
                         * configuration.maskRemapRangeReciprocal;
    return remapped < 0.0f ? 0.0f : (remapped > 1.0f ? 1.0f : remapped);
}

inline int WrapTexelIndex(int value, int size) {
    const int wrapped = value % size;
    return wrapped < 0 ? wrapped + size : wrapped;
}

// Round-to-nearest quantization, matching the GLSL packRgba8 exactly.
inline unsigned int PackRgba8(float red, float green, float blue, float alpha) {
    const float channels[4] = { red, green, blue, alpha };
    unsigned int packed = 0u;
    for (int channel = 0; channel < 4; ++channel) {
        float value = channels[channel];
        value = value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
        packed |= static_cast<unsigned int>(value * bakeByteScale + 0.5f) << (channel * 8);
    }
    return packed;
}

inline float UnpackRgba8Channel(unsigned int packed, int channel) {
    return static_cast<float>((packed >> (channel * 8)) & 0xFFu) * bakeByteScaleReciprocal;
}

// Tiled bilinear albedo fetch, wrapped at the texture edges, tinted by the preview color.
// No texture (width 0) => the flat tint, which is what the preview composite uses today.
inline void SampleStratumColor(const StratumKernelConfiguration& configuration, const unsigned int* texels,
                               float mapU, float mapV, float& red, float& green, float& blue) {
    red = configuration.tintRed; green = configuration.tintGreen; blue = configuration.tintBlue;
    if (configuration.albedoWidth <= 0 || configuration.albedoHeight <= 0 || texels == nullptr) return;
    const float texelX = mapU * configuration.tileCount * static_cast<float>(configuration.albedoWidth) - 0.5f;
    const float texelY = mapV * configuration.tileCount * static_cast<float>(configuration.albedoHeight) - 0.5f;
    const int lowX = static_cast<int>(std::floor(texelX));
    const int lowY = static_cast<int>(std::floor(texelY));
    const float fractionX = texelX - static_cast<float>(lowX);
    const float fractionY = texelY - static_cast<float>(lowY);
    const int columnLow  = WrapTexelIndex(lowX, configuration.albedoWidth);
    const int columnHigh = WrapTexelIndex(lowX + 1, configuration.albedoWidth);
    const int rowLow     = WrapTexelIndex(lowY, configuration.albedoHeight);
    const int rowHigh    = WrapTexelIndex(lowY + 1, configuration.albedoHeight);
    const unsigned int corners[4] = {
        texels[rowLow  * configuration.albedoWidth + columnLow],
        texels[rowLow  * configuration.albedoWidth + columnHigh],
        texels[rowHigh * configuration.albedoWidth + columnLow],
        texels[rowHigh * configuration.albedoWidth + columnHigh] };
    float tint[3] = { red, green, blue };
    float* outputs[3] = { &red, &green, &blue };
    for (int channel = 0; channel < 3; ++channel) {
        const float top    = UnpackRgba8Channel(corners[0], channel)
                           + (UnpackRgba8Channel(corners[1], channel)
                            - UnpackRgba8Channel(corners[0], channel)) * fractionX;
        const float bottom = UnpackRgba8Channel(corners[2], channel)
                           + (UnpackRgba8Channel(corners[3], channel)
                            - UnpackRgba8Channel(corners[2], channel)) * fractionX;
        *outputs[channel] = tint[channel] * (top + (bottom - top) * fractionY);
    }
}

} // namespace Proc
} // namespace SanmapGen
