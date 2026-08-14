// RadialClearance_MATH_Test.cpp — acceptance test for RadialClearance_MATH (M0-5a).
//   g++ -O2 -std=c++17 RadialClearance_MATH_Test.cpp -o t && ./t
#include "RadialClearance_MATH.h"
#include <cstdio>
#include <cmath>
#include <vector>

using namespace SanmapGen::Math;

int main() {
    int failures = 0;
    const int width = 128, height = 128, cx = 64, cy = 64;
    std::vector<float> field(width * height);

    // A flat disk of radius 20 (height 0) surrounded by an out-of-band wall (height 1000).
    const int diskRadius = 20;
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) {
            double d = std::sqrt(double((x-cx)*(x-cx) + (y-cy)*(y-cy)));
            field[y*width + x] = (d <= diskRadius) ? 0.0f : 1000.0f;
        }

    int exact = ScoreRadialClearance(field.data(), width, height, cx, cy, -1.0f, 1.0f, 1.0f, 60, 1);
    if (exact < 18 || exact > 20) { std::printf("FAIL exact clearance = %d (expected ~20)\n", exact); ++failures; }

    int stochastic = ScoreRadialClearanceStochastic(field.data(), width, height, cx, cy, -1.0f, 1.0f, 1.0f, 60, 1, 777u);
    if (stochastic < 15 || stochastic > 20) { std::printf("FAIL stochastic clearance = %d\n", stochastic); ++failures; }

    // Determinism: identical inputs -> identical output.
    int stochastic2 = ScoreRadialClearanceStochastic(field.data(), width, height, cx, cy, -1.0f, 1.0f, 1.0f, 60, 1, 777u);
    if (stochastic != stochastic2) { std::printf("FAIL stochastic not deterministic (%d vs %d)\n", stochastic, stochastic2); ++failures; }

    // Fully flat field: clearance is bounded only by maxSearchRadius (no obstacle, no edge hit).
    std::vector<float> flat(width * height, 0.0f);
    int flatClear = ScoreRadialClearance(flat.data(), width, height, cx, cy, -1.0f, 1.0f, 1.0f, 50, 1);
    if (flatClear != 50) { std::printf("FAIL flat clearance = %d (expected 50)\n", flatClear); ++failures; }

    std::printf("exact=%d stochastic=%d flat=%d\n", exact, stochastic, flatClear);
    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}
