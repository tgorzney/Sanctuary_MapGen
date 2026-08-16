// SliderScalar_UI_Test.cpp — acceptance test for the single-handle scalar slider, float and int.
// Covers the clamp/snap math, the drawn handle offset, the full press-drag-release interaction
// driven by a synthetic pointer sequence in value space, and the integer twin landing on whole
// numbers. No imgui frame, no window, no GL (SliderScalar_UI.h is pure); the rectangles are a
// by-eye check against a live frame.
#include "SliderScalar_UI.h"
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

static Ui::ScalarSliderRange MakeRange(float lowest, float highest, float increment) {
    Ui::ScalarSliderRange range;
    range.minimumValue = lowest; range.maximumValue = highest; range.increment = increment;
    return range;
}

static void TestClampingAndSnappingHoldTheContract() {
    const Ui::ScalarSliderRange continuousRange = MakeRange(0.1f, 5.0f, 0.0f);   // "Blend Sharpness"
    Check(IsNear(Ui::ClampScalarSliderValue(-4.0f, continuousRange), 0.1f), "a value below the range clamps up");
    Check(IsNear(Ui::ClampScalarSliderValue(90.0f, continuousRange), 5.0f), "and above it clamps down");
    Check(IsNear(Ui::ClampScalarSliderValue(2.345f, continuousRange), 2.345f), "a continuous range does not snap");
    const Ui::ScalarSliderRange snappedRange = MakeRange(0.0f, 1.0f, 0.25f);
    Check(IsNear(Ui::ClampScalarSliderValue(0.34f, snappedRange), 0.25f), "a value snaps to the nearest step");
    Check(IsNear(Ui::ClampScalarSliderValue(0.40f, snappedRange), 0.5f), "including upward");
    Check(IsNear(Ui::ClampScalarSliderValue(0.99f, snappedRange), 1.0f), "and the snap never steps past the top");
    // Degenerate input is repaired, not obeyed (Constitution §6).
    const Ui::ScalarSliderRange invertedRange = MakeRange(8.0f, 2.0f, 0.0f);
    Check(IsNear(Ui::ClampScalarSliderValue(1.0f, invertedRange), 2.0f), "inverted limits swap");
    Check(IsNear(Ui::ClampScalarSliderValue(99.0f, invertedRange), 8.0f), "on both ends");
    const Ui::ScalarSliderRange collapsedRange = MakeRange(3.0f, 3.0f, 0.0f);
    Check(IsNear(Ui::ClampScalarSliderValue(7.0f, collapsedRange), 3.0f), "a zero-width range answers its one value");
}

static void TestTheDrawnOffsetAgreesWithTheValue() {
    const Ui::ScalarSliderRange range = MakeRange(0.0f, 1.0f, 0.0f);
    Check(IsNear(Ui::ScalarSliderHandleOffset(0.0f, range, 110.0f, 10.0f), 0.0f), "the handle sits at the origin");
    Check(IsNear(Ui::ScalarSliderHandleOffset(1.0f, range, 110.0f, 10.0f), 100.0f), "and one width in at the top");
    Check(IsNear(Ui::ScalarSliderHandleOffset(0.5f, range, 110.0f, 10.0f), 50.0f), "and is linear between");
    Check(IsNear(Ui::ScalarSliderHandleOffset(0.5f, range, 8.0f, 10.0f), 0.0f), "a track narrower than a handle is safe");
    const Ui::ScalarSliderRange offsetRange = MakeRange(1.0f, 4096.0f, 0.0f);
    Check(IsNear(Ui::ScalarSliderHandleOffset(1.0f, offsetRange, 210.0f, 10.0f), 0.0f),
          "a range that does not start at zero still pins its minimum to the origin");   // "Terrain Max Height"
}

// One synthetic drag: press at the first pointer value, drag through the rest, release.
struct DragTally { int changeCount = 0; int commitCount = 0; bool bCommittedBeforeRelease = false; };

static DragTally DragHandle(Ui::RealtimeToggle& realtimeToggle, float& value,
                            const Ui::ScalarSliderRange& range, const float* pointerValues, int positionCount) {
    DragTally tally;
    Ui::ScalarSliderPointerInput input;
    input.bHandleGrabbed = true;
    for (int position = 0; position < positionCount; ++position) {
        input.pointerValue = pointerValues[position];
        const Ui::WidgetChange change = Ui::StepScalarSliderInteraction(realtimeToggle, value, range, input);
        if (change.bValueChanged) ++tally.changeCount;
        if (change.bCommitted) { ++tally.commitCount; tally.bCommittedBeforeRelease = true; }
    }
    input.bHandleGrabbed = false;                                            // mouse-up frame
    const Ui::WidgetChange release = Ui::StepScalarSliderInteraction(realtimeToggle, value, range, input);
    if (release.bValueChanged) ++tally.changeCount;
    if (release.bCommitted) ++tally.commitCount;
    return tally;
}

