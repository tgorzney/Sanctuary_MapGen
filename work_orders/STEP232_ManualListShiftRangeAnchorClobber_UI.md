# STEP232 — Shift-range anchor (and the list's own multi-select set) get clobbered by the canvas selection-changed callback's synchronous echo

**Layer:** UI. **Domain:** `MapCanvas`'s selection-changed callback plumbing (`MapCanvas_UI.h`, `MapCanvas_UI.cpp`) and its one real consumer, `Application::WireCallbacks()`'s closure (`Application_UI.cpp`). **Executor:** SanGen Coder. Authored by the SanGen UI Expert. Fixes a confirmed REGRESSION, git-bisected to a specific commit — not new law, no ARCH ruling needed for the mechanism (widening an already-established callback signature the same way ARCH §21.1 itself was rolled out, `primary` alongside `selectedKeys`).

## Session coordination (required before EVERY file edit, not just once at ticket start)
Multiple Claude Code sessions may be active on this machine concurrently, editing the SAME working directory. A single check at the start of this ticket is NOT sufficient — a peer can start editing any of this ticket's files at any point after your initial check. Before EACH individual file edit in §1-8 below (not just once, up front):
1. Call `ListAgents` to enumerate active/open peer sessions on this machine.
2. Message each one (`SendMessage`) naming the SPECIFIC file you are about to edit right now, asking if they are currently editing it or planning to.
3. Wait for replies before making that edit.
4. If a peer reports current or planned work in that exact file, do NOT edit concurrently — negotiate a sequential order (whichever session is further along lands and merges first; the other rebases onto that afterward) and record the agreed order in this ticket's own notes before proceeding.
5. If no peer claims that file, proceed with that one edit — then repeat steps 1-4 for the NEXT file before editing it. A "no conflict" answer for one file is not an answer for another, and an answer from earlier in the session is not an answer for right now — re-check per file, every time.

**Sibling-ticket note:** `STEP231` (drafted alongside this ticket) also touches `MapCanvas_UI.h`/`Application_UI.cpp`, but a DIFFERENT region of each (`SetManualMarkerSelectionSource`'s retirement, an earlier part of the same closure in `Application_UI.cpp`) — no overlapping lines with this ticket's own edits, confirmed by direct comparison of both diffs at drafting time; re-diff both at merge time regardless.

## Root cause, confirmed by direct read and `git log -p`
`Application_UI.cpp:93-118`, `canvas.SetSelectionChangedCallback(...)`, specifically line 109:
```cpp
tabState.markers.manualInstanceSelectionAnchorIdentifier = bPrimaryIsManualMarker ? primary.instanceIndex : -1;
```
This fires SYNCHRONOUSLY on every canvas selection change — including changes triggered by the SAME list-row click that just ran. The click flow (`MarkersTab_ManualLayerRowBody_UI.cpp:34-52`, `DrawManualInstanceRow`) is: (1) `ApplyManualInstanceSelectionClick` runs first and, on a Shift-click with a valid pre-existing anchor, computes the range and explicitly does NOT touch `anchorIdentifier` — its own code comment says so verbatim: `"anchor unchanged -- repeated Shift-clicks range from the same start"` (`MarkersTab_ManualInstanceSelection_UI.cpp:21`); (2) then `interaction.selectManualMarkerInstanceCallback(instanceIdentifier, bCtrl, bShift)` fires, which drives `MapCanvas::SelectManualMarkerByInstanceIdentifier` → `ApplySelectionGesture` → since the canvas's own `selectedInstanceKeys` set changed, the selection-changed callback above fires synchronously → line 109 unconditionally overwrites the anchor with `primary.instanceIndex` — silently clobbering the anchor `ApplyManualInstanceSelectionClick` deliberately just preserved, within the SAME click.

**Confirmed via `git log -p -S "manualInstanceSelectionAnchorIdentifier = bPrimaryIsManualMarker" -- src/ui/Application_UI.cpp`: this is a REGRESSION, introduced in commit `f1d35e9` (STEP168, "ordered multi-select set... ARCH §21.1").** Before that commit, this callback only reset the anchor to -1 inside the `!key.bValid` (deselect) branch — a VALID selection never touched the anchor at all. STEP168's rewrite replaced that branch structure and, in the process, made the anchor write unconditional on every valid primary, not just on deselect.

## A second, closely-related clobber discovered while implementing this fix — folded into the same mechanism, same ticket
Application's closure does not JUST clobber the anchor. Its very first statement, `tabState.markers.selectedManualInstanceIdentifiers.clear(); ...push_back(...)` (lines 95-98), rebuilds the LIST's own plural multi-select field from the canvas's own `selectedKeys` — but `selectedManualInstanceIdentifiers` is the EXACT SAME field `ApplyManualInstanceSelectionClick` (step 1 of the SAME click, above) just wrote a correct RANGE into (`interaction.selectedIdentifiers = &selectedManualInstanceIdentifiers`, `MarkersTab_ManualLayerRowBody_UI.cpp:119`), and `DrawManualInstanceRow`'s own row-highlight check (`bRowSelected`) reads THIS field directly. `MapCanvas::SelectManualMarkerByInstanceIdentifier`'s Shift branch only ever `UnionIntoSelectionSet`s the ONE clicked key into the canvas's own `selectedInstanceKeys` — it is NOT a true range-select (confirmed by direct read, `MapCanvas_UI.cpp:85-97`) — so on a Shift-click extending a range, the canvas's own key set ends up STRICTLY NARROWER than the range the list itself just correctly computed (e.g. clicking 20 then Shift-clicking 40: the list correctly computes `{20, 30, 40}`; the canvas's own set is only `{20, 40}`, missing the un-clicked in-between row 30). Since this same-click callback rebuilds `selectedManualInstanceIdentifiers` from the canvas's OWN narrower set immediately after step 1 wrote the correct range, the LIST'S OWN VISIBLE HIGHLIGHT ends up wrong on every single Shift-click — **this, not merely the anchor, is the literal mechanism behind the human's own report ("still selects individual instances and not multiples")** — the anchor bug alone would still leave a correct RANGE selected once the anchor stopped moving; this second clobber is what actually corrupts the displayed range itself, and blocks this ticket's own specified acceptance test (a real 3-click chain ending with the full correct range) from passing if fixed alone. Both clobbers share the identical mechanism (the same synchronous same-click echo) and the identical fix shape, so this ticket fixes both together rather than filing a second ticket for a bug discovered while implementing this one.

