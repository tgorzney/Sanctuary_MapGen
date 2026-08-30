# STEP233 — Manual-marker list click desyncs the CANVAS's own real selection set from the list's own resolved selection (Ctrl-click inside a Shift-range collapses to the wrong keys; a companion anchor clobber found while fixing it)

**Layer:** UI. **Domain:** the list-click → canvas selection-sync boundary (`MapCanvas_UI.h`/`.cpp`,
`Application_UI.cpp`'s `WireCallbacks()`, `MarkersTab_ManualLayerRowBody_UI.cpp`'s `DrawManualInstanceRow`,
and every file that forwards `selectManualMarkerInstanceCallback`'s own type down the
`DrawMarkersTab -> DrawMarkerTypeSections -> {DrawMarkerLayerBundleTree, DrawManualMarkerLayerListBody,
DrawBaseSectionManualInstanceList} -> DrawLayerRowBody -> DrawManualInstanceRow` chain). **Executor:**
SanGen Coder. Authored by the SanGen UI Expert. Fixes a confirmed, root-caused REGRESSION-class bug
(same family as STEP232, one layer deeper) — not new law, no ARCH ruling needed: widens/repurposes an
already-established callback the same way STEP232 itself did (`bWasShiftGesture` alongside
`selectedKeys`), and adds one new canonical selection-mutator method beside `ApplySelectionGesture`,
mirroring that existing method's own shape (ARCH §21.1). Every file below was read FRESH against the
live, post-STEP232 tree while drafting this (STEP232's own changes are already in the working tree,
uncommitted) — this ticket's own diffs are against that exact current text, not any older snapshot.

## Session coordination (required before EVERY file edit, not just once at ticket start)
Same standing requirement as STEP229/230/231/232 — verbatim-equivalent: `ListAgents`/`SendMessage` check
before touching EACH file, not just once up front, negotiate sequential order on any real overlap,
re-verify at actual dispatch time regardless of any pre-check noted in this ticket. This ticket touches
`MapCanvas_UI.h`/`MapCanvas_UI.cpp`/`Application_UI.cpp`/`Application_UI.h` AGAIN (STEP231/STEP232 both
already landed changes there) — re-read the CURRENT state of every file fresh immediately before editing
it, do not trust this ticket's own quoted line numbers if a peer has landed something in between. This
ticket's own file list is unusually large (14 production files + 3 test files) — check per file, every
time, exactly as the prior tickets' own instruction states.

## Root cause, confirmed by direct read (two parts — both fixed by the same mechanism, same ticket)

### Part 1 — the reported bug: the canvas's OWN `selectedInstanceKeys` never actually held the list's range
`Application_UI.cpp:217-219` (current, post-STEP232):
```cpp
selectManualMarkerInstanceCallback = [this](int instanceIdentifier, bool bCtrlHeld, bool bShiftHeld) {
    canvas.SelectManualMarkerByInstanceIdentifier(instanceIdentifier, bCtrlHeld, bShiftHeld);
};
```
This receives ONLY the single clicked `instanceIdentifier` — never the full range/set the list side
(`ApplyManualInstanceSelectionClick`, `MarkersTab_ManualInstanceSelection_UI.cpp`) already computed.
`MapCanvas::SelectManualMarkerByInstanceIdentifier` (`MapCanvas_UI.cpp:51-56`) wraps the single-key
`ApplySelectionGesture` overload, whose Shift branch (`MapCanvas_UI.cpp:85-100`) does
`UnionIntoSelectionSet(selectedInstanceKeys, {touchedKey})` — unions in ONLY the one key just clicked.

Net effect after a list Shift-click chain (plain-click 20, shift-click 40, range shown = {20,25,30,35,40}):
the CANVAS's own real `selectedInstanceKeys` ends up `{20, 40}` — just the two literally-clicked
endpoints — even though the list's own display field (`selectedManualInstanceIdentifiers`, STEP232-fixed
to show the full range) is correct. A subsequent Ctrl-click on 30 (already inside the visible range) is a
canvas-level gesture (`ApplySelectionGesture`'s Ctrl branch, `ToggleInSelectionSet`) that resolves against
the canvas's own narrow `{20,40}` set — 30 is ABSENT from that set, so it gets APPENDED, producing
`{20,40,30}` — exactly the human's reported "collapses to the two shift-range endpoints plus whatever was
Ctrl-clicked." Confirmed by tracing every line above against the live tree.

**Why STEP232 didn't already catch this:** STEP232's own acceptance test
(`MarkersTab_ListCanvasSelectionSync_UI_Test.cpp`) only ever asserted against `tabState`'s OWN plural
field (`selectedManualInstanceIdentifiers`) — protected from the canvas's echo by the `bWasShiftGesture`
gate, which is `true` for a Shift click, so the resync that WOULD have pulled the wrong value in never ran
for STEP232's own scenario. It never asserted against `canvas.SelectedInstanceKeys()` itself (the CANVAS's
own real, independently-drifted set), and its own scenario never Ctrl-clicked a row inside an established
range — both gaps this ticket's own acceptance test closes.

### Part 2 — a second, closely-related clobber discovered while designing the fix (folded into this ticket, same reasoning STEP232 itself used for its own sibling clobber)
`ApplyManualInstanceSelectionClick`'s own Ctrl branch (`MarkersTab_ManualInstanceSelection_UI.cpp:24-30`)
sets `anchorIdentifier = clickedIdentifier` UNCONDITIONALLY — on BOTH a Ctrl-ADD and a Ctrl-DESELECT (the
human's own literal scenario: Ctrl-clicking an ALREADY-selected row). This is the list's own ratified rule
("Ctrl click: ... anchor becomes this one too ... so a FOLLOWING Shift-click ranges from here" — even on
deselect, per that header's own comment).

`Application_UI.cpp`'s `selectionChangedCallback` closure (STEP232) only skips its own anchor-overwrite
(`tabState.markers.manualInstanceSelectionAnchorIdentifier = bPrimaryIsManualMarker ? primary.instanceIndex : -1`)
when `bWasShiftGesture` is true — for a Ctrl click, `bWasShiftGesture` is `false`, so this write STILL
RUNS, deriving the anchor from the canvas's own `primary` (`PrimaryOfSelectionSet(selectedInstanceKeys)`)
instead of leaving the list's own already-correct write alone. For a Ctrl-DESELECT specifically, the
clicked identifier is no longer a member of ANY correctly-resolved set, so `primary` necessarily resolves
to something ELSE (whatever legitimately falls back to being "last" in the reduced set) — clobbering the
list's own correct anchor (the clicked, now-deselected id) with a DIFFERENT value. This is the exact same
clobber CLASS STEP232 fixed one instance of (for Shift); this is the Ctrl-deselect instance of it, newly
found while designing this ticket's own sync mechanism (its own primary-selection rule below makes this
concretely reachable and provably testable — see §7/§17 below).

## What the fix does — the sync mechanism (design finalized here, per this ticket's own dispatch instruction not to leave this for the coder to invent, Constitution §6)

The list side (`ApplyManualInstanceSelectionClick`) is already the single source of truth for the FINAL
resolved selection after ANY list click (plain/Ctrl/Shift) — fully resolved in list-space, ONE call before
the canvas is ever told anything. The canvas must NOT independently re-derive Replace/Toggle/Union from
raw modifier keys for a list-originated click ever again — that redundant re-computation against a
DIFFERENT, independently-drifted copy of the set is the root cause of both Part 1 and Part 2 above.
Instead: a new canonical mutator, `MapCanvas::SyncManualMarkerSelection`, REPLACES the canvas's own
`{collection==Markers, bManual==true}` subset of `selectedInstanceKeys` with the list's own just-resolved
vector, verbatim, in one step — never touching any other domain/kind's keys (procedural Markers,
Props, Decals) — and it fires the SAME `selectionChangedCallback` `ApplySelectionGesture` already fires,
but with its (renamed, generalized — see below) third argument ALWAYS `true`: a list click, of ANY
modifier kind, has ALREADY had its own `tabState.markers.selected*`/anchor fields correctly written by
`ApplyManualInstanceSelectionClick` one call earlier in the SAME click; Application's own re-derivation
of any of that from the canvas's echo is therefore always either a harmless no-op (once this sync makes
the two sets agree) or, as Part 2 proves, actively wrong; suppressing it UNCONDITIONALLY for this path
(not just for Shift) is the correct general rule, not a narrower special case.

