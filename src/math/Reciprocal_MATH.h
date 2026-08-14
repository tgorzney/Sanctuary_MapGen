// Reciprocal_MATH.h — fast reciprocal-square-root and exact reciprocal.
// Layer: MATH. rsqrt via bit-hack seed + Newton-Raphson (genuinely beats sqrt+divide).
// Deterministic (pure float + integer ops, no libm). Accuracy tiers per §4:
//   ReciprocalSquareRootApproximate -> Visual   (one Newton step, ~1.7e-3)
//   ReciprocalSquareRoot            -> Accurate  (two Newton steps, ~1e-6)
//   Reciprocal                      -> Exact     (scalar divide; note below)
#pragma once
#include <cstdint>
#include <cstring>

namespace SanmapGen {
namespace Math {
namespace ReciprocalDetail {
    inline float BitsToFloat(uint32_t bits)  { float value; std::memcpy(&value, &bits, 4); return value; }
    inline uint32_t FloatToBits(float value) { uint32_t bits; std::memcpy(&bits, &value, 4); return bits; }
}

// 1/sqrt(value). Accurate (~1e-6 relative). value must be > 0.
inline float ReciprocalSquareRoot(float value) {
    float half = value * 0.5f;
    float estimate = ReciprocalDetail::BitsToFloat(0x5f3759dfu - (ReciprocalDetail::FloatToBits(value) >> 1));
    estimate = estimate * (1.5f - half * estimate * estimate);   // Newton step 1
    estimate = estimate * (1.5f - half * estimate * estimate);   // Newton step 2
    return estimate;
}

// 1/sqrt(value). Visual (~1.7e-3 relative) — one Newton step, cheapest. value > 0.
inline float ReciprocalSquareRootApproximate(float value) {
    float half = value * 0.5f;
    float estimate = ReciprocalDetail::BitsToFloat(0x5f3759dfu - (ReciprocalDetail::FloatToBits(value) >> 1));
    estimate = estimate * (1.5f - half * estimate * estimate);   // single Newton step
    return estimate;
}

// 1/value. Exact by design: on modern hardware a scalar divide is as fast as a
// bit-hack approximation and fully accurate, so approximating it scalar-side would
// be a pessimization. The Visual-class approximate reciprocal is the SIMD path —
// FloatVector_MATH::ReciprocalApproximate (hardware rcpps). value must be != 0.
inline float Reciprocal(float value) {
    return 1.0f / value;
}

} // namespace Math
} // namespace SanmapGen
