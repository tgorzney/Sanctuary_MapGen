// MarkersTab_ListCanvasSelectionSync_UI_Test.cpp — STEP232/STEP233 acceptance. STEP232 fixed the
// anchor/plural-field clobber for a Shift-click chain (still covered here). STEP233 fixes the deeper
// gap STEP232 left open: the CANVAS's own real selectedInstanceKeys never actually matched the list's
// own resolved selection at all (only tabState's plural MIRROR field did) — this file's own acceptance
// scenario (RunCtrlWithinShiftRangeChecks, below) is the human's own literally-reported bug: plain-click
// A, Shift-click B (establishing a range), Ctrl-click a THIRD row C already inside that range. Pure
// logic, no imgui/GL: MapCanvas's own gesture/sync methods are confirmed imgui-free (MapCanvas_UI.cpp's
// own header comment, "with no imgui in sight") and this test calls none of the picking/draw methods
// that would need a composite/GL context. Per the human's own explicit instruction, this is deliberately
// NOT a GL-backed/real-imgui-frame test.
#include "MapCanvas_UI.h"
#include "MarkersTab_ManualInstanceSelection_UI.h"
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

bool Equal(const std::vector<int>& actual, const std::vector<int>& expected) { return actual == expected; }

// Extracts the {Markers, bManual==true} subset of the canvas's own REAL selectedInstanceKeys, in
// ORDER, as a plain identifier vector — the core assertion vehicle this file's whole point is to
// exercise (STEP233's own bug lived entirely in this set never matching the list's own resolution).
std::vector<int> CanvasManualMarkerIdentifiers(const MapCanvas& canvas) {
    std::vector<int> identifiers;
    for (const OverlayInstanceKey_UI& key : canvas.SelectedInstanceKeys().keys)
        if (key.bValid && key.collection == PlacementCollectionKind_UI::Markers && key.bManual)
            identifiers.push_back(key.instanceIndex);
    return identifiers;
}

// Mirrors MarkersTabState's own three relevant fields exactly (selectedManualInstanceIdentifier /
// selectedManualInstanceIdentifiers / manualInstanceSelectionAnchorIdentifier — MarkersTab_UI.h).
struct FakeMarkersTabState {
    int selectedManualInstanceIdentifier = -1;
    std::vector<int> selectedManualInstanceIdentifiers;
    int manualInstanceSelectionAnchorIdentifier = -1;
};

// Reproduces Application::WireCallbacks()'s own FIXED selectionChangedCallback closure EXACTLY as
// STEP233's own Application_UI.cpp diff specifies it — MapCanvas_Picking_UI_Test.cpp already
// established the precedent of wiring a LOCAL lambda rather than constructing a real Application
// (which needs a GL context this test deliberately avoids).
void WireFixedSelectionChangedCallback(MapCanvas& canvas, FakeMarkersTabState& tabState) {
    canvas.SetSelectionChangedCallback([&tabState](const OverlayInstanceKey_UI& primary,
                                                   const OverlayInstanceKeySet_UI& selectedKeys,
                                                   bool bSuppressTabStateResync) {
        if (!bSuppressTabStateResync) {
            tabState.selectedManualInstanceIdentifiers.clear();
            for (const OverlayInstanceKey_UI& key : selectedKeys.keys)
                if (key.bValid && key.collection == PlacementCollectionKind_UI::Markers && key.bManual)
                    tabState.selectedManualInstanceIdentifiers.push_back(key.instanceIndex);
        }
        const bool bPrimaryIsManualMarker = primary.bValid
            && primary.collection == PlacementCollectionKind_UI::Markers && primary.bManual;
        tabState.selectedManualInstanceIdentifier = bPrimaryIsManualMarker ? primary.instanceIndex : -1;
        if (!bSuppressTabStateResync)
            tabState.manualInstanceSelectionAnchorIdentifier = bPrimaryIsManualMarker ? primary.instanceIndex : -1;
    });
}