Consequently `bCtrlHeld`/`bShiftHeld` are BOTH dropped from `selectManualMarkerInstanceCallback`'s own
type (not just `bCtrlHeld` — investigated per this ticket's own dispatch question): the canvas performs
no modifier-driven resolution of its own for a list-originated click any more (so `bCtrlHeld` carries
nothing useful), and Application's own tabState-suppression is now unconditional for this path regardless
of which modifier was held (so `bShiftHeld` carries nothing useful either, once `SyncManualMarkerSelection`
always passes `true`). Keeping either as a vestigial, unused parameter would be actively misleading (it
would look load-bearing and isn't) — both are removed.

## Required reading
`ARCH_21_01_MultiSelectRepresentation.md` §21.1 (the ordered set / primary-is-last-element contract;
`ToggleInSelectionSet`'s own documented "present -> erase (primary becomes the new back())" rule this
ticket's own fallback-primary behavior mirrors). `MarkersTab_ManualInstanceSelection_UI.cpp` in full
(`ApplyManualInstanceSelectionClick`'s own ratified rule — Ctrl ALWAYS moves the anchor to the clicked id,
even on deselect; Shift never moves it). STEP232's own ticket text (`work_orders/STEP232_...md`) for the
`bWasShiftGesture` mechanism this ticket generalizes.

---

## 1. Modified: `src/ui/MapCanvas_UI.h`

### 1a. Rename/generalize `SetSelectionChangedCallback`'s third parameter
Currently (lines 87-99):
```cpp
    void SetSelectionChangedCallback(
        std::function<void(const OverlayInstanceKey_UI& primary, const OverlayInstanceKeySet_UI& selectedKeys,
                           bool bWasShiftGesture)>
            selectionChanged) {
        selectionChangedCallback = std::move(selectionChanged);
    }
```
Replace with:
```cpp
    // STEP233 — the third parameter is RENAMED from STEP232's own `bWasShiftGesture` to
    // `bSuppressTabStateResync` and its own MEANING is generalized: STEP232 introduced it narrowly (a
    // gesture-level "was Shift held" signal) to protect ONE clobber it found (a Shift-list-click's own
    // anchor/plural-field writes getting overwritten by this same-click echo). STEP233 found a SECOND
    // instance of the identical clobber class for a Ctrl-deselect (see this ticket's own root-cause
    // writeup, Part 2) that "was Shift held" cannot express — the real, general condition a consumer
    // needs is "did the caller that changed this set already fully and correctly own every field my own
    // resync would otherwise touch." `ApplySelectionGesture` (below — canvas-NATIVE gestures, which have
    // no list to defer to) still passes its own literal `bShiftHeld` here, UNCHANGED, preserving STEP232's
    // own already-correct, already-tested canvas-native behavior byte-for-byte. `SyncManualMarkerSelection`
    // (STEP233, below — LIST-driven syncs) always passes `true`: a list click, of ANY modifier kind, has
    // ALREADY had its own tabState.markers.selected*/anchor fields correctly written by
    // ApplyManualInstanceSelectionClick one call earlier in the SAME click, so its caller
    // (Application::WireCallbacks()) must suppress its OWN resync unconditionally for that path, not only
    // when Shift happened to be held.
    void SetSelectionChangedCallback(
        std::function<void(const OverlayInstanceKey_UI& primary, const OverlayInstanceKeySet_UI& selectedKeys,
                           bool bSuppressTabStateResync)>
            selectionChanged) {
        selectionChangedCallback = std::move(selectionChanged);
    }
```
Currently (the field, lines 377-379):
```cpp
    std::function<void(const OverlayInstanceKey_UI& primary, const OverlayInstanceKeySet_UI& selectedKeys,
                       bool bWasShiftGesture)>
        selectionChangedCallback;
```
Replace with:
```cpp
    std::function<void(const OverlayInstanceKey_UI& primary, const OverlayInstanceKeySet_UI& selectedKeys,
                       bool bSuppressTabStateResync)>
        selectionChangedCallback;
```
This is a RENAME only — the type (`bool`) is unchanged, so none of the five existing
`SetSelectionChangedCallback` call sites (`Application_UI.cpp`, `MapCanvas_ActivePanelGate_UI_Test.cpp`,
`MapCanvas_GestureOwnership_UI_Test.cpp`, `MapCanvas_Picking_UI_Test.cpp` x3) require ANY edit from this
sub-step alone — none of them name the third lambda parameter (all take a bare, unnamed/discarded `bool`).
Only `Application_UI.cpp` (§15 below, where the logic actually changes) and the new
`MarkersTab_ListCanvasSelectionSync_UI_Test.cpp` local mirror (§17 below, which DOES name it) need edits.

### 1b. Add the new canonical mutator, beside `SelectManualMarkerByInstanceIdentifier`
`SelectManualMarkerByInstanceIdentifier`'s own doc comment (lines 216-227) currently claims it is "the
shell-mediated list-click-to-canvas path's landing point" — after this ticket that is no longer true (see
§3 below). Update its header comment's opening sentence only (do not touch its signature/body — it stays
byte-identical, still correct, still used by the canvas-native picking test suite and available for any
FUTURE non-list-driven caller that needs a single-key Ctrl/Shift-aware setter):
Currently (lines 216-223):
```cpp
    // ARCH §19.25, item 5 — the shell-mediated list-click-to-canvas path: a Markers-tab instance-list
    // Selectable click resolves through Application's own `selectManualMarkerInstanceCallback` (bound
    // to this method in WireCallbacks(), mirroring this file's own established push-in-pointer
    // injection pattern, STEP231's retirement of the former SetManualMarkerSelectionSource
    // notwithstanding) so the SAME real icon-sprite render path a canvas click drives
    // (MapCanvas_IconLayer_CullEmit_UI.cpp's `instance.bSelected`) also lights up for a list click —
    // never a second, parallel highlight mechanism. A negative `instanceIdentifier` clears the
    // selection (mirrors MarkersTabState::selectedManualInstanceIdentifier's own `-1` sentinel).
```
Replace its opening two sentences with:
```cpp
    // STEP233 — NO LONGER the production list-click landing point: Application's own
    // `selectManualMarkerInstanceCallback` (WireCallbacks()) now calls SyncManualMarkerSelection
    // instead (below), which syncs against the list's own already-resolved full selection rather than
    // re-deriving Toggle/Union/Replace from raw modifier keys against this method's own single-key
    // resolution (the redundant-computation trap that caused STEP233's own bug — see that ticket's own
    // root-cause writeup). This method itself is UNCHANGED and remains correct: it is still exercised by
    // MapCanvas_Picking_UI_Test.cpp's own canvas-native-selection coverage and remains available as a
    // general-purpose Ctrl/Shift-aware single-key setter for any future NON-list-driven caller.
    // ARCH §19.25, item 5 (historical) — originally the shell-mediated list-click-to-canvas path: a
    // Markers-tab instance-list Selectable click resolved through Application's own
    // `selectManualMarkerInstanceCallback` so the SAME real icon-sprite render path a canvas click
    // drives (MapCanvas_IconLayer_CullEmit_UI.cpp's `instance.bSelected`) also lit up for a list click —
    // never a second, parallel highlight mechanism. A negative `instanceIdentifier` clears the
    // selection (mirrors MarkersTabState::selectedManualInstanceIdentifier's own `-1` sentinel).
```
Immediately after `SelectManualMarkerByInstanceIdentifier`'s declaration (currently ending line 229),
insert the new method:
```cpp
    // ARCH §21.1 — STEP233: the list-driven sync entry point, and the production replacement for
    // SelectManualMarkerByInstanceIdentifier's own former role (see that method's own updated comment
    // above). A Markers-tab manual-instance-list click (plain/Ctrl/Shift) is ALREADY fully resolved,
    // list-side, by ApplyManualInstanceSelectionClick (MarkersTab_ManualInstanceSelection_UI.cpp) one
    // call before this is ever reached — this method makes the canvas's own real selectedInstanceKeys
    // subset for {collection==Markers, bManual==true} match that list-side resolution EXACTLY, a
    // REPLACE-THIS-SUBSET operation. It performs NO Toggle/Union/Replace resolution of its own from raw
    // modifier keys — doing that against the canvas's OWN, independently-touched copy of the set is
    // exactly the redundant-computation trap that caused STEP233's own bug (both the reported
    // selection-set desync AND a second, closely-related anchor clobber found while designing this fix
    // — see that ticket's own root-cause writeup). Every OTHER key already in the set — procedural
    // Markers (bManual==false), and every Props/Decals key regardless of bManual — survives byte-for-
    // byte, in its existing relative order, positioned BEFORE the freshly-synced manual-marker keys (so
    // this sync's own primary, below, always wins PrimaryOfSelectionSet's own last-element rule whenever
    // `selectedInstanceIdentifiers` is non-empty).
    //
    // `selectedInstanceIdentifiers`: the list's own just-resolved full selection (e.g.
    // `*interaction.selectedIdentifiers`, already written one call earlier by
    // ApplyManualInstanceSelectionClick) — becomes, in that exact order, the new manual-marker key
    // subset.
    // `clickedInstanceIdentifier`: the row the list itself just says is "current" (the plain/Ctrl/
    // Shift-clicked identifier). If present in `selectedInstanceIdentifiers`, it is moved to the END of
    // the synced subset — becomes the new overall PrimaryOfSelectionSet — REGARDLESS of its own position
    // within `selectedInstanceIdentifiers` (a Shift-range's own clicked endpoint can sit at EITHER end of
    // the [anchor..clicked] span depending on drag direction, so "whatever the vector's own last element
    // is" is NOT a safe substitute for this explicit argument — this method's own acceptance test proves
    // the distinction concretely). If it is NOT present (a Ctrl-click that just toggled ITSELF off), the
    // synced subset keeps `selectedInstanceIdentifiers`'s own trailing order and its own last element
    // becomes the fallback primary — mirroring ToggleInSelectionSet's own documented "present -> erase
    // (primary becomes the new back())" rule.
    //
    // Fires SetSelectionChangedCallback exactly like ApplySelectionGesture does — once, only if the
    // resulting set actually differs from the set going in (SelectionSetsEqual) — with
    // bSuppressTabStateResync ALWAYS true (never `bShiftHeld`-conditional the way ApplySelectionGesture's
    // own literal pass-through is): see SetSelectionChangedCallback's own updated header comment for why.
    void SyncManualMarkerSelection(const std::vector<int>& selectedInstanceIdentifiers,
                                   int clickedInstanceIdentifier);   // MapCanvas_UI.cpp
```

---

## 2. Modified: `src/ui/MapCanvas_UI.cpp`

### 2a. Comment-only updates at both existing `ApplySelectionGesture` overloads
Both currently end with (single-key overload, lines 94-99; batch overload, lines 117-120 — same shape):
```cpp
    if (SelectionSetsEqual(previous, selectedInstanceKeys)) return;
    // STEP232 — bShiftHeld, already this function's own parameter, threaded through unchanged so the
    // callback's consumer can apply the SAME "don't move the anchor on Shift" rule
    // ApplyManualInstanceSelectionClick's own list-side logic already implements.
    if (selectionChangedCallback)
        selectionChangedCallback(PrimaryOfSelectionSet(selectedInstanceKeys), selectedInstanceKeys, bShiftHeld);
```
Replace the comment (in BOTH overloads; the code line itself — `..., bShiftHeld);` — is UNCHANGED, still
passing this function's own local `bShiftHeld` positionally into the renamed third parameter):
```cpp
    if (SelectionSetsEqual(previous, selectedInstanceKeys)) return;
    // STEP232/STEP233 — bShiftHeld, already this function's own parameter, threaded through unchanged
    // into the callback's now-renamed, generalized `bSuppressTabStateResync` slot (SetSelectionChangedCallback's
    // own header comment, MapCanvas_UI.h) — a canvas-NATIVE gesture (this function's only caller class)
    // has no list to have already synced tabState correctly, so its own consumer (Application::
    // WireCallbacks()) must still resync from THIS gesture's own result whenever Shift wasn't held,
    // exactly as STEP232 established; SyncManualMarkerSelection below (list-driven, never reaches this
    // function) always passes `true` instead, unconditionally.
    if (selectionChangedCallback)
        selectionChangedCallback(PrimaryOfSelectionSet(selectedInstanceKeys), selectedInstanceKeys, bShiftHeld);
```

### 2b. New method definition
Add after the batch `ApplySelectionGesture` overload (currently ending line 121), before the closing
`} // namespace Ui`:
```cpp
// ARCH §21.1 — STEP233. See MapCanvas_UI.h's own doc comment for the full contract.
void MapCanvas::SyncManualMarkerSelection(const std::vector<int>& selectedInstanceIdentifiers,
                                          int clickedInstanceIdentifier) {
    const OverlayInstanceKeySet_UI previous = selectedInstanceKeys;
    OverlayInstanceKeySet_UI next;
    next.keys.reserve(selectedInstanceKeys.keys.size());
    // Every OTHER domain/kind survives untouched, in its existing relative order, ahead of the
    // freshly-synced manual-marker subset below — see this method's own header comment for why "ahead
    // of" matters: it keeps this sync's own primary as the WHOLE set's PrimaryOfSelectionSet.
    for (const OverlayInstanceKey_UI& key : selectedInstanceKeys.keys)
        if (!(key.collection == PlacementCollectionKind_UI::Markers && key.bManual))
            next.keys.push_back(key);

    bool bClickedIsSelected = false;
    for (const int identifier : selectedInstanceIdentifiers) {
        if (identifier == clickedInstanceIdentifier) { bClickedIsSelected = true; continue; }
        next.keys.push_back(OverlayInstanceKey_UI{PlacementCollectionKind_UI::Markers, identifier,
                                                   /*bValid=*/true, /*bManual=*/true});
    }
    // The clicked row becomes the new primary whenever it's actually still selected — held back and
    // appended LAST regardless of where it sat in `selectedInstanceIdentifiers` (a Shift-range's own
    // clicked endpoint can be at either end of the span). A Ctrl-click that just deselected ITSELF falls
    // through with no special case: `selectedInstanceIdentifiers`'s own trailing order stands, so its
    // own last element becomes the fallback primary (ToggleInSelectionSet's own documented "present ->
    // erase (primary becomes the new back())" rule).
    if (bClickedIsSelected)
        next.keys.push_back(OverlayInstanceKey_UI{PlacementCollectionKind_UI::Markers,
                                                   clickedInstanceIdentifier, /*bValid=*/true,
                                                   /*bManual=*/true});

    selectedInstanceKeys = next;
    if (SelectionSetsEqual(previous, selectedInstanceKeys)) return;
    // Always suppresses Application::WireCallbacks()'s own tabState resync (bSuppressTabStateResync,
    // SetSelectionChangedCallback's own header comment) — a list click, of ANY modifier kind, has
    // ALREADY had its own tabState.markers.selected*/anchor fields correctly written by
    // ApplyManualInstanceSelectionClick one call earlier, so any re-derivation of that from THIS
    // callback would be redundant at best and, for a Ctrl-deselect specifically, was actively wrong
    // before this ticket (STEP233's own root-cause writeup, Part 2).
    if (selectionChangedCallback)
        selectionChangedCallback(PrimaryOfSelectionSet(selectedInstanceKeys), selectedInstanceKeys,
                                 /*bSuppressTabStateResync=*/true);
}
```

---

## 3. Modified: `src/ui/MarkersTab_ManualInstanceSelection_UI.h`

Widen `ManualInstanceRowInteractionContext_UI::selectManualMarkerInstanceCallback`'s type. Currently
(lines 46-48):
```cpp
    // STEP205 — widened from `void(int)` so the row's own click can forward the SAME bCtrl/bShift it
    // already read for ApplyManualInstanceSelectionClick, instead of the canvas always Replacing.
    std::function<void(int, bool bCtrlHeld, bool bShiftHeld)>  selectManualMarkerInstanceCallback;
```
Replace with:
```cpp
    // STEP205 — widened from `void(int)` so the row's own click could forward the SAME bCtrl/bShift it
    // already read for ApplyManualInstanceSelectionClick, instead of the canvas always Replacing.
    // STEP233 — widened AGAIN, and simplified: carries this row's own clicked identifier plus the
    // list's OWN already-resolved full selection (*interaction.selectedIdentifiers, written by
    // ApplyManualInstanceSelectionClick one call earlier in the SAME click) instead of raw bCtrl/bShift
    // — the canvas syncs its own manual-marker subset to match this resolution directly
    // (MapCanvas::SyncManualMarkerSelection) rather than re-deriving Toggle/Union/Replace against its
    // OWN, independently-touched copy of the set — the redundant-computation trap that caused STEP233's
    // own bug. Neither bCtrl nor bShift is forwarded any more: the canvas performs no modifier-driven
    // resolution of its own for a list-originated click, and Application::WireCallbacks()'s own tabState
    // resync is unconditionally suppressed for this path regardless of which modifier was held (see
    // MapCanvas_UI.h's SyncManualMarkerSelection and Application_UI.cpp's own bSuppressTabStateResync).
    std::function<void(int clickedInstanceIdentifier, const std::vector<int>& selectedInstanceIdentifiers)>
        selectManualMarkerInstanceCallback;
```

---

## 4. Modified: `src/ui/MarkersTab_ManualLayerRowBody_UI.h`

Widen `DrawLayerRowBody`'s trailing parameter type and fix its own comment's dangling reference. Currently
(lines 81-99, the relevant slice):
```cpp
// ARCH §19.25, item 5 — `selectManualMarkerInstanceCallback` (Application's own shell-mediated
// closure, empty default so every existing call site compiles unchanged) is called BY the instance-
// list Selectable click below, IN ADDITION TO the existing `selectedManualInstanceIdentifier`
// tab-local write, not instead of it: the tab-local write keeps the list's own highlight in sync
// with itself, and the callback additionally drives the canvas's REAL selection (the actual fix —
// see MapCanvas_UI.h's SelectManualMarkerByInstanceIdentifier).
...
bool DrawLayerRowBody(Params::MarkerInstanceLayer& layer, int layerIndex,
                      const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                      std::vector<Params::MarkerInstanceGroup>& markers, const Params::Geometry& geometry,
                      int globalSymmetryMask, int globalRadialRepeatCount,
                      Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings, ManualMarkerLayersState& state,
                      const ManualInstanceLayerIndex_UI& instanceIndex, int& selectedManualInstanceIdentifier,
                      std::vector<int>& selectedManualInstanceIdentifiers, int& anchorIdentifier,
                      const std::function<void(int, bool bCtrlHeld, bool bShiftHeld)>&
                          selectManualMarkerInstanceCallback = {});
```
Replace the comment's last sentence (`... see MapCanvas_UI.h's SelectManualMarkerByInstanceIdentifier).`)
with `... see MapCanvas_UI.h's SyncManualMarkerSelection, STEP233).` and replace the trailing parameter
type with:
```cpp
                      const std::function<void(int clickedInstanceIdentifier,
                                               const std::vector<int>& selectedInstanceIdentifiers)>&
                          selectManualMarkerInstanceCallback = {});
```

---

## 5. Modified: `src/ui/MarkersTab_ManualLayerRowBody_UI.cpp`

The actual behavior change — `DrawManualInstanceRow`. Currently (lines 34-52):
```cpp
    if (ImGui::Selectable(rowLabel.c_str(), bRowSelected)) {
        // STEP141 — Ctrl (toggle)/Shift (range within THIS list)/plain click, "typical expectations".
        const bool bCtrl  = ImGui::GetIO().KeyCtrl;
        const bool bShift = ImGui::GetIO().KeyShift;
        if (interaction.selectedIdentifiers != nullptr && interaction.anchorIdentifier != nullptr
            && interaction.rowOrder != nullptr)
            ApplyManualInstanceSelectionClick(*interaction.rowOrder, instanceIdentifier, bCtrl, bShift,
                                              *interaction.selectedIdentifiers, *interaction.anchorIdentifier);
        if (interaction.primaryIdentifier != nullptr) *interaction.primaryIdentifier = instanceIdentifier;
        // ARCH §19.25, item 5 — IN ADDITION TO the tab-local write above, not instead of it:
        // drives the canvas's own real selection, so the REAL icon-sprite render path
        // (MapCanvas_IconLayer_CullEmit_UI.cpp's `instance.bSelected`) reflects this click too.
        // STEP205 — reuses the SAME `bCtrl`/`bShift` this row already read above (not a second
        // `ImGui::GetIO()` read) so the canvas's own `ApplySelectionGesture` resolves to the SAME
        // Toggle/Union/Replace outcome the tab-local write above just applied, instead of always
        // Replace clobbering it within the same click.
        if (interaction.selectManualMarkerInstanceCallback)
            interaction.selectManualMarkerInstanceCallback(instanceIdentifier, bCtrl, bShift);
    }
```
Replace with:
```cpp
    if (ImGui::Selectable(rowLabel.c_str(), bRowSelected)) {
        // STEP141 — Ctrl (toggle)/Shift (range within THIS list)/plain click, "typical expectations".
        const bool bCtrl  = ImGui::GetIO().KeyCtrl;
        const bool bShift = ImGui::GetIO().KeyShift;
        if (interaction.selectedIdentifiers != nullptr && interaction.anchorIdentifier != nullptr
            && interaction.rowOrder != nullptr)
            ApplyManualInstanceSelectionClick(*interaction.rowOrder, instanceIdentifier, bCtrl, bShift,
                                              *interaction.selectedIdentifiers, *interaction.anchorIdentifier);
        if (interaction.primaryIdentifier != nullptr) *interaction.primaryIdentifier = instanceIdentifier;
        // ARCH §19.25, item 5 — IN ADDITION TO the tab-local write above, not instead of it: drives the
        // canvas's own real selection, so the REAL icon-sprite render path
        // (MapCanvas_IconLayer_CullEmit_UI.cpp's `instance.bSelected`) reflects this click too.
        // STEP233 — widened from (instanceIdentifier, bCtrl, bShift) to (clickedInstanceIdentifier,
        // *interaction.selectedIdentifiers): the canvas now SYNCS its own manual-marker subset to match
        // this list-side resolution exactly (MapCanvas::SyncManualMarkerSelection) instead of
        // independently re-deriving Toggle/Union/Replace from bCtrl/bShift against its OWN narrower,
        // single-key-touched copy of the set — the exact redundant-computation trap STEP233 fixes (see
        // this callback's own type comment, MarkersTab_ManualInstanceSelection_UI.h). Neither bCtrl nor
        // bShift is forwarded any more — see that same comment for why both are now fully vestigial on
        // this path. `interaction.selectedIdentifiers` is guaranteed non-null whenever this row's own
        // click branch above ran (the same null-check gates both), but a static empty fallback is used
        // regardless — Constitution §6, never assume a caller-owned pointer stays valid past a null-check
        // written for a DIFFERENT purpose two lines up.
        if (interaction.selectManualMarkerInstanceCallback) {
            static const std::vector<int> kNoSelectedIdentifiers;
            interaction.selectManualMarkerInstanceCallback(instanceIdentifier,
                interaction.selectedIdentifiers != nullptr ? *interaction.selectedIdentifiers
                                                            : kNoSelectedIdentifiers);
        }
    }
```

---

## 6. Modified: `src/ui/MarkersTab_ManualLayers_UI.h`

Two declarations, mechanical type widening only (no logic/forwarding change). At line 138-139
(`DrawLayerList`) and line 168-169 (`DrawManualMarkerLayerListBody`), replace both occurrences of:
```cpp
                                  const std::function<void(int, bool bCtrlHeld, bool bShiftHeld)>&
                                      selectManualMarkerInstanceCallback = {});
```
with:
```cpp
                                  const std::function<void(int clickedInstanceIdentifier,
                                                           const std::vector<int>& selectedInstanceIdentifiers)>&
                                      selectManualMarkerInstanceCallback = {});
```
(Indentation/exact column will differ slightly between the two call sites per their own existing
alignment — match each site's own pre-existing indent, mirror `MarkersTab_ManualLayerRowBody_UI.h`'s own
wrapped-type formatting above.)

---

## 7. Modified: `src/ui/MarkersTab_ManualLayers_UI.cpp`

Two definitions, mechanical type widening only — the forwarding calls inside (`DrawLayerRowBody(...,
selectManualMarkerInstanceCallback)` at line 159, `DrawLayerList(..., selectManualMarkerInstanceCallback)`
at line 219) reference the parameter by NAME and are UNCHANGED; only the declared TYPE at each function's
own signature (lines 134-135 for `DrawLayerList`, lines 207-208 for `DrawManualMarkerLayerListBody`)
changes, identically to §6 above.

---

## 8. Modified: `src/ui/MarkersTab_Bundles_UI.h`

One declaration (`DrawMarkerLayerBundleTree`, lines 306-307). Currently:
```cpp
                               const std::function<void(int, bool bCtrlHeld, bool bShiftHeld)>&
                                   selectManualMarkerInstanceCallback = {});
```
Replace with:
```cpp
                               const std::function<void(int clickedInstanceIdentifier,
                                                        const std::vector<int>& selectedInstanceIdentifiers)>&
                                   selectManualMarkerInstanceCallback = {});
```

---

## 9. Modified: `src/ui/MarkersTab_Bundles_UI.cpp`

Two definitions, mechanical widening, forwarding unchanged:
- `DrawMarkerGroupLeafBody` (file-local, lines 25-33) — its trailing parameter type (lines 32-33).
- `DrawMarkerLayerBundleTree` (lines 100-111) — its trailing parameter type (lines 110-111).
Both become the same widened type as §8. The one call site inside each (`DrawLayerRowBody(...,
selectManualMarkerInstanceCallback)` at line 45; `DrawMarkerGroupLeafBody(...,
selectManualMarkerInstanceCallback)` at line 147) forward the parameter by name — unchanged.

---

## 10. Modified: `src/ui/MarkersTab_TypeSections_UI.h`

`DrawMarkerTypeSections`'s FIRST callback parameter only (`selectManualMarkerInstanceCallback`, lines
72-73) — its procedural sibling (`selectProceduralMarkerInstanceCallback`, lines 75-76) is EXPLICITLY
**not** touched by this ticket (see Explicit out-of-scope). Currently:
```cpp
void DrawMarkerTypeSections(Params::MapRecipe& recipe, MarkersTabState& state,
                            Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest,
                            const std::function<void(int, bool bCtrlHeld, bool bShiftHeld)>&
                                selectManualMarkerInstanceCallback = {},
                            const Data::PlacementInstances* placedMarkers = nullptr,
                            const std::function<void(int, bool bCtrlHeld, bool bShiftHeld)>&
                                selectProceduralMarkerInstanceCallback = {});
```
Replace ONLY the first callback's type:
```cpp
void DrawMarkerTypeSections(Params::MapRecipe& recipe, MarkersTabState& state,
                            Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest,
                            const std::function<void(int clickedInstanceIdentifier,
                                                     const std::vector<int>& selectedInstanceIdentifiers)>&
                                selectManualMarkerInstanceCallback = {},
                            const Data::PlacementInstances* placedMarkers = nullptr,
                            const std::function<void(int, bool bCtrlHeld, bool bShiftHeld)>&
                                selectProceduralMarkerInstanceCallback = {});
```

