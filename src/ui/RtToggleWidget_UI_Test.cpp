// RtToggleWidget_UI_Test.cpp — acceptance test for the M5-1 RT (realtime) defer semantics.
// Drives Ui::RealtimeToggle with synthetic mouse-down/drag/up sequences: no imgui frame, no
// window, no GL — the state machine is pure by construction (RtToggleWidget_UI.h "THE SPLIT").
// The claim under test (UI_FRAMEWORK_SPEC §7): with RT OFF the value tracks the drag every frame
// while the expensive commit fires exactly ONCE, on release.
#include "RtToggleWidget_UI.h"
#include <cstdio>

using namespace SanmapGen;

static int failureCount = 0;

static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

// One synthetic drag: press, `moveCount` frames that move the value, then release. Reports how
// many live changes and how many commits the toggle produced.
struct DragTally { int changeCount = 0; int commitCount = 0; int commitFrame = -1; };

static DragTally RunDrag(Ui::RealtimeToggle& realtimeToggle, int moveCount) {
    DragTally tally;
    int frame = 0;
    for (int move = 0; move < moveCount; ++move, ++frame) {
        const Ui::WidgetChange change = realtimeToggle.Update(true, true);
        if (change.bValueChanged) ++tally.changeCount;
        if (change.bCommitted) { ++tally.commitCount; tally.commitFrame = frame; }
    }
    const Ui::WidgetChange releaseChange = realtimeToggle.Update(false, false);   // mouse-up frame
    if (releaseChange.bValueChanged) ++tally.changeCount;
    if (releaseChange.bCommitted) { ++tally.commitCount; tally.commitFrame = frame; }
    return tally;
}

static void TestRealtimeOffDefersToRelease() {
    Ui::RealtimeToggle realtimeToggle;                       // default OFF
    Check(!realtimeToggle.IsRealtimeEnabled(), "realtime defaults to off");
    const DragTally tally = RunDrag(realtimeToggle, 5);
    Check(tally.changeCount == 5, "value tracks every frame of the drag");
    Check(tally.commitCount == 1, "exactly one commit per deferred drag");
    Check(tally.commitFrame == 5, "the commit lands on the release frame");
    Check(!realtimeToggle.IsCommitDeferred(), "nothing left pending after the release");
    // Idle frames after the release cost nothing.
    for (int frame = 0; frame < 3; ++frame) {
        const Ui::WidgetChange idle = realtimeToggle.Update(false, false);
        Check(!idle.bValueChanged && !idle.bCommitted, "idle frames are silent");
    }
    // A second drag behaves identically — no state leaks between drags.
    const DragTally secondTally = RunDrag(realtimeToggle, 3);
    Check(secondTally.changeCount == 3 && secondTally.commitCount == 1, "second drag repeats cleanly");
}

static void TestDeferredCommitSurvivesAStillLastFrame() {
    // The value stops moving before the mouse comes up: the commit must still arrive on release.
    Ui::RealtimeToggle realtimeToggle;
    realtimeToggle.Update(true, true);
    realtimeToggle.Update(true, false);
    Check(realtimeToggle.IsCommitDeferred(), "a change stays pending while the drag continues");
    const Ui::WidgetChange release = realtimeToggle.Update(false, false);
    Check(release.bCommitted, "commit still fires when the last drag frame was still");
}

static void TestClickWithoutMovementCostsNothing() {
    Ui::RealtimeToggle realtimeToggle;
    realtimeToggle.Update(true, false);                      // press, no movement
    realtimeToggle.Update(true, false);
    const Ui::WidgetChange release = realtimeToggle.Update(false, false);
    Check(!release.bCommitted, "a click that moves nothing never commits");
    Check(!realtimeToggle.IsCommitDeferred(), "and leaves nothing pending");
}

static void TestRealtimeOnCommitsEveryChange() {
    Ui::RealtimeToggle realtimeToggle(true);
    Check(realtimeToggle.IsRealtimeEnabled(), "constructed on");
    const DragTally tally = RunDrag(realtimeToggle, 4);
    Check(tally.changeCount == 4, "value still tracks every frame");
    Check(tally.commitCount == 4, "realtime commits on every change");
    // A frame with no movement does not commit even in realtime.
    const Ui::WidgetChange still = realtimeToggle.Update(true, false);
    Check(!still.bCommitted, "no movement, no commit");
}

static void TestModeSwitchAndOutOfDragEdits() {
    // Switching to realtime mid-drag flushes the deferred change on the next frame.
    Ui::RealtimeToggle realtimeToggle;
    realtimeToggle.Update(true, true);
    Check(realtimeToggle.IsCommitDeferred(), "pending while off");
    realtimeToggle.SetRealtimeEnabled(true);
    const Ui::WidgetChange flushed = realtimeToggle.Update(true, false);
    Check(flushed.bCommitted && !flushed.bValueChanged, "switching to realtime pays the pending change");

    // An edit outside any drag (a typed field, a keyboard step) has no release to wait for.
    Ui::RealtimeToggle typedToggle;
    const Ui::WidgetChange typed = typedToggle.Update(false, true);
    Check(typed.bValueChanged && typed.bCommitted, "a non-drag edit commits immediately");
    Check(!typedToggle.IsCommitDeferred(), "a deferral can never get stuck");

    // FlushPendingCommit settles a drag that will never see a release frame.
    Ui::RealtimeToggle abandonedToggle;
    abandonedToggle.Update(true, true);
    const Ui::WidgetChange flush = abandonedToggle.FlushPendingCommit();
    Check(flush.bCommitted, "flush pays an abandoned drag");
    Check(!abandonedToggle.FlushPendingCommit().bCommitted, "flushing twice owes nothing");
}

int main() {
    TestRealtimeOffDefersToRelease();
    TestDeferredCommitSurvivesAStillLastFrame();
    TestClickWithoutMovementCostsNothing();
    TestRealtimeOnCommitsEveryChange();
    TestModeSwitchAndOutOfDragEdits();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
