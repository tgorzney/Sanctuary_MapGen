// RtToggleWidget_UI_Test.cpp — acceptance test for the M5-1 RT (realtime) defer semantics, plus
// (STEP213) the universal DrawToggleButton draw path both DrawRealtimeToggleButton and the new
// Auto-Level toolbar toggle share. The RealtimeToggle state-machine checks below still drive it
// with synthetic mouse-down/drag/up sequences and link no imgui at all (RtToggleWidget_UI.h
// "THE SPLIT"). The STEP213 additions at the bottom close a real pre-existing gap:
// DrawRealtimeToggleButton's own draw path had NO test before this ticket (only the pure
// RealtimeToggle machinery did) — they use the SAME headless imgui harness
// (ListWidget_TestFrame_UI.h's HeadlessImguiSession/RunHeadlessFrame) a dozen other SanGen UI
// tests already share; no window, no GL, no new CMake linkage (every add_sangen_test target
// already links SanGenV2, which links imgui PUBLIC — CMakeLists.txt:398).
// The claim under test (UI_FRAMEWORK_SPEC §7): with RT OFF the value tracks the drag every frame
// while the expensive commit fires exactly ONCE, on release.
#include "ListWidget_TestFrame_UI.h"
#include "RtToggleWidget_UI.h"
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

// STEP213 — DrawToggleButton's own click contract, driven with a real headless imgui frame
// (ImGui::Button commits on the RELEASE frame, not the press — the same probe/hover/press/
// release shape MarkersTab_Bundles_UI_Test.cpp's own button tests already use).
static void TestDrawToggleButtonFlipsPlainBoolOnClickRelease() {
    Ui::HeadlessImguiSession session;
    bool enabled = false;
    const ImVec2 windowSize(300.0f, 100.0f);

    ImVec2 itemMin, itemMax;
    Ui::RunHeadlessFrame(Ui::HeadlessMouseState(), windowSize, [&] {
        const bool bClicked = Ui::DrawToggleButton("autoLevelToggle", "Auto-Level", enabled);
        Check(!bClicked, "no click reported while the mouse is nowhere near the button");
        itemMin = ImGui::GetItemRectMin();
        itemMax = ImGui::GetItemRectMax();
    });
    Check(!enabled, "state has not moved yet");

    const ImVec2 center((itemMin.x + itemMax.x) * 0.5f, (itemMin.y + itemMax.y) * 0.5f);
    // A settle/hover frame (mouse over the button, not yet pressed) is required before the press
    // frame -- imgui's window-level hover routing needs one frame at the target position before a
    // same-frame press registers a valid click owner; MarkersTab_Bundles_UI_Test.cpp's own button
    // tests use the identical probe -> hover -> press -> release shape.
    Ui::HeadlessMouseState hover; hover.position = center;
    Ui::RunHeadlessFrame(hover, windowSize, [&] { Ui::DrawToggleButton("autoLevelToggle", "Auto-Level", enabled); });

    Ui::HeadlessMouseState press = hover; press.bLeftButtonDown = true;
    Ui::RunHeadlessFrame(press, windowSize, [&] {
        const bool bClicked = Ui::DrawToggleButton("autoLevelToggle", "Auto-Level", enabled);
        Check(!bClicked, "the PRESS frame reports no click yet (imgui commits on release)");
        Check(!enabled, "and does not flip the state early");
    });

    Ui::HeadlessMouseState release = press; release.bLeftButtonDown = false;
    bool bClickedOnRelease = false;
    Ui::RunHeadlessFrame(release, windowSize, [&] {
        bClickedOnRelease = Ui::DrawToggleButton("autoLevelToggle", "Auto-Level", enabled);
    });
    Check(bClickedOnRelease, "the RELEASE frame (still hovering) reports the click");
    Check(enabled, "the caller's own bool flips in place -- exactly what Auto-Level's toolbar "
                  "wiring reads back");

    // A second click cycle flips it back off, confirming the widget owns no latched state of its own.
    Ui::RunHeadlessFrame(press, windowSize, [&] { Ui::DrawToggleButton("autoLevelToggle", "Auto-Level", enabled); });
    bool bSecondClick = false;
    Ui::RunHeadlessFrame(release, windowSize, [&] {
        bSecondClick = Ui::DrawToggleButton("autoLevelToggle", "Auto-Level", enabled);
    });
    Check(bSecondClick && !enabled, "clicking again flips it back off");
}