---

## 11. Modified: `src/ui/MarkersTab_TypeSections_UI.cpp`

`DrawMarkerTypeSections`'s definition (lines 61-67) — mirror §10's split exactly: widen the FIRST
callback's type (lines 63-64) only, leave the procedural one (lines 66-67) untouched. Every forwarding
call inside (`DrawMarkerLayerBundleTree(..., selectManualMarkerInstanceCallback)` line 83;
`DrawManualMarkerLayerListBody(..., selectManualMarkerInstanceCallback)` line 107) is unchanged (by name).

---

## 12. Modified: `src/ui/MarkersTab_UI.h`

`DrawMarkersTab`'s declaration (lines 194-197) — same first-parameter-only split as §10. Currently:
```cpp
                    const std::function<void(int, bool bCtrlHeld, bool bShiftHeld)>&
                        selectManualMarkerInstanceCallback = {},
                    const std::function<void(int, bool bCtrlHeld, bool bShiftHeld)>&
                        selectProceduralMarkerInstanceCallback = {});
```
Replace with:
```cpp
                    const std::function<void(int clickedInstanceIdentifier,
                                             const std::vector<int>& selectedInstanceIdentifiers)>&
                        selectManualMarkerInstanceCallback = {},
                    const std::function<void(int, bool bCtrlHeld, bool bShiftHeld)>&
                        selectProceduralMarkerInstanceCallback = {});
```
Also update the header comment (lines 183-184, `MarkersTabState`'s own or the surrounding doc block if
present) if it names the OLD `(int)`/`(int, bool, bool)` shape explicitly — grep this file's own comments
for `selectManualMarkerInstanceCallback` once at edit time and correct any that assert the retired shape.

