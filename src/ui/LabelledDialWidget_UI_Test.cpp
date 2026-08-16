// LabelledDialWidget_UI_Test.cpp — acceptance test for the M5-1 labelled dial.
// Covers the clamp/snap math, the drag mapping, the drawn-angle mapping, and the RT defer
// semantics, driven headless from a synthetic vertical-drag sequence. No imgui frame, no window,
// no GL. The arcs themselves are a by-eye check against a live frame.
#include "LabelledDialWidget_UI.h"
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

static Ui::DialRange MakeRange(float minimumValue, float maximumValue, float increment,
                               float pixelsForFullSweep = 200.0f) {
    Ui::DialRange range;
    range.minimumValue = minimumValue;
    range.maximumValue = maximumValue;
    range.increment = increment;
    range.pixelsForFullSweep = pixelsForFullSweep;
    return range;
}

static void TestClampAndSnap() {
    const Ui::DialRange continuous = MakeRange(0.0f, 10.0f, 0.0f);
    Check(IsNear(Ui::ClampDialValue(3.3f, continuous), 3.3f), "a continuous dial keeps the value");
    Check(IsNear(Ui::ClampDialValue(-4.0f, continuous), 0.0f), "below the range clamps up");
    Check(IsNear(Ui::ClampDialValue(40.0f, continuous), 10.0f), "above the range clamps down");

    const Ui::DialRange snapped = MakeRange(0.0f, 10.0f, 0.5f);
    Check(IsNear(Ui::ClampDialValue(3.3f, snapped), 3.5f), "snaps up to the nearest increment");
    Check(IsNear(Ui::ClampDialValue(3.2f, snapped), 3.0f), "snaps down to the nearest increment");
    Check(IsNear(Ui::ClampDialValue(10.0f, snapped), 10.0f), "an exact limit stays put");

    // The clamp runs AFTER the snap, so a snap can never step past the top of the range.
    const Ui::DialRange offLattice = MakeRange(0.0f, 9.9f, 0.5f);
    Check(IsNear(Ui::ClampDialValue(9.9f, offLattice), 9.9f), "a snap past the maximum is clamped back");

    // Degenerate input is repaired, not obeyed (Constitution §6).
    const Ui::DialRange inverted = MakeRange(10.0f, 0.0f, 0.0f);
    Check(IsNear(Ui::ClampDialValue(5.0f, inverted), 5.0f), "inverted limits swap");
    Check(IsNear(Ui::ClampDialValue(-1.0f, inverted), 0.0f), "and still clamp");
    const Ui::DialRange negativeIncrement = MakeRange(0.0f, 10.0f, -2.0f);
    Check(IsNear(Ui::ClampDialValue(3.3f, negativeIncrement), 3.3f), "a negative increment means continuous");
}

static void TestDragMapping() {
    const Ui::DialRange range = MakeRange(0.0f, 10.0f, 0.0f, 200.0f);
    Check(IsNear(Ui::DialValueAfterDrag(5.0f, range, -20.0f), 6.0f), "dragging up raises the value");
    Check(IsNear(Ui::DialValueAfterDrag(5.0f, range, 20.0f), 4.0f), "dragging down lowers it");
    Check(IsNear(Ui::DialValueAfterDrag(5.0f, range, -200.0f), 10.0f), "a full sweep hits the top and clamps");
    Check(IsNear(Ui::DialValueAfterDrag(5.0f, range, 0.0f), 5.0f), "a still frame moves nothing");

    const Ui::DialRange snapped = MakeRange(0.0f, 10.0f, 0.5f, 200.0f);
    Check(IsNear(Ui::DialValueAfterDrag(5.0f, snapped, -13.0f), 5.5f), "a dragged value lands on the lattice");

    const Ui::DialRange frozen = MakeRange(0.0f, 10.0f, 0.0f, 0.0f);
    Check(IsNear(Ui::DialValueAfterDrag(5.0f, frozen, -50.0f), 5.0f),
          "a zero sweep distance freezes the drag instead of dividing by zero");
}