// STEP213 — DrawRealtimeToggleButton is now a thin adapter over DrawToggleButton; this proves the
// port is behavior-preserving: same click contract, same exact drawn width
// (SliderScalar_UI_Test.cpp:201-221's own width assertion depends on this staying exact).
static void TestDrawRealtimeToggleButtonStillFlipsThroughTheGenericPrimitive() {
    Ui::HeadlessImguiSession session;
    Ui::RealtimeToggle realtimeToggle;   // default OFF
    const ImVec2 windowSize(300.0f, 100.0f);

    ImVec2 itemMin, itemMax;
    Ui::RunHeadlessFrame(Ui::HeadlessMouseState(), windowSize, [&] {
        Ui::DrawRealtimeToggleButton("rtToggle", realtimeToggle);
        itemMin = ImGui::GetItemRectMin();
        itemMax = ImGui::GetItemRectMax();
    });
    Check(IsNear(itemMax.x - itemMin.x, Ui::WidgetStyle().realtimeButtonWidth),
         "the RT button keeps its exact historical fixed width after the generalization");

    const ImVec2 center((itemMin.x + itemMax.x) * 0.5f, (itemMin.y + itemMax.y) * 0.5f);
    // Settle over the button before pressing (see TestDrawToggleButtonFlipsPlainBoolOnClickRelease's
    // own note on why a bare press frame from "mouse elsewhere" doesn't register).
    Ui::HeadlessMouseState hover; hover.position = center;
    Ui::RunHeadlessFrame(hover, windowSize, [&] { Ui::DrawRealtimeToggleButton("rtToggle", realtimeToggle); });

    Ui::HeadlessMouseState press = hover; press.bLeftButtonDown = true;
    Ui::RunHeadlessFrame(press, windowSize, [&] { Ui::DrawRealtimeToggleButton("rtToggle", realtimeToggle); });
    Check(!realtimeToggle.IsRealtimeEnabled(), "still off mid-press");

    Ui::HeadlessMouseState release = press; release.bLeftButtonDown = false;
    bool bClickedOnRelease = false;
    Ui::RunHeadlessFrame(release, windowSize, [&] {
        bClickedOnRelease = Ui::DrawRealtimeToggleButton("rtToggle", realtimeToggle);
    });
    Check(bClickedOnRelease && realtimeToggle.IsRealtimeEnabled(),
         "the click flips RealtimeToggle's own state, same as before the refactor");
}

// STEP213 — the default buttonWidth (0) auto-fits the label, unlike RT's own fixed 30px slot --
// required for a longer label like "Auto-Level" that would otherwise visibly clip.
static void TestDrawToggleButtonAutoSizesUnlikeTheFixedRtWidth() {
    Ui::HeadlessImguiSession session;
    bool enabled = false;
    ImVec2 itemMin, itemMax;
    Ui::RunHeadlessFrame(Ui::HeadlessMouseState(), ImVec2(300.0f, 100.0f), [&] {
        Ui::DrawToggleButton("autoLevelToggle", "Auto-Level", enabled);
        itemMin = ImGui::GetItemRectMin();
        itemMax = ImGui::GetItemRectMax();
    });
    Check(itemMax.x - itemMin.x > Ui::WidgetStyle().realtimeButtonWidth,
         "a longer label auto-sizes wider than the RT button's fixed 30px slot by default");
}

int main() {
    TestRealtimeOffDefersToRelease();
    TestDeferredCommitSurvivesAStillLastFrame();
    TestClickWithoutMovementCostsNothing();
    TestRealtimeOnCommitsEveryChange();
    TestModeSwitchAndOutOfDragEdits();
    TestDrawToggleButtonFlipsPlainBoolOnClickRelease();
    TestDrawRealtimeToggleButtonStillFlipsThroughTheGenericPrimitive();
    TestDrawToggleButtonAutoSizesUnlikeTheFixedRtWidth();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
