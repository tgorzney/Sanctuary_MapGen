#pragma once
#include <cmath>

namespace SanmapGen {
    namespace Math {
        inline float FastInv(float x) {
            return 1.0f / x; // Or intrinsic _mm_cvtss_f32(_mm_rcp_ss(_mm_set_ss(x)))
        }

        inline float GetSlopeSquaredThreshold(float maxSlopeDegrees) {
            if (maxSlopeDegrees >= 89.9f) return 9999999.0f;
            float rad = maxSlopeDegrees * (3.14159265f / 180.0f);
            float cosS = std::cos(rad);
            if (cosS <= 0.0001f) return 9999999.0f;
            return (1.0f / (cosS * cosS)) - 1.0f;
        }
    }
}
