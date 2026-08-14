// Reciprocal_MATH_Test.cpp — acceptance test for Reciprocal_MATH (M0-4a).
//   g++ -O2 -std=c++17 Reciprocal_MATH_Test.cpp -o t && ./t
#include "Reciprocal_MATH.h"
#include <cstdio>
#include <cmath>

using namespace SanmapGen::Math;

static double maxRelative(float (*fn)(float), double (*ref)(double), double lo, double hi) {
    double worst = 0.0;
    for (double x = lo; x <= hi; x *= 1.0009) {
        double got = fn(static_cast<float>(x));
        double want = ref(x);
        double rel = std::fabs(got - want) / std::fabs(want);
        if (rel > worst) worst = rel;
    }
    return worst;
}
static double refRsqrt(double x) { return 1.0 / std::sqrt(x); }
static double refRecip(double x) { return 1.0 / x; }

int main() {
    int failures = 0;
    double rsqrtAccurate = maxRelative(ReciprocalSquareRoot,            refRsqrt, 1e-3, 1e4);
    double rsqrtVisual   = maxRelative(ReciprocalSquareRootApproximate, refRsqrt, 1e-3, 1e4);

    if (rsqrtAccurate > 5e-6) { std::printf("FAIL rsqrt accurate %.3e\n", rsqrtAccurate); ++failures; }
    if (rsqrtVisual   > 2e-3) { std::printf("FAIL rsqrt visual %.3e\n",   rsqrtVisual);   ++failures; }

    // Reciprocal is Exact by design (scalar divide) — must equal 1.0f/x bit-for-bit.
    for (double x = 1e-3; x <= 1e4; x *= 1.0009) {
        float xf = static_cast<float>(x);
        if (Reciprocal(xf) != 1.0f / xf) { std::printf("FAIL reciprocal not exact at %g\n", x); ++failures; break; }
    }

    std::printf("rsqrt: accurate=%.3e visual=%.3e | reciprocal: exact\n", rsqrtAccurate, rsqrtVisual);
    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}