---

## 13. Modified: `src/ui/MarkersTab_UI.cpp`

Three sites, all mechanical type widening (forwarding calls at lines 191, 206, 364, 430, 436 all reference
the parameter by NAME — unchanged):
1. `DrawBaseSectionManualInstanceList`'s trailing parameter (lines 165-166) — same split as §10/§12.
2. `DrawMarkersTab`'s definition (lines 263-266) — same split.

---

## 14. Modified: `src/ui/Application_UI.h`

The field declaration (line 187) and its own doc comment (lines 180-186). Currently:
```cpp
    // ARCH §19.25, item 5 — the shell-mediated list-click-to-canvas closure, bound in WireCallbacks()
    // (mirroring SetManualMarkerSelectionSource's existing injection pattern) and threaded down
    // through DrawMarkersTab -> DrawMarkerTypeSections -> DrawLayerRowBody's existing call chain
    // (the same chain previewDriver/iconManifest already ride down).
    // STEP205 (ARCH §21.1's own deferred follow-up) — widened from `void(int)` to carry the row
    // click's real Ctrl/Shift modifier state through to `MapCanvas::ApplySelectionGesture`, so this
    // shell-mediated path joins/ranges into the canvas's real multi-select instead of always Replace.
    std::function<void(int, bool bCtrlHeld, bool bShiftHeld)>     selectManualMarkerInstanceCallback;
```
Replace with:
```cpp
    // ARCH §19.25, item 5 — the shell-mediated list-click-to-canvas closure, bound in WireCallbacks()
    // (mirroring SetManualMarkerSelectionSource's existing injection pattern) and threaded down
    // through DrawMarkersTab -> DrawMarkerTypeSections -> DrawLayerRowBody's existing call chain
    // (the same chain previewDriver/iconManifest already ride down).
    // STEP205 (ARCH §21.1's own deferred follow-up, historical) — widened from `void(int)` to carry
    // the row click's real Ctrl/Shift modifier state through to `MapCanvas::ApplySelectionGesture`.
    // STEP233 — widened AGAIN and SIMPLIFIED: carries the clicked identifier plus the list's own
    // already-resolved full selection instead of raw bCtrl/bShift — bound to
    // `MapCanvas::SyncManualMarkerSelection` now, not `ApplySelectionGesture`/
    // `SelectManualMarkerByInstanceIdentifier` — see that method's own header comment
    // (MapCanvas_UI.h) for why re-deriving Toggle/Union/Replace here was the bug.
    std::function<void(int clickedInstanceIdentifier, const std::vector<int>& selectedInstanceIdentifiers)>
        selectManualMarkerInstanceCallback;
```
The procedural sibling (`selectProceduralMarkerInstanceCallback`, next field) is UNCHANGED — see Explicit
out-of-scope.

