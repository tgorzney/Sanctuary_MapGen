// Application_PreviewRamps_UI_Test.cpp — STEP267 acceptance: MakeSlopeRamp()'s stop positions.
// Pure: no imgui frame, no window, no GL context.
#include "Application_PreviewRamps_UI.h"
#include <cstdio>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

bool NearlyEqual(float left, float right, float tolerance) {
    const float difference = left - right;
    return (difference < 0.0f ? -difference : difference) <= tolerance;
}

// STEP267: the yellow caution stop moved from 2 deg to 9 deg; green stays at 0 deg and red stays
// at 30 deg (the Steep Angle default this ticket also moves in Application_PreviewSetup_UI.cpp).
void RunSlopeRampStopChecks() {
    const Params::GradientRamp ramp = MakeSlopeRamp();
    Check(ramp.stops.size() == 3, "the slope ramp carries exactly three stops");
    if (ramp.stops.size() != 3) return;

    Check(NearlyEqual(ramp.stops[0].location, 0.0f, 1.0e-6f), "green sits at 0 degrees");
    Check(NearlyEqual(ramp.stops[1].location, 9.0f / 90.0f, 1.0e-6f),
          "yellow moved to 9 degrees (STEP267)");
    Check(NearlyEqual(ramp.stops[2].location, 30.0f / 90.0f, 1.0e-6f),
          "red stays at 30 degrees");

    Check(ramp.stops[0].location < ramp.stops[1].location &&
          ramp.stops[1].location < ramp.stops[2].location,
          "the stops are in ascending order: green, yellow, red");
}

} // namespace

int main() {
    RunSlopeRampStopChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
