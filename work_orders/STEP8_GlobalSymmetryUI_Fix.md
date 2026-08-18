# Work-Order — Step 8: fix the Global Symmetry tab's exclusive-choice bug

*Constitution §7. Executor: SanGen Coder. Closes the second standing recorded defect from
`sangen_arch_pack/INDEX.md`: "Global Symmetry tab UI is exclusive-choice, can't combine axes,
while the per-rule symmetry UI already correctly allows combinations. The tab's existing
'Radial' checkbox is currently a stale stand-in mapped to QuarterTurns." Verified directly
against the real code — this is a genuine, currently-live bug, not a forward-looking risk (unlike
the 16-slot orbit-buffer item, which turned out to depend on an unimplemented `Radial` axis and
was not pursued this round — see the orchestrating session's notes).*

## Root problem
`SymmetryTab_UI.h`/`.cpp` presents the global symmetry mask as a 5-option EXCLUSIVE checkbox row
(`SymmetryAxisOption::{Point, MirrorX, MirrorZ, MirrorXZ, Radial}`, via `DrawExclusiveCheckboxRow`)
mapped onto 5 fixed, non-combinable presets. `Params::SymmetryAxis` is a real OR-able 4-bit mask
(`MirrorAcrossX`, `MirrorAcrossZ`, `RotateHalfTurn`, `QuarterTurns`) — the tab cannot express, for
example, "Mirror X AND Half Turn together," even though that's a perfectly legal mask a per-rule
override can already hold. The code already half-admits this: `DrawAxisRow`
(`SymmetryTab_UI.cpp:28-31`) has a fallback branch that prints "Recipe mask 0x%X is a combination
this row cannot show" when a hand-edited or per-rule-derived mask doesn't match one of the 5
presets — a workaround for the bug, not a fix.

Separately, `SymmetryAxisOption::Radial` (labelled `"Radial"` in the UI) maps to
`Params::SymmetryAxis::QuarterTurns` (`SymmetryTab_UI.h:53`) — a fixed 4-way rotation, not
adjustable-fold radial symmetry. Real Radial N-fold symmetry (`SymmetryAxis::Radial`,
`radialSymmetryRepeatCount`, ARCH §13) does not exist in code anywhere yet — this is confirmed by
grep across the whole tree. So `"Radial"` here has always been a misleading label for
`QuarterTurns`, exactly as `INDEX.md` describes, and this ticket does NOT implement real Radial
symmetry (that remains its own future, larger, deferred feature) — it only removes the mislabeling.

`PlacementRuleSections_UI.h`'s `DrawPlacementSymmetryAxes` (used by every per-rule symmetry
override — Markers/Props/Decals/Units, including this session's Step 7 fix) already solves this
exact problem correctly: four INDEPENDENT tick boxes (`"Mirror X"`, `"Mirror Z"`, `"Half Turn"`,
`"Quarter Turns"`) over the real bit mask, with no exclusivity and no unshowable-combination case.
That file's own header comment (lines 23-27) explains exactly why: "v1 offered five EXCLUSIVE
symmetry choices... Independent tick boxes are drawn rather than Checkbox_UI's exclusive row: an
exclusive row cannot express X|Z, and dropping a combination a recipe already holds would be a
widget overruling PARAMS." The Global Symmetry tab needs the identical treatment — it's the one
place in the codebase this fix was never applied.

## Target files
- `src/ui/PlacementRuleSections_UI.h`/`.cpp` — extract the per-axis independent-checkbox LOOP out
  of `DrawPlacementSymmetryAxes` into a new, smaller shared function:
  ```cpp
  // Four independent tick boxes over the real bit mask — no "Use Global" wrapper, for callers
  // that ARE the global setting itself (unlike DrawPlacementSymmetryAxes's per-rule override use).
  void DrawIndependentSymmetryAxes(int& symmetryMask, Pipeline::PreviewDriver* previewDriver);
  ```
  `DrawPlacementSymmetryAxes` keeps its "Use Global Symmetry" checkbox and early-return, then calls
  this new function for the per-axis loop instead of inlining it — this is the correct home for the
  extraction (Constitution §8: the loop is about to have TWO callers, so it earns a shared name;
  before this ticket it had exactly one, correctly staying inline).
- `src/ui/SymmetryTab_UI.h` — remove `SymmetryAxisOption` (the enum), `symmetryAxisOptionLabels`,
  `SymmetryAxisMaskOfOption`, `SymmetryAxisOptionOfMask`, `SymmetryOptionBitsOfMask`,
  `SymmetryAxisMaskOfOptionBits`, and `SymmetryTabState::axisOptionBits` — all of this existed only
  to support the flawed exclusive-5-preset scheme and has no purpose once the tab edits
  `recipe.globalSymmetryMask` directly via independent bits. `LoadSymmetryTabValues`/
  `StoreSymmetryTabValues` likely become unnecessary too (there's no longer a separate mirror word
  to load into/store from) — confirm and remove if so, or simplify if some other state still needs
  the round-trip shape.
- `src/ui/SymmetryTab_UI.cpp` — `DrawAxisRow`: replace the `DrawExclusiveCheckboxRow(...)` call and
  the "cannot show" fallback text with a call to the new `DrawIndependentSymmetryAxes(recipe.
  globalSymmetryMask, previewDriver)`. The fallback text becomes dead code once every mask
  combination is representable — delete it, don't leave it unreachable.
- `src/ui/SymmetryTab_UI_Test.cpp` — `RunAxisOptionChecks`/`RunMirrorChecks` are built entirely
  around the removed exclusive-option scheme and need rewriting to test the new independent-bit
  behavior instead: setting/clearing each of the 4 real axis bits independently, combinations that
  were previously "unshowable" (e.g. `MirrorAcrossX | QuarterTurns`) now round-tripping correctly,
  and — the actual regression case — a mask like `MirrorAcrossX | RotateHalfTurn` (X + Half Turn
  together) surviving the tab exactly, which the old exclusive row could never represent at all.
  Check this file's own header comment ("NOT YET REGISTERED IN CMake") — confirm current status in
  `CMakeLists.txt` and register it if it's genuinely still dormant; don't assume the comment is
  stale without checking.

## Layer & accuracy class
UI. Accuracy class: Visual/Exact — the mask value itself must round-trip exactly (it's real recipe
content, `Params::MapRecipe::globalSymmetryMask`), even though the widget presentation is visual.

## Backend policy
N/A — pure UI/imgui composition, no dispatch.

## ARCH rules invoked
- `PLACEMENT_SCATTER_SPEC.md` — the already-ratified per-rule symmetry pattern this ticket extends
  to the global tab; no new design, direct reuse of an existing, tested mechanism.
- Constitution §6 — a hand-edited or otherwise-arrived-at mask combination is now always
  representable and editable, never silently unshowable or clamped to a neighboring preset.
- This codebase's established "share only when a second real caller exists" convention (applied
  repeatedly this session in the IO layer) — the extraction of `DrawIndependentSymmetryAxes` is
  exactly that bar applied the moment a second caller exists, not premature abstraction.