static void TestDrawnAngleMapping() {
    const Ui::DialRange range = MakeRange(0.0f, 10.0f, 0.0f);
    Check(IsNear(Ui::DialNormalizedPosition(5.0f, range), 0.5f), "the midpoint normalizes to 0.5");
    Check(IsNear(Ui::DialNormalizedPosition(-3.0f, range), 0.0f), "out-of-range normalizes into 0..1");

    constexpr float kPi = 3.14159265358979323846f;
    Check(IsNear(Ui::DialAngleRadians(0.0f, 135.0f, 270.0f), kPi * 0.75f), "the dial starts at 135 degrees");
    Check(IsNear(Ui::DialAngleRadians(1.0f, 135.0f, 270.0f), kPi * 2.25f), "and ends 270 degrees clockwise");
    Check(IsNear(Ui::DialAngleRadians(0.5f, 135.0f, 270.0f), kPi * 1.5f), "the midpoint points straight up");
    Check(IsNear(Ui::DialAngleRadians(4.0f, 135.0f, 270.0f), Ui::DialAngleRadians(1.0f, 135.0f, 270.0f)),
          "an out-of-range position cannot sweep past the end of the arc");
}

// One synthetic vertical drag: `frameCount` frames of dragDeltaY, then release.
struct DragTally { int changeCount = 0; int commitCount = 0; bool bCommittedBeforeRelease = false; };

static DragTally DragKnob(Ui::RealtimeToggle& realtimeToggle, float& value, const Ui::DialRange& range,
                          const float* frameDeltas, int frameCount) {
    DragTally tally;
    Ui::DialPointerInput input;
    input.bDragInProgress = true;
    for (int frame = 0; frame < frameCount; ++frame) {
        input.dragDeltaY = frameDeltas[frame];
        const Ui::WidgetChange change = Ui::StepDialInteraction(realtimeToggle, value, range, input);
        if (change.bValueChanged) ++tally.changeCount;
        if (change.bCommitted) { ++tally.commitCount; tally.bCommittedBeforeRelease = true; }
    }
    input.bDragInProgress = false;
    input.dragDeltaY = 0.0f;
    const Ui::WidgetChange release = Ui::StepDialInteraction(realtimeToggle, value, range, input);
    if (release.bCommitted) ++tally.commitCount;
    return tally;
}

static void TestKnobDragDefersItsCommit() {
    const Ui::DialRange range = MakeRange(0.0f, 10.0f, 0.0f, 200.0f);
    const float frameDeltas[4] = {-20.0f, -20.0f, 0.0f, -20.0f};             // frame 2 does not move
    float value = 5.0f;
    Ui::RealtimeToggle realtimeToggle;                                        // RT off

    const DragTally tally = DragKnob(realtimeToggle, value, range, frameDeltas, 4);
    Check(IsNear(value, 8.0f), "the value tracks the drag");
    Check(tally.changeCount == 3, "one live change per frame that actually moved");
    Check(!tally.bCommittedBeforeRelease, "no commit is paid for during the drag");
    Check(tally.commitCount == 1, "exactly one commit, on release");

    float realtimeValue = 5.0f;
    Ui::RealtimeToggle alwaysOnToggle(true);
    const DragTally realtimeTally = DragKnob(alwaysOnToggle, realtimeValue, range, frameDeltas, 4);
    Check(realtimeTally.changeCount == 3 && realtimeTally.commitCount == 3, "realtime commits live");
    Check(IsNear(realtimeValue, 8.0f), "and lands on the same value");

    // A numeric-field edit the draw path already applied commits immediately.
    float fieldValue = 5.0f;
    Ui::RealtimeToggle fieldToggle;
    Ui::DialPointerInput fieldInput;
    fieldInput.bFieldEdited = true;
    const Ui::WidgetChange typed = Ui::StepDialInteraction(fieldToggle, fieldValue, range, fieldInput);
    Check(typed.bValueChanged && typed.bCommitted, "a typed field edit commits immediately");
}

int main() {
    TestClampAndSnap();
    TestDragMapping();
    TestDrawnAngleMapping();
    TestKnobDragDefersItsCommit();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