// Reproduces DrawManualInstanceRow's own real click-handler call order EXACTLY, post-STEP233
// (MarkersTab_ManualLayerRowBody_UI.cpp): the list-local write first, THEN SyncManualMarkerSelection —
// which fires the callback above SYNCHRONOUSLY before this function returns, the exact same-click race
// both STEP232 and STEP233's own bugs lived in.
void SimulateListClick(MapCanvas& canvas, FakeMarkersTabState& tabState, const std::vector<int>& rowOrder,
                       int clickedIdentifier, bool bCtrl, bool bShift) {
    ApplyManualInstanceSelectionClick(rowOrder, clickedIdentifier, bCtrl, bShift,
                                      tabState.selectedManualInstanceIdentifiers,
                                      tabState.manualInstanceSelectionAnchorIdentifier);
    tabState.selectedManualInstanceIdentifier = clickedIdentifier;
    canvas.SyncManualMarkerSelection(tabState.selectedManualInstanceIdentifiers, clickedIdentifier);
}

// STEP232's own original scenario — still fully valid post-STEP233 (SyncManualMarkerSelection produces
// the SAME correct list-mirroring result STEP232's narrower fix already achieved for a pure Shift
// chain; this proves STEP233 didn't regress it), PLUS a new assertion this ticket adds: the CANVAS's
// own real set now ALSO matches, not just tabState's mirror field (STEP232 never actually proved this).
void RunPlainThenTwoShiftClicksChecks() {
    const std::vector<int> rowOrder{ 10, 20, 30, 40, 50 };
    MapCanvas canvas;
    FakeMarkersTabState tabState;
    WireFixedSelectionChangedCallback(canvas, tabState);

    SimulateListClick(canvas, tabState, rowOrder, 20, /*bCtrl=*/false, /*bShift=*/false);
    Check(Equal(tabState.selectedManualInstanceIdentifiers, { 20 }), "plain click selects just 20");
    Check(tabState.manualInstanceSelectionAnchorIdentifier == 20, "and the anchor becomes 20");
    Check(Equal(CanvasManualMarkerIdentifiers(canvas), { 20 }),
         "STEP233 - the CANVAS's own real selection set ALSO matches {20}, not just tabState's mirror");

    SimulateListClick(canvas, tabState, rowOrder, 40, /*bCtrl=*/false, /*bShift=*/true);
    Check(Equal(tabState.selectedManualInstanceIdentifiers, { 20, 30, 40 }),
         "Shift-click ranges the FULL [anchor..clicked] span, including the un-clicked in-between row (30)");
    Check(tabState.manualInstanceSelectionAnchorIdentifier == 20, "the anchor survives this Shift-click unmoved");
    Check(Equal(CanvasManualMarkerIdentifiers(canvas), { 20, 30, 40 }),
         "STEP233 - the CANVAS's own real selection set is the FULL {20,30,40} range too - THIS is the "
         "part STEP232's own test never actually proved (it only checked tabState's mirror field, which "
         "a Shift-gated resync happened to leave untouched-and-therefore-still-correct — the canvas's "
         "OWN independently-touched set was silently wrong the whole time until this ticket)");

    SimulateListClick(canvas, tabState, rowOrder, 10, /*bCtrl=*/false, /*bShift=*/true);
    Check(Equal(tabState.selectedManualInstanceIdentifiers, { 10, 20 }),
         "a SECOND Shift-click still ranges from the SAME original anchor (20), not from click 2's own "
         "clicked row (40)");
    Check(tabState.manualInstanceSelectionAnchorIdentifier == 20, "the anchor is STILL 20 after the second Shift-click too");
    Check(Equal(CanvasManualMarkerIdentifiers(canvas), { 20, 10 }),
         "STEP233 - the canvas's own set is {20,10}, NOT {10,20}: SyncManualMarkerSelection's own primary "
         "rule holds the just-clicked id (10) back and appends it LAST, regardless of where it sits in the "
         "row-order-based resolved range going in ({10,20} here, with 10 at the FRONT since this Shift-click "
         "extends BACKWARD from anchor 20) — the canvas's own key ORDER is therefore allowed to diverge from "
         "tabState's list-order storage; only membership must match, which it does");
}