---

## 15. Modified: `src/ui/Application_UI.cpp`

### 15a. `WireCallbacks()`'s `SetSelectionChangedCallback` closure — rename + Part 2's actual fix
Currently (lines 93-118, the closure):
```cpp
    // STEP232 — widened to also receive bWasShiftGesture (sourced from ApplySelectionGesture's own
    // bShiftHeld at the point it fires this callback, MapCanvas_UI.cpp). TWO independent same-click
    // clobbers are fixed by gating on it, both discovered/fixed together (see this ticket's own
    // root-cause writeup for the full mechanism and git-bisect):
    ...
    canvas.SetSelectionChangedCallback([this](const OverlayInstanceKey_UI& primary,
                                              const OverlayInstanceKeySet_UI& selectedKeys,
                                              bool bWasShiftGesture) {
        if (!bWasShiftGesture) {
            tabState.markers.selectedManualInstanceIdentifiers.clear();
            for (const OverlayInstanceKey_UI& key : selectedKeys.keys)
                if (key.bValid && key.collection == PlacementCollectionKind_UI::Markers && key.bManual)
                    tabState.markers.selectedManualInstanceIdentifiers.push_back(key.instanceIndex);
        }
        ...
        const bool bPrimaryIsManualMarker = primary.bValid
            && primary.collection == PlacementCollectionKind_UI::Markers && primary.bManual;
        tabState.markers.selectedManualInstanceIdentifier = bPrimaryIsManualMarker ? primary.instanceIndex : -1;
        ...
        if (!bWasShiftGesture)
            tabState.markers.manualInstanceSelectionAnchorIdentifier = bPrimaryIsManualMarker ? primary.instanceIndex : -1;
        ...
    });
```
Replace the closure's own parameter name and both `bWasShiftGesture` reads with `bSuppressTabStateResync`
(mechanical rename, logic otherwise unchanged in THIS closure — the actual behavior change is entirely in
which VALUE now reaches it, from §2b's new `SyncManualMarkerSelection`), and rewrite the surrounding
comment block to describe the generalization and Part 2's own newly-found companion clobber:
```cpp
    // STEP232/STEP233 — this closure's third parameter is Application's own consumer of
    // MapCanvas_UI.h's `bSuppressTabStateResync` (renamed/generalized from STEP232's narrower
    // `bWasShiftGesture` — see that setter's own header comment for the full "why"). Gating BOTH writes
    // below on it fixes THREE independent same-click clobbers total, across STEP232 and STEP233 together
    // (all three share the identical mechanism: a synchronous same-click echo from the canvas
    // overwriting a value the LIST side had already correctly computed one call earlier):
    //  1. (STEP232) selectedManualInstanceIdentifiers (plural) used to resync UNCONDITIONALLY from the
    //     canvas's own key set on every change — clobbering a Shift-click's own richer list-computed
    //     range.
    //  2. (STEP232) manualInstanceSelectionAnchorIdentifier — a Shift-click's own PRESERVED anchor
    //     getting overwritten.
    //  3. (STEP233) manualInstanceSelectionAnchorIdentifier AGAIN, for a Ctrl-DESELECT specifically:
    //     ApplyManualInstanceSelectionClick's own Ctrl branch sets the anchor to the clicked id
    //     UNCONDITIONALLY (even when deselecting it), but this closure's old `!bWasShiftGesture` gate
    //     only protected Shift, so a Ctrl-deselect's own correct anchor write still got clobbered by
    //     whatever the canvas's own (differently-computed, necessarily-different-since-the-clicked-id-
    //     is-no-longer-selected) fallback primary happened to be. `bSuppressTabStateResync` generalizes
    //     the same protection to EVERY list-driven change, not just a Shift one: MapCanvas::
    //     SyncManualMarkerSelection (the new production landing point for a list click, MapCanvas_UI.h)
    //     always fires this callback with it `true`, since the list has ALREADY correctly written both
    //     fields for EVERY modifier kind (ApplyManualInstanceSelectionClick), one call earlier in the
    //     SAME click — there is no modifier kind for which THIS closure's own re-derivation from the
    //     canvas's echo is ever the right thing to do on that path.
    // A canvas-NATIVE gesture (ApplySelectionGesture, no list involved) still passes its own literal
    // bShiftHeld here — STEP232's own documented trade-off (a Shift-held canvas MARQUEE no longer syncs
    // into selectedManualInstanceIdentifiers either) is UNCHANGED by STEP233: SyncManualMarkerSelection
    // is reached ONLY from a list click, never from ApplyMarqueeGesture/ApplyClickGesture, so it neither
    // fixes nor worsens that documented gap — see STEP233's own Explicit out-of-scope for the explicit
    // confirmation.
    canvas.SetSelectionChangedCallback([this](const OverlayInstanceKey_UI& primary,
                                              const OverlayInstanceKeySet_UI& selectedKeys,
                                              bool bSuppressTabStateResync) {
        if (!bSuppressTabStateResync) {
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
        // both fields to "nothing selected" together. UNCHANGED by STEP232/STEP233 — always runs,
        // regardless of bSuppressTabStateResync, since it is not the field either ticket's bug lives in.
        const bool bPrimaryIsManualMarker = primary.bValid
            && primary.collection == PlacementCollectionKind_UI::Markers && primary.bManual;
        tabState.markers.selectedManualInstanceIdentifier = bPrimaryIsManualMarker ? primary.instanceIndex : -1;
        // STEP232 fix #2 / STEP233 fix #3, the anchor — see this closure's own header comment above for
        // the full mechanism (both are the SAME clobber class, now both closed by the SAME general gate).
        if (!bSuppressTabStateResync)
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

### 15b. Retarget `selectManualMarkerInstanceCallback`'s own lambda
Currently (lines 217-219):
```cpp
    selectManualMarkerInstanceCallback = [this](int instanceIdentifier, bool bCtrlHeld, bool bShiftHeld) {
        canvas.SelectManualMarkerByInstanceIdentifier(instanceIdentifier, bCtrlHeld, bShiftHeld);
    };
```
Replace with:
```cpp
    // STEP233 — retargeted from SelectManualMarkerByInstanceIdentifier (which re-derived Toggle/Union/
    // Replace against the canvas's OWN, independently-touched copy of the set — the root cause) to
    // SyncManualMarkerSelection (which instead REPLACES the canvas's own manual-marker subset with the
    // list's own already-resolved full selection, verbatim). See MapCanvas_UI.h's own header comment on
    // SyncManualMarkerSelection for the full contract.
    selectManualMarkerInstanceCallback = [this](int clickedInstanceIdentifier,
                                                const std::vector<int>& selectedInstanceIdentifiers) {
        canvas.SyncManualMarkerSelection(selectedInstanceIdentifiers, clickedInstanceIdentifier);
    };
```
The procedural sibling immediately below (`selectProceduralMarkerInstanceCallback`) is UNCHANGED — see
Explicit out-of-scope.

---

## 16. Modified: `src/ui/MarkersTab_ManualInstanceListRows_UI_Test.cpp`

### 16a. Widen the two helpers' callback parameter types
`RunRowBodyFrame` (line 56-57) and `ClickAt` (lines 85-86) both currently take
`const std::function<void(int, bool, bool)>& selectManualMarkerInstanceCallback = {}`. Widen both to:
```cpp
const std::function<void(int, const std::vector<int>&)>& selectManualMarkerInstanceCallback = {}
```
Add `#include <vector>` if not already present (this file already includes `<functional>` and `<utility>`
— `<vector>` is very likely already transitively available via `MarkersTab_ManualLayerRowBody_UI.h`, but
confirm at edit time rather than assuming).

