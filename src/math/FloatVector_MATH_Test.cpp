// FloatVector_MATH_Test.cpp — acceptance test for FloatVector_MATH (Work-Order M0-2).
// Verifies each op against a plain scalar reference. Run the same source twice:
//   scalar backend:  g++ -O2 -std=c++17 FloatVector_MATH_Test.cpp -o t && ./t
//   AVX2 backend:    g++ -O2 -std=c++17 -mavx2 -mfma FloatVector_MATH_Test.cpp -o t && ./t
// Both must print ALL PASS — proving both backends are correct and agree.
#include "FloatVector_MATH.h"
#include <cstdio>
#include <cmath>
#include <cstdint>

using namespace SanmapGen::Math;

static int failures = 0;
static void check(bool ok, const char* label) { if (!ok) { std::printf("FAIL: %s\n", label); ++failures; } }

static bool close(float a, float b, float relTol) {
    float diff = std::fabs(a - b);
    float scale = std::fabs(a) > std::fabs(b) ? std::fabs(a) : std::fabs(b);
    return diff <= relTol * (scale + 1.0f);
}

int main() {
    float a[8] = { 1.0f, 2.5f, -3.0f, 4.25f,  0.5f, 10.0f, -7.5f, 100.0f };
    float b[8] = { 4.0f, 0.5f,  2.0f, -1.0f,  9.0f,  3.0f,  6.0f,   0.25f };
    float c[8] = { 0.1f, 1.0f,  2.0f,  3.0f, -1.0f,  0.0f,  5.0f,  -2.0f };

    FloatVector va = FloatVector::Load(a), vb = FloatVector::Load(b), vc = FloatVector::Load(c);
    float out[8];

    (va + vb).Store(out); for (int i=0;i<8;++i) check(close(out[i], a[i]+b[i], 1e-6f), "add");
    (va - vb).Store(out); for (int i=0;i<8;++i) check(close(out[i], a[i]-b[i], 1e-6f), "sub");
    (va * vb).Store(out); for (int i=0;i<8;++i) check(close(out[i], a[i]*b[i], 1e-6f), "mul");
    (va / vb).Store(out); for (int i=0;i<8;++i) check(close(out[i], a[i]/b[i], 1e-6f), "div");
    Minimum(va, vb).Store(out); for (int i=0;i<8;++i) check(out[i]==(a[i]<b[i]?a[i]:b[i]), "min");
    Maximum(va, vb).Store(out); for (int i=0;i<8;++i) check(out[i]==(a[i]>b[i]?a[i]:b[i]), "max");

    float pos[8] = { 1.0f, 4.0f, 9.0f, 16.0f, 25.0f, 2.0f, 0.25f, 121.0f };
    FloatVector vp = FloatVector::Load(pos);
    SquareRoot(vp).Store(out); for (int i=0;i<8;++i) check(close(out[i], std::sqrt(pos[i]), 1e-6f), "sqrt");
    ReciprocalApproximate(vp).Store(out); for (int i=0;i<8;++i) check(close(out[i], 1.0f/pos[i], 2e-3f), "reciprocal(approx)");
    FusedMultiplyAdd(va, vb, vc).Store(out); for (int i=0;i<8;++i) check(close(out[i], std::fma(a[i],b[i],c[i]), 1e-6f), "fma");

    // Select via compare mask: pick a where a<=b, else b.
    FloatVector mask = CompareLessOrEqual(va, vb);
    Select(mask, va, vb).Store(out);
    for (int i=0;i<8;++i) check(out[i]==((a[i]<=b[i])?a[i]:b[i]), "select/compare");

    float sum = HorizontalSum(va);
    float ref = 0.0f; for (int i=0;i<8;++i) ref += a[i];
    check(close(sum, ref, 1e-6f), "horizontal-sum");

    check(FloatVector::laneCount == 8, "laneCount==8");

#if defined(__AVX2__)
    std::printf("[AVX2 backend] ");
#else
    std::printf("[scalar backend] ");
#endif
    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}
