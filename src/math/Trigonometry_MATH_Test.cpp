// Trigonometry_MATH_Test.cpp — acceptance test for Trigonometry_MATH (M0-3).
//   g++ -O2 -std=c++17 Trigonometry_MATH_Test.cpp -o t && ./t
// Compares the portable Sine/Cosine against std::sin/std::cos and checks the
// Pythagorean identity. Exits 0 on success.
#include "Trigonometry_MATH.h"
#include <cstdio>
#include <cmath>

using namespace SanmapGen::Math;

int main() {
    int failures = 0;
    double maxSineError = 0.0, maxCosineError = 0.0, maxIdentityError = 0.0;

    // Sweep a practical angle range at fine resolution.
    for (double angle = -200.0; angle <= 200.0; angle += 0.0007) {
        float a = static_cast<float>(angle);
        double sineError   = std::fabs((double)Sine(a)   - std::sin(angle));
        double cosineError = std::fabs((double)Cosine(a) - std::cos(angle));
        if (sineError   > maxSineError)   maxSineError   = sineError;
        if (cosineError > maxCosineError) maxCosineError = cosineError;
        double s = Sine(a), c = Cosine(a);
        double identityError = std::fabs(s*s + c*c - 1.0);
        if (identityError > maxIdentityError) maxIdentityError = identityError;
    }

    // Exact-ish anchor values.
    const float pi = 3.14159265358979323846f;
    struct { float in; float sin; float cos; } anchors[] = {
        {0.0f, 0.0f, 1.0f}, {pi/2, 1.0f, 0.0f}, {pi, 0.0f, -1.0f}, {3*pi/2, -1.0f, 0.0f},
    };
    for (auto& a : anchors) {
        if (std::fabs(Sine(a.in)   - a.sin) > 2e-6f) { std::printf("FAIL anchor sin(%g)\n", a.in);  ++failures; }
        if (std::fabs(Cosine(a.in) - a.cos) > 2e-6f) { std::printf("FAIL anchor cos(%g)\n", a.in);  ++failures; }
    }

    const double tolerance = 1.2e-5;  // Accurate class over the wide ±200 sweep
                                      // (near-origin is ~1e-7; float range reduction
                                      // widens the bound at large |radians|)
    if (maxSineError   > tolerance) { std::printf("FAIL sine max error %.3e\n", maxSineError);     ++failures; }
    if (maxCosineError > tolerance) { std::printf("FAIL cosine max error %.3e\n", maxCosineError); ++failures; }
    if (maxIdentityError > 1e-5)    { std::printf("FAIL identity max error %.3e\n", maxIdentityError); ++failures; }

    std::printf("max sine err=%.3e  max cosine err=%.3e  max identity err=%.3e\n",
                maxSineError, maxCosineError, maxIdentityError);
    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}
