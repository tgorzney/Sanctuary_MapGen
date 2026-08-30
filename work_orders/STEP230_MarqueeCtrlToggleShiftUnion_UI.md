# STEP230 — Marquee Ctrl toggles, Shift unions: fix the batch `ApplySelectionGesture` overload treating both modifiers identically

**Layer:** UI. **Domain:** `MapCanvas`'s selection-set mutators and the marquee/list-batch counterpart of `ApplySelectionGesture` (`MapCanvas_SelectionSet_UI.h`/`.cpp`, `MapCanvas_UI.cpp`). **Executor:** SanGen Coder. Authored by the SanGen UI Expert. Implements an already-ratified ARCH addition — `ARCH_21_01_MultiSelectRepresentation.md` §21.1's new `ToggleEachInSelectionSet` function (lines 28-39) and its resolution rule in the widened `ApplySelectionGesture` contract (lines 66-75) — it invents no new law. Every file this ticket cites was read directly against the live tree while drafting it.

## Session coordination (required before EVERY file edit, not just once at ticket start)
Multiple Claude Code sessions may be active on this machine concurrently, editing the SAME working directory. A single check at the start of this ticket is NOT sufficient — a peer can start editing any of this ticket's files at any point after your initial check. Before EACH individual file edit in §1-5 below (not just once, up front):
1. Call `ListAgents` to enumerate active/open peer sessions on this machine.
2. Message each one (`SendMessage`) naming the SPECIFIC file you are about to edit right now, asking if they are currently editing it or planning to.
3. Wait for replies before making that edit.
4. If a peer reports current or planned work in that exact file, do NOT edit concurrently — negotiate a sequential order (whichever session is further along lands and merges first; the other rebases onto that afterward) and record the agreed order in this ticket's own notes before proceeding.
5. If no peer claims that file, proceed with that one edit — then repeat steps 1-4 for the NEXT file before editing it. A "no conflict" answer for one file is not an answer for another, and an answer from earlier in the session is not an answer for right now — re-check per file, every time.

**Pre-check already completed by the drafting session** (2026-08-30, before this ticket was written): steps 1-2 above were run personally against this ticket's likely file list (`MapCanvas_UI.cpp`, `MapCanvas_SelectionSet_UI.h`/`.cpp`, `MapCanvas_SelectionSet_UI_Test.cpp`) and both `map-generator-12` and `map-generator-47` replied with clear "no conflict." **Re-verify at actual dispatch time regardless** — this ticket may sit un-dispatched for a while and the pre-check does not substitute for the standing instruction above.

**Sibling-ticket merge-order note (EXECUTION_CONFLICT_MAP.md convention):** `STEP229` (multi-select canvas highlighting, draw-pass-only "primary" gap) touches `MapCanvas_Draw_UI.cpp` and `MapCanvas_IconLayer_*` files and, per its own brief, *possibly* `MapCanvas_SelectionSet_UI.h`/`.cpp` if it needs `SelectionSetContains`. `SelectionSetContains` already exists today (`MapCanvas_SelectionSet_UI.h:23`, `.cpp:12-16`) and is untouched by this ticket, so if STEP229 only *calls* it (read-only usage), the two tickets are fully additive/disjoint on this file regardless of order. If STEP229 turns out to need a NEW function in `MapCanvas_SelectionSet_UI.h`/`.cpp`, land STEP230 first — its own edit is a small, self-contained insertion of one function between the existing `ToggleInSelectionSet` and `UnionIntoSelectionSet` declarations/definitions (see §1/§2 below), unlikely to collide with an addition elsewhere in the file, but re-diff both at merge time regardless. **Files touched by this ticket:** `src/ui/MapCanvas_SelectionSet_UI.h`, `src/ui/MapCanvas_SelectionSet_UI.cpp`, `src/ui/MapCanvas_SelectionSet_UI_Test.cpp`, `src/ui/MapCanvas_UI.cpp`, `src/ui/MapCanvas_GestureOwnership_UI_Test.cpp`.

