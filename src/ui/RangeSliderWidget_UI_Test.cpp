// RangeSliderWidget_UI_Test.cpp — acceptance test for the M5-1 dual-handle range slider.
// Covers the range/clamping math and the full press-drag-release interaction, driven by a
// synthetic pointer sequence in value space. No imgui frame, no window, no GL: the interaction
// is pure by construction (RangeSliderWidget_UI.h). The ImDrawList rectangles themselves are a
// by-eye check against a live frame — nothing here asserts on pixels.
#include "RangeSliderWidget_UI.h"
#include <cstdio>

using namespace SanmapGen;

static int failureCount = 0;

static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

static bool IsNear(float value, float expected, float tolerance = 1.0e-5f) {
    const float difference = value - expected;
    return difference < tolerance && difference > -tolerance;
}

static Ui::RangeSliderBounds MakeBounds(float lowerLimit, float upperLimit, float minimumSeparation) {
    Ui::RangeSliderBounds bounds;
    bounds.lowerLimit = lowerLimit; bounds.upperLimit = upperLimit;
    bounds.minimumSeparation = minimumSeparation;
    return bounds;
}

static void TestClampingHoldsTheContract() {
    const Ui::RangeSliderBounds bounds = MakeBounds(0.0f, 1.0f, 0.1f);
    Ui::RangeSliderValues outside = Ui::ClampRangeSliderValues({-5.0f, 9.0f}, bounds);
    Check(IsNear(outside.minimumValue, 0.0f) && IsNear(outside.maximumValue, 1.0f), "values clamp into the limits");

    Ui::RangeSliderValues inverted = Ui::ClampRangeSliderValues({0.9f, 0.2f}, bounds);
    Check(IsNear(inverted.minimumValue, 0.2f) && IsNear(inverted.maximumValue, 0.9f),
          "an inverted pair is put back in order without losing either value");

    Ui::RangeSliderValues tooClose = Ui::ClampRangeSliderValues({0.50f, 0.52f}, bounds);
    Check(IsNear(tooClose.maximumValue - tooClose.minimumValue, 0.1f), "separation is enforced");
    Ui::RangeSliderValues atTheTop = Ui::ClampRangeSliderValues({1.0f, 1.0f}, bounds);
    Check(IsNear(atTheTop.minimumValue, 0.9f) && IsNear(atTheTop.maximumValue, 1.0f),
          "separation at the top pushes the minimum down, never past the limit");

    // Degenerate input is repaired, not obeyed (Constitution §6).
    const Ui::RangeSliderBounds invertedLimits = MakeBounds(1.0f, 0.0f, -3.0f);
    Ui::RangeSliderValues repaired = Ui::ClampRangeSliderValues({0.4f, 0.6f}, invertedLimits);
    Check(IsNear(repaired.minimumValue, 0.4f) && IsNear(repaired.maximumValue, 0.6f), "inverted limits swap");
    const Ui::RangeSliderBounds impossible = MakeBounds(0.0f, 1.0f, 4.0f);   // separation wider than the track
    Ui::RangeSliderValues collapsed = Ui::ClampRangeSliderValues({0.4f, 0.6f}, impossible);
    Check(IsNear(collapsed.minimumValue, 0.0f) && IsNear(collapsed.maximumValue, 1.0f),
          "an impossible separation collapses onto the track instead of escaping it");
}

static void TestHandlesStopAtEachOtherWithoutShoving() {
    const Ui::RangeSliderBounds bounds = MakeBounds(0.0f, 1.0f, 0.05f);
    const Ui::RangeSliderValues start{0.2f, 0.8f};

    Ui::RangeSliderValues pushedUp = Ui::MoveRangeSliderHandle(start, bounds, Ui::RangeSliderHandle::Minimum, 0.95f);
    Check(IsNear(pushedUp.minimumValue, 0.75f), "the minimum handle stops at the separation gap");
    Check(IsNear(pushedUp.maximumValue, 0.8f), "and does not shove the maximum handle");

    Ui::RangeSliderValues pushedDown = Ui::MoveRangeSliderHandle(start, bounds, Ui::RangeSliderHandle::Maximum, -2.0f);
    Check(IsNear(pushedDown.maximumValue, 0.25f), "the maximum handle stops at the separation gap");
    Check(IsNear(pushedDown.minimumValue, 0.2f), "and does not shove the minimum handle");

    Ui::RangeSliderValues freeMove = Ui::MoveRangeSliderHandle(start, bounds, Ui::RangeSliderHandle::Minimum, 0.35f);
    Check(IsNear(freeMove.minimumValue, 0.35f) && IsNear(freeMove.maximumValue, 0.8f), "a legal move lands exactly");
    Ui::RangeSliderValues untouched = Ui::MoveRangeSliderHandle(start, bounds, Ui::RangeSliderHandle::None, 0.35f);
    Check(IsNear(untouched.minimumValue, 0.2f) && IsNear(untouched.maximumValue, 0.8f), "no handle, no move");
    // The drawn offset agrees with the values at both ends of the track.
    Check(IsNear(Ui::RangeSliderHandleOffset(0.0f, bounds, 110.0f, 10.0f), 0.0f), "low handle sits at the origin");
    Check(IsNear(Ui::RangeSliderHandleOffset(1.0f, bounds, 110.0f, 10.0f), 100.0f), "high handle sits one width in");
    Check(IsNear(Ui::RangeSliderHandleOffset(0.5f, bounds, 110.0f, 10.0f), 50.0f), "and is linear between");
    Check(IsNear(Ui::RangeSliderHandleOffset(0.5f, bounds, 8.0f, 10.0f), 0.0f), "a track narrower than a handle is safe");
}

