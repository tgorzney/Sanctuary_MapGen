// GradientLut_UI_Test.cpp — acceptance test for the M4-2 color-ramp LUT bake.
//   cl /std:c++17 /EHsc GradientLut_UI.cpp GradientLut_UI_Test.cpp   (or g++ -O2 -std=c++17)
// Pure CPU, no GL, no DATA — the whole unit is sandbox-testable by design (ARCH §8.1).
#include "GradientLut_UI.h"
#include <cstdio>

using namespace SanmapGen;

static int failureCount = 0;

static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

static bool IsNear(float value, float expected) {
    const float difference = value - expected;
    return difference < 1.0e-6f && difference > -1.0e-6f;
}

static Params::GradientStop MakeStop(float location, float grey, float alpha = 1.0f) {
    Params::GradientStop stop;
    stop.location = location;
    stop.color[0] = grey; stop.color[1] = grey; stop.color[2] = grey; stop.color[3] = alpha;
    return stop;
}

static bool EntryEquals(const std::vector<float>& lookupTable, int entry, float grey, float alpha) {
    const float* const values = &lookupTable[static_cast<std::size_t>(entry) * 4];
    return values[0] == grey && values[1] == grey && values[2] == grey && values[3] == alpha;
}

// A 2-stop black -> white ramp, the acceptance case.
static Params::GradientRamp MakeBlackToWhiteRamp(bool bSmoothInterpolation) {
    Params::GradientRamp ramp;
    ramp.stops.push_back(MakeStop(0.0f, 0.0f));
    ramp.stops.push_back(MakeStop(1.0f, 1.0f));
    ramp.bSmoothInterpolation = bSmoothInterpolation;
    return ramp;
}

static void TestLinearGreyRamp() {
    const Params::GradientRamp ramp = MakeBlackToWhiteRamp(false);
    const std::vector<float> lookupTable = Ui::BakeGradientLut(ramp, 5);
    Check(lookupTable.size() == 5u * 4u, "linear entry count");
    Check(EntryEquals(lookupTable, 0, 0.0f, 1.0f), "linear low endpoint exact");
    Check(EntryEquals(lookupTable, 4, 1.0f, 1.0f), "linear high endpoint exact");
    Check(IsNear(lookupTable[2 * 4], 0.5f), "linear midpoint ~0.5");
    for (int entry = 1; entry < 5; ++entry)                        // strictly increasing grey
        Check(lookupTable[entry * 4] > lookupTable[(entry - 1) * 4], "linear monotonic");
    for (int entry = 0; entry < 5; ++entry)                        // channels agree, alpha held
        Check(IsNear(lookupTable[entry * 4], lookupTable[entry * 4 + 2]) &&
              IsNear(lookupTable[entry * 4 + 3], 1.0f), "linear channels");
    // The default resolution comes from the ramp, never from the call site.
    Check(Ui::BakeGradientLut(ramp).size() == 256u * 4u, "default resolution from ramp");
    Check(Ui::BakeGradientLut(ramp, 16).size() == 16u * 4u, "explicit resolution override");
}

static void TestSmoothFlagChangesTheCurve() {
    const std::vector<float> smoothTable = Ui::BakeGradientLut(MakeBlackToWhiteRamp(true), 5);
    const std::vector<float> linearTable = Ui::BakeGradientLut(MakeBlackToWhiteRamp(false), 5);
    Check(EntryEquals(smoothTable, 0, 0.0f, 1.0f), "smooth low endpoint exact");
    Check(EntryEquals(smoothTable, 4, 1.0f, 1.0f), "smooth high endpoint exact");
    // Smoothstep is symmetric, so the midpoint of a symmetric ramp stays 0.5...
    Check(IsNear(smoothTable[2 * 4], 0.5f), "smooth midpoint ~0.5");
    // ...while the quarter point is pulled toward the low stop: 0.25^2 * (3 - 0.5) = 0.15625.
    Check(IsNear(smoothTable[1 * 4], 0.15625f), "smooth quarter point");
    Check(IsNear(linearTable[1 * 4], 0.25f), "linear quarter point");
    Check(smoothTable[1 * 4] < linearTable[1 * 4], "smooth flag changes the curve");
    // An off-center stop moves the LUT midpoint, and the smooth flag moves it again.
    Params::GradientRamp offsetRamp = MakeBlackToWhiteRamp(true);
    offsetRamp.stops[1].location = 0.75f;
    const std::vector<float> offsetSmooth = Ui::BakeGradientLut(offsetRamp, 5);
    offsetRamp.bSmoothInterpolation = false;
    const std::vector<float> offsetLinear = Ui::BakeGradientLut(offsetRamp, 5);
    Check(IsNear(offsetLinear[2 * 4], 2.0f / 3.0f), "offset linear midpoint");
    Check(IsNear(offsetSmooth[2 * 4], 0.7407407f), "offset smooth midpoint");
    Check(EntryEquals(offsetSmooth, 4, 1.0f, 1.0f), "past-last-stop holds the last color");
}