## Summary
Drag-box (marquee) selection's Ctrl and Shift currently do the exact same thing. `MapCanvas_UI.cpp`'s batch overload of `ApplySelectionGesture` (lines 99-114) treats `bCtrlHeld || bShiftHeld` as a single branch that always calls `UnionIntoSelectionSet` — Ctrl-marquee never removes anything, it only adds, identically to Shift-marquee. Single-click Ctrl (toggle) vs Shift (union), via the single-key overload two lines above (`MapCanvas_UI.cpp:85-97`), already work correctly and are untouched by this ticket.

`ARCH_21_01_MultiSelectRepresentation.md` §21.1 (lines 28-39) now names the fix: `ToggleEachInSelectionSet`, the batch counterpart to the existing single-key `ToggleInSelectionSet` — each key in the batch, in turn, in order, toggled exactly as a single Ctrl-click would (present → erase, absent → append/becomes primary). The six-query marquee concatenation (`MapCanvas_SelectionGesture_UI.cpp:108-175`, three procedural `PickInstancesInRegion` calls plus three lock-gated manual `CollectManualInstancesInWorldRegion` calls) is guaranteed duplicate-free per the ARCH text's own note — procedural and manual key spaces are disjoint (`bManual` differs) and no single query enumerates the same instance twice — so the documented duplicate-key re-toggle caveat in the ARCH text is not exercised by any real call site and is not separately tested here.

The ratified resolution rule (§21.1 lines 66-75): neither modifier → Replace; Shift held (Ctrl not) → Union; Ctrl held → Toggle (`ToggleInSelectionSet` for the single-key overload, `ToggleEachInSelectionSet` for the batch overload) — Ctrl wins if both are somehow held. This ticket implements exactly that for the batch overload; the single-key overload (`MapCanvas_UI.cpp:85-97`) already implements it correctly today and is not touched.

