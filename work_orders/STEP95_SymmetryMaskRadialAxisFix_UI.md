# Work-Order — Step 95: fix `ResolvedPlacementSymmetryMask`'s Radial-bit strip (the shared symmetry widget)

*Constitution §2/§6/§7/§8. Executor: SanGen Coder. Authored by the SanGen UI Expert. Closes the
pre-existing defect that `STEP81`, `STEP94`, and `CONSOLIDATION_MASTER.md` (finding N1) each
independently found, flagged, and explicitly routed around rather than fixed.*

## Root problem
`ResolvedPlacementSymmetryMask` (`src/ui/PlacementRuleSections_UI.h:56-61`) exists to repair a
symmetry mask arriving from outside the tab — a hand-edited `.sanmap`, a per-rule override copied to
global — per Constitution §6 ("the bits a mask may legally carry... how a mask... is repaired rather
than obeyed"). It does this by ANDing the incoming mask against `legalBits`, built by OR-ing
`PlacementSymmetryAxisBit(axisIndex)` for `axisIndex` in `[0, kPlacementSymmetryAxisCount)`.

`kPlacementSymmetryAxisCount` is **4** (`PlacementRuleSections_UI.h:28`), and
`placementSymmetryAxisLabels`/`PlacementSymmetryAxisBit`'s switch (`PlacementRuleSections_UI.h:29-31,
33-41`) name only `MirrorAcrossX`, `MirrorAcrossZ`, `RotateHalfTurn`, `QuarterTurns`.
`Params::SymmetryAxis::Radial` (`src/params/Symmetry_PARAMS.h:23`, `1 << 4`, the fifth bit) is not
one of them, so `legalBits` never includes it — the repair silently clears the Radial bit from
**any** mask it touches.

Because `DrawIndependentSymmetryAxes` (`PlacementRuleSections_UI.cpp:30-32`) runs this repair
unconditionally at the top of every draw — *"the repair runs the moment the row is drawn rather than
at the next click"* per its own comment (`PlacementRuleSections_UI.cpp:27-29`) — **merely opening a
tab that shows the standard symmetry control silently drops Radial from whatever mask was loaded or
authored**, before the user touches anything.

## Verified real callers (all five go through the one shared function)
`ResolvedPlacementSymmetryMask` has exactly one call site, `DrawIndependentSymmetryAxes`
(`PlacementRuleSections_UI.cpp:31`), which is itself reached two ways:

| Caller | File:line | Domain |
|---|---|---|
| `DrawPlacementSymmetryAxes` → `DrawIndependentSymmetryAxes` | `PlacementRuleSections_UI.cpp:23` | shared per-rule wrapper |
| `DrawPlacementSymmetryAxes(...)` | `MarkersTab_UI.cpp:100` | Markers rule |
| `DrawPlacementSymmetryAxes(...)` | `ArmiesTab_Units_UI.cpp:117` | Units rule |
| `DrawPlacementSymmetryAxes(...)` | `PropsTab_UI.cpp:86` | Props rule |
| `DrawPlacementSymmetryAxes(...)` | `PropsTab_Decals_UI.cpp:111` | Decals rule |
| `DrawIndependentSymmetryAxes(recipe.globalSymmetryMask, ...)` directly, via `DrawAxisRow` | `SymmetryTab_UI.cpp:23` (called from `SymmetryTab_UI.cpp:41`) | the global Symmetry tab |

Confirmed by grep across `src/ui/`: no other call site exists. **Markers, Units, Props, Decals, and
the global Symmetry tab are all affected identically** — this is one shared-widget defect, not five
per-domain ones.

Also confirmed by grep: `radialSymmetryRepeatCount` (the count paired with the Radial bit,
`Symmetry_PARAMS.h:26-31`) has **zero** call sites anywhere under `src/ui/`. No placement tab has
ever exposed a control for it. This is a real, related gap — see "Explicit out-of-scope" below — but
it is not this ticket's defect and, per the analysis below, is not a blocker for this ticket's fix.

## Why this is a table gap, not an architectural defect
The four helper functions built on `kPlacementSymmetryAxisCount` — `PlacementSymmetryAxisBit`,
`IsPlacementSymmetryAxisSet`, `PlacementSymmetryMaskAfterToggle`, and `ResolvedPlacementSymmetryMask`
itself — are already written generically over the count and a switch table; none of them hardcodes
"four" in its logic, only in the table's extent. Bumping the count to 5 and adding one label + one
switch case is the **entire** required change; the loop bodies, the repair AND, the toggle XOR-ish
logic, and the checkbox-drawing loop in `DrawIndependentSymmetryAxes` all generalize for free.

Every other layer already treats Radial as a first-class axis: `Symmetry_PARAMS.h:23` defines the
bit alongside its siblings with the same comment style; `Placement_Symmetry_PROC.h`/
`Placement_SymmetryOrbit_PROC.h`'s `BuildSymmetryOrbit`/`AppendRadialTurns` branch on it correctly,
last in bit-check order by design (`Symmetry_PARAMS.h:20-22`); IO reads, writes, and range-clamps its
companion `radialSymmetryRepeatCount` field across `MapRecipe_PARAMS.h:87`, `Layer_PARAMS.h:69`,
`GeoLayer_PARAMS.h:33`, `ScatterRule_PARAMS.h:35,59,86`, and `MarkerRule_PARAMS.h:64` — every one
defaulting to `3`, already inside `[radialSymmetryRepeatCountMinimum=2,
radialSymmetryRepeatCountMaximum=12]`. UI is the only layer that never picked the bit up.

**Verdict: extend the shared axis table to 5 entries. This is not a structural/architectural
problem.** The widget was already built to scale (a count + a table), and PARAMS/PROC/IO already
carry a fully valid, sanely-defaulted Radial story; only the UI's own table missed the addition.

**A toggle alone is meaningful without a slider.** Because `radialSymmetryRepeatCount` already
defaults to `3` (a legal, non-degenerate value) on every struct that carries it, and an imported
`.sanmap` already carries whatever count it shipped with, adding a fifth "Radial" checkbox that only
flips the bit — with no accompanying count slider — immediately does two useful things: (1) it stops
the silent clear on every draw, so an imported or hand-authored Radial mask now survives, and (2) it
lets a user turn Radial on for the first time and get a real, working N=3 orbit. The fix's value does
not depend on a slider landing first. The missing slider remains a real, separate gap (see
"Explicit out-of-scope").

**A second, smaller defect to correct in the same edit:** the comment at
`PlacementRuleSections_UI.h:23-27` claims "v1 offered five EXCLUSIVE symmetry choices
(Point/X/Z/XY/Radial)... the five collapse onto four independent bits." That was accurate when it was
written — v1's own `Radial` choice was a placeholder mapped onto `QuarterTurns` (`STEP16`'s
"`SymmetryAxisOption::Radial`'s stale `QuarterTurns` stand-in mapping", `STEP23`'s "designers cannot
reach the real `Radial` bit through the exclusive [enum]"; the legacy `SymmetryAxisOption` type no
longer exists anywhere in `src/`, confirmed by grep). It stopped being true the moment `STEP16` added
a real `Params::SymmetryAxis::Radial` bit to PARAMS and nobody revisited this comment. Update it to
describe five bits, not four, and drop the now-wrong "collapse" claim.

## Solution
1. `src/params/Symmetry_PARAMS.h` — no change; `Radial` is already correctly defined.
2. `src/ui/PlacementRuleSections_UI.h`:
   - `kPlacementSymmetryAxisCount`: `4` → `5`.
   - `placementSymmetryAxisLabels`: append `"Radial"`.
   - `PlacementSymmetryAxisBit`: add `case 4: return Params::SymmetryAxis::Radial;`.
   - Rewrite the stale comment block (lines 23-27) per the "second, smaller defect" note above —
     five bits, no "collapse" framing, and note that `Radial`'s companion
     `radialSymmetryRepeatCount` is a separate, not-yet-UI-exposed field (cross-reference this
     ticket's out-of-scope entry so a future reader isn't left to rediscover the gap).
3. `src/ui/PlacementRuleSections_UI.cpp` — no logic change required; `DrawIndependentSymmetryAxes`'s
   loop and `ResolvedPlacementSymmetryMask`'s AND already generalize over the count. Update the
   "Four independent tick boxes" wording in the header comment block
   (`PlacementRuleSections_UI.h:112-118`, "four independent tick boxes") to "five."
4. `src/ui/PlacementRuleSections_UI_Test.cpp` — extend `RunSymmetryRepairChecks`
   (`PlacementRuleSections_UI_Test.cpp:48-61`) with an explicit case: build `legalBits` over the new
   count (already generic, no test code change needed there) — but add a **named regression
   assertion** that `ResolvedPlacementSymmetryMask(Params::SymmetryAxis::Radial) ==
   Params::SymmetryAxis::Radial` and that `Radial` composed with another axis (e.g.
   `Radial | MirrorAcrossX`) survives the repair unchanged. This is the test that would have caught
   the original omission and must exist so it cannot silently regress again.
5. `src/ui/SymmetryTab_UI_Test.cpp` — the "stray bit" comments at lines 47/93-98 ("a bit no v2 axis
   owns", "a stray bit outside the four real axes") stay correct in behavior (they use `1 << 20` as
   the probe, not `Radial`) but the prose says "four" — update to "five real axes" so the comment
   doesn't misdescribe the fixed table. No assertion changes required; the existing checks still
   pass unchanged.

No PROC, IO, or PARAMS file needs to change. This is a pure UI-layer table extension.

## Target files
- `src/ui/PlacementRuleSections_UI.h` (currently 129 lines — already over the
  ARCH_01_05_FileSizeCeilings.md §1.5 soft-100
  ceiling, pre-existing and not this ticket's to fix; the added label/case/comment edits are a few
  lines and must not push it past the hard-150 ceiling. If they would, trim the rewritten comment
  rather than requesting an exception — the content fits in the existing budget.)
- `src/ui/PlacementRuleSections_UI.cpp` (102 lines; comment-only change, no growth of consequence)
- `src/ui/PlacementRuleSections_UI_Test.cpp`
- `src/ui/SymmetryTab_UI_Test.cpp` (comment-only)

## Layer & accuracy class
UI. Accuracy class: **Visual** (a checkbox and a mask-repair function; no numeric tolerance
applies). No PROC/PIPELINE change, so no accuracy-class interaction with the Exact/Accurate/
Deterministic chain — `BuildSymmetryOrbit`'s own Radial handling is unaffected and already correct.

## Backend policy
CPU only; imgui immediate-mode UI code. Not applicable to GPU dispatch.

## ARCH rules invoked
- Constitution §6 — "the bits a mask may legally carry... repaired rather than obeyed." This ticket
  makes the repair table match the actual legal bit set instead of silently narrowing it.
- Constitution §7 — work-order schema (this document).
- Constitution §8 — total tweakability. Directly relevant to why the missing
  `radialSymmetryRepeatCount` slider (out-of-scope here) is a real, tracked gap and not a dismissal.
- `ARCH_01_05_FileSizeCeilings.md` §1.5 — soft 100 / hard 150 lines, functions ≤ 40 lines; governs
  the "Target files" note above.

## Explicit out-of-scope
- **A `radialSymmetryRepeatCount` UI control (slider or otherwise).** Zero call sites exist under
  `src/ui/` today for any domain (confirmed by grep, same finding `STEP60`/`STEP80`/`STEP94`/
  `CONSOLIDATION_MASTER.md` finding N3 already made). This ticket's fix does not require one to be
  meaningful (see "Why this is a table gap" above) and does not add one. It remains an open,
  separately-tracked cross-tab follow-up — this ticket does not close N3, and no coder should treat
  landing this ticket as having done so.
- **Any change to `Placement_Symmetry_PROC.h`/`Placement_SymmetryOrbit_PROC.h`, IO read/write sites,
  or any PARAMS struct.** All already correctly support `Radial`; nothing there is broken.
- **Adding a "Use Global" default for Radial or changing `globalSymmetryMask`'s default.** Out of
  scope; unrelated to the repair-table bug.
- **`SymmetryBlend`/`SymAlgorithm`** (`Symmetry_PARAMS.h:55-70`) — unrelated exotic-blend scalars,
  explicitly out of scope per `STEP16`'s own ruling; this ticket does not touch them.

## Acceptance test
1. `PlacementRuleSections_UI_Test.cpp`'s new regression assertion
   (`ResolvedPlacementSymmetryMask(Radial) == Radial`, and `Radial | MirrorAcrossX` surviving intact)
   passes.
2. Every existing assertion in `PlacementRuleSections_UI_Test.cpp` and `SymmetryTab_UI_Test.cpp`
   continues to pass unchanged (the count/table generalization must not alter behavior for the
   original four bits).
3. Grep confirms `kPlacementSymmetryAxisCount == 5` and a `case 4:` returning
   `Params::SymmetryAxis::Radial` exist in `PlacementRuleSections_UI.h`.
4. Manual trace (not a build check, a review check): every caller table row above still compiles and
   draws a fifth "Radial" checkbox with no other behavior change to its four existing boxes.
5. Full `SanGenV2` build stays clean; every existing test continues to pass.

## What this closes — flags to clear once this lands
This is a single shared function with five real call surfaces (table above); landing this ticket
fixes Markers, Units, Props, Decals, and the global Symmetry tab **simultaneously**. No per-domain
follow-up ticket is needed.

- **`work_orders/STEP81_MarkersTabManualLayers_UI.md`** — clear the "⚠️ Pre-existing shared-widget
  defect" section (lines 363-388) and its two out-of-scope/landing-order cross-references (lines
  529-537, 555-558). This is the ticket that carries the full, precise disclosure of the bug this
  ticket fixes.
- **`work_orders/STEP94_MarkerDragAndFollowSymmetry_UI.md`** — clear the "Known pre-existing defect
  this ticket must route around, not inherit" section (lines 78-92) and its out-of-scope entry (lines
  384-386). Note: STEP94's own text states its live-recompute path never calls
  `ResolvedPlacementSymmetryMask` at all (it reads `symmetryMask` fields directly), so STEP94 was
  never actually *exposed* to this bug — only routing around a defect it correctly diagnosed as
  reachable elsewhere. Clearing its flag documents that the defect it routed around no longer exists,
  not that STEP94 itself needed the fix.
- **`work_orders/CONSOLIDATION_MASTER.md`** — finding **N1** (the Radial-axis bug row in the "New
  defects found while authoring" table, ~line 248) and its cross-reference at lines 367-371 should be
  marked resolved.
- **`work_orders/STEP80_MarkersTabRulesLayerSymmetry_UI.md`** — **does not itself contain a
  "known bug, not fixed here" flag for this defect.** Its own text (lines 172-175) only flags the
  separate, still-open `radialSymmetryRepeatCount`-has-no-UI-control gap, which this ticket does not
  close. However, `STEP81` asserts on STEP80's behalf that it is "Confirmed co-affected by the
  Radial-bit defect" and "blocked" (`STEP81:532-536`, since STEP80 moves the same
  `DrawPlacementSymmetryAxes` call to its own layer tier). That cross-reference resolves once this
  ticket lands, even though there is no text inside STEP80 itself to edit.

## Dispatch recommendation (human/scheduling call — flagging, not deciding)
This fix is small, fully self-contained, touches only already-built and already-tested files, and
does not depend on any type or file that STEP80/STEP81 introduce — it can be dispatched and land
standalone at any time, independent of STEP80/STEP81's own landing order. Landing it standalone also
clears the STEP81/STEP94/CONSOLIDATION_MASTER flags immediately rather than leaving them open until a
much larger ticket (STEP80 or STEP81, both multi-hundred-line marker-tab rebuilds) eventually lands.

Against that: `STEP81:386-388` explicitly anticipated one shared fix landing once, saying "neither
should land a private version" — which could be read as an intent to land this alongside whichever of
STEP80/STEP81 ships first, so the same diff/review pass covers both. Both are reasonable; this is a
scheduling preference, not a technical dependency, so the human should decide whether to dispatch
this standalone now or bundle it with STEP80/STEP81's landing.
