// Morton_MATH.h — portable Morton (Z-order) encode/decode + tiled block-linear index.
// Layer: MATH. Accuracy: Exact (pure integer bit operations).
// One definition for the whole project (replaces the triplicated Morton copies).
// BMI2 pdep/pext fast path when available, portable magic-number fallback otherwise.
#pragma once
#include <cstdint>
#if defined(__BMI2__)
#include <immintrin.h>
#endif

namespace SanmapGen {
namespace Math {

// --- Portable magic-number bit-interleave (always compiled; used by the fallback
//     path and available to tests for backend-parity checks) -------------------
namespace MortonDetail {

// 2D: spread the low 16 bits of value so one zero bit sits between each source bit.
inline uint32_t SpreadEveryOtherBit(uint32_t value) {
    value &= 0x0000ffffu;
    value = (value ^ (value << 8)) & 0x00ff00ffu;
    value = (value ^ (value << 4)) & 0x0f0f0f0fu;
    value = (value ^ (value << 2)) & 0x33333333u;
    value = (value ^ (value << 1)) & 0x55555555u;
    return value;
}

// 2D inverse: gather every other bit back down into the low 16 bits.
inline uint32_t GatherEveryOtherBit(uint32_t value) {
    value &= 0x55555555u;
    value = (value ^ (value >> 1)) & 0x33333333u;
    value = (value ^ (value >> 2)) & 0x0f0f0f0fu;
    value = (value ^ (value >> 4)) & 0x00ff00ffu;
    value = (value ^ (value >> 8)) & 0x0000ffffu;
    return value;
}

// 3D: spread the low 10 bits so two zero bits sit between each source bit.
inline uint32_t SpreadEveryThirdBit(uint32_t value) {
    value &= 0x000003ffu;
    value = (value ^ (value << 16)) & 0xff0000ffu;
    value = (value ^ (value <<  8)) & 0x0300f00fu;
    value = (value ^ (value <<  4)) & 0x030c30c3u;
    value = (value ^ (value <<  2)) & 0x09249249u;
    return value;
}

// 3D inverse: gather every third bit back down into the low 10 bits.
inline uint32_t GatherEveryThirdBit(uint32_t value) {
    value &= 0x09249249u;
    value = (value ^ (value >>  2)) & 0x030c30c3u;
    value = (value ^ (value >>  4)) & 0x0300f00fu;
    value = (value ^ (value >>  8)) & 0xff0000ffu;
    value = (value ^ (value >> 16)) & 0x000003ffu;
    return value;
}

} // namespace MortonDetail

// --- Public 2D (16 bits per axis) --------------------------------------------
inline uint32_t EncodeMorton2D(uint32_t coordinateX, uint32_t coordinateY) {
#if defined(__BMI2__)
    return _pdep_u32(coordinateX, 0x55555555u) | _pdep_u32(coordinateY, 0xaaaaaaaau);
#else
    return MortonDetail::SpreadEveryOtherBit(coordinateX)
         | (MortonDetail::SpreadEveryOtherBit(coordinateY) << 1);
#endif
}

inline void DecodeMorton2D(uint32_t code, uint32_t& coordinateX, uint32_t& coordinateY) {
#if defined(__BMI2__)
    coordinateX = _pext_u32(code, 0x55555555u);
    coordinateY = _pext_u32(code, 0xaaaaaaaau);
#else
    coordinateX = MortonDetail::GatherEveryOtherBit(code);
    coordinateY = MortonDetail::GatherEveryOtherBit(code >> 1);
#endif
}

// --- Public 3D (10 bits per axis) --------------------------------------------
inline uint32_t EncodeMorton3D(uint32_t coordinateX, uint32_t coordinateY, uint32_t coordinateZ) {
#if defined(__BMI2__)
    return _pdep_u32(coordinateX, 0x09249249u)
         | _pdep_u32(coordinateY, 0x12492492u)
         | _pdep_u32(coordinateZ, 0x24924924u);
#else
    return MortonDetail::SpreadEveryThirdBit(coordinateX)
         | (MortonDetail::SpreadEveryThirdBit(coordinateY) << 1)
         | (MortonDetail::SpreadEveryThirdBit(coordinateZ) << 2);
#endif
}

inline void DecodeMorton3D(uint32_t code, uint32_t& coordinateX, uint32_t& coordinateY, uint32_t& coordinateZ) {
#if defined(__BMI2__)
    coordinateX = _pext_u32(code, 0x09249249u);
    coordinateY = _pext_u32(code, 0x12492492u);
    coordinateZ = _pext_u32(code, 0x24924924u);
#else
    coordinateX = MortonDetail::GatherEveryThirdBit(code);
    coordinateY = MortonDetail::GatherEveryThirdBit(code >> 1);
    coordinateZ = MortonDetail::GatherEveryThirdBit(code >> 2);
#endif
}

// --- Tiled block-linear index -------------------------------------------------
// Surface split into square tiles of side (1 << tileSizeLog2), tiles laid out
// row-major across surfaceWidth; cells within a tile are Morton-ordered (tiled-Z).
inline uint32_t BlockLinearIndex(uint32_t coordinateX, uint32_t coordinateY,
                                 uint32_t surfaceWidth, uint32_t tileSizeLog2) {
    const uint32_t tileSize     = 1u << tileSizeLog2;
    const uint32_t tileMask     = tileSize - 1u;
    const uint32_t tilesPerRow  = (surfaceWidth + tileMask) >> tileSizeLog2;
    const uint32_t tileIndex    = (coordinateY >> tileSizeLog2) * tilesPerRow
                                + (coordinateX >> tileSizeLog2);
    const uint32_t inTileOffset = EncodeMorton2D(coordinateX & tileMask, coordinateY & tileMask);
    return tileIndex * (tileSize * tileSize) + inTileOffset;
}

} // namespace Math
} // namespace SanmapGen