## Required reading
`ARCH_21_01_MultiSelectRepresentation.md` lines 28-39 (`ToggleEachInSelectionSet`'s full ratified contract, including the documented duplicate-key caveat) and lines 66-75 (the `ApplySelectionGesture` resolution rule, now correctly describing BOTH overloads once this ticket ships). Note `MapCanvas_UI.h:255-259`'s own private-method header comment ("Both resolve to exactly one of Replace/Toggle/Union") is **already accurate to the ratified design** and needs no edit — it was apparently written ahead of the batch overload's own body catching up; this ticket brings the body into line with a comment that was already correct.

---

## 1. Modified: `src/ui/MapCanvas_SelectionSet_UI.h`

Currently (`MapCanvas_SelectionSet_UI.h:36-44`):
```cpp
// Ctrl-click (single key only — never called with a marquee batch): present -> erase (primary
// becomes the new back(), or the set empties); absent -> append (becomes primary).
void ToggleInSelectionSet(OverlayInstanceKeySet_UI& set, const OverlayInstanceKey_UI& key);

// Shift-click / Shift-marquee: every key in `keys`, in order, not already present is appended
// (becomes primary in turn — the LAST newly-appended key is the final primary). An already-present
// key keeps its existing position, never re-touched/reordered by a union. If every key in `keys`
// was already present, the primary is unchanged.
void UnionIntoSelectionSet(OverlayInstanceKeySet_UI& set, const std::vector<OverlayInstanceKey_UI>& keys);
```
Insert the new declaration between them (mirroring `ARCH_21_01_MultiSelectRepresentation.md:28-38` verbatim for the comment):
```cpp
// Ctrl-click (single key only — never called with a marquee batch): present -> erase (primary
// becomes the new back(), or the set empties); absent -> append (becomes primary).
void ToggleInSelectionSet(OverlayInstanceKeySet_UI& set, const OverlayInstanceKey_UI& key);

// Ctrl-marquee (batch counterpart to the single-key `ToggleInSelectionSet` above, never called with
// a single key): each key in `keys`, in turn, in order — present -> erase, absent -> append (becomes
// primary in turn, so the LAST key from `keys` that ends up present is the final primary; if the
// last key's own toggle erases it, the primary falls back to the set's new back(), same as
// `ToggleInSelectionSet`'s erase case). This is Toggle applied per element, NOT a hybrid of
// Toggle+Union. Duplicate-key caveat: if `keys` itself contains the same key more than once (a
// marquee region query is not expected to, but this function does not assume it and performs no
// de-duplication), each repeat re-toggles that key's presence in place — two repeats net to a
// no-op for that key, an odd count of repeats nets to the same single toggle a lone occurrence
// would. Callers passing an already-deduplicated `keys` never observe this; it is documented so a
// coder who feeds a raw/undeduplicated hit list is not surprised by it.
void ToggleEachInSelectionSet(OverlayInstanceKeySet_UI& set, const std::vector<OverlayInstanceKey_UI>& keys);

// Shift-click / Shift-marquee: every key in `keys`, in order, not already present is appended
// (becomes primary in turn — the LAST newly-appended key is the final primary). An already-present
// key keeps its existing position, never re-touched/reordered by a union. If every key in `keys`
// was already present, the primary is unchanged.
void UnionIntoSelectionSet(OverlayInstanceKeySet_UI& set, const std::vector<OverlayInstanceKey_UI>& keys);
```

---

## 2. Modified: `src/ui/MapCanvas_SelectionSet_UI.cpp`

Currently (`MapCanvas_SelectionSet_UI.cpp:29-42`):
```cpp
void ToggleInSelectionSet(OverlayInstanceKeySet_UI& set, const OverlayInstanceKey_UI& key) {
    for (std::size_t index = 0; index < set.keys.size(); ++index) {
        if (OverlayInstanceKeysEqual(set.keys[index], key)) {
            set.keys.erase(set.keys.begin() + static_cast<std::ptrdiff_t>(index));
            return;
        }
    }
    set.keys.push_back(key);
}

void UnionIntoSelectionSet(OverlayInstanceKeySet_UI& set, const std::vector<OverlayInstanceKey_UI>& keys) {
    for (const OverlayInstanceKey_UI& key : keys)
        if (!SelectionSetContains(set, key)) set.keys.push_back(key);
}
```
Insert the new definition between them — implemented as a loop calling the existing single-key `ToggleInSelectionSet` for each element, in order, mirroring its own implementation style exactly (no new erase/append logic invented — reuses the already-correct primitive):
```cpp
void ToggleInSelectionSet(OverlayInstanceKeySet_UI& set, const OverlayInstanceKey_UI& key) {
    for (std::size_t index = 0; index < set.keys.size(); ++index) {
        if (OverlayInstanceKeysEqual(set.keys[index], key)) {
            set.keys.erase(set.keys.begin() + static_cast<std::ptrdiff_t>(index));
            return;
        }
    }
    set.keys.push_back(key);
}

void ToggleEachInSelectionSet(OverlayInstanceKeySet_UI& set, const std::vector<OverlayInstanceKey_UI>& keys) {
    for (const OverlayInstanceKey_UI& key : keys) ToggleInSelectionSet(set, key);
}

void UnionIntoSelectionSet(OverlayInstanceKeySet_UI& set, const std::vector<OverlayInstanceKey_UI>& keys) {
    for (const OverlayInstanceKey_UI& key : keys)
        if (!SelectionSetContains(set, key)) set.keys.push_back(key);
}
```
This one-line-per-key delegation is provably correct against the ARCH contract: each call to `ToggleInSelectionSet(set, key)` does exactly "present → erase (primary falls back to new `back()`), absent → append (becomes primary)" — precisely the per-element behavior §21.1 lines 28-38 specifies, including the erase-fallback-to-`back()` clause, for free, with zero duplicated logic.

---

## 3. Modified: `src/ui/MapCanvas_UI.cpp`

Currently (`MapCanvas_UI.cpp:99-114`):
```cpp
// The marquee/list-batch counterpart. `ToggleInSelectionSet` takes a single key only (never a legal
// operation for a batch, per its own header comment), so a batch gesture only ever resolves to
// Replace (no modifier) or Union (Ctrl OR Shift held) — the same "no per-batch toggle mechanism
// exists" reading `ManualInstanceHitTest_UI.h`'s own release-time consumer (ARCH §21.2) depends on.
void MapCanvas::ApplySelectionGesture(const std::vector<OverlayInstanceKey_UI>& touchedKeys, bool bCtrlHeld,
                                      bool bShiftHeld) {
    const OverlayInstanceKeySet_UI previous = selectedInstanceKeys;
    if (bCtrlHeld || bShiftHeld) {
        UnionIntoSelectionSet(selectedInstanceKeys, touchedKeys);
    } else {
        ReplaceSelectionSet(selectedInstanceKeys, touchedKeys);
    }
    if (SelectionSetsEqual(previous, selectedInstanceKeys)) return;
    if (selectionChangedCallback)
        selectionChangedCallback(PrimaryOfSelectionSet(selectedInstanceKeys), selectedInstanceKeys);
}
```
Replace with:
```cpp
// The marquee/list-batch counterpart. STEP230 (ARCH §21.1, ToggleEachInSelectionSet) — Ctrl held
// resolves to a real per-element TOGGLE (`ToggleEachInSelectionSet`), not Union: pre-STEP230, Ctrl
// and Shift were treated identically here (both unioned), so a Ctrl-marquee over an
// already-selected box was a silent no-op instead of deselecting. Ctrl wins if both are somehow
// held, matching the single-key overload's own tie-break two functions above.
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
No other line in this file changes — the single-key overload immediately above (`MapCanvas_UI.cpp:85-97`) already implements the identical three-way Replace/Toggle/Union resolution and is untouched.

---

## 4. Modified: `src/ui/MapCanvas_SelectionSet_UI_Test.cpp`

Add a new check function mirroring `RunToggleChecks`/`RunUnionChecks`'s existing style exactly (same `Check`/`MakeKey` helpers, same file, no new includes). Insert after `RunUnionChecks` (currently ending line 98, before the closing `} // namespace` at line 100):
```cpp
// ARCH §21.1 (STEP230) — ToggleEachInSelectionSet is Toggle applied per element, in order, NOT a
// hybrid of Toggle+Union: each key present->erase, absent->append, exactly as a loop of
// ToggleInSelectionSet calls would produce (which is in fact its real implementation).
void RunToggleEachChecks() {
    // Fresh batch, all absent: every key appended in order, last becomes primary.
    OverlayInstanceKeySet_UI freshSet;
    ToggleEachInSelectionSet(freshSet, {MakeKey(1), MakeKey(2), MakeKey(3)});
    Check(freshSet.keys.size() == 3, "toggling a fresh batch in appends every key");
    Check(OverlayInstanceKeysEqual(PrimaryOfSelectionSet(freshSet), MakeKey(3)),
          "the LAST key in a fresh all-absent batch becomes primary");

    // Mixed batch, last operation is an append: present key(s) erased, absent key(s) appended, the
    // appended (present) key is the final primary — the ordinary case.
    OverlayInstanceKeySet_UI mixedAppendLast;
    mixedAppendLast.keys.push_back(MakeKey(5));
    ToggleEachInSelectionSet(mixedAppendLast, {MakeKey(5), MakeKey(6)});
    Check(mixedAppendLast.keys.size() == 1 && OverlayInstanceKeysEqual(mixedAppendLast.keys[0], MakeKey(6)),
          "a mixed batch erases the present key and appends the absent one, ending with just the appended key");
    Check(OverlayInstanceKeysEqual(PrimaryOfSelectionSet(mixedAppendLast), MakeKey(6)),
          "the last key that ENDS UP present (the appended one) is the final primary");

    // Mixed batch, last operation is an ERASE: the last key's own toggle removes it from the set, so
    // primary must fall back to the set's new back() — NOT to the just-erased key, and NOT left
    // pointing at a key no longer in the set.
    OverlayInstanceKeySet_UI mixedEraseLast;
    mixedEraseLast.keys.push_back(MakeKey(1));
    mixedEraseLast.keys.push_back(MakeKey(2));   // primary starts as 2
    ToggleEachInSelectionSet(mixedEraseLast, {MakeKey(2), MakeKey(3), MakeKey(1)});
    // Step by step: 2 present->erase (set={1}); 3 absent->append (set={1,3}, primary 3);
    // 1 present->erase (set={3}) — the LAST key's own toggle (key 1) erased it, so primary falls
    // back to the set's new back(), which is 3.
    Check(mixedEraseLast.keys.size() == 1 && OverlayInstanceKeysEqual(mixedEraseLast.keys[0], MakeKey(3)),
          "erase-fallback case: the final set is exactly {3} after the 3-step mixed toggle sequence");
    Check(OverlayInstanceKeysEqual(PrimaryOfSelectionSet(mixedEraseLast), MakeKey(3)),
          "erase-fallback case: when the LAST key's own toggle erases it, primary falls back to the "
          "set's new back() (3), matching ToggleInSelectionSet's own erase-case contract, not the "
          "erased key (1) and not the batch's own last-listed key blindly");
}
```
Add the call to `main()` (currently `RunUnionChecks();` at line 107, immediately before the `if (failureCount == 0)` check):
```cpp
    RunUnionChecks();
    RunToggleEachChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
```
No `CMakeLists.txt` change needed — `MapCanvas_SelectionSet_UI_Test` (`CMakeLists.txt:865`) is already a single-file `add_sangen_test` target; this ticket adds code inside the existing file, not a new file.