// A Ctrl-click DOES move the anchor and matches the list exactly (proves the gate generalization didn't
// break the already-correct Ctrl-ADD case).
void RunCtrlClickStillMovesAnchorChecks() {
    const std::vector<int> rowOrder{ 10, 20, 30 };
    MapCanvas canvas;
    FakeMarkersTabState tabState;
    WireFixedSelectionChangedCallback(canvas, tabState);

    SimulateListClick(canvas, tabState, rowOrder, 10, /*bCtrl=*/false, /*bShift=*/false);
    SimulateListClick(canvas, tabState, rowOrder, 20, /*bCtrl=*/true, /*bShift=*/false);
    Check(Equal(tabState.selectedManualInstanceIdentifiers, { 10, 20 }), "Ctrl-click adds 20, keeping 10");
    Check(tabState.manualInstanceSelectionAnchorIdentifier == 20, "a Ctrl-click DOES move the anchor to the just-clicked row");
    Check(Equal(CanvasManualMarkerIdentifiers(canvas), { 10, 20 }), "and the canvas's own set matches exactly");
    Check(canvas.SelectedEntityIdentifier() == 20u, "the canvas's own overall primary is the just-Ctrl-added row (20)");
}

// A canvas-ONLY plain click (no list click involved) still moves the anchor via the UNCHANGED
// ApplySelectionGesture path — proves this ticket doesn't touch/regress that path at all.
void RunCanvasOnlyClickEstablishesAnchorChecks() {
    MapCanvas canvas;
    FakeMarkersTabState tabState;
    WireFixedSelectionChangedCallback(canvas, tabState);
    canvas.SelectManualMarkerByInstanceIdentifier(77, /*bCtrlHeld=*/false, /*bShiftHeld=*/false);
    Check(tabState.manualInstanceSelectionAnchorIdentifier == 77,
         "a canvas-only plain click establishes a sane anchor too, so a SUBSEQUENT list Shift-click can "
         "range from wherever the user last clicked on the canvas, not just from a prior list click "
         "(unchanged from STEP232 — SelectManualMarkerByInstanceIdentifier and ApplySelectionGesture are "
         "byte-identical to before this ticket)");
}

// STEP233's own core acceptance scenario — the human's LITERAL bug report, reproduced exactly: plain-
// click A (20), Shift-click B (50) establishing a range, then Ctrl-click a THIRD row C (30) already
// INSIDE that range (not an endpoint). Proves the canvas's own real set — not just tabState's mirror —
// matches the list's own correctly-resolved final selection exactly (C toggled off, the rest of the
// range intact), AND that the anchor (which ApplyManualInstanceSelectionClick's own Ctrl branch always
// moves to the clicked id, C, even on a deselect) survives this click too — the second, previously-
// undiscovered clobber this ticket also fixes (Part 2 of its own root-cause writeup).
void RunCtrlWithinShiftRangeChecks() {
    const std::vector<int> rowOrder{ 10, 20, 30, 40, 50, 60 };
    MapCanvas canvas;
    FakeMarkersTabState tabState;
    WireFixedSelectionChangedCallback(canvas, tabState);

    // Plain-click 20: anchor = 20.
    SimulateListClick(canvas, tabState, rowOrder, 20, /*bCtrl=*/false, /*bShift=*/false);
    // Shift-click 50: range = [20,30,40,50] (rowOrder indices 1..4). Reproduces the human's own "5
    // highlighted rows" shape (one fewer here, sufficient to place a non-endpoint row inside it).
    SimulateListClick(canvas, tabState, rowOrder, 50, /*bCtrl=*/false, /*bShift=*/true);
    Check(Equal(tabState.selectedManualInstanceIdentifiers, { 20, 30, 40, 50 }), "the Shift-range is the full {20,30,40,50}");
    Check(Equal(CanvasManualMarkerIdentifiers(canvas), { 20, 30, 40, 50 }), "and the canvas's own set matches it exactly");

    // Ctrl-click 30 — already selected, NOT an endpoint of the range. Toggles it OFF.
    SimulateListClick(canvas, tabState, rowOrder, 30, /*bCtrl=*/true, /*bShift=*/false);
    Check(Equal(tabState.selectedManualInstanceIdentifiers, { 20, 40, 50 }),
         "ApplyManualInstanceSelectionClick's own Ctrl branch toggles 30 OFF, leaving {20,40,50}");
    Check(Equal(CanvasManualMarkerIdentifiers(canvas), { 20, 40, 50 }),
         "STEP233 - THE HUMAN'S OWN REPORTED BUG: the canvas's own real selectedInstanceKeys now matches "
         "{20,40,50} exactly, not the pre-fix {20,50,30} (the two shift-range endpoints plus whatever was "
         "Ctrl-clicked, reconstructed against the canvas's own narrow, independently-touched copy of the "
         "set)");
    Check(tabState.manualInstanceSelectionAnchorIdentifier == 30,
         "STEP233 - Part 2 of this ticket's own root-cause: the anchor becomes the CTRL-CLICKED id (30) "
         "even though 30 is no longer selected (ApplyManualInstanceSelectionClick's own ratified rule — "
         "'anchor becomes this one too', unconditionally, even on a deselect) - and SURVIVES the "
         "same-click canvas echo, which pre-fix would have clobbered it to whatever the canvas's own "
         "fallback primary (50) computed to instead");
    Check(canvas.SelectedEntityIdentifier() == 50u,
         "the canvas's own overall PRIMARY correctly falls back to the new back() of the reduced set (50) "
         "— ToggleInSelectionSet's own documented rule — a DIFFERENT, deliberately-different value from "
         "the anchor (30): this test proves both are independently correct, not accidentally the same");
}

