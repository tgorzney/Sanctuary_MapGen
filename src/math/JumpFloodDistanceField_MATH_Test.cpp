// JumpFloodDistanceField_MATH_Test.cpp — acceptance test (M0-5b).
//   g++ -O2 -std=c++17 JumpFloodDistanceField_MATH_Test.cpp -o t && ./t
// Verifies the Jump-Flood result against a brute-force nearest-seed distance.
#include "JumpFloodDistanceField_MATH.h"
#include <cstdio>
#include <cmath>
#include <vector>

using namespace SanmapGen::Math;

int main() {
    int failures = 0;
    const int width = 96, height = 96;
    const float maxDistance = 1000.0f;
    std::vector<float> field(width * height, 0.0f);   // flat (band-valid, no gradient)

    // Make a handful of obstacle cells (out of band) as seeds.
    struct P { int x, y; };
    P seeds[] = { {10,10}, {80,20}, {45,70}, {90,90}, {5,88} };
    for (auto& s : seeds) field[s.y*width + s.x] = 5000.0f;

    // Huge gradient tolerance so ONLY the out-of-band obstacle cells seed (no gradient
    // halo) — makes the brute-force reference over the 5 explicit seeds exact.
    std::vector<float> got(width * height);
    ComputeJumpFloodDistanceField(field.data(), width, height, -1.0f, 1.0f, 1e9f, maxDistance, got.data());

    // Brute-force reference: nearest seed distance for every cell.
    double maxError = 0.0;
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) {
            double best = 1e18;
            for (auto& s : seeds) {
                double d = std::sqrt(double((x-s.x)*(x-s.x) + (y-s.y)*(y-s.y)));
                if (d < best) best = d;
            }
            double err = std::fabs(best - got[y*width + x]);
            if (err > maxError) maxError = err;
        }

    if (maxError > 1e-3) { std::printf("FAIL max distance error %.4f\n", maxError); ++failures; }

    // Seeds themselves have distance 0.
    for (auto& s : seeds)
        if (got[s.y*width + s.x] != 0.0f) { std::printf("FAIL seed (%d,%d) dist=%f\n", s.x, s.y, got[s.y*width+s.x]); ++failures; }

    std::printf("max error vs brute force = %.4e\n", maxError);
    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}