## Solution
1. Extract `DrawIndependentSymmetryAxes` in `PlacementRuleSections_UI.h`/`.cpp` per "Target files"
   above. **UI Expert ruling (fold in, not optional):** perform the `ResolvedPlacementSymmetryMask`
   repair INSIDE `DrawIndependentSymmetryAxes` itself, not left to each caller — today the repair
   runs unconditionally in `DrawPlacementSymmetryAxes` before its "Use Global" early-return, while
   the old `SymmetryTab_UI.cpp` never repaired the global mask at all (only warned). Putting the
   repair inside the new shared function gives both call sites identical Constitution §6 behavior
   going forward, which is the more consistent choice and costs nothing extra. Verify
   `DrawPlacementSymmetryAxes`'s existing behavior is unchanged after the extraction —
   `PlacementRuleSections_UI_Test.cpp` (already registered, `CMakeLists.txt:458`) covers its pure
   helpers and must keep passing.
2. Rewrite `SymmetryTab_UI.h`/`.cpp` to use it directly on `recipe.globalSymmetryMask`, removing
   the dead exclusive-option machinery.
3. Rewrite `SymmetryTab_UI_Test.cpp` to test the new behavior. **UI Expert confirmed (do not
   re-derive):** this test binary is ALREADY registered (`CMakeLists.txt:430`,
   `add_sangen_test(SymmetryTab_UI_Test src/ui/SymmetryTab_UI_Test.cpp)`) — the file's own header
   comment claiming "NOT YET REGISTERED IN CMake" is stale, not current. Delete that stale comment
   when rewriting the file; do NOT add a second `add_sangen_test` line for it, that would create a
   duplicate/conflicting CMake target.
4. Leave `SymmetryTab_UI.h`'s SCOPE NOTE 1 (the Algorithm group — Fold/Blur/CrossFade/etc.) and
   SCOPE NOTE 2 (`SymmetryDetection`'s caller-owned home) exactly as they are — unrelated to this
   fix, still correctly out of scope.

## Explicit out-of-scope
- **Real Radial N-fold symmetry** (`SymmetryAxis::Radial`, `radialSymmetryRepeatCount`, ARCH §13) —
  not implemented by this ticket. This ticket only removes the misleading `"Radial"` label that
  incorrectly stood in for `QuarterTurns`; it does not add a new axis or a fold-count setting.
- **The 16-slot symmetry-orbit buffer** — separate, unrelated defect; not pursued this round since
  it depends on the not-yet-implemented Radial axis (see "Root problem" above for why).
- **The heightfield-symmetry PROC stage** (SCOPE NOTE 1) — still entirely unbuilt; this ticket only
  touches how the existing entity-placement symmetry mask is EDITED, not how/whether a heightfield
  gets symmetrized.

## Acceptance test
`SymmetryTab_UI_Test.exe` passes with the new coverage: each of the 4 real axis bits toggles
independently and round-trips through `recipe.globalSymmetryMask` correctly; a combination the old
exclusive row could never represent (e.g. `MirrorAcrossX | RotateHalfTurn`) is both showable
(ticks appear correctly for a mask set this way from outside the tab, e.g. a per-rule override
copied to global, or a hand-edited `.sanmap`) and settable (ticking two boxes produces the OR of
their bits) through the tab. No `"Radial"` label appears anywhere in `SymmetryTab_UI.h`/`.cpp`
after this ticket. Full `SanGenV2` build stays clean; `PlacementRuleSections_UI` and every tab that
depends on `DrawPlacementSymmetryAxes` (Markers/Props/Decals/Units) still passes its existing
tests unchanged.