### 16b. Rewrite `RunCtrlHeldClickSyncsTabLocalAndCanvasCallbackCheck` (lines 303-372) for the new contract
This test's own PURPOSE (proving the tab-local write and the canvas-sync callback agree instead of
racing) is unchanged; only WHAT it asserts the callback reports changes, since the callback no longer
carries `bCtrlHeld`/`bShiftHeld` — it now carries the list's own full resolved selection, which is the
actually load-bearing thing to prove agrees with `selectedManualInstanceIdentifiers`. Replace the whole
function body with:
```cpp
// STEP205 (historical)/STEP233 — a Ctrl-held row click must widen BOTH the tab-local multi-select write
// (ApplyManualInstanceSelectionClick, already inside DrawManualInstanceRow) AND the canvas-sync callback
// with the SAME resolved result, so the two agree instead of one clobbering the other within the same
// click (STEP233's own root-cause: the OLD (id, bCtrl, bShift) shape let the canvas re-derive a
// DIFFERENT, narrower result from its own copy of the set — this test now proves the callback instead
// reports the list's OWN already-resolved full selection verbatim). Reuses the SAME two-row fixture
// RunInstanceRowClickChecks does.
void RunCtrlHeldClickSyncsTabLocalAndCanvasCallbackCheck() {
    HeadlessImguiSession session;
    std::vector<Params::MarkerInstanceLayer> markerLayers(1);
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].name = "Resources";
    Params::MarkerTransform first;  first.name = "A"; first.layerIndex = 0; first.instanceIdentifier = 100;
    Params::MarkerTransform second; second.name = "B"; second.layerIndex = 0; second.instanceIdentifier = 101;
    markers[0].transforms.push_back(first);
    markers[0].transforms.push_back(second);
    const ManualInstanceLayerIndex_UI instanceIndex = BuildManualInstanceLayerIndex(markers);

    ManualMarkerLayersState state;
    int selectedManualInstanceIdentifier = -1;
    std::vector<int> selectedManualInstanceIdentifiers;
    int anchorIdentifier = -1;

    int  reportedIdentifier = -1;
    std::vector<int> reportedSelectedIdentifiers;
    int  callbackFireCount  = 0;
    const std::function<void(int, const std::vector<int>&)> selectManualMarkerInstanceCallback =
        [&](int instanceIdentifier, const std::vector<int>& selectedIdentifiers) {
            reportedIdentifier = instanceIdentifier;
            reportedSelectedIdentifiers = selectedIdentifiers;
            ++callbackFireCount;
        };

    const FrameResult settle = RunRowBodyFrame(HeadlessMouseState(), markerLayers[0], markerLayers, markers,
                                               instanceIndex, state, selectedManualInstanceIdentifier,
                                               selectedManualInstanceIdentifiers, anchorIdentifier,
                                               selectManualMarkerInstanceCallback);
    const float rowHeight  = settle.lastItemMax.y - settle.lastItemMin.y;
    const float rowSpacing = ImGui::GetStyle().ItemSpacing.y;
    const ImVec2 secondRowCenter((settle.lastItemMin.x + settle.lastItemMax.x) * 0.5f,
                                 (settle.lastItemMin.y + settle.lastItemMax.y) * 0.5f);
    const ImVec2 firstRowCenter(secondRowCenter.x, secondRowCenter.y - (rowHeight + rowSpacing));

    // Baseline: a plain click on the FIRST row establishes the tab-local multi-select at {100}, and
    // fires the canvas-sync callback with (100, {100}).
    ClickAt(firstRowCenter, markerLayers[0], markerLayers, markers, instanceIndex, state,
           selectedManualInstanceIdentifier, selectedManualInstanceIdentifiers, anchorIdentifier,
           /*bCtrlHeld=*/false, /*bShiftHeld=*/false, selectManualMarkerInstanceCallback);
    Check(selectedManualInstanceIdentifier == 100, "the plain baseline click selects the first row (100)");
    Check(selectedManualInstanceIdentifiers.size() == 1 && selectedManualInstanceIdentifiers[0] == 100,
          "the plain baseline click's own tab-local multi-select is exactly {100}");
    Check(callbackFireCount == 1 && reportedIdentifier == 100
              && reportedSelectedIdentifiers.size() == 1 && reportedSelectedIdentifiers[0] == 100,
          "STEP233 - the plain baseline click's own canvas-sync callback fires with (100, {100}) - the "
          "list's OWN already-resolved full selection, not a bare (id, bCtrl, bShift) triple the canvas "
          "would otherwise have to re-resolve itself");

    // A Ctrl-held click on the SECOND row: ADDS. The callback must report the FULL resolved set
    // ({100, 101}), matching selectedManualInstanceIdentifiers exactly — proving the two writes agree
    // instead of the canvas-sync half carrying a narrower/different result the canvas would have to
    // reconstruct on its own.
    callbackFireCount = 0;
    ClickAt(secondRowCenter, markerLayers[0], markerLayers, markers, instanceIndex, state,
           selectedManualInstanceIdentifier, selectedManualInstanceIdentifiers, anchorIdentifier,
           /*bCtrlHeld=*/true, /*bShiftHeld=*/false, selectManualMarkerInstanceCallback);
    Check(callbackFireCount == 1 && reportedIdentifier == 101
              && reportedSelectedIdentifiers.size() == 2
              && reportedSelectedIdentifiers[0] == 100 && reportedSelectedIdentifiers[1] == 101,
          "STEP233 - a Ctrl-held click on the second row fires the canvas-sync callback with "
          "(101, {100, 101}) - the FULL resolved set");
    Check(selectedManualInstanceIdentifiers.size() == 2
              && selectedManualInstanceIdentifiers[0] == 100 && selectedManualInstanceIdentifiers[1] == 101,
          "the tab-local multi-select still contains the FIRST row's id (100) after the Ctrl-held click "
          "— proving the tab-local write and the canvas-sync write agree instead of racing");
}
```

