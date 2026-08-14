// FloatVector_MATH.h — portable 8-lane SIMD float vector (Work-Order M0-2).
// Layer: MATH. The primitive every kernel builds on. AVX2 backend when available,
// otherwise the portable scalar fallback (same API, same 8-lane width, so callers
// never change — Constitution §5). Fused-multiply-add and reciprocal are Accurate/
// Visual class (backends may differ within tolerance); all other ops are Exact.
#pragma once

#if defined(__AVX2__)
#include <immintrin.h>

namespace SanmapGen {
namespace Math {

struct FloatVector {
    static constexpr int laneCount = 8;
    __m256 value;

    FloatVector() : value(_mm256_setzero_ps()) {}
    explicit FloatVector(__m256 rawValue) : value(rawValue) {}

    static FloatVector Broadcast(float scalar) { return FloatVector(_mm256_set1_ps(scalar)); }
    static FloatVector Load(const float* source) { return FloatVector(_mm256_loadu_ps(source)); }
    void Store(float* destination) const { _mm256_storeu_ps(destination, value); }

    FloatVector operator+(const FloatVector& other) const { return FloatVector(_mm256_add_ps(value, other.value)); }
    FloatVector operator-(const FloatVector& other) const { return FloatVector(_mm256_sub_ps(value, other.value)); }
    FloatVector operator*(const FloatVector& other) const { return FloatVector(_mm256_mul_ps(value, other.value)); }
    FloatVector operator/(const FloatVector& other) const { return FloatVector(_mm256_div_ps(value, other.value)); }
};

inline FloatVector Minimum(const FloatVector& a, const FloatVector& b) { return FloatVector(_mm256_min_ps(a.value, b.value)); }
inline FloatVector Maximum(const FloatVector& a, const FloatVector& b) { return FloatVector(_mm256_max_ps(a.value, b.value)); }
inline FloatVector SquareRoot(const FloatVector& a) { return FloatVector(_mm256_sqrt_ps(a.value)); }
inline FloatVector ReciprocalApproximate(const FloatVector& a) { return FloatVector(_mm256_rcp_ps(a.value)); }

inline FloatVector FusedMultiplyAdd(const FloatVector& a, const FloatVector& b, const FloatVector& c) {
#if defined(__FMA__)
    return FloatVector(_mm256_fmadd_ps(a.value, b.value, c.value));
#else
    return FloatVector(_mm256_add_ps(_mm256_mul_ps(a.value, b.value), c.value));
#endif
}

inline FloatVector CompareLessOrEqual(const FloatVector& a, const FloatVector& b) {
    return FloatVector(_mm256_cmp_ps(a.value, b.value, _CMP_LE_OQ));
}
inline FloatVector Select(const FloatVector& mask, const FloatVector& whenTrue, const FloatVector& whenFalse) {
    return FloatVector(_mm256_blendv_ps(whenFalse.value, whenTrue.value, mask.value));
}
inline float HorizontalSum(const FloatVector& v) {
    __m128 low  = _mm256_castps256_ps128(v.value);
    __m128 high = _mm256_extractf128_ps(v.value, 1);
    __m128 sum  = _mm_add_ps(low, high);
    sum = _mm_hadd_ps(sum, sum);
    sum = _mm_hadd_ps(sum, sum);
    return _mm_cvtss_f32(sum);
}

} // namespace Math
} // namespace SanmapGen

#else  // no SIMD backend — use the portable scalar fallback
#include "FloatVector_Scalar_MATH.h"
#endif