## Required reading
`ARCH_21_01_MultiSelectRepresentation.md` §21.1 (the ordered set / primary-is-last-element contract, and `ApplySelectionGesture`'s own three-way Replace/Toggle/Union resolution this ticket's new parameter is sourced FROM, not a new resolution rule). `MarkersTab_ManualInstanceSelection_UI.cpp` in full (`ApplyManualInstanceSelectionClick`'s own ratified rule: plain/Ctrl click moves the anchor to the new primary; Shift click does not — confirmed lines 12-33).

---

## 1. Modified: `src/ui/MapCanvas_UI.h`

Widen the callback's own type in both places it appears. Currently (lines 87-91, `SetSelectionChangedCallback`):
```cpp
    void SetSelectionChangedCallback(
        std::function<void(const OverlayInstanceKey_UI& primary, const OverlayInstanceKeySet_UI& selectedKeys)>
            selectionChanged) {
        selectionChangedCallback = std::move(selectionChanged);
    }
```
Replace with:
```cpp
    // STEP232 — widened again to also carry whether the gesture that produced this change was
    // Shift-modified, sourced from ApplySelectionGesture's own bShiftHeld argument at the exact point
    // it fires this callback (MapCanvas_UI.cpp, both overloads). Every existing caller (there is
    // exactly one production wiring site, Application::WireCallbacks()) must add the third parameter;
    // it is NOT optional/defaulted, deliberately — a caller silently ignoring it would reproduce
    // exactly the bug this ticket exists to fix if it ever needed the anchor-preservation rule and
    // forgot to read the new parameter.
    void SetSelectionChangedCallback(
        std::function<void(const OverlayInstanceKey_UI& primary, const OverlayInstanceKeySet_UI& selectedKeys,
                           bool bWasShiftGesture)>
            selectionChanged) {
        selectionChangedCallback = std::move(selectionChanged);
    }
```
Currently (lines 369-370, the field):
```cpp
    std::function<void(const OverlayInstanceKey_UI& primary, const OverlayInstanceKeySet_UI& selectedKeys)>
        selectionChangedCallback;
```
Replace with:
```cpp
    std::function<void(const OverlayInstanceKey_UI& primary, const OverlayInstanceKeySet_UI& selectedKeys,
                       bool bWasShiftGesture)>
        selectionChangedCallback;
```

---

## 2. Modified: `src/ui/MapCanvas_UI.cpp`

Both `ApplySelectionGesture` overloads fire the callback; both gain the third argument, each from its OWN already-in-scope `bShiftHeld` parameter — no new plumbing, no new state. Currently (lines 85-97, the single-key overload):
```cpp
void MapCanvas::ApplySelectionGesture(const OverlayInstanceKey_UI& touchedKey, bool bCtrlHeld, bool bShiftHeld) {
    const OverlayInstanceKeySet_UI previous = selectedInstanceKeys;
    if (bCtrlHeld) {
        ToggleInSelectionSet(selectedInstanceKeys, touchedKey);
    } else if (bShiftHeld) {
        UnionIntoSelectionSet(selectedInstanceKeys, {touchedKey});
    } else {
        ReplaceSelectionSet(selectedInstanceKeys, {touchedKey});
    }
    if (SelectionSetsEqual(previous, selectedInstanceKeys)) return;
    if (selectionChangedCallback)
        selectionChangedCallback(PrimaryOfSelectionSet(selectedInstanceKeys), selectedInstanceKeys);
}
```
Replace the final two lines with:
```cpp
    if (SelectionSetsEqual(previous, selectedInstanceKeys)) return;
    // STEP232 — bShiftHeld, already this function's own parameter, threaded through unchanged so the
    // callback's consumer can apply the SAME "don't move the anchor on Shift" rule
    // ApplyManualInstanceSelectionClick's own list-side logic already implements.
    if (selectionChangedCallback)
        selectionChangedCallback(PrimaryOfSelectionSet(selectedInstanceKeys), selectedInstanceKeys, bShiftHeld);
}
```
Currently (lines 104-117, the batch overload):
```cpp
void MapCanvas::ApplySelectionGesture(const std::vector<OverlayInstanceKey_UI>& touchedKeys, bool bCtrlHeld,
                                      bool bShiftHeld) {
    const OverlayInstanceKeySet_UI previous = selectedInstanceKeys;
    if (bCtrlHeld) {
        ToggleEachInSelectionSet(selectedInstanceKeys, touchedKeys);
    } else if (bShiftHeld) {
        UnionIntoSelectionSet(selectedInstanceKeys, touchedKeys);
    } else {
        ReplaceSelectionSet(selectedInstanceKeys, touchedKeys);
    }
    if (SelectionSetsEqual(previous, selectedInstanceKeys)) return;
    if (selectionChangedCallback)
        selectionChangedCallback(PrimaryOfSelectionSet(selectedInstanceKeys), selectedInstanceKeys);
}
```
Replace the final two lines with:
```cpp
    if (SelectionSetsEqual(previous, selectedInstanceKeys)) return;
    // STEP232 — same widening as the single-key overload above, for the marquee/list-batch path.
    if (selectionChangedCallback)
        selectionChangedCallback(PrimaryOfSelectionSet(selectedInstanceKeys), selectedInstanceKeys, bShiftHeld);
}
```

---

## 3. Modified: `src/ui/Application_UI.cpp`

The actual behavior fix — both clobbers gated on the same new signal. Currently (lines 93-118):
```cpp
    canvas.SetSelectionChangedCallback([this](const OverlayInstanceKey_UI& primary,
                                              const OverlayInstanceKeySet_UI& selectedKeys) {
        tabState.markers.selectedManualInstanceIdentifiers.clear();
        for (const OverlayInstanceKey_UI& key : selectedKeys.keys)
            if (key.bValid && key.collection == PlacementCollectionKind_UI::Markers && key.bManual)
                tabState.markers.selectedManualInstanceIdentifiers.push_back(key.instanceIndex);

        // STEP143 (human's own bug report) — an empty-space click's own synthetic miss-key always
        // constructs with bManual == false, so gating purely on bManual could never route a miss back
        // to the Markers tab's own manual selection — the row stayed highlighted after clicking empty
        // space. Re-derived from the SET's primary here (not the lone callback argument §19.25 read),
        // for exactly the same reason: an invalid primary (miss or explicit clear) always resolves
        // both fields to "nothing selected" together.
        const bool bPrimaryIsManualMarker = primary.bValid
            && primary.collection == PlacementCollectionKind_UI::Markers && primary.bManual;
        tabState.markers.selectedManualInstanceIdentifier        = bPrimaryIsManualMarker ? primary.instanceIndex : -1;
        tabState.markers.manualInstanceSelectionAnchorIdentifier = bPrimaryIsManualMarker ? primary.instanceIndex : -1;

        if (!primary.bValid) {
            lastSelectedEntityIdentifier = Data::EntityIdBuffer::emptySentinel;
        } else if (!primary.bManual) {
            lastSelectedEntityIdentifier = static_cast<std::uint32_t>(primary.instanceIndex);
        }
        // A valid MANUAL primary touches neither `lastSelectedEntityIdentifier` — it stays whatever
        // the last procedural selection left it, exactly §19.25's own original behavior.
    });
```
Replace with:
```cpp
    // STEP232 — widened to also receive bWasShiftGesture (sourced from ApplySelectionGesture's own
    // bShiftHeld at the point it fires this callback, MapCanvas_UI.cpp). TWO independent same-click
    // clobbers are fixed by gating on it, both discovered/fixed together (see this ticket's own
    // root-cause writeup for the full mechanism and git-bisect):
    //  1. selectedManualInstanceIdentifiers (plural) used to resync UNCONDITIONALLY from the canvas's
    //     own key set every time. On a list Shift-click, ApplyManualInstanceSelectionClick had ALREADY
    //     written the real, richer [anchor..clicked] RANGE into this exact field one call earlier
    //     (MarkersTab_ManualLayerRowBody_UI.cpp) — but SelectManualMarkerByInstanceIdentifier's own
    //     Shift branch only Unions the ONE clicked key into the canvas's set, never the whole range,
    //     so resyncing from THAT set immediately discarded every "in-between" row the list itself had
    //     just correctly selected. This is the actual mechanism behind "still selects individual
    //     instances and not multiples."
    //  2. manualInstanceSelectionAnchorIdentifier — see its own comment below.
    // Deliberate, documented trade-off (not a new regression — see this ticket's own Explicit
    // out-of-scope): a Shift-held CANVAS MARQUEE (no list click involved at all) no longer syncs its
    // resulting set into selectedManualInstanceIdentifiers either, since this callback has no way to
    // distinguish "the list just computed something richer, don't clobber it" from "the canvas is the
    // ONLY source of truth here, please sync it in" — both look identical as bWasShiftGesture=true. A
    // plain/Ctrl gesture (list-driven OR canvas-driven) is completely unaffected either way:
    // SelectManualMarkerByInstanceIdentifier's Replace/Toggle branches already produce a set IDENTICAL
    // to what the list itself computed, so resyncing from the canvas's own set there was always a
    // harmless no-op.
    canvas.SetSelectionChangedCallback([this](const OverlayInstanceKey_UI& primary,
                                              const OverlayInstanceKeySet_UI& selectedKeys,
                                              bool bWasShiftGesture) {
        if (!bWasShiftGesture) {
            tabState.markers.selectedManualInstanceIdentifiers.clear();
            for (const OverlayInstanceKey_UI& key : selectedKeys.keys)
                if (key.bValid && key.collection == PlacementCollectionKind_UI::Markers && key.bManual)
                    tabState.markers.selectedManualInstanceIdentifiers.push_back(key.instanceIndex);
        }

        // STEP143 (human's own bug report) — an empty-space click's own synthetic miss-key always
        // constructs with bManual == false, so gating purely on bManual could never route a miss back
        // to the Markers tab's own manual selection — the row stayed highlighted after clicking empty
        // space. Re-derived from the SET's primary here (not the lone callback argument §19.25 read),
        // for exactly the same reason: an invalid primary (miss or explicit clear) always resolves
        // both fields to "nothing selected" together. UNCHANGED by this ticket — always runs,
        // regardless of bWasShiftGesture, since it is not the field the confirmed bug lives in.
        const bool bPrimaryIsManualMarker = primary.bValid
            && primary.collection == PlacementCollectionKind_UI::Markers && primary.bManual;
        tabState.markers.selectedManualInstanceIdentifier = bPrimaryIsManualMarker ? primary.instanceIndex : -1;
        // STEP232 fix #2, the anchor. ApplyManualInstanceSelectionClick's own rule (the list-click
        // path, MarkersTab_ManualInstanceSelection_UI.cpp) is: plain/Ctrl click -> anchor becomes the
        // new primary; Shift click -> anchor is UNCHANGED, "so repeated Shift-clicks range from the
        // same start." Before this fix this write ran unconditionally on every valid selection change,
        // including the SAME-click, canvas-driven echo a list Shift-click's own
        // selectManualMarkerInstanceCallback triggers synchronously — silently overwriting the anchor
        // ApplyManualInstanceSelectionClick had just deliberately preserved a few lines earlier in the
        // exact same click (git-bisected to STEP168/f1d35e9 — see this ticket's own root-cause
        // writeup). A canvas-only plain/Ctrl click (no list involved at all) still moves the anchor
        // here — a deliberate, desired side effect: it means a SUBSEQUENT list Shift-click ranges from
        // wherever the user last plain/Ctrl-clicked on the CANVAS too, not just from a prior list
        // click, the more useful cross-context behavior a user would expect (see this ticket's own
        // Interpretation calls for why a narrower "list-only" anchor was rejected).
        if (!bWasShiftGesture)
            tabState.markers.manualInstanceSelectionAnchorIdentifier = bPrimaryIsManualMarker ? primary.instanceIndex : -1;

        if (!primary.bValid) {
            lastSelectedEntityIdentifier = Data::EntityIdBuffer::emptySentinel;
        } else if (!primary.bManual) {
            lastSelectedEntityIdentifier = static_cast<std::uint32_t>(primary.instanceIndex);
        }
        // A valid MANUAL primary touches neither `lastSelectedEntityIdentifier` — it stays whatever
        // the last procedural selection left it, exactly §19.25's own original behavior.
    });
```

---

## 4. Modified: `src/ui/MapCanvas_ActivePanelGate_UI_Test.cpp`

Widen the lambda signature (the `std::function` type change means this no longer compiles unmodified). Currently (line 228):
```cpp
    canvas.SetSelectionChangedCallback([&](const OverlayInstanceKey_UI& key, const OverlayInstanceKeySet_UI&) {
        lastKey = key; ++selectionChangeCount;
    });
```
Replace with:
```cpp
    canvas.SetSelectionChangedCallback([&](const OverlayInstanceKey_UI& key, const OverlayInstanceKeySet_UI&, bool) {
        lastKey = key; ++selectionChangeCount;
    });
```

---

## 5. Modified: `src/ui/MapCanvas_GestureOwnership_UI_Test.cpp`

Currently (line 145):
```cpp
    canvas.SetSelectionChangedCallback([&](const OverlayInstanceKey_UI&, const OverlayInstanceKeySet_UI& selectedKeys) {
        lastReportedSet = selectedKeys;
    });
```
Replace with:
```cpp
    canvas.SetSelectionChangedCallback([&](const OverlayInstanceKey_UI&, const OverlayInstanceKeySet_UI& selectedKeys, bool) {
        lastReportedSet = selectedKeys;
    });
```

---

## 6. Modified: `src/ui/MapCanvas_Picking_UI_Test.cpp`

Three call sites, all the same mechanical widening. Currently (line 59):
```cpp
    canvas.SetSelectionChangedCallback([&](const OverlayInstanceKey_UI& key, const OverlayInstanceKeySet_UI&) {
        reportedSelection = static_cast<std::uint32_t>(key.instanceIndex);
        ++selectionChangeCount;
    });
```
Replace with:
```cpp
    canvas.SetSelectionChangedCallback([&](const OverlayInstanceKey_UI& key, const OverlayInstanceKeySet_UI&, bool) {
        reportedSelection = static_cast<std::uint32_t>(key.instanceIndex);
        ++selectionChangeCount;
    });
```
Currently (line 123):
```cpp
    canvas.SetSelectionChangedCallback([&](const OverlayInstanceKey_UI& key, const OverlayInstanceKeySet_UI&) {
        lastReportedKey = key; ++selectionChangeCount;
    });
```
Replace with:
```cpp
    canvas.SetSelectionChangedCallback([&](const OverlayInstanceKey_UI& key, const OverlayInstanceKeySet_UI&, bool) {
        lastReportedKey = key; ++selectionChangeCount;
    });
```
Currently (line 189):
```cpp
    canvas.SetSelectionChangedCallback([&](const OverlayInstanceKey_UI& key, const OverlayInstanceKeySet_UI&) {
        lastReportedKey = key; ++selectionChangeCount;
    });
```
Replace with:
```cpp
    canvas.SetSelectionChangedCallback([&](const OverlayInstanceKey_UI& key, const OverlayInstanceKeySet_UI&, bool) {
        lastReportedKey = key; ++selectionChangeCount;
    });
```

---

## 7. New test file: `src/ui/MarkersTab_ListCanvasSelectionSync_UI_Test.cpp`

Headless, pure-data-structure, no imgui/GL — per the human's own explicit constraint. `MapCanvas`'s own gesture methods (`SelectManualMarkerByInstanceIdentifier`/`ApplySelectionGesture`) are confirmed imgui-free (`MapCanvas_UI.cpp`'s own header comment: "with no imgui in sight") and this test calls none of the composite/picking/draw methods that would need a GL context — a bare, default-constructed `MapCanvas` is sufficient. Mirrors `MarkersTab_ManualInstanceSelection_UI_Test.cpp`'s own established `int main()` style exactly (same `Check`/failure-count idiom, no gtest). Reproduces `Application::WireCallbacks()`'s own FIXED closure logic locally (that closure is a private lambda inside `Application`, which needs a GL context this test deliberately avoids to construct) — the SAME established precedent `MapCanvas_Picking_UI_Test.cpp`'s own tests already use (wiring a LOCAL lambda into `SetSelectionChangedCallback` rather than constructing a real `Application`). Also reproduces `DrawManualInstanceRow`'s own real click-handler CALL ORDER (list-write, then canvas-sync call) — the exact same-click race the real bug lived in — driving the REAL, compiled `ApplyManualInstanceSelectionClick` (`MarkersTab_ManualInstanceSelection_UI.cpp`) and the REAL, compiled `MapCanvas::SelectManualMarkerByInstanceIdentifier`/`ApplySelectionGesture` (`MapCanvas_UI.cpp`), not a reimplementation of either.

```cpp
// MarkersTab_ListCanvasSelectionSync_UI_Test.cpp — STEP232 acceptance: a real plain-click-then-two-
// Shift-clicks-in-sequence chain, driving ApplyManualInstanceSelectionClick (the list side,
// MarkersTab_ManualInstanceSelection_UI.cpp) and MapCanvas::ApplySelectionGesture (the canvas side,
// MapCanvas_UI.cpp) through plain C++ calls exactly as DrawManualInstanceRow's own real click handler
// does (MarkersTab_ManualLayerRowBody_UI.cpp:34-52) — ends with the FULL correct range selected, and
// proves the anchor survives TWO consecutive Shift-clicks, not just one. Pure logic, no imgui/GL:
// MapCanvas's own gesture methods are confirmed imgui-free (MapCanvas_UI.cpp's own header comment,
// "with no imgui in sight") and this test calls none of the picking/draw methods that would need a
// composite/GL context. Per the human's own explicit instruction, this is deliberately NOT a
// GL-backed/real-imgui-frame test — mirrors MarkersTab_ManualInstanceSelection_UI_Test.cpp's own
// established int main() style exactly.
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

// Mirrors MarkersTabState's own three relevant fields exactly (selectedManualInstanceIdentifier /
// selectedManualInstanceIdentifiers / manualInstanceSelectionAnchorIdentifier — MarkersTab_UI.h).
struct FakeMarkersTabState {
    int selectedManualInstanceIdentifier = -1;
    std::vector<int> selectedManualInstanceIdentifiers;
    int manualInstanceSelectionAnchorIdentifier = -1;
};

// Reproduces Application::WireCallbacks()'s own selectionChangedCallback closure EXACTLY as this
// ticket's own Application_UI.cpp diff specifies it (both gated writes) — MapCanvas_Picking_UI_Test.cpp
// already established the precedent of wiring a LOCAL lambda rather than constructing a real
// Application (which needs a GL context this test deliberately avoids).
void WireFixedSelectionChangedCallback(MapCanvas& canvas, FakeMarkersTabState& tabState) {
    canvas.SetSelectionChangedCallback([&tabState](const OverlayInstanceKey_UI& primary,
                                                   const OverlayInstanceKeySet_UI& selectedKeys,
                                                   bool bWasShiftGesture) {
        if (!bWasShiftGesture) {
            tabState.selectedManualInstanceIdentifiers.clear();
            for (const OverlayInstanceKey_UI& key : selectedKeys.keys)
                if (key.bValid && key.collection == PlacementCollectionKind_UI::Markers && key.bManual)
                    tabState.selectedManualInstanceIdentifiers.push_back(key.instanceIndex);
        }
        const bool bPrimaryIsManualMarker = primary.bValid
            && primary.collection == PlacementCollectionKind_UI::Markers && primary.bManual;
        tabState.selectedManualInstanceIdentifier = bPrimaryIsManualMarker ? primary.instanceIndex : -1;
        if (!bWasShiftGesture)
            tabState.manualInstanceSelectionAnchorIdentifier = bPrimaryIsManualMarker ? primary.instanceIndex : -1;
    });
}

// Reproduces DrawManualInstanceRow's own real click-handler call order EXACTLY
// (MarkersTab_ManualLayerRowBody_UI.cpp:34-52): the list-local write first, THEN the canvas-sync
// call, which fires the callback above SYNCHRONOUSLY before this function returns — the exact
// same-click race the real bug lived in.
void SimulateListClick(MapCanvas& canvas, FakeMarkersTabState& tabState, const std::vector<int>& rowOrder,
                       int clickedIdentifier, bool bCtrl, bool bShift) {
    ApplyManualInstanceSelectionClick(rowOrder, clickedIdentifier, bCtrl, bShift,
                                      tabState.selectedManualInstanceIdentifiers,
                                      tabState.manualInstanceSelectionAnchorIdentifier);
    tabState.selectedManualInstanceIdentifier = clickedIdentifier;
    canvas.SelectManualMarkerByInstanceIdentifier(clickedIdentifier, bCtrl, bShift);
}

// The actual acceptance scenario: plain click, then TWO Shift-clicks in sequence, extending in
// different directions from the SAME original anchor — proves the anchor (and the full range)
// survives both, not just one.
void RunPlainThenTwoShiftClicksChecks() {
    const std::vector<int> rowOrder{ 10, 20, 30, 40, 50 };
    MapCanvas canvas;
    FakeMarkersTabState tabState;
    WireFixedSelectionChangedCallback(canvas, tabState);

    // Click 1 — plain click on 20: establishes the anchor at 20.
    SimulateListClick(canvas, tabState, rowOrder, 20, /*bCtrl=*/false, /*bShift=*/false);
    Check(Equal(tabState.selectedManualInstanceIdentifiers, { 20 }), "plain click selects just 20");
    Check(tabState.manualInstanceSelectionAnchorIdentifier == 20, "and the anchor becomes 20");

    // Click 2 — Shift-click on 40: ranges [20..40] = {20,30,40}. Pre-fix, the canvas's own
    // synchronous echo would have clobbered this down to whatever SelectManualMarkerByInstanceIdentifier's
    // single-key Union produced instead ({20,40}, missing the un-clicked in-between row 30).
    SimulateListClick(canvas, tabState, rowOrder, 40, /*bCtrl=*/false, /*bShift=*/true);
    Check(Equal(tabState.selectedManualInstanceIdentifiers, { 20, 30, 40 }),
         "STEP232 - Shift-click ranges the FULL [anchor..clicked] span, INCLUDING the un-clicked "
         "in-between row (30) - the canvas's own narrower same-click echo no longer clobbers it");
    Check(tabState.manualInstanceSelectionAnchorIdentifier == 20,
         "STEP232 - the anchor survives this Shift-click unmoved (the actual bug: it used to snap to "
         "40, the just-clicked row, within this SAME click)");

    // Click 3 — a SECOND Shift-click, this time on 10 (the OTHER direction from the SAME anchor).
    // Pre-fix, this would range from whatever click 2 had silently snapped the anchor to (40), giving
    // a wrong span instead of the correct [10..20] from the TRUE original anchor.
    SimulateListClick(canvas, tabState, rowOrder, 10, /*bCtrl=*/false, /*bShift=*/true);
    Check(Equal(tabState.selectedManualInstanceIdentifiers, { 10, 20 }),
         "STEP232 - a SECOND Shift-click still ranges from the SAME original anchor (20), not from "
         "click 2's own clicked row (40) - proves the anchor survived TWO consecutive Shift-clicks, "
         "not just one, matching the human's own reported symptom (a collapsed 1-2-row span on every "
         "click after the first)");
    Check(tabState.manualInstanceSelectionAnchorIdentifier == 20,
         "the anchor is STILL 20 after the second Shift-click too");
}

// A Ctrl-click DOES move the anchor (matching ApplyManualInstanceSelectionClick's own rule) — proves
// the fix's gate is specifically on Shift, not on "any modifier."
void RunCtrlClickStillMovesAnchorChecks() {
    const std::vector<int> rowOrder{ 10, 20, 30 };
    MapCanvas canvas;
    FakeMarkersTabState tabState;
    WireFixedSelectionChangedCallback(canvas, tabState);

    SimulateListClick(canvas, tabState, rowOrder, 10, /*bCtrl=*/false, /*bShift=*/false);
    SimulateListClick(canvas, tabState, rowOrder, 20, /*bCtrl=*/true, /*bShift=*/false);
    Check(Equal(tabState.selectedManualInstanceIdentifiers, { 10, 20 }), "Ctrl-click adds 20, keeping 10");
    Check(tabState.manualInstanceSelectionAnchorIdentifier == 20,
         "a Ctrl-click DOES move the anchor to the just-clicked row, matching "
         "ApplyManualInstanceSelectionClick's own rule — this fix's gate is specifically on Shift, not "
         "on 'any modifier'");
}

// A canvas-ONLY plain click (no list click involved) still moves the anchor — the deliberate,
// desired cross-context behavior this ticket's own Interpretation calls settle on.
void RunCanvasOnlyClickEstablishesAnchorChecks() {
    MapCanvas canvas;
    FakeMarkersTabState tabState;
    WireFixedSelectionChangedCallback(canvas, tabState);

    // A canvas-only selection with no prior list interaction at all — SelectManualMarkerByInstanceIdentifier
    // is exactly what a real canvas click resolves to (MapCanvas_UI.cpp:51-56).
    canvas.SelectManualMarkerByInstanceIdentifier(77, /*bCtrlHeld=*/false, /*bShiftHeld=*/false);
    Check(tabState.manualInstanceSelectionAnchorIdentifier == 77,
         "STEP232 - a canvas-only plain click establishes a sane anchor too, so a SUBSEQUENT list "
         "Shift-click can range from wherever the user last clicked on the canvas, not just from a "
         "prior list click");
}

} // namespace

int main() {
    RunPlainThenTwoShiftClicksChecks();
    RunCtrlClickStillMovesAnchorChecks();
    RunCanvasOnlyClickEstablishesAnchorChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
```

---

## 8. Modified: `CMakeLists.txt`

Register the new standalone test target. Insert near the existing `MarkersTab_ManualInstanceSelection_UI_Test` registration (currently lines 830-833), mirroring its exact shape:
```cmake
# STEP141: MarkersTab_ManualInstanceSelection_UI.h's own pure logic (Ctrl/Shift multi-select,
# ReassignManualInstanceLayers) — pure, no imgui frame needed.
add_sangen_test(MarkersTab_ManualInstanceSelection_UI_Test
    src/ui/MarkersTab_ManualInstanceSelection_UI_Test.cpp)
```
becomes
```cmake
# STEP141: MarkersTab_ManualInstanceSelection_UI.h's own pure logic (Ctrl/Shift multi-select,
# ReassignManualInstanceLayers) — pure, no imgui frame needed.
add_sangen_test(MarkersTab_ManualInstanceSelection_UI_Test
    src/ui/MarkersTab_ManualInstanceSelection_UI_Test.cpp)
# STEP232 — a real ApplyManualInstanceSelectionClick + MapCanvas::ApplySelectionGesture click-chain
# acceptance, no imgui/GL (MapCanvas's own gesture methods are imgui-free).
add_sangen_test(MarkersTab_ListCanvasSelectionSync_UI_Test
    src/ui/MarkersTab_ListCanvasSelectionSync_UI_Test.cpp)
```

---

## ARCH rules invoked
- `ARCH_21_01_MultiSelectRepresentation.md` §21.1 — `ApplySelectionGesture`'s own three-way Replace/Toggle/Union resolution and `bShiftHeld` parameter are UNCHANGED; this ticket only widens what already gets threaded OUT of it via the callback, using data the function already has in scope.
- Constitution §1 — UI sets PARAMS and reacts to gestures; this ticket adds no sim logic, only corrects an ordering/clobber bug in a pure state-sync closure.
- Constitution §6 — no new crash surface: every gated write still has a well-defined value on every path (the anchor/list-set simply keep their PRIOR value instead of a stale overwrite when `bWasShiftGesture` is true).

## Explicit out-of-scope
- **No fix for the Shift-marquee → `selectedManualInstanceIdentifiers` sync gap this ticket's own gate introduces** (a Shift-held canvas marquee, with no list click involved, no longer resyncs the list's plural field). This is a genuine, narrower trade-off of the single `bWasShiftGesture` signal — an ORIGIN-aware fix (distinguishing "list already computed something richer" from "canvas is the only source of truth") would need a separate signal threaded from `ApplyMarqueeGesture`/`ApplyClickGesture` (canvas-driven) vs. `SelectManualMarkerByInstanceIdentifier`/`SelectProceduralMarkerInstanceByArrayPosition` (list-driven) — a larger redesign than this ticket's specified scope (the anchor clobber, plus the one directly-blocking sibling clobber discovered while implementing it).
- **No change to `MapCanvas_SelectionSet_UI.h`/`.cpp`'s own `ToggleInSelectionSet`/`UnionIntoSelectionSet`/`ToggleEachInSelectionSet`/`ReplaceSelectionSet`** — all already correct (STEP230), untouched.
- **No change to `ApplyManualInstanceSelectionClick`'s own rule** (`MarkersTab_ManualInstanceSelection_UI.cpp`) — it was always correct; the bug was entirely in how its OWN output got clobbered downstream, one call later, in the SAME click.
- **No change to `SelectProceduralMarkerInstanceByArrayPosition`'s own call path** — per STEP205's own confirmed finding, "no tab-local plural-selection field exists for Procedural instances to stomp," so the second (plural-field) clobber this ticket fixes has no procedural analog. The anchor-only fix (§3 above) still applies uniformly to `bWasShiftGesture` regardless of which entry point produced it, at zero extra cost.
- **No GL-backed/real-imgui-frame test** — per the human's own explicit instruction; §7's new test is deliberately pure-data-structure, headless, `int main()`-style.
- **No ARCH file edit** — this ticket fixes a confirmed regression against already-ratified §21.1 behavior; it invents no new rule.

## Acceptance test
1. `MarkersTab_ListCanvasSelectionSync_UI_Test` (new `ctest` binary) passes `ALL PASS`: a plain-click-then-two-Shift-clicks-in-sequence chain ends with the FULL correct range selected on both the list's own plural field AND the anchor staying pinned at the original click across both Shift-clicks (not collapsing to a 1-2-row span, matching the human's own reported symptom exactly); a Ctrl-click still moves the anchor (proving the gate is Shift-specific, not "any modifier"); a canvas-only plain click still establishes a usable anchor for a later list Shift-click.
2. `MapCanvas_ActivePanelGate_UI_Test`, `MapCanvas_GestureOwnership_UI_Test`, `MapCanvas_Picking_UI_Test` (all pre-existing `ctest` binaries) continue to pass `ALL PASS` unmodified in substance, with only the mechanical 3-parameter lambda-signature widening applied.
3. Full `SanGenV2` build stays clean; every existing test in the suite continues to pass.