---

## 17. New/rewritten test file: `src/ui/MarkersTab_ListCanvasSelectionSync_UI_Test.cpp`

This file is currently UNTRACKED (STEP232's own new file, not yet committed) — this ticket rewrites it
in place rather than layering a second diff on top. Headless, pure-data-structure, no imgui/GL, per the
human's own explicit constraint (repeated verbatim in this ticket's own dispatch instruction) — mirrors
this file's own already-established `int main()` style exactly, driving the REAL, compiled
`ApplyManualInstanceSelectionClick` and the REAL, compiled `MapCanvas::SyncManualMarkerSelection`/
`SelectManualMarkerByInstanceIdentifier` through plain C++ calls.

```cpp
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
```

---

## 18. Modified (comment only): `src/ui/MapCanvas_Picking_UI_Test.cpp`

Currently (lines 86-90):
```cpp
// ARCH_19_25_SelectionRepresentationUnification.md items 3-5: the linear manual-marker hit-test
// fallback (a canvas click at a manual marker's screen position, with no procedural hit) and the
// shell-mediated list-click landing point (SelectManualMarkerByInstanceIdentifier) — both drive
// MapCanvas's own widened selectedInstanceKey through the SAME canonical SetSelection the
// procedural checks above already exercise, never a second, divergent setter.
```
Replace with:
```cpp
// ARCH_19_25_SelectionRepresentationUnification.md items 3-5: the linear manual-marker hit-test
// fallback (a canvas click at a manual marker's screen position, with no procedural hit) and
// SelectManualMarkerByInstanceIdentifier (its own general-purpose Ctrl/Shift-aware single-key setter,
// STEP233 — no longer the production list-click landing point, see that method's own updated header
// comment, MapCanvas_UI.h; the list now syncs through MapCanvas::SyncManualMarkerSelection instead,
// MarkersTab_ListCanvasSelectionSync_UI_Test.cpp's own newer coverage) — both drive MapCanvas's own
// widened selectedInstanceKey through the SAME canonical SetSelection the procedural checks above
// already exercise, never a second, divergent setter.
```
No assertion/logic in this file changes — `SelectManualMarkerByInstanceIdentifier`'s own signature and
behavior are byte-identical to before this ticket.

---

## ARCH rules invoked
- `ARCH_21_01_MultiSelectRepresentation.md` §21.1 — the ordered set / primary-is-last-element contract
  this ticket's new `SyncManualMarkerSelection` conforms to exactly (its own "hold back and append last"
  primary rule is a direct, careful application of that contract to a REPLACE-A-SUBSET operation, not a
  new resolution rule); `ToggleInSelectionSet`'s own documented fallback-primary rule, which this
  ticket's Ctrl-deselect fallback mirrors verbatim.
- Constitution §1 — UI sets PARAMS/tabState and reacts to gestures; no new sim logic anywhere in this
  ticket, only a pure state-sync mechanism correction.
- Constitution §6 — no new crash surface (every write still has a well-defined value on every path; a
  null `interaction.selectedIdentifiers` falls back to a static empty vector rather than dereferencing);
  every design decision this ticket makes (the primary/fallback rule, the `bSuppressTabStateResync`
  generalization, dropping both `bCtrlHeld`/`bShiftHeld`) is pinned down explicitly in this ticket's own
  text, not left for the coder to invent.

## Explicit out-of-scope
- **The procedural sibling (`selectProceduralMarkerInstanceCallback` / `SelectProceduralMarkerInstanceByArrayPosition`)
  is completely untouched** — per STEP205/STEP232's own confirmed finding, "no tab-local plural-selection
  field exists for Procedural instances to stomp," so the desync class this ticket fixes has no
  procedural analog. `MarkersTab_TypeSections_UI.h`/`.cpp` and `MarkersTab_UI.h`/`.cpp` each widen ONLY
  their FIRST (`selectManualMarkerInstanceCallback`) parameter — the procedural one keeps its original
  `(int, bool bCtrlHeld, bool bShiftHeld)` shape verbatim.
- **No change to `MapCanvas_SelectionSet_UI.h`/`.cpp`'s own `ToggleInSelectionSet`/`UnionIntoSelectionSet`/
  `ToggleEachInSelectionSet`/`ReplaceSelectionSet`/`PrimaryOfSelectionSet`** — all already correct
  (STEP230), untouched; `SyncManualMarkerSelection` is a NEW, separate mutator beside them, not a
  modification of any of them.