static void TestDragDefersItsCommitUntilRelease() {
    const Ui::ScalarSliderRange range = MakeRange(0.0f, 1.0f, 0.0f);
    const float pointerValues[4] = {0.30f, 0.45f, 0.45f, 0.60f};             // frame 2 does not move
    float value = 0.0f;
    Ui::RealtimeToggle realtimeToggle;                                       // RT off

    const DragTally tally = DragHandle(realtimeToggle, value, range, pointerValues, 4);
    Check(IsNear(value, 0.60f), "the value tracks the pointer during the drag");
    Check(tally.changeCount == 3, "one live change per frame that actually moved");
    Check(!tally.bCommittedBeforeRelease, "no commit is paid for during the drag");
    Check(tally.commitCount == 1, "exactly one commit, on release");

    // Realtime on: the same drag commits on every moving frame instead.
    float realtimeValue = 0.0f;
    Ui::RealtimeToggle alwaysOnToggle(true);
    const DragTally realtimeTally = DragHandle(alwaysOnToggle, realtimeValue, range, pointerValues, 4);
    Check(realtimeTally.changeCount == 3 && realtimeTally.commitCount == 3, "realtime commits live");
    Check(IsNear(realtimeValue, 0.60f), "and lands on the same value");

    // A pointer beyond the track cannot drag the value off it, and a typed field commits at once.
    const float escapingValues[2] = {-9.0f, 42.0f};
    float clampedValue = 0.5f;
    Ui::RealtimeToggle escapeToggle;
    DragHandle(escapeToggle, clampedValue, range, escapingValues, 2);
    Check(IsNear(clampedValue, 1.0f), "a drag past the end stops at the limit");

    float typedValue = 0.25f;
    Ui::RealtimeToggle fieldToggle;
    Ui::ScalarSliderPointerInput fieldInput;
    fieldInput.bNumericFieldEdited = true;
    const Ui::WidgetChange typed = Ui::StepScalarSliderInteraction(fieldToggle, typedValue, range, fieldInput);
    Check(typed.bValueChanged && typed.bCommitted, "a typed field edit commits immediately");
}

static void TestTheIntegerTwinLandsOnWholeNumbers() {
    const Ui::ScalarSliderRange octaveRange = Ui::IntegerScalarSliderRange(1, 10);   // "Octaves"
    Check(IsNear(octaveRange.increment, 1.0f), "an integer range snaps by one by default");
    Check(Ui::ClampScalarSliderInteger(-6, octaveRange) == 1, "an integer below the range clamps up");
    Check(Ui::ClampScalarSliderInteger(77, octaveRange) == 10, "and above it clamps down");
    const Ui::ScalarSliderRange steppedRange = Ui::IntegerScalarSliderRange(0, 100, 25);
    Check(Ui::ClampScalarSliderInteger(60, steppedRange) == 50, "a coarse integer step snaps to its lattice");
    const Ui::ScalarSliderRange guardedRange = Ui::IntegerScalarSliderRange(0, 8, 0);
    Check(IsNear(guardedRange.increment, 1.0f), "an increment below one is raised, never left to land between values");

    // The drag runs through the SAME float math, then rounds — so a mid-step pointer resolves.
    int octaveCount = 1;
    Ui::RealtimeToggle realtimeToggle;
    Ui::ScalarSliderPointerInput input;
    input.bHandleGrabbed = true;
    input.pointerValue   = 4.6f;
    const Ui::WidgetChange dragged =
        Ui::StepScalarSliderIntegerInteraction(realtimeToggle, octaveCount, octaveRange, input);
    Check(octaveCount == 5, "the integer drag lands on the nearest whole number");
    Check(dragged.bValueChanged && !dragged.bCommitted, "and defers its commit like the float slider");
    input.pointerValue = 4.8f;                                               // same snapped value
    const Ui::WidgetChange held =
        Ui::StepScalarSliderIntegerInteraction(realtimeToggle, octaveCount, octaveRange, input);
    Check(octaveCount == 5 && !held.bValueChanged, "a pointer move inside one step reports no change");
    input.bHandleGrabbed = false;
    const Ui::WidgetChange released =
        Ui::StepScalarSliderIntegerInteraction(realtimeToggle, octaveCount, octaveRange, input);
    Check(released.bCommitted && octaveCount == 5, "and the one commit arrives on release");
}

int main() {
    TestClampingAndSnappingHoldTheContract();
    TestTheDrawnOffsetAgreesWithTheValue();
    TestDragDefersItsCommitUntilRelease();
    TestTheIntegerTwinLandsOnWholeNumbers();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