## Interpretation calls made
1. **`bWasShiftGesture` is threaded as a THIRD callback parameter, sourced from `ApplySelectionGesture`'s own already-in-scope `bShiftHeld`**, rather than any origin-aware ("did this come from the list or the canvas") signal. This is the exact mechanism the ticket brief itself proposed, and it is the only signal `ApplySelectionGesture` can express without inventing new state anywhere else — both `SelectManualMarkerByInstanceIdentifier` (list-driven) and `ApplyClickGesture`/`ApplyMarqueeGesture` (canvas-driven) already funnel through this SAME function, so "was Shift held" is the natural, already-available discriminator matching `ApplyManualInstanceSelectionClick`'s own rule bit-for-bit.
2. **A canvas-only plain/Ctrl click DOES move the anchor.** The callback cannot distinguish "this echo came from a list click" from "this is a genuine canvas-only click" — gating purely on `bWasShiftGesture` (not on origin) means a canvas click naturally ALSO establishes a sane anchor for a later list Shift-click. Investigated and deliberately kept: this is the more useful, expected cross-context behavior (§7's `RunCanvasOnlyClickEstablishesAnchorChecks` proves it explicitly), not an accidental side effect being papered over.
3. **The second clobber (`selectedManualInstanceIdentifiers`, plural) is fixed in THIS ticket, not filed separately.** It shares the exact same mechanism (a same-click synchronous echo overwriting what the list just correctly computed) and the exact same fix shape (`!bWasShiftGesture` gate) as the anchor bug this ticket was already scoped to fix — and, critically, it directly blocks this ticket's OWN specified acceptance test (a full correct range ending up selected) from being achievable at all if left unfixed. Filing it separately would mean this ticket's own acceptance test could never pass on its own merits.
4. **The Shift-marquee list-sync trade-off is accepted, not fixed, and explicitly documented** (see Explicit out-of-scope) rather than pursuing a full origin-aware redesign — the latter is a materially larger, separate architectural change (a new signal threaded through `ApplyMarqueeGesture`/`ApplyClickGesture` in addition to the list-driven entry points) that goes well beyond "the anchor gets clobbered," the ticket's own specified scope.
5. **`SetSelectionChangedCallback`'s new third parameter is NOT defaulted.** A silent default would let a future caller ignore it and reproduce a class of the same bug (never knowing to gate anything on it); every one of the five existing call sites is a `ctest` binary that gets updated mechanically in this same ticket, so there is no compatibility cost to making it mandatory.

## Key files read/cited while drafting
`D:\Projects\Sanctuary\Map Generator\src\ui\Application_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MarkersTab_ManualInstanceSelection_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MarkersTab_ManualInstanceSelection_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MarkersTab_ManualInstanceSelection_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MarkersTab_ManualLayerRowBody_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_ActivePanelGate_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_GestureOwnership_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_Picking_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\ARCH_21_01_MultiSelectRepresentation.md`,
`D:\Projects\Sanctuary\Map Generator\CMakeLists.txt` (line 828-833 region, `MarkersTab_ManualInstanceSelection_UI_Test` target),
`git log -p -S "manualInstanceSelectionAnchorIdentifier = bPrimaryIsManualMarker" -- src/ui/Application_UI.cpp` (the regression bisect to commit `f1d35e9`, STEP168),
and `work_orders\STEP214_AreaAltCenterResizeModifier_UI.md`/`STEP229_MarqueeMultiSelectHighlight_UI.md`/`STEP230_MarqueeCtrlToggleShiftUnion_UI.md` (structural/rigor templates and session-coordination wording, per the dispatching instruction).