// Proves SyncManualMarkerSelection's own "held back, appended LAST regardless of position" primary rule
// concretely: a Shift-click EXTENDING BACKWARD (the clicked row lands at the FRONT of the resolved
// [low..high] array, not the back) must still make that clicked row the canvas's own overall primary —
// a naive "whatever the vector's own last element is" implementation would get this wrong.
void RunShiftClickBackwardStillPrimarysTheClickedRowChecks() {
    const std::vector<int> rowOrder{ 10, 20, 30, 40 };
    MapCanvas canvas;
    FakeMarkersTabState tabState;
    WireFixedSelectionChangedCallback(canvas, tabState);

    SimulateListClick(canvas, tabState, rowOrder, 30, /*bCtrl=*/false, /*bShift=*/false);   // anchor = 30
    SimulateListClick(canvas, tabState, rowOrder, 10, /*bCtrl=*/false, /*bShift=*/true);    // range [10,20,30]
    Check(Equal(tabState.selectedManualInstanceIdentifiers, { 10, 20, 30 }), "the backward range is {10,20,30}");
    Check(Equal(CanvasManualMarkerIdentifiers(canvas), { 20, 30, 10 }),
         "STEP233 - this IS the test's whole point, made explicit: the canvas's own key order is {20,30,10}, "
         "NOT tabState's row-order {10,20,30} — the just-clicked id (10) is held back and appended LAST by "
         "SyncManualMarkerSelection's own primary rule even though it sits FIRST in the resolved "
         "[10,20,30] range, which is EXACTLY why it becomes primary below (PrimaryOfSelectionSet reads the "
         "back() of the set) — asserting the canvas matches tabState's own order here would assert the "
         "opposite of what this test exists to prove");
    Check(canvas.SelectedEntityIdentifier() == 10u,
         "STEP233 - the JUST-CLICKED row (10) is the canvas's own overall primary, even though it sits at "
         "the FRONT of the resolved [10,20,30] array, not the back - proves SyncManualMarkerSelection's "
         "own explicit clickedInstanceIdentifier argument (not \"whatever the vector's own last element "
         "is\") determines primary");
}

} // namespace

int main() {
    RunPlainThenTwoShiftClicksChecks();
    RunCtrlClickStillMovesAnchorChecks();
    RunCanvasOnlyClickEstablishesAnchorChecks();
    RunCtrlWithinShiftRangeChecks();
    RunShiftClickBackwardStillPrimarysTheClickedRowChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
