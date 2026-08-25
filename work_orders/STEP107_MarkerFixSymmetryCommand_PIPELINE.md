# STEP107 — "Fix Symmetry" backfill command for manual/imported marker layers

**Layer:** PIPELINE (new read-only detection query) + UI (the command body that performs the PARAMS
write, the checkbox, and the button). **Domain:** `MarkerTransform::symmetryGroupIdentifier`
backfill for one marker layer's own candidate pool, `src/ui/MarkersTab_ManualLayers_UI.cpp`'s
`DrawLayerRowBody`. **Executor:** SanGen Coder. Authored by the SanGen Generator Expert (detection
algorithm), SanGen Compute Optimization Expert (performance/determinism), SanGen Format Expert
(never-auto-on-import ruling), and this session's product decision (overwrite toggle), transcribed
into ticket form; **re-verified line-by-line against the real code a second time** after two
structural changes landed on top of the original authoring pass — see the banner immediately below.

---

## ⚠️ RE-TARGETED AFTER TWO LANDED STRUCTURAL CHANGES — READ BEFORE IMPLEMENTING

This ticket was originally drafted against a version of `MarkersTab_ManualLayers_UI.cpp` that no
longer exists on disk. Two changes have landed since:

1. **STEP106** (`work_orders/STEP106_MarkerLayerLockAndGridSnap_PARAMS.md`) — **still NOT landed**
   as of this rewrite (confirmed by re-reading the current source: no `bLocked`/`bGridSnapEnabled`/
   `gridSnapSizeWorldUnits` fields on `Params::MarkerInstanceLayer`, no `IsMarkerInstanceLayerLocked`/
   `QuantizeMarkerPositionToLayerGrid` in `MarkersTab_ManualLayers_UI.h`). The original "strictly
   after STEP106" landing-order rule (below) still applies verbatim: STEP106 also edits the same
   function this ticket inserts into, and that function's exact shape will change again once STEP106
   lands. Re-verify this ticket's insertion point against the post-STEP106 file before implementing,
   the same way this rewrite re-verified it against the current (pre-STEP106) file.