---

## 5. Modified: `src/ui/MapCanvas_GestureOwnership_UI_Test.cpp`

Add a live end-to-end Ctrl-marquee-toggle / Shift-marquee-union case, mirroring this file's own established technique (`SimulatePressDragRelease` with a real `bCtrl`/`bShift` argument, `io.AddKeyEvent`-driven modifiers via `SetModifierKeys`, `ScreenPositionFor` for world→screen projection) exactly as its existing box-marquee check (lines 231-245) and single-click Ctrl-toggle check (lines 247-255) already do. Insert immediately after the existing Ctrl-click block (currently lines 247-255), before `ImGui::DestroyContext();` (currently line 257):
```cpp
    // --- ARCH §21.1 (STEP230): a Ctrl-held marquee TOGGLES each touched key, not Union. Pre-fix,
    // Ctrl and Shift were resolved identically in the batch overload (both called
    // UnionIntoSelectionSet), so a Ctrl-marquee over an already-selected box was a silent no-op
    // instead of deselecting — this is the exact bug this ticket exists to prove fixed. ---
    lastReportedSet.keys.clear();
    SimulatePressDragRelease(canvas, boxPressPosition, boxReleasePosition, /*button=*/0);   // baseline: plain marquee selects {A, B}
    check(lastReportedSet.keys.size() == 2,
          "baseline: a plain marquee over the box re-selects both unlocked markers");

    SimulatePressDragRelease(canvas, boxPressPosition, boxReleasePosition, /*button=*/0, /*bCtrl=*/true);
    check(lastReportedSet.keys.empty(),
          "STEP230 — a Ctrl-held marquee over an ALREADY-selected box TOGGLES every touched key OFF; "
          "pre-fix this stayed at {A, B} (Union-with-itself is a no-op), which this check would have "
          "caught as a FAILure");

    SimulatePressDragRelease(canvas, boxPressPosition, boxReleasePosition, /*button=*/0, /*bCtrl=*/true);
    check(lastReportedSet.keys.size() == 2,
          "STEP230 — Ctrl-marquee toggles absent keys back ON symmetrically (present->erase, "
          "absent->append, applied per element)");

    // --- ARCH §21.1: a Shift-held marquee still UNIONS (adds, never removes) — distinct from Ctrl's
    // toggle proven above; the set is already {A, B} here, so a union over the same box is a no-op
    // that stays at 2, not empty. ---
    SimulatePressDragRelease(canvas, boxPressPosition, boxReleasePosition, /*button=*/0, /*bCtrl=*/false,
                             /*bShift=*/true);
    check(lastReportedSet.keys.size() == 2,
          "ARCH §21.1 — a Shift-held marquee over an already-selected box UNIONS (never removes), "
          "staying at 2 — distinct from Ctrl's toggle-off behavior proven above");
```
No other line in this file changes — `boxPressPosition`/`boxReleasePosition` (lines 233-234), `SimulatePressDragRelease` (lines 59-78), and `lastReportedSet` (line 144, updated by the callback set at lines 145-147) are all pre-existing and reused exactly as-is; the new block is purely additive.