static void TestUnsortedMatchesPreSorted() {
    Params::GradientRamp unsortedRamp;
    unsortedRamp.stops.push_back(MakeStop(1.0f, 1.0f));
    unsortedRamp.stops.push_back(MakeStop(0.25f, 0.75f));
    unsortedRamp.stops.push_back(MakeStop(0.0f, 0.0f));
    unsortedRamp.stops.push_back(MakeStop(0.5f, 0.25f));
    Params::GradientRamp sortedRamp;
    sortedRamp.stops.push_back(MakeStop(0.0f, 0.0f));
    sortedRamp.stops.push_back(MakeStop(0.25f, 0.75f));
    sortedRamp.stops.push_back(MakeStop(0.5f, 0.25f));
    sortedRamp.stops.push_back(MakeStop(1.0f, 1.0f));
    // 65 entries so sample positions are i/64 and entry 16 lands exactly on the 0.25 stop.
    const std::vector<float> unsortedTable = Ui::BakeGradientLut(unsortedRamp, 65);
    const std::vector<float> sortedTable = Ui::BakeGradientLut(sortedRamp, 65);
    Check(unsortedTable == sortedTable, "unsorted stops match pre-sorted");
    Check(IsNear(unsortedTable[16 * 4], 0.75f), "stop location hit exactly");

    // The input is never mutated by the defensive sort/clamp (Constitution §6).
    Check(unsortedRamp.stops.size() == 4u && unsortedRamp.stops[0].location == 1.0f &&
          unsortedRamp.stops[1].location == 0.25f && unsortedRamp.stops[2].location == 0.0f &&
          unsortedRamp.stops[3].location == 0.5f, "input stop order unmodified");
    Check(unsortedRamp.stops[0].color[0] == 1.0f && unsortedRamp.stops[2].color[0] == 0.0f &&
          unsortedRamp.name == "New Ramp" && unsortedRamp.lookupResolution == 256 &&
          unsortedRamp.bSmoothInterpolation, "input ramp fields unmodified");
}

static void TestDegenerateInputIsSafe() {
    const Params::GradientRamp emptyRamp;                          // no stops at all
    const std::vector<float> emptyTable = Ui::BakeGradientLut(emptyRamp, 8);
    Check(emptyTable.size() == 8u * 4u, "empty ramp entry count");
    for (int entry = 0; entry < 8; ++entry)                        // constant, the PARAMS default
        Check(EntryEquals(emptyTable, entry, 1.0f, 1.0f), "empty ramp constant LUT");

    Params::GradientRamp singleStopRamp;
    singleStopRamp.stops.push_back(MakeStop(0.7f, 0.25f, 0.5f));
    const std::vector<float> singleTable = Ui::BakeGradientLut(singleStopRamp, 8);
    for (int entry = 0; entry < 8; ++entry)
        Check(EntryEquals(singleTable, entry, 0.25f, 0.5f), "one-stop constant LUT");

    // Nonsense resolutions clamp instead of dividing by zero or allocating wildly.
    Check(Ui::BakeGradientLut(MakeBlackToWhiteRamp(true), 1).size() == 4u, "resolution 1 safe");
    Check(Ui::BakeGradientLut(MakeBlackToWhiteRamp(true), 0).size() == 4u, "resolution 0 clamped");
    // Out-of-range and duplicate stop locations clamp/collapse without a crash.
    Params::GradientRamp wildRamp;
    wildRamp.stops.push_back(MakeStop(4.0f, 1.0f));
    wildRamp.stops.push_back(MakeStop(-3.0f, 0.0f));
    wildRamp.stops.push_back(MakeStop(-3.0f, 0.0f));
    const std::vector<float> wildTable = Ui::BakeGradientLut(wildRamp, 5);
    Check(EntryEquals(wildTable, 0, 0.0f, 1.0f) && EntryEquals(wildTable, 4, 1.0f, 1.0f),
          "out-of-range locations clamped");
    Check(wildRamp.stops[0].location == 4.0f && wildRamp.stops[1].location == -3.0f,
          "wild input unmodified");
}

int main() {
    TestLinearGreyRamp();
    TestSmoothFlagChangesTheCurve();
    TestUnsortedMatchesPreSorted();
    TestDegenerateInputIsSafe();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