2. **A row-based UI restructure (STEP110, commit `04750e4`) — ALREADY LANDED**, independent of
   STEP106. This is the change this rewrite exists to account for. It moved per-item settings out of
   a bottom-of-tab "selected item" detail panel into each list row's own inline expanded body — the
   same shape now used throughout the UI (`MarkersTab_Manual_UI.cpp`'s group list,
   `MarkersTab_ManualInstance_UI.cpp`'s instance list, and this ticket's own target,
   `MarkersTab_ManualLayers_UI.cpp`'s layer list, all converted the same way). Concretely, for this
   ticket:
   - **`DrawSelectedLayer` no longer exists.** Its content (Name, Color, Icon Scale, the "Layer
     Symmetry" section) now lives in a differently-shaped, differently-named function,
     `DrawLayerRowBody(Params::MarkerInstanceLayer& layer, ManualMarkerLayersState& state)`
     (`MarkersTab_ManualLayers_UI.cpp:36-52`, anonymous namespace).
   - `DrawLayerRowBody` takes the layer **by reference to the row's own element**
     (`Params::MarkerInstanceLayer& layer`), not a `layer*` pointer resolved from
     `state.selectedLayerIndex` via `SelectedManualMarkerLayer`. There is no more null-check branch
     to sit inside — the caller only invokes this function for a row that already exists.
   - It is called from `DrawLayerList`'s row-body lambda (`MarkersTab_ManualLayers_UI.cpp:69-72`)
     **once per row whose own `ImGui::CollapsingHeader` is expanded** — imgui's own per-row
     open/closed state (`DraggableListWidget_UI.h`'s `ImGuiTreeNodeFlags_DefaultOpen`), which is
     **every row, by default**, simultaneously, not gated on `state.selectedLayerIndex` at all.
     `state.selectedLayerIndex` still exists (`ManualMarkerLayersState`, header line 52) and is still
     read/written — it now only drives the `_Selected` header highlight and the click-to-select
     signal, nothing about which row's body draws.
   - **Consequence for this ticket:** "the selected layer" is no longer the right mental model for
     scope. §1 below already scoped this ticket to "one `Params::MarkerInstanceLayer`, its own
     candidate pool" — under the row-based shape that layer is simply **the row `DrawLayerRowBody`
     was called for**, identified by that row's own index into `markerLayers` (the `rowIndex` the
     lambda already receives). This is, if anything, a cleaner fit for the per-layer scope than the
     old "whichever layer happens to be selected" indirection was — no feature-spec change, just a
     different (and simpler) way to name "the target layer."

**Every line number, function name, and code sample below is re-verified against the file as it
exists right now** (pre-STEP106, post-STEP110). Do not trust STEP106's own citations of this file —
that ticket has not been re-verified against STEP110's landing and still assumes the old
`DrawSelectedLayer` shape; STEP106's own coder will need to re-locate its insertion point the same
way this rewrite did, but that is STEP106's problem, not this ticket's.

## Original landing-order rule (unchanged)

`STEP106` also edits the function this ticket inserts into (today `DrawLayerRowBody`, formerly
`DrawSelectedLayer`) — **this ticket must not be dispatched until STEP106 has actually landed in the
working tree.** Two tickets editing the same function concurrently is exactly the colliding-edit
scenario `STEP79`'s single-dispatch-unit banner and `STEP94`'s STEP49-coordination note both exist to
avoid; the fix here is a strict landing order, not a joint dispatch unit. **Re-verify this ticket's
insertion point and function/parameter shapes against the tree again at that time** — a third
structural change between now and then is not ruled out, and the coder must confirm reality before
editing, not trust any cached description (including this one).

## The problem this backfills

`MarkerTransform::symmetryGroupIdentifier` (`src/params/MarkerInstance_PARAMS.h:44-47`, sentinel
`0` = ungrouped) is only ever **forward-propagated** by `STEP94`'s drag-and-follow machinery
(`src/ui/MarkerDragGesture_UI.h/.cpp`) — it is written when an already-tagged marker is dragged, and
by `MaterializeMarkerDragGesture`-family code when a drag grows the orbit. Nothing ever *originates*
the tag. A marker imported from a `.sanmap` (which never wrote this SanGen-added field) or hand-placed
one at a time by a designer stays permanently ungrouped, with no drag-and-follow behavior, even when
its position is a perfect mirror of another marker in the same layer. This ticket adds a
human-triggered "Fix Symmetry" command that detects those already-mirrored positions and backfills
the id, so STEP94's drag-and-follow becomes available on them retroactively.

**Never auto-runs on import** (SanGen Format Expert's ruling this session): `0` is a legitimate,
permanent "ungrouped" value under `ARCH_16_05_MarkerTransformFields.md` §16.5, not a to-be-filled-in
placeholder — running detection silently on every import would rewrite `symmetryGroupIdentifier` on
every re-import of the same file and violate round-trip fidelity (a file exported, then re-imported
unchanged, must produce the same PARAMS). This command is reachable ONLY via the button this ticket
adds; no import/load path calls it.

## Required reading

- `src/params/MarkerInstance_PARAMS.h:39-48` — `MarkerTransform`, the field this backfills, and its
  existing "0 = ungrouped, future drag-and-follow UI writes into it" comment (STEP94 is that UI; this
  ticket is the backfill STEP94's own comment anticipated but did not build). Field itself at `:47`.
- `src/proc/Placement_Symmetry_PROC.h` — `Proc::BuildSymmetryOrbit`, the ONE mirror-math
  implementation. This ticket's detection reuses it (via the PIPELINE wrapper below), never
  re-derives mirror geometry.
- `src/pipeline/SymmetryOrbitQuery_PIPELINE.h` — `Pipeline::BuildWorldSymmetryOrbit`
  (`:34-37`), the existing stateless world-space wrapper around `BuildSymmetryOrbit` this ticket's
  new PIPELINE function calls once per candidate seed. Also its `WorldSymmetryOrbitPoint` struct
  (`:25-28`), reused verbatim as this ticket's orbit-point type.
- `src/ui/MarkerOrbitCorrespondence_UI.h` + `.cpp` — `MatchCorrespondenceToOrbit`, the existing
  "rank every (entry, slot) pair by distance once, claim smallest-first" greedy matcher STEP94
  already ships. This ticket's own matching step (§3 below) is the SAME algorithmic shape, scaled
  from "one gesture's siblings against one seed's orbit" to "every candidate in a layer against
  every seed's orbit, processed seed-by-seed in a canonical order" — read this file's own header
  comment (`:8-22`) for why global-greedy beats per-entry-independent nearest search; that reasoning
  applies identically here.
- `src/ui/MarkerDragGesture_UI.h:62-77` — `ResolveEffectiveMarkerSymmetry`, the existing
  `bSymmetryUseGlobal` ternary this ticket calls to resolve the target layer's effective mask/count.
  **Do not re-derive this ternary a third time** (STEP94's own header already flags
  `ResolvedPlacementSymmetryMask`/`DrawPlacementSymmetryAxes` as the wrong source — see below).
  Confirmed still present, same signature, same lines, unaffected by the STEP110 restructure (that
  restructure only touched the two Markers-tab list/detail files named in the banner above, not this
  drag-gesture file).
- `src/data/SpatialGrid_DATA.h` + `src/ui/Picking_UI.cpp:29-63` (`PickMarker`) — the bucket-lookup
  pattern (`Configure`/`Build`/`CellIndexAt`/`BucketBegin`/`BucketEnd`/`InstanceIndexAt`) this
  ticket's detection reuses for O(candidate count) matching instead of an O(n²) all-pairs scan.
  `PickMarker` is precedent for the exact "one bucket lookup per query point" posture this ticket
  also uses (see §3's boundary-case note). Confirmed still at these exact lines.
- `src/pipeline/GenerationAssembler_Stages_PIPELINE.cpp:32-39` (`BuildMarkerSpatialGrid`) — precedent
  that PIPELINE code may construct and use a `Data::SpatialGrid` directly (`markerSpatialGrid
  .Configure(mapSize * WorldUnitsPerCell(), resolution)`); this ticket's new PIPELINE function does
  the same with a fresh, LOCAL grid instance, not the persistent single-writer one that function owns.
  Confirmed still at these exact lines.
- `ARCH_16_05_MarkerTransformFields.md` §16.5 — `symmetryGroupIdentifier`'s sentinel/grouping meaning.
- `ARCH_16_03_ModuleBoundaryChain.md` §16.3 — the `UI -> PIPELINE -> PROC` chain and the "stateless
  query lives in PIPELINE with no DAG participation" ruling this ticket's new function follows,
  identical posture to `SymmetryOrbitQuery_PIPELINE.h`.
- `ARCH_16_08_SpawnArmyShrink.md` §16.8 — read for context only; this ticket does not change
  cardinality (it never creates or deletes a `MarkerTransform`, only writes an existing one's id), so
  the orphan/Spawn-refusal ruling it governs does not apply here. Stated explicitly so nobody assumes
  this ticket needs a Spawn carve-out — it does not.

## Known pre-existing defect this ticket must route around, not inherit

Same defect STEP94 already flagged and routed around: `ResolvedPlacementSymmetryMask`
(`src/ui/PlacementRuleSections_UI.h:58-63`) never performs the layer-vs-global
`bSymmetryUseGlobal` resolution at all — it resolves a mask straight from a rule's own
`symmetryMask`/`DrawPlacementSymmetryAxes` output with no awareness of the
use-global-or-override-per-layer ternary this ticket's target layer needs evaluated. (An earlier
draft of this ticket additionally cited a stale claim that `ResolvedPlacementSymmetryMask`'s axis
table silently drops `Params::SymmetryAxis::Radial` — not true of the current code:
`src/ui/PlacementRuleSections_UI.h:29` defines `kPlacementSymmetryAxisCount = 5` with Radial
present as case 4. That stale citation is retracted; the real, still-valid reason to avoid this
path is the missing `bSymmetryUseGlobal` resolution above.) **This ticket's detection never calls
it or `DrawPlacementSymmetryAxes`** — mask/count resolution goes exclusively through
`ResolveEffectiveMarkerSymmetry` (`MarkerDragGesture_UI.h:66-77`), the same already-valid ternary
STEP94 uses, which DOES perform that resolution. Do not add a second resolution path.

---

## 1. Scope: per-layer only

Operates on **one `Params::MarkerInstanceLayer`** — under the current row-based UI (see the banner
above), that is whichever row's own `DrawLayerRowBody` call the "Fix Symmetry" button was pressed in,
identified by that row's own index into `recipe.markerLayers` (the `rowIndex` `DrawLayerList`'s
row-body lambda already receives; call it `layerIndex` at the `DrawLayerRowBody` level — see §2's
signature). The candidate pool is every `Params::MarkerTransform` across every
`recipe.markers[*].transforms` whose `layerIndex == <that row's index>` — `layerIndex` is a plain
vector position into `recipe.markerLayers` (`MarkerInstance_PARAMS.h:43`), NOT scoped by
`MarkerInstanceGroup` (a marker "type" like "Spawn"/"Alloys"), so the pool spans every group. **Not a
global cross-layer sweep** — a marker on a different layer is never a match candidate, even if its
position happens to mirror one in the target layer. **Not a global "run for every layer" sweep
either** — even though every row typically draws expanded at once under the row-based UI, each row's
own "Fix Symmetry" button only ever acts on that one row's own layer; pressing one row's button never
touches another row's markers, expanded or not.

## 2. Overwrite/skip toggle (product decision this session)

A checkbox labeled **"Overwrite manually-adjusted positions"**, drawn directly ABOVE the "Fix
Symmetry" button, both inside the existing `DrawSectionBegin("Layer Symmetry", state.symmetrySection)`
block of `DrawLayerRowBody`, directly after the existing `DrawPlacementSymmetryAxes` call (today:
`MarkersTab_ManualLayers_UI.cpp:46-50`; see the STEP106 landing-order note above for why these line
numbers are not to be trusted at dispatch time — re-locate by searching for the literal
`DrawSectionBegin("Layer Symmetry"` string):

```cpp
if (DrawSectionBegin("Layer Symmetry", state.symmetrySection)) {
    DrawPlacementSymmetryAxes("markerLayerSymmetry", layer.symmetry.bSymmetryUseGlobal,
                              layer.symmetry.symmetryMask, nullptr);
    int effectiveMask = 0;
    int effectiveRadialRepeatCount = 0;
    ResolveEffectiveMarkerSymmetry(markerLayers, layerIndex, globalSymmetryMask,
                                   globalRadialRepeatCount, effectiveMask, effectiveRadialRepeatCount);
    DrawSliderScalar("Fix Symmetry Distance Tolerance", markerSymmetryFixSettings.distanceTolerance,
                     state.fixSymmetryToleranceRange, state.fixSymmetryToleranceToggle, WidgetStyle(),
                     "%.2f");
    DrawCheckbox("Overwrite manually-adjusted positions", state.bFixSymmetryOverwrite);
    if (ImGui::Button("Fix Symmetry")) {
        state.lastFixSymmetryResult = FixMarkerLayerSymmetry(markers, geometry, layerIndex,
            effectiveMask, effectiveRadialRepeatCount, markerSymmetryFixSettings.distanceTolerance,
            state.bFixSymmetryOverwrite);
        state.bHasFixSymmetryResult = true;
        state.bFixSymmetryOverwrite = false;   // consumed per-use, see below — NOT sticky
    }
    if (state.bHasFixSymmetryResult) {
        ImGui::Text("Fix Symmetry: %d group(s) created, %d slot(s) unmatched",
                    state.lastFixSymmetryResult.confirmedGroupCount,
                    state.lastFixSymmetryResult.unmatchedSlotCount);
    }
    DrawSectionEnd();
}
```

Note `layer.symmetry...` (dot, not arrow) — `DrawLayerRowBody` receives the row's own layer **by
reference to the element itself**, not a pointer resolved from a selection index (see the banner).
`ResolveEffectiveMarkerSymmetry` is called unqualified, not `Ui::`-qualified, because
`MarkersTab_ManualLayers_UI.cpp` is itself inside `namespace SanmapGen { namespace Ui { ... } }` —
same unqualified-call convention every other function in this file already uses.

`markerSymmetryFixSettings` (mutable reference, `Params::MarkerSymmetryFixSettings&`) is a new
parameter threaded down to `DrawLayerRowBody` the same way `geometry`/`globalSymmetryMask`/
`globalRadialRepeatCount` already are per this ticket's plan (see §5 and "Files touched" below) —
sourced from `recipe.markerSymmetryFixSettings` at the `MarkersTab_UI.cpp` call site, since the
tolerance is a recipe-level setting, not per-layer. The slider itself follows the exact
`DrawSliderScalar(label, value, state.<sharedRange>, state.<sharedToggle>, WidgetStyle(), format)`
shape STEP106 uses for its own `Grid Size` slider in this same function
(`STEP106_MarkerLayerLockAndGridSnap_PARAMS.md` §7) — one shared `ScalarSliderRange` +
`RealtimeToggle` pair on `ManualMarkerLayersState` for the whole block, not per-row, matching every
other shared-control field this state struct already carries (`iconScaleRange`/
`selectedLayerIconScaleToggle`, and per STEP106, `gridSnapSizeRange`/
`selectedLayerGridSnapToggle`).

(`DrawCheckbox` is the existing shared widget already used elsewhere in this file, e.g.
`DrawLayerSettings`'s `"Use Group Color"` checkbox at `:21` — reuse it, do not call raw
`ImGui::Checkbox`; `Checkbox_UI.h` is already `#include`d by this file (line 6), no new include
needed for the checkbox itself. The exact wording/placement of the result text is the one
coder-level polish call in this ticket, same posture STEP94 leaves for its own ghost-tint styling —
the count semantics themselves, defined in §5 below, are not negotiable.)

- **Default: OFF (skip mode)** — only markers with `symmetryGroupIdentifier == 0` in the candidate
  pool participate; already-grouped markers (STEP94-authored or a prior "Fix Symmetry" run) are never
  read, matched, or written.
- **ON (overwrite mode):** first zero `symmetryGroupIdentifier` on every in-scope (in-layer) marker
  — including ones that already carried a nonzero id — then run the identical matching algorithm
  (§3-§5) over the now-fully-ungrouped pool. This does not change the matching algorithm itself, only
  widens the candidate pool and its starting id state (Generator Expert confirmed this session).
- **No confirmation modal.** The checkbox itself is the explicit opt-in, same low-friction posture
  `DESIGN_MarkerLayerSymmetry_R2.md`'s orphan ruling already established for this codebase (cited
  verbatim in STEP94's own Spawn-refusal section: "no new confirmation dialog either").
- **Auto-reset after every press**, whether or not any group was actually found/created: the checkbox
  returns to unchecked immediately after `FixMarkerLayerSymmetry` returns, so overwrite mode is
  consumed per-button-press, never silently sticky across an unrelated later press.
- **Known accepted limitation, documented here and not fixed by this ticket:** overwrite mode cannot
  distinguish a marker a designer deliberately left off-symmetry (e.g. an intentionally-asymmetric
  decoration) from one detection simply has not reached yet — it force-groups anything within
  tolerance of a valid partner, full stop. A future `symmetryLocked`-style per-marker opt-out bit is
  a plausible follow-on; it is explicitly OUT OF SCOPE here (see "Explicit out-of-scope").
- **New known accepted limitation, a direct consequence of the row-based UI (not present when this
  ticket was first drafted against the old "selected layer" model):** `bFixSymmetryOverwrite`,
  `bHasFixSymmetryResult`, and `lastFixSymmetryResult` all live on `ManualMarkerLayersState` — ONE
  shared instance for the whole block, not one per row — because `Params::MarkerInstanceLayer` is a
  pure round-tripping type and cannot carry UI-only scratch state, the same reason
  `ManualMarkerLayersState::selectedLayerColorToggle`/`selectedLayerIconScaleToggle` are already a
  single shared pair reused across every row's own color/scale controls
  (`MarkersTab_ManualLayers_UI.h:45-49`'s own comment on this exact constraint). Since every row's
  `DrawLayerRowBody` typically draws expanded at once (imgui's per-row `DefaultOpen`, not gated on
  selection — see the banner), the checkbox's checked state and the most recent result line are
  visually shared across every expanded row's "Layer Symmetry" section until the next press. Pressing
  Fix Symmetry on a given row still only writes THAT row's own markers (`layerIndex` is captured
  correctly per-row at the button's own call site — see §2's code sample and the button lambda's own
  `layerIndex` parameter), so this is a display-only quirk, not a data-correctness bug — same category
  of accepted tradeoff the file already ships for the toggle fields, not a new kind of defect
  introduced here. Not fixed by this ticket; a future per-row-keyed UI-state container is a plausible
  follow-on if the human wants row isolation, out of scope here.

## 3. Detection algorithm (Generator Expert's design — implement exactly this shape)

Two new pieces, split across layers per the module-boundary ruling in §4:

- A **read-only PIPELINE query** (new) that, given a candidate pool's positions plus a mask/count/
  tolerance, returns which candidates form fully-confirmed symmetry orbits. Writes nothing.
- A **UI-layer command function** (new) that builds the candidate pool from `recipe.markers`, calls
  the query, allocates fresh ids, and performs the only PARAMS writes in this ticket.

### 3a. New PIPELINE file — `src/pipeline/MarkerSymmetryDetection_PIPELINE.h` / `.cpp`

**Naming flagged for one-line ARCH Expert confirmation before dispatch** — same posture STEP68 used
for its own connector filename and STEP79 used for `Placement_MarkerRules_PROC.cpp`/
`Placement_RuleAppend_PROC.h`. The direction (a new stateless PIPELINE file, same category as
`SymmetryOrbitQuery_PIPELINE.h`, ARCH §16.3) is settled; only the exact spelling
(`MarkerSymmetryDetection_PIPELINE`) is a proposal.

```cpp
// MarkerSymmetryDetection_PIPELINE.h — pure, read-only detection of already-mirrored candidate
// positions (STEP107's "Fix Symmetry" command). Layer: PIPELINE, same category as
// SymmetryOrbitQuery_PIPELINE.h (ARCH_16_03_ModuleBoundaryChain.md §16.3): a stateless query with no
// DAG/dirty-hash participation, callable on demand from UI. Wraps
// Pipeline::BuildWorldSymmetryOrbit (itself a wrapper over Proc::BuildSymmetryOrbit) as the SOLE
// source of mirror geometry — this file adds no independent mirror math, only the
// candidate-to-orbit-slot matching STEP94's MatchCorrespondenceToOrbit already established the shape
// of, scaled to run once per candidate seed across a whole pool instead of once per drag gesture.
// Writes nothing to PARAMS — see MarkerSymmetryFixCommand_UI.h for the UI-layer function that
// performs the actual symmetryGroupIdentifier writes this query's results feed.
#pragma once
#include <vector>
#include "../params/Geometry_PARAMS.h"

namespace SanmapGen {
namespace Pipeline {

// One fully-confirmed orbit found in the candidate pool. `seedCandidateIndex` plus one entry in
// `matchedCandidateIndices` per non-seed orbit slot the seed's orbit produced — every index is an
// offset into the CALLER's own candidatePositionX/candidatePositionZ arrays, not a PARAMS index of
// any kind (this file knows nothing about MarkerTransform).
struct MarkerSymmetryOrbitMatch {
    int              seedCandidateIndex = -1;
    std::vector<int> matchedCandidateIndices;
};

// Finds every FULLY-matched symmetry orbit among `candidateCount` candidate positions (parallel
// arrays `candidatePositionX`/`candidatePositionZ`, world space, already restricted by the caller to
// one layer's own in-scope markers) under `symmetryMask`/`radialSymmetryRepeatCount`.
//
// Processes candidates as seeds in a CANONICAL order independent of array/insertion order: sorted by
// (round(positionX / positionQuantizationStep), round(positionZ / positionQuantizationStep),
// original candidate index) — see positionQuantizationStep below. For each seed not yet consumed by
// an earlier-confirmed orbit:
//   1. Compute its orbit via BuildWorldSymmetryOrbit(geometry, symmetryMask,
//      radialSymmetryRepeatCount, seed.x, seed.z, orbitPoints, symmetryOrbitMaximum). Slot 0 is
//      always the seed itself (Proc::BuildSymmetryOrbit's own seed-first convention) and is never a
//      match target. If the returned count is <= 1 (no mirrors under this mask), skip — nothing to
//      detect for this seed, no unmatched-slot contribution.
//   2. For each slot 1..orbitCount-1: ONE Data::SpatialGrid bucket lookup at that slot's world point
//      (grid built once, up front, over the full unconsumed candidate pool — see below), collecting
//      every unconsumed candidate in that bucket within `distanceTolerance` (squared-distance
//      compare, no sqrt) as a (slotIndex, candidateIndex, distanceSquared) tuple. This is the ONE
//      bucket-lookup-per-query-point pattern Picking_UI::PickMarker already uses — same accepted
//      single-cell-lookup posture, including its edge case (a genuine in-tolerance match that falls
//      just across a cell boundary from the query point can be missed). Choose the grid's chunk
//      resolution so a cell's width is comfortably larger than `distanceTolerance` (a small
//      multiple, e.g. >= 4x) to keep that edge case rare; do not attempt a 3x3-neighbor lookup to
//      close it completely — that is a heavier query this ticket does not require.
//   3. Sort THIS seed's own tuples by distanceSquared ascending, tied-broken by the same canonical
//      (quantized position, index) order as step 1's seed ordering. Claim greedily, smallest first:
//      each slot claimed at most once, each candidate claimed at most once — IDENTICAL claim loop
//      shape to MarkerOrbitCorrespondence_UI.cpp's MatchCorrespondenceToOrbit, scoped to this one
//      seed's own slots/tuples rather than reused as a shared function (that function's type,
//      Ui::MarkerOrbitCorrespondence, is UI-layer and PIPELINE may not depend on UI — ARCH §3.1's
//      dependency direction — so this is a small, independent re-implementation of the SAME shape,
//      not a call into STEP94's function).
//   4. If every slot got claimed: this seed's orbit is CONFIRMED. Record a MarkerSymmetryOrbitMatch
//      {seed, claimed candidates in slot order}. Remove the seed and every claimed candidate from
//      the unconsumed pool (they are never reconsidered as a later seed or a later seed's candidate).
//   5. Otherwise: record nothing, add (orbitCount - 1 - claimedCount) to the running unmatched-slot
//      total, and leave every considered candidate (claimed-this-attempt or not) back in the
//      unconsumed pool for a later seed's consideration. See "Correctness note" below for why this
//      is safe.
//
// Correctness note on WHY confirmed-only consumption avoids double-detection: two markers A, B that
// mirror each other are each other's sole match. Whichever of the two sorts first in the canonical
// seed order is processed first, confirms {A, B}, and consumes BOTH — so the second one is never
// later reprocessed as its own seed (it was already removed from the pool in step 4). This is what
// makes seed order matter for WHICH candidate is nominally "the seed" of a confirmed pair/set (an
// arbitrary, deterministic choice with no observable effect — the written id lands on every member
// of the set identically, see MarkerSymmetryFixCommand_UI.h) while still guaranteeing every physical
// orbit is detected and reported exactly once, never twice.
//
// `outUnmatchedSlotCount`, if non-null, receives the total from step 5 across every attempted
// (orbitCount > 1) but not-fully-confirmed seed — the count this command reports to the user via the
// "Fix Symmetry" button's result line (see MarkersTab_ManualLayers_UI.cpp).
std::vector<MarkerSymmetryOrbitMatch> FindMarkerSymmetryMatches(
    const Params::Geometry& geometry, int symmetryMask, int radialSymmetryRepeatCount,
    const float* candidatePositionX, const float* candidatePositionZ, int candidateCount,
    float distanceTolerance, int* outUnmatchedSlotCount);

// The quantization step (world units) used to build the canonical, insertion-order-independent
// candidate/tie-break ordering above. Deliberately much finer than any realistic position
// difference (so it never conflates two genuinely-distinct positions), and independent of
// `distanceTolerance` (a bucketing/ordering constant, not a match-acceptance one).
constexpr float positionQuantizationStep = 0.01f;

} // namespace Pipeline
} // namespace SanmapGen
```

`.cpp` implementation notes for the coder (not independently negotiable, but not spelled out as
literal code above so this section stays under the file-size ceiling — see §6):
- Build ONE local `Data::SpatialGrid` (`#include "../data/SpatialGrid_DATA.h"`), configured the same
  way `GenerationAssembler_Stages_PIPELINE.cpp:34-35` configures the persistent one —
  `grid.Configure(geometry.mapSize * geometry.worldUnitsPerCell, someChunkResolution)` — over the
  FULL initial candidate pool (`grid.Build(candidatePositionX, candidatePositionZ, candidateCount)`),
  once, before the seed loop. `someChunkResolution` is a coder-level tuning constant satisfying the
  "cell width >= 4x distanceTolerance" guidance above given a realistic map size; `Data::SpatialGrid
  ::defaultChunkResolution` (32) is a reasonable starting point, adjusted only if that guidance would
  be violated at typical map sizes.
- The grid is built ONCE over the ORIGINAL full pool and is never rebuilt as candidates are consumed
  — a claimed/consumed candidate's grid entry simply gets skipped by an "already consumed" check at
  match time (a `std::vector<bool> consumed(candidateCount, false)` alongside the grid, checked
  before accepting any tuple in step 2/3). Rebuilding the grid per confirmed set would be correct but
  wasteful; skipping consumed entries at query time is the same O(candidateCount) total-work shape
  `PickMarker` already establishes and needs no extra data structure.

### 3b. New UI file — `src/ui/MarkerSymmetryFixCommand_UI.h` / `.cpp`

Pure, imgui-free (same posture as `MarkerDragGesture_UI.h` — testable with no window). This is where
the ARCH's "only UI may mutate PARAMS" rule places the actual `symmetryGroupIdentifier` writes.

```cpp
// MarkerSymmetryFixCommand_UI.h — the "Fix Symmetry" button's command body (STEP107). Layer: UI.
// Pure, imgui-free — same testable-with-no-window posture as MarkerDragGesture_UI.h. Performs the
// ONLY PARAMS writes in this ticket: Pipeline::FindMarkerSymmetryMatches (read-only) supplies which
// candidates form confirmed orbits; this function allocates fresh symmetryGroupIdentifier values and
// writes them, per ARCH's "UI sets PARAMS, PIPELINE/PROC never do" rule (Constitution §1).
#pragma once
#include <vector>
#include "../params/Geometry_PARAMS.h"
#include "../params/MarkerInstance_PARAMS.h"

namespace SanmapGen {
namespace Ui {

struct MarkerSymmetryFixResult {
    int confirmedGroupCount = 0;   // fresh symmetryGroupIdentifier values allocated this run
    int unmatchedSlotCount  = 0;   // Pipeline::FindMarkerSymmetryMatches's own unmatched-slot total
};

// `markers` is `recipe.markers` (mutated). `layerIndex` is the target layer — the row this command
// was invoked from (see §1/§2: no longer `state.selectedLayerIndex` under the row-based UI, but the
// parameter's own meaning — "which position in recipe.markerLayers" — is unchanged).
// `effectiveSymmetryMask`/`effectiveRadialRepeatCount` are the layer's already-resolved values
// (ResolveEffectiveMarkerSymmetry, MarkerDragGesture_UI.h:66-77 — call it at the button call site, do
// not re-derive the bSymmetryUseGlobal ternary here). `distanceTolerance` is the caller's current
// `Params::MarkerSymmetryFixSettings::distanceTolerance` value (world units, §5 — a real,
// designer-editable PARAMS field, read by value here since this function never mutates it).
// `bOverwrite` selects skip vs overwrite mode per STEP107 section 2.
MarkerSymmetryFixResult FixMarkerLayerSymmetry(std::vector<Params::MarkerInstanceGroup>& markers,
                                               const Params::Geometry& geometry, int layerIndex,
                                               int effectiveSymmetryMask,
                                               int effectiveRadialRepeatCount,
                                               float distanceTolerance, bool bOverwrite);

} // namespace Ui
} // namespace SanmapGen
```

`.cpp` shape:
1. Walk every `(groupIndex, transformIndex)` pair across `markers` whose `transform.layerIndex ==
   layerIndex`. If `bOverwrite`, zero `symmetryGroupIdentifier` on every one of them as this pass
   walks them (do this in the same walk that builds the candidate pool, not a separate pass). Track,
   in this same walk, `existingMaximumId` = the highest `symmetryGroupIdentifier` value seen among
   this layer's markers AFTER the overwrite-mode zeroing (so in overwrite mode it is always 0 going
   in; in skip mode it reflects whatever ids STEP94/a prior Fix Symmetry run already assigned).
   **Note the sentinel divergence from `NextMarkerLayerId` (`MarkerLayerId_UI.h`): that helper's
   sentinel is `-1` and its default id is `0`; here the sentinel is `0` and the first fresh id must
   be `1`, so seed `existingMaximumId` at `0`, not `-1`, and only fold in values that are `> 0`.**
2. Build parallel `candidatePositionX`/`candidatePositionZ`/`candidateOriginalIndex` (the last one an
   index back into step 1's `(groupIndex, transformIndex)` list) arrays. **Skip mode only** includes
   entries with `symmetryGroupIdentifier == 0`; **overwrite mode** includes every in-layer entry
   (all now zero from step 1).
3. Call `Pipeline::FindMarkerSymmetryMatches(geometry, effectiveSymmetryMask,
   effectiveRadialRepeatCount, candidatePositionX.data(), candidatePositionZ.data(),
   candidateCount, distanceTolerance, &result.unmatchedSlotCount)` — `distanceTolerance` is this
   function's own parameter (see the signature above), sourced ultimately from the caller's
   `Params::MarkerSymmetryFixSettings::distanceTolerance` field (§5) — never a bare constant.
4. For each returned `MarkerSymmetryOrbitMatch`: `nextId = ++existingMaximumId` (pre-increment, so
   the first confirmed set in a fully-fresh layer gets `1`, matching `symmetryGroupIdentifier`'s own
   `0`-is-ungrouped convention); write `nextId` into the seed's and every matched candidate's real
   `MarkerTransform::symmetryGroupIdentifier` (translate each `MarkerSymmetryOrbitMatch` index back
   through `candidateOriginalIndex` to the real `(groupIndex, transformIndex)`, then into
   `markers[groupIndex].transforms[transformIndex]`). `++result.confirmedGroupCount`.
5. Return `result`.

## 4. Module boundary (ARCH ruling this session)

Detection is a pure, READ-ONLY function of PARAMS positions + the tolerance value (itself a PARAMS
field, §5) — it belongs as
a new stateless PIPELINE query passthrough to PROC's orbit math (`Pipeline::
MarkerSymmetryDetection_PIPELINE`, same category as `SymmetryOrbitQuery_PIPELINE.h`'s existing
pattern, ARCH §16.3), never a second independent mirror-math implementation. The actual PARAMS write
(`symmetryGroupIdentifier` backfill) happens in UI code (`Ui::FixMarkerLayerSymmetry`), since only UI
is permitted to mutate PARAMS (Constitution §1) — identical split to STEP94's own
`Pipeline::BuildWorldSymmetryOrbit` (read-only) vs. `Ui::MarkerDragGesture_UI`'s writes.

## 5. The tolerance value's home — ARCH ruling (dispatch-ready, not an open item)

This needs its own world-space distance tolerance. **Do NOT reuse
`Params::SymmetryDetection::detectionTolerance`** (`src/params/Symmetry_PARAMS.h:59-67`) — that field
is a normalized-height DELTA (`0..1` heightfield-sample space) for recognizing near-symmetric
terrain, a completely different unit space from "how far apart, in world units, can two
`MarkerTransform` positions be and still count as the same mirrored point." The two fields must stay
DISTINCT — never conflated, never one repurposed to feed the other.

**ARCH ruling: a bare `constexpr float` is REJECTED.** Constitution §8 (Tier-1 law) requires
"nothing is hardcoded beyond a designer's reach: any variable can be changed — even constants,"
naming "thresholds" as the canonical example this match-distance tolerance is one of. A
match/reject distance threshold that silently ships non-editable is exactly the class of value §8
exists to forbid, regardless of how reasonable its default is. The ticket's own previously-flagged
"Alternative" is therefore the adopted plan, not a fallback:

- **New PARAMS field**, a sibling struct of `SymmetryDetection` in `src/params/Symmetry_PARAMS.h`
  (immediately after it), kept as a DISTINCT type/field from `SymmetryDetection::detectionTolerance`
  per the unit-space note above:
  ```cpp
  // The world-space distance tolerance STEP107's "Fix Symmetry" backfill command uses to decide
  // whether two MarkerTransform positions are the same mirrored point. A SEPARATE field from
  // SymmetryDetection::detectionTolerance above — that one is a normalized-height (0..1) DELTA for
  // recognizing near-symmetric terrain; this one is a world-space DISTANCE for marker positions.
  // Never conflate the two (Constitution §8 — every threshold stays a designer-reachable field).
  struct MarkerSymmetryFixSettings {
      // World units. A marker placed by hand rarely lands more than a fraction of a unit off a
      // true mirror position; 0.5 is a generous but not indiscriminate default.
      float distanceTolerance = 0.5f;
  };
  ```
  Added to `Params::MapRecipe` (`src/params/MapRecipe_PARAMS.h`) as a new field,
  `MarkerSymmetryFixSettings markerSymmetryFixSettings;`, next to the existing `symmetryDetection`
  field (`:96`) — same aggregate-home posture, same file region.
- **Additive IO round-trip, no `SanGenVersion` bump** — same posture as every other bare-scalar
  PARAMS addition in this codebase (STEP106's three `MarkerInstanceLayer` fields, STEP16's
  `detectionTolerance`). Wire key `"MarkerSymmetryFixDistanceTolerance"`, written/read alongside the
  existing `"SymmetryDetectionTolerance"` key in the SAME `Symmetry` JSON section, since both are
  small recipe-level symmetry-adjacent scalars:
  - `src/io/MapExporter_Symmetry_IO.cpp` (which already writes `json["SymmetryDetectionTolerance"] =
    detection.detectionTolerance;` at `:21`) — add
    `json["MarkerSymmetryFixDistanceTolerance"] = recipe.markerSymmetryFixSettings.distanceTolerance;`
    immediately after it.
  - `src/io/MapImporter_Symmetry_IO.cpp` (which already reads
    `ReadJsonFloat(json, "SymmetryDetectionTolerance", detection.detectionTolerance);` at `:24`) —
    add `ReadJsonFloat(json, "MarkerSymmetryFixDistanceTolerance",
    outRecipe.markerSymmetryFixSettings.distanceTolerance);` immediately after it. Absent key
    (legacy `.sanmap` files saved before this ticket) keeps the struct's own `0.5f` default — no
    clamp needed, same posture as `SymmetryDetection`'s own IO read site.
- **UI slider**, placed next to the "Overwrite manually-adjusted positions" checkbox in
  `DrawLayerRowBody` — same shape/pattern STEP106 already uses for its `gridSnapSizeWorldUnits`
  field in this same UI block (a `DrawSliderScalar` call bound to a shared `ScalarSliderRange` +
  `RealtimeToggle` pair on `ManualMarkerLayersState`, not per-row storage, since this field is
  recipe-level, not per-layer — see §2's updated code sample and "Files touched" below for the
  exact threading).

This changes which files this ticket touches (a new PARAMS field, two IO edits, one new UI state
pair) — reflected in §2's code sample and "Files touched" below. The coder does not need a further
ARCH confirmation before implementing §3; this ruling is final.

## 6. Performance and determinism (Compute Optimization Expert's flags this session)

- **Not O(n²).** The spatial-grid bucket-lookup design in §3a is the whole reason for
  `Data::SpatialGrid`'s presence here: each seed's per-slot query is one bucket lookup (O(1)
  amortized, same as `PickMarker`), so total work is O(candidateCount * average orbit size), not
  O(candidateCount²). A naive nested "for every candidate, for every other candidate, check
  distance" implementation is a rejected design, not a starting point to optimize later.
- **Determinism.** Fixed `distanceTolerance`/`positionQuantizationStep` (no runtime-varying epsilon);
  the canonical (quantized position, then original index) ordering for BOTH the seed-processing
  order and the per-seed tie-break order means results cannot depend on `recipe.markers`' container/
  insertion order or on which physical machine runs the detection — re-running "Fix Symmetry" twice
  in a row on the same PARAMS state (second run in skip mode finds nothing new, since every match is
  already grouped) and detecting on a re-imported-then-re-exported file must both be reproducible.
  Use plain scalar float compares (`a*a + b*b <= tolerance*tolerance`) — no platform SIMD intrinsics,
  matching this codebase's existing "portable reflection math" posture for CPU-path symmetry code.
- **Reject unstable orderings.** Do not sort by pointer/iterator identity, container index alone (a
  reorder of `recipe.markers` groups — e.g. via the roster's own drag-reorder, if any — must not
  change which candidate is picked as "the seed" of an already-confirmed set on a re-run), or any
  `std::unordered_*` iteration order.
- **Performance estimate (rough-estimate basis tag, Constitution §7)** — no benchmark exists yet
  since this ticket's code has not been written; this is complexity-class reasoning translated into
  an order-of-magnitude wall-clock figure, not a measured or cycle-counted number, and must be
  replaced with a real measurement once acceptance test #8's stress case is running. At STEP49's
  typical marker-layer sizing (tens of candidates), `FindMarkerSymmetryMatches`'s O(candidateCount *
  average orbit size) shape is expected to complete in well under 1ms on typical development
  hardware — a handful of spatial-grid bucket lookups, no allocation in the hot loop beyond the
  one-time grid build. At acceptance test #8's synthetic stress size (several hundred candidates,
  deliberately past typical sizing to make an accidental O(n²) implementation observable), the
  linear-shape implementation is expected to stay in the low-single-digit-milliseconds range; a
  bug that regresses this to O(n²) at that same input size would be expected to land in the
  seconds range, which is what makes test #8's wall-clock ceiling a meaningful, non-flaky
  regression guard rather than an arbitrarily tight one.

## Files touched

**New:**
- `src/pipeline/MarkerSymmetryDetection_PIPELINE.h` / `.cpp` — §3a. Estimated ~55-70 line header
  (mostly comments per the shape above; trim comments if needed to clear the soft-100 ceiling) + a
  ~40-60 line `.cpp` (the seed loop, spatial-grid build, per-seed sort/claim). Split the per-seed
  sort/claim step into its own file-local (anonymous-namespace) helper function if the `.cpp` would
  otherwise exceed ARCH_01_05_FileSizeCeilings.md §1.5's 40-line-per-function ceiling — report real
  line counts at implementation time.
- `src/ui/MarkerSymmetryFixCommand_UI.h` / `.cpp` — §3b. Estimated ~25-line header + ~35-45 line
  `.cpp` (the candidate-pool walk + id-allocation loop described in §3b's five steps).
- `src/pipeline/MarkerSymmetryDetection_PIPELINE_Test.cpp` — new acceptance-test binary, hosting the
  acceptance tests that exercise `Pipeline::FindMarkerSymmetryMatches` directly (unmatched-slot
  reporting #3, cross-layer isolation is a caller-side concern but the underlying determinism test #7
  and performance-shape test #8 are both query-level, plus the mask-resolution grep check #9's
  PIPELINE-file half). Same one-binary-per-new-source-file convention every other PIPELINE ticket in
  this codebase uses (e.g. `SymmetryOrbitQuery_PIPELINE_Test.cpp` for `SymmetryOrbitQuery_PIPELINE`).
  Registered in `CMakeLists.txt` — see the CMakeLists bullet below.
- `src/ui/MarkerSymmetryFixCommand_UI_Test.cpp` — new acceptance-test binary, hosting the acceptance
  tests that exercise `Ui::FixMarkerLayerSymmetry` (skip/overwrite mode #1/#2/#5, fresh id allocation
  #4, cross-layer isolation #6, never-touches-import grep #10, row-only-writes isolation #11) plus the
  determinism test #7 at the command level (id-allocation is order-sensitive in a way the pure query
  test alone does not cover). Same convention as the existing `MarkerDragGesture_UI_Test.cpp` —
  STEP94's own registered UI-layer test binary (`CMakeLists.txt:749`; `MarkerOrbitCorrespondence_UI.h`/
  `.cpp` has no separate registered binary of its own — its `MatchCorrespondenceToOrbit` is exercised
  indirectly through `MarkerDragGesture_UI_Test.cpp`) — for its own new UI-layer file. Registered in
  `CMakeLists.txt` — see the CMakeLists bullet below.

**Modified (re-verified against the current, post-STEP110 file — see the banner at the top of this
ticket):**
- `src/params/Symmetry_PARAMS.h` — §5. New sibling struct `MarkerSymmetryFixSettings` (one field,
  `float distanceTolerance = 0.5f;`), added immediately after `SymmetryDetection`.
- `src/params/MapRecipe_PARAMS.h` — §5. `Params::MapRecipe` gains
  `MarkerSymmetryFixSettings markerSymmetryFixSettings;`, next to the existing `symmetryDetection`
  field (`:96`). Additive only, no `SanGenVersion` bump.
- `src/io/MapExporter_Symmetry_IO.cpp` — §5. `BuildSymmetryJson`-family function adds
  `json["MarkerSymmetryFixDistanceTolerance"] = recipe.markerSymmetryFixSettings.distanceTolerance;`
  immediately after the existing `json["SymmetryDetectionTolerance"] = ...` line (`:21`).
- `src/io/MapImporter_Symmetry_IO.cpp` — §5. Adds `ReadJsonFloat(json,
  "MarkerSymmetryFixDistanceTolerance", outRecipe.markerSymmetryFixSettings.distanceTolerance);`
  immediately after the existing `ReadJsonFloat(json, "SymmetryDetectionTolerance", ...)` line
  (`:24`). Absent key (legacy files) keeps the struct default `0.5f`.
- `src/ui/MarkersTab_ManualLayers_UI.cpp` — `DrawLayerRowBody` (today `:36-52`; formerly
  `DrawSelectedLayer` before STEP110 — see the STEP106 landing-order note above for why these line
  numbers are not to be trusted at dispatch time either): add the tolerance slider + checkbox +
  button + result line inside the existing `DrawSectionBegin("Layer Symmetry", ...)` block (today
  `:46-50`), directly after the existing `DrawPlacementSymmetryAxes` call. Add `MarkerDragGesture_UI.h`
  (for `ResolveEffectiveMarkerSymmetry`) and the new `MarkerSymmetryFixCommand_UI.h` includes.
  `DrawLayerRowBody` gains new parameters, and its two callers must thread them through — this is a
  genuinely new plumbing requirement the original (pre-STEP110) version of this ticket did not need,
  because `DrawSelectedLayer` used to be called exactly once per frame with no `rowIndex` at all:
  - **`DrawLayerRowBody`** (anonymous namespace): signature becomes
    `bool DrawLayerRowBody(Params::MarkerInstanceLayer& layer, int layerIndex, const std::vector<
    Params::MarkerInstanceLayer>& markerLayers, std::vector<Params::MarkerInstanceGroup>& markers,
    const Params::Geometry& geometry, int globalSymmetryMask, int globalRadialRepeatCount,
    Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings, ManualMarkerLayersState& state)`.
    `layerIndex` is the row's own index (see §1); `markerLayers` is the full vector, needed only for
    the `ResolveEffectiveMarkerSymmetry` call (the row's OWN layer is still read via the `layer`
    reference, not re-looked-up through `markerLayers[layerIndex]`);
    `markers`/`geometry`/`globalSymmetryMask`/`globalRadialRepeatCount` are `FixMarkerLayerSymmetry`'s
    own required inputs, unavailable to this function before this ticket;
    `markerSymmetryFixSettings` is the new §5 field, by mutable reference so the slider can write it.
  - **`DrawLayerList`** (anonymous namespace, `MarkersTab_ManualLayers_UI.cpp:60-74`): its own
    signature gains the same five new parameters it does not yet have —
    `markers`/`geometry`/`globalSymmetryMask`/`globalRadialRepeatCount`/`markerSymmetryFixSettings` —
    appended after its existing `markerLayers` parameter (which it already has, and which the
    row-body lambda closes over unchanged for the new `DrawLayerRowBody` call's `markerLayers`
    argument). Its row-body lambda (`:69-72`) changes from `DrawLayerRowBody(markerLayers[rowIndex],
    state)` to `DrawLayerRowBody(markerLayers[rowIndex], rowIndex, markerLayers, markers, geometry,
    globalSymmetryMask, globalRadialRepeatCount, markerSymmetryFixSettings, state)`.
  - **`DrawManualMarkerLayers`** (the public entry point, `:118-131`): gains the same four new
    parameters the original ticket already planned (`geometry`/`globalSymmetryMask`/
    `globalRadialRepeatCount`/`markerSymmetryFixSettings`) — `markers` is ALREADY one of its existing
    parameters (`std::vector<Params::MarkerInstanceGroup>& markers`, used today for the delete/reorder
    repair functions), so it is passed through to the new `DrawLayerList` call unchanged, not newly
    added.
- `src/ui/MarkersTab_ManualLayers_UI.h` — `ManualMarkerLayersState` gains `bool
  bFixSymmetryOverwrite = false;`, `bool bHasFixSymmetryResult = false;`,
  `Ui::MarkerSymmetryFixResult lastFixSymmetryResult;`, and (§5) a shared
  `ScalarSliderRange fixSymmetryToleranceRange{ 0.01f, 10.0f, 0.0f };` +
  `RealtimeToggle fixSymmetryToleranceToggle;` pair for the new slider, matching STEP106's
  `gridSnapSizeRange`/`selectedLayerGridSnapToggle` shape (include the new
  `MarkerSymmetryFixCommand_UI.h` header for the result type — see §2's shared-state note on why
  these fields are one instance for the whole block, not one per row). `DrawManualMarkerLayers`'s
  declaration gains four new parameters this command needs but the function did not previously take:
  `const Params::Geometry& geometry, int globalSymmetryMask, int globalRadialRepeatCount,
  Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings` — add
  `#include "../params/Geometry_PARAMS.h"` and `#include "../params/Symmetry_PARAMS.h"` for the new
  parameters' types (neither currently included by this header).
- `src/ui/MarkersTab_UI.cpp:49` (not `:65` — re-verify by searching for the literal
  `DrawManualMarkerLayers(state.manualLayers` call, do not trust either line number blindly) —
  `DrawManualMarkerLayers(state.manualLayers, recipe.markerLayers, recipe.markers)` becomes
  `DrawManualMarkerLayers(state.manualLayers, recipe.markerLayers, recipe.markers, recipe.geometry,
  recipe.globalSymmetryMask, recipe.radialSymmetryRepeatCount, recipe.markerSymmetryFixSettings)` —
  all four new arguments are already-available fields on `recipe` at this call site
  (`Params::MapRecipe::geometry`, `MapRecipe_PARAMS.h:34`; `globalSymmetryMask`, `:83`;
  `radialSymmetryRepeatCount`, `:91`; `markerSymmetryFixSettings`, newly added next to
  `symmetryDetection` per the PARAMS bullet above — no new plumbing needed above `DrawMarkersTab`).
- The relevant `CMakeLists.txt` — **edit required**, corrected from the original "no edit needed"
  claim: the `src/pipeline/*.cpp`/`src/ui/*.cpp` GLOB coverage at `CMakeLists.txt:174-183`
  (`file(GLOB_RECURSE SANGEN_V2_SOURCES CONFIGURE_DEPENDS ...)`) covers compiling the two new
  production files into the main target, but every test binary in this codebase is registered
  separately via its own explicit `add_sangen_test(...)` line — the GLOB does not create test
  binaries. Add two new lines:
  - After `add_sangen_test(SymmetryOrbitQuery_PIPELINE_Test  src/pipeline/
    SymmetryOrbitQuery_PIPELINE_Test.cpp)` (currently `CMakeLists.txt:542`, the last line of the
    `# PIPELINE` block): `add_sangen_test(MarkerSymmetryDetection_PIPELINE_Test
    src/pipeline/MarkerSymmetryDetection_PIPELINE_Test.cpp)`.
  - After `add_sangen_test(MapCanvas_MarkerDrag_UI_Test src/ui/MapCanvas_MarkerDrag_UI_Test.cpp)`
    (currently `CMakeLists.txt:753`, STEP94's own UI test registration, immediately before
    `ArmiesTab_UI_Test`): `add_sangen_test(MarkerSymmetryFixCommand_UI_Test
    src/ui/MarkerSymmetryFixCommand_UI_Test.cpp)`.
  Re-verify both line numbers against the live `CMakeLists.txt` at implementation time, same posture
  as every other line-number citation in this ticket. (The pre-existing `src/pipeline/*.cpp`/
  `src/ui/*.cpp` GLOB coverage at `CMakeLists.txt:174-183`,
  `file(GLOB_RECURSE SANGEN_V2_SOURCES CONFIGURE_DEPENDS ...)`, still needs no edit — it compiles the
  two new production files into the main target automatically. Only the TEST binaries need the two
  new `add_sangen_test(...)` lines above; that GLOB does not register tests.)

## Layer & accuracy class

PIPELINE query: Exact, Deterministic (Constitution §4) — same accuracy class as
`Pipeline::BuildWorldSymmetryOrbit`, whose output it consumes unmodified. UI command: Visual for the
button/checkbox/result-text presentation, but the `symmetryGroupIdentifier` VALUES it writes are
gameplay-relevant (STEP94's drag-and-follow keys off them) — the id-allocation rule in §3b step 4
(fresh, layer-scoped, collision-free) is not a presentation choice and must be followed exactly.

## Backend policy

CPU only, synchronous, on the UI thread — human-triggered by a single button press, not a per-frame
or DAG-participating operation. No `Dispatch_SYS` involvement; this authors no PROC stage and no GPU
kernel, matching STEP94's identical backend posture for its own PIPELINE call.

## ARCH rules invoked

- `ARCH_16_03_ModuleBoundaryChain.md` §16.3 — the PIPELINE-hosted stateless query pattern this
  ticket's new file follows.
- `ARCH_16_05_MarkerTransformFields.md` §16.5 — `symmetryGroupIdentifier`'s sentinel/grouping meaning,
  which this ticket backfills rather than redefines.
- Constitution §1 — UI sets PARAMS, never PIPELINE/PROC; every write in this ticket lands in
  `recipe.markers` from `Ui::FixMarkerLayerSymmetry`, never from the PIPELINE query.
- Constitution §4 — Deterministic sub-mode; §6 above is this rule applied to a one-shot detection
  rather than a per-frame stage.
- Constitution §6 — every index (group/transform/candidate/orbit slot) is range-checked before
  dereference, mirroring STEP94/STEP81's own discipline for their picks/binds.
- Constitution §7 — the rough-estimate-tagged performance note in §6 above satisfies the
  benchmark-basis-tag requirement pending a real measurement once the code exists.
- Constitution §8 — the match-distance tolerance is a real, designer-editable `Params` field
  (`Params::MarkerSymmetryFixSettings::distanceTolerance`, §5), not a bare `constexpr`; this is the
  rule the original bare-constant plan violated and §5's ARCH ruling corrects.
- `ARCH_01_05_FileSizeCeilings.md` §1.5 — governs the file split in "Files touched" above; measure,
  don't assume, at implementation time.

## Explicit out-of-scope

- **A `symmetryLocked`-style per-marker opt-out bit** — the known accepted limitation of overwrite
  mode (§2) is documented, not fixed, here. A future ticket's job if the human wants it.
- **Auto-running on import** — Format Expert's ruling (see "The problem this backfills"); this
  command is human-triggered only, via the button this ticket adds. No import/load-path call site is
  touched.
- **Global cross-layer detection** — §1's per-layer scope is deliberate; a marker on a different
  layer is never a match candidate even at zero distance-tolerance-adjusted mirror position.
  Multi-layer detection, if ever wanted, is a new ticket, not a mode flag on this one.
- **Per-row isolation of the checkbox/result state** — §2's new "known accepted limitation" note;
  the shared-across-rows display quirk is inherited from the row-based UI's own established
  `MarkerInstanceLayer`-cannot-carry-UI-state constraint, not something this ticket introduces or
  fixes independently.
- **Any change to STEP94's drag-and-follow machinery** (`MarkerDragGesture_UI.h/.cpp`,
  `MarkerOrbitCorrespondence_UI.h/.cpp`, `MapCanvas_MarkerDrag_UI.h/.cpp`) — this ticket only calls
  `ResolveEffectiveMarkerSymmetry` (read-only reuse) and reuses `Pipeline::
  BuildWorldSymmetryOrbit`/`WorldSymmetryOrbitPoint`; every file STEP94 created is otherwise
  untouched.
- **Cascade behavior for a marker deleted/moved after being grouped by this command** — unchanged;
  once `symmetryGroupIdentifier` is set, that marker behaves exactly as any STEP94-tagged marker
  under every existing STEP94 rule (drag-and-follow, cardinality-change handling, Spawn refusal).
  This ticket only originates the tag; it does not add a new consumer of it.
- **STEP106's own scope, whatever it is** — this ticket does not know STEP106's contents beyond "it
  also edits the same function"; the landing-order banner exists precisely so this ticket does not
  need to.
- **Un-doing a "Fix Symmetry" run** — no undo/history is added; a designer who dislikes the result
  edits the affected markers' groupings by hand via STEP94's existing "ungroup" affordances (roster
  editor), same as any other manual PARAMS edit in this codebase.

## Acceptance test

1. **Basic skip-mode detection.** Two ungrouped markers in one layer, positions exact mirrors under
   `MirrorAcrossX`. Press "Fix Symmetry" with the checkbox OFF. Assert both markers now share one
   fresh, positive `symmetryGroupIdentifier`; assert the result text/state reports
   `confirmedGroupCount == 1`, `unmatchedSlotCount == 0`.
2. **Skip mode never touches an already-grouped marker.** Same layer, add a third pair already
   sharing a nonzero id (simulating a prior STEP94 drag or an earlier Fix Symmetry run) with one
   member's position deliberately NOT a mirror of the other (simulating a manual nudge after
   grouping). Press "Fix Symmetry" (checkbox OFF). Assert that pair's ids and positions are
   byte-identical before/after — skip mode must never read or rewrite them.
3. **Unmatched-slot reporting, no force-assignment.** One ungrouped marker under a mask whose mirror
   position has NO candidate within tolerance. Press "Fix Symmetry". Assert
   `symmetryGroupIdentifier` stays `0` on that marker (not force-assigned), and
   `unmatchedSlotCount >= 1` reflects it.
4. **Fresh id allocation, no collisions.** A layer already containing ids `1` and `3` (a gap is
   legal — ids are never renumbered/compacted) plus two new ungrouped mirrored markers. Press "Fix
   Symmetry". Assert the newly-confirmed pair receives id `4` (max existing + 1), not `2` or any
   value colliding with `1`/`3`.
5. **Overwrite mode widens the pool and re-derives ids.** A layer with one pre-existing grouped pair
   (id `1`) whose positions are STILL mirrors, plus a second ungrouped mirrored pair. Check
   "Overwrite manually-adjusted positions", press "Fix Symmetry". Assert every marker in the layer
   was zeroed first (verifiable via an instrumented/mocked pre-write snapshot in the test, or by
   asserting the FIRST pair's post-run id is NOT necessarily `1` again — it is whatever the fresh
   allocation assigns, since overwrite mode does not special-case "was already correctly grouped").
   Assert the checkbox state (`state.bFixSymmetryOverwrite`) is `false` immediately after the press
   completes, in the same test — the auto-reset behavior.
6. **Cross-layer isolation.** Two markers on DIFFERENT layers whose positions happen to be exact
   mirrors of each other. Run "Fix Symmetry" on either layer. Assert neither is grouped with the
   other (different `layerIndex`, never a candidate pair).
7. **Determinism.** Run detection twice on the identical PARAMS state (same recipe, same layer, same
   checkbox state) without any mutation between runs, feeding `recipe.markers` in two different but
   equivalent orderings (e.g. reverse the `MarkerInstanceGroup` vector order, or reverse a group's own
   `transforms` order, between the two runs — the underlying position/mask/tolerance data is
   unchanged). Assert both runs produce identical `confirmedGroupCount`/`unmatchedSlotCount` and
   assign ids to the same physical marker SETS (not necessarily the same literal id NUMBERS if the
   pre-existing max differs due to the reordering being combined with skip-mode's read of existing
   ids — hold the existing-id state constant across the two runs to isolate ordering as the only
   variable).
8. **Performance shape.** With several hundred ungrouped candidate markers in one layer (a synthetic
   stress case well past STEP49's "tens, not tens of thousands" typical sizing, chosen specifically
   to make an accidental O(n²) implementation observable), assert `FindMarkerSymmetryMatches`
   completes and its own internal bucket-lookup call count (instrument or reason from a
   `visitedEntryCount`-style counter, mirroring `PickMarker`'s existing diagnostic output parameter
   if one is added to this ticket's own function) scales linearly with candidate count, not
   quadratically — a literal `O(n^2)` implementation should visibly fail this test at a large enough
   n via a wall-clock ceiling, not just a hand-wave "looks fine."
9. **Zero calls to the wrong mask-resolution path.** Grep confirms zero calls to
   `ResolvedPlacementSymmetryMask`/`DrawPlacementSymmetryAxes`'s mask output from either new file
   this ticket creates, and zero calls into `Ui::MarkerOrbitCorrespondence_UI`'s
   `MatchCorrespondenceToOrbit` from the new PIPELINE file (PIPELINE may not depend on UI — see §3a's
   own note on why the claim loop is reimplemented rather than called).
10. **Never touches import.** Grep confirms no call to `FixMarkerLayerSymmetry` or
    `FindMarkerSymmetryMatches` from anywhere under `src/io/**` or any `.sanmap` load/import path.
11. **Fix Symmetry acts only on the pressed row's own layer.** With two rows expanded simultaneously
    (the row-based UI's default state — see the banner), each with its own ungrouped mirrored pair,
    pressing ROW A's "Fix Symmetry" button asserts row A's pair is grouped and row B's pair is
    UNTOUCHED (`symmetryGroupIdentifier` still `0`) — the `layerIndex` captured at each row's own
    button call site must not leak across rows even though `state.bFixSymmetryOverwrite`/
    `bHasFixSymmetryResult`/`lastFixSymmetryResult` are shared display state (§2's accepted
    limitation) — this test isolates that the WRITE path stays row-correct even though the display
    does not.
12. Full `SanGenV2` build stays clean; every existing test continues to pass, including every STEP94
    test (`MarkerDragGesture_UI_Test`, `MarkerOrbitCorrespondence_UI_Test` or their equivalents) —
    this ticket must not have altered any STEP94 file's behavior.

## Landing order

`STEP106` → **this ticket**. No other prerequisite: every type/function this ticket consumes
(`Proc::BuildSymmetryOrbit`, `Pipeline::BuildWorldSymmetryOrbit`, `Data::SpatialGrid`,
`ResolveEffectiveMarkerSymmetry`, `MarkerInstanceLayer`/`MarkerTransform`,
`DrawLayerRowBody`/`DrawLayerList`/`DrawManualMarkerLayers`/`ManualMarkerLayersState`) already exists
on disk today, re-verified while rewriting this ticket. The STEP106 ordering is purely to avoid the
colliding-edit scenario described at the top of this ticket — it carries no data or type dependency
in either direction. **Because a second, independent structural change (STEP110) already landed and
invalidated this ticket's original citations once, the coder must re-verify every function name and
line number in this ticket against the tree again immediately before implementing** — do not assume
no third change has landed between this rewrite and actual dispatch.
