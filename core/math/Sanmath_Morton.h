#pragma once
#include <cstdint>

namespace SanmapGen {
    namespace Math {
        inline void DecodeMorton2D(uint32_t code, uint32_t& x, uint32_t& y) {
            x = code & 0x55555555;
            x = (x ^ (x >> 1)) & 0x33333333;
            x = (x ^ (x >> 2)) & 0x0f0f0f0f;
            x = (x ^ (x >> 4)) & 0x00ff00ff;
            x = (x ^ (x >> 8)) & 0x0000ffff;

            y = (code >> 1) & 0x55555555;
            y = (y ^ (y >> 1)) & 0x33333333;
            y = (y ^ (y >> 2)) & 0x0f0f0f0f;
            y = (y ^ (y >> 4)) & 0x00ff00ff;
            y = (y ^ (y >> 8)) & 0x0000ffff;
        }

        inline uint32_t EncodeMorton2D(uint32_t x, uint32_t y) {
            x = (x | (x << 8)) & 0x00FF00FF;
            x = (x | (x << 4)) & 0x0F0F0F0F;
            x = (x | (x << 2)) & 0x33333333;
            x = (x | (x << 1)) & 0x55555555;

            y = (y | (y << 8)) & 0x00FF00FF;
            y = (y | (y << 4)) & 0x0F0F0F0F;
            y = (y | (y << 2)) & 0x33333333;
            y = (y | (y << 1)) & 0x55555555;

            return x | (y << 1);
        }
    }
}