No `CMakeLists.txt` change needed — `MapCanvas_GestureOwnership_UI_Test.cpp` is already wired into the `MapCanvas_UI_Test` target (per STEP214's own citation, `CMakeLists.txt:576-588` region) and `MapCanvas_UI_Test.cpp` already forward-declares and calls `RunMapCanvasGestureOwnershipChecks`; this ticket adds code inside that existing function body only.

**Before implementing §5, re-verify against the live file** that `SimulatePressDragRelease` actually has `bCtrl`/`bShift` parameters in that exact position and that `boxPressPosition`/`boxReleasePosition`/`lastReportedSet` are named and scoped exactly as cited — this section was drafted from a summarized read of the file, not a fresh one, and its exact signatures must be confirmed against the real source before the coder writes this block verbatim.

---

## ARCH rules invoked
- `ARCH_21_01_MultiSelectRepresentation.md` §21.1, lines 28-39 — `ToggleEachInSelectionSet`'s full ratified contract (per-element toggle, primary-tracking, erase-fallback-to-`back()`, documented duplicate-key caveat). This ticket's `.h`/`.cpp` additions (§1/§2 above) implement this contract verbatim, via delegation to the already-correct `ToggleInSelectionSet`.
- `ARCH_21_01_MultiSelectRepresentation.md` §21.1, lines 66-75 — the `ApplySelectionGesture` resolution rule (neither→Replace, Shift-only→Union, Ctrl(-and-maybe-Shift)→Toggle, Ctrl wins the tie). This ticket's `MapCanvas_UI.cpp` change (§3 above) brings the batch overload into line with this rule; the single-key overload already conformed.
- Constitution §6 — the exact semantics of `ToggleEachInSelectionSet` (ordering, primary derivation, the erase-fallback edge case) were resolved by the ARCH Expert rather than left for this ticket or its coder to invent; this ticket implements that resolution, it does not make a new one.

## Explicit out-of-scope
- **No change to the single-key `ApplySelectionGesture` overload** (`MapCanvas_UI.cpp:85-97`) — Ctrl-click (toggle) vs Shift-click (union) already work correctly today and are confirmed unmodified by this ticket.
- **No change to `ApplyMarqueeGesture`'s six-query concatenation** (`MapCanvas_SelectionGesture_UI.cpp:108-175`) — the procedural/manual key-space disjointness that makes the batch guaranteed duplicate-free is a pre-existing property of that function, unaffected by and not re-verified beyond citing it here.
- **No change to `MapCanvas_UI.h`** — its private-method header comment (lines 255-259) already describes the ratified three-way Replace/Toggle/Union behavior accurately; this ticket's code change simply catches the body up to a comment that was already correct.
- **No de-duplication logic added to `ToggleEachInSelectionSet`** — the ARCH text explicitly specifies it performs none, and no real call site (the six-query marquee) can produce duplicates, per the ARCH text's own note.
- **No change to `MarkersTab_ManualInstanceSelection_UI.h`'s list-local multi-select** or any convergence with it — ARCH §21.1's own text explicitly scopes that out as "a separate, larger change," unrelated to this ticket.
- **No STEP229 file read for its final content or edited** — this ticket is fully self-contained; the merge-order note above is precautionary only.
- **No ARCH file edit** — this ticket never writes `ARCH.md` or any `ARCH_NN_*.md` file; §21.1 was already ratified with the exact function this ticket implements before this ticket was drafted.

## Acceptance test
1. `MapCanvas_SelectionSet_UI_Test` (`ctest` binary) passes `ALL PASS`, including the new `RunToggleEachChecks`: a fresh all-absent batch appends every key with the last as primary; a mixed batch ending in an append leaves the appended key as primary; a mixed batch ending in an erase falls the primary back to the set's new `back()`, not the erased key.
2. `MapCanvas_UI_Test` (`ctest` binary) passes `ALL PASS`, including `RunMapCanvasGestureOwnershipChecks`'s new block: a live Ctrl-held marquee over an already-selected box empties the selection (proves real removal, not the pre-fix Union-no-op); a second Ctrl-marquee over the same box re-adds both; a Shift-held marquee over an already-selected box stays at 2 (proves Shift still only unions, never removes) — this is the actual proof the bug is fixed, since a pre-fix build would fail check #2 in this list.
3. Every pre-existing case in both test files (`RunReplaceChecks`, `RunToggleChecks`, `RunUnionChecks`, the pan/no-pan/rubber-band/lock-gate/plain-marquee/single-Ctrl-click checks in `MapCanvas_GestureOwnership_UI_Test.cpp`) continues to pass unmodified.
4. Full `SanGenV2` build stays clean; every existing test in the suite continues to pass.

## Interpretation calls made
1. **`ToggleEachInSelectionSet` is implemented as a loop delegating to the existing `ToggleInSelectionSet`**, not a hand-rolled independent erase/append scan. This is not a stylistic preference invented here — it is the only implementation that is trivially, structurally correct against the ARCH contract (which is itself defined in terms of "each key... toggled exactly as a single Ctrl-click would"), and it introduces zero duplicated logic for a second maintainer to keep in sync.
2. **Insertion order in `MapCanvas_SelectionSet_UI.h`/`.cpp`** — placed between `ToggleInSelectionSet` and `UnionIntoSelectionSet`, matching the ARCH text's own presentation order (Replace, Toggle, ToggleEach, Union) exactly, so the file's own reading order stays a straight mirror of the ratified spec.
3. **Test case selection for the erase-fallback scenario** (`RunToggleEachChecks`'s `mixedEraseLast`) was hand-derived step-by-step in a code comment rather than asserted opaquely, since this is the one non-obvious edge case in the whole contract (primary falling back to `back()` rather than tracking the just-erased key) and is exactly the kind of case a coder could get subtly wrong with a hand-rolled (non-delegating) implementation — see Interpretation call 1.
4. **The `MapCanvas_GestureOwnership_UI_Test.cpp` addition reuses the exact same box positions** (`boxPressPosition`/`boxReleasePosition`) the pre-existing plain-marquee lock-gate check already established, rather than picking new world coordinates — this keeps the new block's own baseline ("plain marquee selects {A, B}") trivially consistent with a check the file already proved true a few lines above, and avoids re-deriving a second geometric marquee box. **This section was drafted from a summarized read, not a fresh one — the coder must re-verify the exact helper signatures against the live file before writing it (flagged again at the end of §5).**
5. **The new GestureOwnership block is sequenced to actively distinguish pre-fix from post-fix behavior** (Ctrl-marquee-over-already-selected-box → must go to empty, not stay at 2) rather than merely exercising the new code path — a Union-only regression (the actual pre-STEP230 bug) would still leave the set non-empty and this check would visibly fail, which a weaker "set ends up non-empty" assertion would not have caught.
6. **`STEP230` was chosen as the ticket number** after confirming `work_orders/` ran `STEP200`-`STEP228` with no `STEP229` present at drafting time (reserved by a sibling ticket being drafted in parallel) — re-check for the actual next-free number at dispatch time, since peer sessions are active and numbering may have shifted.

## Key files read/cited while drafting
`D:\Projects\Sanctuary\Map Generator\ARCH_21_01_MultiSelectRepresentation.md`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_SelectionSet_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_SelectionSet_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_SelectionSet_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_SelectionGesture_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_GestureOwnership_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\ManualInstanceHitTest_UI.h`,
`D:\Projects\Sanctuary\Map Generator\CMakeLists.txt` (line 865, `MapCanvas_SelectionSet_UI_Test` target; lines 576-588 region, `MapCanvas_UI_Test` target),
`D:\Projects\Sanctuary\Map Generator\work_orders\EXECUTION_CONFLICT_MAP.md` (merge-order table convention),
`D:\Projects\Sanctuary\Map Generator\work_orders\STEP214_AreaAltCenterResizeModifier_UI.md` (structural/rigor template, per the dispatching instruction),
and a `Glob` over `work_orders/STEP2*` to confirm the next-free ticket number (`STEP200`-`STEP228` present, no `STEP229`/`STEP230`).
