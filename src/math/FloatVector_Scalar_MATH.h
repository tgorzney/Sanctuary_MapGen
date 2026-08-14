// FloatVector_Scalar_MATH.h — portable scalar fallback for FloatVector.
// Included by FloatVector_MATH.h only when no SIMD backend is available.
// Same public API and 8-lane width as the AVX2 path, so callers never change.
#pragma once
#include <cstdint>
#include <cmath>
#include <cstring>

namespace SanmapGen {
namespace Math {

struct FloatVector {
    static constexpr int laneCount = 8;
    float lane[laneCount];

    FloatVector() { for (int index = 0; index < laneCount; ++index) lane[index] = 0.0f; }

    static FloatVector Broadcast(float scalar) {
        FloatVector result;
        for (int index = 0; index < laneCount; ++index) result.lane[index] = scalar;
        return result;
    }
    static FloatVector Load(const float* source) {
        FloatVector result;
        for (int index = 0; index < laneCount; ++index) result.lane[index] = source[index];
        return result;
    }
    void Store(float* destination) const {
        for (int index = 0; index < laneCount; ++index) destination[index] = lane[index];
    }

    FloatVector operator+(const FloatVector& other) const { FloatVector r; for (int i=0;i<laneCount;++i) r.lane[i]=lane[i]+other.lane[i]; return r; }
    FloatVector operator-(const FloatVector& other) const { FloatVector r; for (int i=0;i<laneCount;++i) r.lane[i]=lane[i]-other.lane[i]; return r; }
    FloatVector operator*(const FloatVector& other) const { FloatVector r; for (int i=0;i<laneCount;++i) r.lane[i]=lane[i]*other.lane[i]; return r; }
    FloatVector operator/(const FloatVector& other) const { FloatVector r; for (int i=0;i<laneCount;++i) r.lane[i]=lane[i]/other.lane[i]; return r; }
};

inline FloatVector Minimum(const FloatVector& a, const FloatVector& b) { FloatVector r; for (int i=0;i<8;++i) r.lane[i]=a.lane[i]<b.lane[i]?a.lane[i]:b.lane[i]; return r; }
inline FloatVector Maximum(const FloatVector& a, const FloatVector& b) { FloatVector r; for (int i=0;i<8;++i) r.lane[i]=a.lane[i]>b.lane[i]?a.lane[i]:b.lane[i]; return r; }
inline FloatVector SquareRoot(const FloatVector& a) { FloatVector r; for (int i=0;i<8;++i) r.lane[i]=std::sqrt(a.lane[i]); return r; }
inline FloatVector ReciprocalApproximate(const FloatVector& a) { FloatVector r; for (int i=0;i<8;++i) r.lane[i]=1.0f/a.lane[i]; return r; }
inline FloatVector FusedMultiplyAdd(const FloatVector& a, const FloatVector& b, const FloatVector& c) { FloatVector r; for (int i=0;i<8;++i) r.lane[i]=std::fma(a.lane[i],b.lane[i],c.lane[i]); return r; }
inline FloatVector CompareLessOrEqual(const FloatVector& a, const FloatVector& b) { FloatVector r; for (int i=0;i<8;++i) r.lane[i]=(a.lane[i]<=b.lane[i])?-1.0f:0.0f; return r; }
inline FloatVector Select(const FloatVector& mask, const FloatVector& whenTrue, const FloatVector& whenFalse) {
    FloatVector r;
    for (int i=0;i<8;++i) { uint32_t bits; std::memcpy(&bits,&mask.lane[i],4); r.lane[i]=(bits & 0x80000000u)?whenTrue.lane[i]:whenFalse.lane[i]; }
    return r;
}
inline float HorizontalSum(const FloatVector& v) { float s=0.0f; for (int i=0;i<8;++i) s+=v.lane[i]; return s; }

} // namespace Math
} // namespace SanmapGen
