// Trigonometry_MATH.h — portable, deterministic single-precision sine/cosine.
// Layer: MATH. Accuracy class: Accurate (~1e-7 near the origin; range-reduction
// error grows for very large |radians|). Uses minimax polynomials with cephes-style
// extended-precision range reduction — NO std::sin/cos and no libm call, so the
// result is bit-identical across machines/compilers (the transcendental the
// deterministic path requires, DETERMINISM_SPEC). Pure float + int ops; the
// deterministic build must disable fast-math reassociation/contraction.
#pragma once

namespace SanmapGen {
namespace Math {
namespace TrigonometryDetail {

constexpr float fourOverPi    = 1.27323954473516f;
constexpr float quarterPiHigh = 0.78515625f;                 // pi/4 split into three
constexpr float quarterPiMid  = 2.4187564849853515625e-4f;   // parts for extended-
constexpr float quarterPiLow  = 3.77489497744594108e-8f;     // precision reduction
constexpr float sineCoefficient0   = -1.9515295891e-4f;
constexpr float sineCoefficient1   =  8.3321608736e-3f;
constexpr float sineCoefficient2   = -1.6666654611e-1f;
constexpr float cosineCoefficient0 =  2.443315711809948e-5f;
constexpr float cosineCoefficient1 = -1.388731625493765e-3f;
constexpr float cosineCoefficient2 =  4.166664568298827e-2f;

inline float SinePolynomial(float angleSquared, float reducedAngle) {
    return ((sineCoefficient0 * angleSquared + sineCoefficient1) * angleSquared + sineCoefficient2)
           * angleSquared * reducedAngle + reducedAngle;
}
inline float CosinePolynomial(float angleSquared) {
    float value = ((cosineCoefficient0 * angleSquared + cosineCoefficient1) * angleSquared + cosineCoefficient2)
                  * angleSquared * angleSquared;
    return value - 0.5f * angleSquared + 1.0f;
}

} // namespace TrigonometryDetail

inline float Sine(float radians) {
    using namespace TrigonometryDetail;
    int sign = 1;
    float angle = radians;
    if (radians < 0.0f) { sign = -1; angle = -radians; }
    int octant = static_cast<int>(fourOverPi * angle);
    float octantFloat = static_cast<float>(octant);
    if (octant & 1) { octant += 1; octantFloat += 1.0f; }
    octant &= 7;
    if (octant > 3) { sign = -sign; octant -= 4; }
    float reduced = ((angle - octantFloat * quarterPiHigh) - octantFloat * quarterPiMid) - octantFloat * quarterPiLow;
    float squared = reduced * reduced;
    float result = (octant == 1 || octant == 2) ? CosinePolynomial(squared) : SinePolynomial(squared, reduced);
    return sign < 0 ? -result : result;
}

inline float Cosine(float radians) {
    using namespace TrigonometryDetail;
    int sign = 1;
    float angle = radians < 0.0f ? -radians : radians;
    int octant = static_cast<int>(fourOverPi * angle);
    float octantFloat = static_cast<float>(octant);
    if (octant & 1) { octant += 1; octantFloat += 1.0f; }
    octant &= 7;
    if (octant > 3) { octant -= 4; sign = -sign; }
    if (octant > 1) { sign = -sign; }
    float reduced = ((angle - octantFloat * quarterPiHigh) - octantFloat * quarterPiMid) - octantFloat * quarterPiLow;
    float squared = reduced * reduced;
    float result = (octant == 1 || octant == 2) ? SinePolynomial(squared, reduced) : CosinePolynomial(squared);
    return sign < 0 ? -result : result;
}

} // namespace Math
} // namespace SanmapGen
