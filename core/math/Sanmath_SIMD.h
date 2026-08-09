#pragma once
#include <immintrin.h>

namespace SanmapGen {
    namespace Math {
        inline int CheckThreshold8_AVX(__m256 values, float threshold) {
            __m256 threshVec = _mm256_set1_ps(threshold);
            __m256 cmp = _mm256_cmp_ps(values, threshVec, _CMP_LE_OQ);
            return _mm256_movemask_ps(cmp);
        }
        
        inline __m256 CheckThreshold8_AVX_Mask(__m256 values, float threshold) {
            __m256 threshVec = _mm256_set1_ps(threshold);
            return _mm256_cmp_ps(values, threshVec, _CMP_LE_OQ);
        }
    }
}