// One synthetic drag of a handle: press at the first position, drag through the rest, release.
struct DragTally { int changeCount = 0; int commitCount = 0; bool bCommittedBeforeRelease = false; };

static DragTally DragHandle(Ui::RealtimeToggle& realtimeToggle, Ui::RangeSliderValues& values,
                            const Ui::RangeSliderBounds& bounds, Ui::RangeSliderHandle handle,
                            const float* pointerValues, int positionCount) {
    DragTally tally;
    Ui::RangeSliderPointerInput input;
    input.grabbedHandle = handle;
    for (int position = 0; position < positionCount; ++position) {
        input.pointerValue = pointerValues[position];
        const Ui::WidgetChange change = Ui::StepRangeSliderInteraction(realtimeToggle, values, bounds, input);
        if (change.bValueChanged) ++tally.changeCount;
        if (change.bCommitted) { ++tally.commitCount; tally.bCommittedBeforeRelease = true; }
    }
    input.grabbedHandle = Ui::RangeSliderHandle::None;                       // mouse-up frame
    const Ui::WidgetChange release = Ui::StepRangeSliderInteraction(realtimeToggle, values, bounds, input);
    if (release.bValueChanged) ++tally.changeCount;
    if (release.bCommitted) ++tally.commitCount;
    return tally;
}

static void TestDragDefersItsCommitUntilRelease() {
    const Ui::RangeSliderBounds bounds = MakeBounds(0.0f, 1.0f, 0.001f);
    const float pointerValues[4] = {0.70f, 0.60f, 0.60f, 0.55f};            // frame 2 does not move
    Ui::RangeSliderValues values{0.2f, 0.8f};
    Ui::RealtimeToggle realtimeToggle;                                       // RT off

    const DragTally tally = DragHandle(realtimeToggle, values, bounds, Ui::RangeSliderHandle::Maximum,
                                       pointerValues, 4);
    Check(IsNear(values.maximumValue, 0.55f), "the value tracks the pointer during the drag");
    Check(IsNear(values.minimumValue, 0.2f), "the other handle is untouched");
    Check(tally.changeCount == 3, "one live change per frame that actually moved");
    Check(!tally.bCommittedBeforeRelease, "no commit is paid for during the drag");
    Check(tally.commitCount == 1, "exactly one commit, on release");

    // Realtime on: the same drag commits on every moving frame instead.
    Ui::RangeSliderValues realtimeValues{0.2f, 0.8f};
    Ui::RealtimeToggle alwaysOnToggle(true);
    const DragTally realtimeTally = DragHandle(alwaysOnToggle, realtimeValues, bounds,
                                               Ui::RangeSliderHandle::Maximum, pointerValues, 4);
    Check(realtimeTally.changeCount == 3 && realtimeTally.commitCount == 3, "realtime commits live");
    Check(IsNear(realtimeValues.maximumValue, 0.55f), "and lands on the same value");
}

static void TestDragCannotCrossOrEscape() {
    const Ui::RangeSliderBounds bounds = MakeBounds(0.0f, 1.0f, 0.05f);
    const float pointerValues[3] = {0.5f, 0.9f, 4.0f};                       // dragged past the maximum handle
    Ui::RangeSliderValues values{0.2f, 0.8f};
    Ui::RealtimeToggle realtimeToggle;
    DragHandle(realtimeToggle, values, bounds, Ui::RangeSliderHandle::Minimum, pointerValues, 3);
    Check(IsNear(values.minimumValue, 0.75f), "a drag past the partner stops at the gap");
    Check(IsNear(values.maximumValue, 0.8f), "the partner never moves");

    // A numeric-field edit the draw path already applied still routes through the same commit.
    Ui::RangeSliderValues fieldValues{0.2f, 0.8f};
    Ui::RealtimeToggle fieldToggle;
    Ui::RangeSliderPointerInput fieldInput;
    fieldInput.bNumericFieldEdited = true;
    const Ui::WidgetChange typed = Ui::StepRangeSliderInteraction(fieldToggle, fieldValues, bounds, fieldInput);
    Check(typed.bValueChanged && typed.bCommitted, "a typed field edit commits immediately");
}

int main() {
    TestClampingHoldsTheContract();
    TestHandlesStopAtEachOtherWithoutShoving();
    TestDragDefersItsCommitUntilRelease();
    TestDragCannotCrossOrEscape();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
