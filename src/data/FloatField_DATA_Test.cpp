// FloatField_DATA_Test.cpp — acceptance test for FloatField_DATA (M1-1).
//   g++ -O2 -std=c++17 -fsanitize=address,undefined FloatField_DATA_Test.cpp -o t && ./t
#include "FloatField_DATA.h"
#include <cstdio>
#include <cmath>

using namespace SanmapGen::Data;

static int failures = 0;
static void check(bool ok, const char* label) { if (!ok) { std::printf("FAIL: %s\n", label); ++failures; } }

int main() {
    FloatField field(4, 3, 1.5f);
    check(field.Width() == 4 && field.Height() == 3, "dimensions");
    check(field.CellCount() == 12, "cell count");
    check(field.Get(0, 0) == 1.5f && field.Get(3, 2) == 1.5f, "fill value");

    field.Set(2, 1, 9.0f);
    check(field.Get(2, 1) == 9.0f, "set/get");
    field.At(0, 0) = -4.0f;
    check(field.Get(0, 0) == -4.0f, "At() write");

    // Row-major contiguity: (x,y) at index y*width + x.
    check(&field.At(2, 1) - field.Data() == 1 * 4 + 2, "row-major index");

    field.Fill(0.0f);
    check(field.Get(2, 1) == 0.0f, "fill clears");

    // Bilinear: exact at cell centers, midpoint average between them.
    FloatField ramp(2, 2, 0.0f);
    ramp.Set(0, 0, 0.0f); ramp.Set(1, 0, 10.0f);
    ramp.Set(0, 1, 20.0f); ramp.Set(1, 1, 30.0f);
    check(ramp.SampleBilinear(0.0f, 0.0f) == 0.0f, "bilinear corner");
    check(ramp.SampleBilinear(1.0f, 0.0f) == 10.0f, "bilinear corner 2");
    check(std::fabs(ramp.SampleBilinear(0.5f, 0.0f) - 5.0f) < 1e-6f, "bilinear x-mid");
    check(std::fabs(ramp.SampleBilinear(0.5f, 0.5f) - 15.0f) < 1e-6f, "bilinear center");
    // Out-of-range clamps to the edge.
    check(ramp.SampleBilinear(5.0f, 5.0f) == 30.0f, "bilinear clamp");

    // Resize reshapes and refills.
    field.Resize(8, 8, 2.0f);
    check(field.Width() == 8 && field.CellCount() == 64 && field.Get(7, 7) == 2.0f, "resize");

    FloatField empty;
    check(empty.IsEmpty() && empty.SampleBilinear(0, 0) == 0.0f, "empty safe");

    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}