- **No change to `ApplyManualInstanceSelectionClick`'s own rule** (`MarkersTab_ManualInstanceSelection_UI.cpp`)
  — it was always correct (including its own "Ctrl always moves the anchor, even on deselect" rule,
  which this ticket's Part 2 fix now correctly SURVIVES downstream instead of getting clobbered); the bug
  was entirely in how its OWN output got re-derived/clobbered downstream, one or two calls later, in the
  SAME click.
- **No change to `ApplyClickGesture`/`ApplyMarqueeGesture`, or to `SelectManualMarkerByInstanceIdentifier`'s
  own signature or behavior** — confirmed by direct read: canvas-originated clicks/marquees never routed
  through the list-click callback this ticket retargets; `SelectManualMarkerByInstanceIdentifier` keeps
  its exact pre-ticket Ctrl/Shift-resolution behavior (still exercised, unmodified, by
  `MapCanvas_Picking_UI_Test.cpp` and by this ticket's own `RunCanvasOnlyClickEstablishesAnchorChecks`),
  only its own DOC COMMENT changes (§1b/§18) to stop claiming a role it no longer plays in production.
  This ticket's own new acceptance test (`RunCanvasOnlyClickEstablishesAnchorChecks`) explicitly proves
  this path is untouched.
- **STEP232's own documented "Shift-marquee → `selectedManualInstanceIdentifiers` sync gap" trade-off is
  UNCHANGED by this ticket** — investigated per this ticket's own dispatch instruction. A canvas-only
  Shift-marquee (no list click involved) still reaches `ApplyMarqueeGesture` → `ApplySelectionGesture`'s
  batch overload, which still passes its own literal `bShiftHeld` (now into the renamed
  `bSuppressTabStateResync` slot) — `SyncManualMarkerSelection` is reached ONLY from a list click and
  never from a marquee, so this ticket neither resolves nor worsens that documented gap; it stays exactly
  as STEP232 described and accepted it.
- **No new signal threaded to distinguish "list-driven" from "canvas-driven" origin at the
  `SetSelectionChangedCallback` boundary itself** — this ticket achieves the SAME practical effect a
  narrower, origin-aware redesign would (list-driven changes never get resynced by Application's own
  closure) WITHOUT adding a fourth callback parameter, by having the ONE list-driven entry point
  (`SyncManualMarkerSelection`) simply always pass `true` for the existing (renamed) third parameter — a
  materially smaller change than the "separate signal threaded from `ApplyMarqueeGesture`/
  `ApplyClickGesture`" redesign STEP232's own Explicit out-of-scope already flagged and deferred as too
  large; that redesign is STILL deferred (it would be needed only to ALSO fix the marquee-sync gap noted
  above, which is not this ticket's scope).
- **No GL-backed/click-simulation test may be written or run for this ticket** — per the human's own
  explicit, repeated instruction. §17's rewritten test is deliberately pure-data-structure, headless,
  `int main()`-style, mirroring its own prior STEP232 shape exactly.
- **No ARCH file edit.** This ticket fixes a confirmed bug against already-ratified §21.1 behavior and
  extends an already-established callback-widening pattern (STEP232's own precedent); it invents no new
  law.

## Acceptance test
1. `MarkersTab_ListCanvasSelectionSync_UI_Test` (rewritten `ctest` binary, §17) passes `ALL PASS`:
   - STEP232's own original plain-then-two-Shift-clicks scenario still passes, PLUS a new assertion this
     ticket adds that the CANVAS's own real `SelectedInstanceKeys()` — not just `tabState`'s mirror field
     — matches the resolved range at every step (`RunPlainThenTwoShiftClicksChecks`).
   - The human's own literally-reported scenario — plain-click, Shift-click (establishing a range),
     Ctrl-click a THIRD row already inside that range (not an endpoint) — ends with the canvas's own real
     selection set matching the list's own correctly-resolved result EXACTLY (the reported bug, fixed),
     AND the anchor surviving that same click correctly (the newly-discovered companion bug, fixed)
     (`RunCtrlWithinShiftRangeChecks`).
   - A Shift-click extending BACKWARD (the clicked row lands at the front, not the back, of the resolved
     range array) still makes that row the canvas's own overall primary — proves the explicit
     `clickedInstanceIdentifier` argument, not vector-position, drives primary. Note the canvas's own
     `SelectedInstanceKeys()` ORDER legitimately diverges from `tabState`'s list-order storage in exactly
     this backward-extend case (the clicked id is held back and appended LAST by
     `SyncManualMarkerSelection`'s own primary rule regardless of its position in the resolved range) —
     the test's own `CanvasManualMarkerIdentifiers(canvas)` assertion expects that reordered form
     (`{20,30,10}`, not `{10,20,30}`), not tabState's own order
     (`RunShiftClickBackwardStillPrimarysTheClickedRowChecks`).
   - A Ctrl-click still moves the anchor and matches the list exactly for the ADD case
     (`RunCtrlClickStillMovesAnchorChecks`).
   - A canvas-only plain click (no list involved) still establishes a usable anchor, proving that path
     is completely untouched (`RunCanvasOnlyClickEstablishesAnchorChecks`).
2. `MarkersTab_ManualInstanceListRows_UI_Test` continues to pass `ALL PASS`, with
   `RunCtrlHeldClickSyncsTabLocalAndCanvasCallbackCheck` rewritten to assert the new (id, resolved-list)
   callback contract instead of the retired (id, bCtrl, bShift) one.
3. `MapCanvas_Picking_UI_Test`, `MapCanvas_ActivePanelGate_UI_Test`, `MapCanvas_GestureOwnership_UI_Test`
   (all pre-existing `ctest` binaries) continue to pass `ALL PASS` unmodified in substance — none of their
   own assertions reference the renamed callback parameter by name, and `SelectManualMarkerByInstanceIdentifier`'s
   own behavior is byte-identical to before this ticket.
4. Full `SanGenV2` build stays clean; every existing test in the suite continues to pass.

## Interpretation calls made
1. **The fix is a NEW canonical mutator (`SyncManualMarkerSelection`), not a modified
   `SelectManualMarkerByInstanceIdentifier`/`ApplySelectionGesture`.** The dispatch brief's own framing
   ("does this need a NEW canvas method... design its exact contract/signature") is answered yes:
   `ApplySelectionGesture`'s own Toggle/Union/Replace resolution remains exactly right for a canvas-NATIVE
   gesture (which has no list to defer to); overloading it to ALSO handle "replace this subset with an
   already-resolved list" would conflate two genuinely different operations under one name.
2. **Both `bCtrlHeld` and `bShiftHeld` are dropped from `selectManualMarkerInstanceCallback`'s own type**,
   not just `bCtrlHeld` as the dispatch brief's own narrower framing suggested investigating. Investigated
   per that brief's own explicit instruction: once the canvas performs no modifier-driven resolution of
   its own for a list click, AND `SyncManualMarkerSelection` unconditionally suppresses Application's own
   tabState resync for every modifier kind (not just Shift, given Part 2's own finding), neither boolean
   carries any information any consumer on this path still needs. Keeping either as a vestigial, silently-
   ignored parameter was judged actively worse than dropping it (a future reader would reasonably assume
   it's load-bearing).
3. **`SetSelectionChangedCallback`'s own third parameter is RENAMED from `bWasShiftGesture` to
   `bSuppressTabStateResync` and its MEANING is generalized**, rather than adding a FOURTH parameter or
   leaving the name stale-but-functionally-repurposed. This was the single largest design decision in this
   ticket: it is what lets the SAME mechanism close BOTH the reported bug and the newly-discovered
   Ctrl-deselect anchor clobber (Part 2) without inventing a second, parallel guard. The rename costs
   nothing at any of the five existing `SetSelectionChangedCallback` call sites (none name the parameter);
   only `Application_UI.cpp` (where the logic lives) and the new test file's own local mirror need it
   spelled out.
4. **The Ctrl-deselect anchor clobber (Part 2) is fixed in THIS ticket, not filed separately** — same
   reasoning STEP232 itself already used for its own second, closely-related clobber: it shares the
   identical mechanism (a same-click synchronous echo overwriting a value the list had already correctly
   computed) and is a direct, natural consequence of designing this ticket's own `SyncManualMarkerSelection`
   primary/fallback rule correctly (the fallback case is exactly where the pre-existing gate's own
   Shift-only scope proved insufficient) — discovering it while implementing STEP233 and NOT fixing it
   would leave this ticket's own new acceptance test (`RunCtrlWithinShiftRangeChecks`) unable to pass on
   its own merits, mirroring STEP232's own stated reasoning for the same class of decision verbatim.
5. **The marquee-sync gap STEP232 already documented and accepted is explicitly re-confirmed UNCHANGED**,
   not silently left ambiguous — the dispatch brief specifically asked this question be answered one way
   or the other; investigated directly (`SyncManualMarkerSelection` is reached only from
   `DrawManualInstanceRow`'s own callback, never from `ApplyMarqueeGesture`) and confirmed unaffected.
6. **`SelectManualMarkerByInstanceIdentifier` is kept, unmodified in signature/behavior, rather than
   retired**, even though it loses its sole production caller. Investigated: it remains a correct,
   still-tested, general-purpose Ctrl/Shift-aware single-key canvas setter that a genuinely separate,
   narrower cleanup ticket could retire later if truly dead — retiring it in THIS ticket would mean
   rewriting `MapCanvas_Picking_UI_Test.cpp`'s own substantial existing Ctrl/Shift coverage of it for no
   functional gain, widening this already-large ticket's blast radius for a code-cleanliness concern
   unrelated to the confirmed bug (Constitution's own "smallest reusable, hyper-specific units; minimal
   blast radius" law, and this agent's own charter bias against inventing extra scope beyond a confirmed
   bug fix). Its own doc comment is corrected (§1b/§18) so it no longer asserts a role it doesn't play.

## Key files read/cited while drafting
`D:\Projects\Sanctuary\Map Generator\src\ui\Application_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\Application_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_SelectionSet_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MarkersTab_ManualInstanceSelection_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MarkersTab_ManualInstanceSelection_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MarkersTab_ManualLayerRowBody_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MarkersTab_ManualLayerRowBody_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MarkersTab_ManualLayers_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MarkersTab_ManualLayers_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MarkersTab_Bundles_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MarkersTab_Bundles_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MarkersTab_TypeSections_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MarkersTab_TypeSections_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MarkersTab_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MarkersTab_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MarkersTab_ManualInstanceListRows_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MarkersTab_ListCanvasSelectionSync_UI_Test.cpp` (untracked, STEP232's own new file),
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_Picking_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\CMakeLists.txt` (lines 826-838, confirmed both relevant test targets
already registered — no CMake edit needed this ticket),
`D:\Projects\Sanctuary\Map Generator\ARCH_21_01_MultiSelectRepresentation.md`,
and `work_orders\STEP232_ManualListShiftRangeAnchorClobber_UI.md`/`STEP214_AreaAltCenterResizeModifier_UI.md`
(structural/rigor templates and session-coordination wording, per the dispatching instruction).
