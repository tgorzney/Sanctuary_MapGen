// Morton_MATH_Test.cpp — acceptance test for Morton_MATH (Work-Order M0-1).
// Standalone: compile and run. Exits 0 on success, non-zero on first failure.
//   without BMI2:  g++ -O2 -std=c++17 Morton_MATH_Test.cpp -o t && ./t
//   with BMI2:     g++ -O2 -std=c++17 -mbmi2 Morton_MATH_Test.cpp -o t && ./t
#include "Morton_MATH.h"
#include <cstdio>
#include <cstdint>

using namespace SanmapGen::Math;

static int failures = 0;
static void expect(bool condition, const char* label) {
    if (!condition) { std::printf("FAIL: %s\n", label); ++failures; }
}

int main() {
    // 1. 2D round-trip over the full 16-bit-per-axis domain (strided sample).
    for (uint32_t x = 0; x < 65536u; x += 131u)
        for (uint32_t y = 0; y < 65536u; y += 137u) {
            uint32_t decodedX, decodedY;
            DecodeMorton2D(EncodeMorton2D(x, y), decodedX, decodedY);
            if (decodedX != x || decodedY != y) { expect(false, "2D round-trip"); goto after2D; }
        }
after2D:
    // 2. 3D round-trip over the full 10-bit-per-axis domain (strided sample).
    for (uint32_t x = 0; x < 1024u; x += 7u)
        for (uint32_t y = 0; y < 1024u; y += 11u)
            for (uint32_t z = 0; z < 1024u; z += 13u) {
                uint32_t dx, dy, dz;
                DecodeMorton3D(EncodeMorton3D(x, y, z), dx, dy, dz);
                if (dx != x || dy != y || dz != z) { expect(false, "3D round-trip"); goto after3D; }
            }
after3D:
    // 3. Backend parity: public path (BMI2 when built with -mbmi2) must equal the
    //    always-present magic-number fallback across the sampled domain.
    for (uint32_t x = 0; x < 65536u; x += 271u)
        for (uint32_t y = 0; y < 65536u; y += 269u) {
            uint32_t fallback = MortonDetail::SpreadEveryOtherBit(x)
                              | (MortonDetail::SpreadEveryOtherBit(y) << 1);
            if (EncodeMorton2D(x, y) != fallback) { expect(false, "2D backend parity"); goto afterP2; }
        }
afterP2:
    for (uint32_t x = 0; x < 1024u; x += 7u)
        for (uint32_t y = 0; y < 1024u; y += 11u)
            for (uint32_t z = 0; z < 1024u; z += 13u) {
                uint32_t fallback = MortonDetail::SpreadEveryThirdBit(x)
                                  | (MortonDetail::SpreadEveryThirdBit(y) << 1)
                                  | (MortonDetail::SpreadEveryThirdBit(z) << 2);
                if (EncodeMorton3D(x, y, z) != fallback) { expect(false, "3D backend parity"); goto afterP3; }
            }
afterP3:
    // 4. 2D known-vector compatibility (the historical magic-number layout).
    expect(EncodeMorton2D(0, 0) == 0u,   "2D (0,0)=0");
    expect(EncodeMorton2D(1, 0) == 1u,   "2D (1,0)=1");
    expect(EncodeMorton2D(0, 1) == 2u,   "2D (0,1)=2");
    expect(EncodeMorton2D(1, 1) == 3u,   "2D (1,1)=3");
    expect(EncodeMorton2D(3, 0) == 5u,   "2D (3,0)=5");   // bits 0 and 2

    // 5. Block-linear: first tile is Morton-ordered; next tile across starts at tileArea.
    expect(BlockLinearIndex(0, 0, 64, 3) == 0u,               "tiled (0,0)=0");
    expect(BlockLinearIndex(1, 1, 64, 3) == EncodeMorton2D(1,1), "tiled in-tile == morton");
    expect(BlockLinearIndex(8, 0, 64, 3) == 64u,              "tiled next-tile-x == tileArea");
    // tileSize=8 -> tilesPerRow = 64/8 = 8; tile (0,1) index=8 -> offset 8*64=512
    expect(BlockLinearIndex(0, 8, 64, 3) == 512u,             "tiled next-tile-y");

    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}
