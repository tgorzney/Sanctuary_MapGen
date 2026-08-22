# STEP94 — Canvas drag-and-follow for manually-placed symmetry-grouped markers

**Layer:** UI. **Domain:** `MapCanvas`, the manual marker roster (`recipe.markers`/`recipe.markerLayers`).
**Executor:** SanGen Coder, once every prerequisite below has actually landed — see "Landing order."
Authored by the SanGen UI Expert.

⚠️ **This ticket is drafted entirely against specified shapes that do not exist on disk yet.**
`STEP47`/`STEP48`/`STEP49`/`STEP68` are drafted work-orders, not built code (verified against the
working tree while authoring this ticket: `src/ui/MarkersTab_Manual_UI.*` does not exist,
`src/pipeline/SymmetryOrbitQuery_PIPELINE.*` does not exist, `MarkerTransform` carries no
`layerIndex`/`symmetryGroupIdentifier` field today). Every function/type cited from those four
tickets is cited as **"per STEPxx's specified signature,"** never as a `file:line` — there is no
real line to cite yet. This is a deliberate, human-directed exception to normal ticket-authoring
practice (see the dispatch brief this ticket was written from), not an oversight.

## Why this ticket was held, and why it isn't anymore
This is the manual-marker "drag one member of a symmetry group, the others recompute live"
interaction from `work_orders/DESIGN_MarkerLayerSymmetry_R2.md` §1/§2, ratified into
`ARCH_16_MarkerLayerSymmetry.md` (§16.5's `symmetryGroupIdentifier` field, §16.8's orphan ruling,
and §14.8's C2 dirty-flag tier, which names this exact interaction: *"a symmetry-group drag under
§16 is exactly the C2 'active drag/edit' case, and the passthrough's whole point is that it can be
called every frame of that gesture with zero DAG/dirty-hash involvement"*). It was deferred because
its prerequisites — STEP47 (world↔screen projection), STEP48 (spatial-grid click picking), STEP49
(per-instance marker editor), STEP68 (symmetry orbit PIPELINE wrapper) — were drafted but not built,
so there were no real line numbers to cite. The human has now ruled: draft it anyway, against those
tickets' own specified shapes, the same way any forward-looking ticket in this codebase already
must.

## ⚠️ A fifth and sixth gap, found while drafting — not one of the four cited prerequisites
Tracing the actual data flow a canvas drag would need surfaced two more load-bearing absences that
none of STEP47/48/49/68 supplies. Flagged here plainly, per this session's own standing practice
(the same posture `STEP81`'s Radial-bit disclosure and this ticket's own dispatch brief both use) —
**not fixed by those four tickets, and not silently assumed away by this one either.**

**Gap 5 — manual markers have no `Data::PlacementInstances` presence, so STEP48's picking path
cannot see them.** Confirmed three independent ways: `STEP49_ManualMarkersUI.md` ("`recipe.markers`
feeds no PROC stage today — confirmed, no `MarkerInstanceGroup` reference anywhere under
`src/proc/`"), `STEP60`'s identical grep result (cited verbatim inside `STEP81`), and
`ARCH_14_13_OpenItems.md` item 3 Ruling 3 ("`recipe.markers` [...] never appear[s] anywhere in
`src/proc/` — the existing hand-placed-entity family already never runs through
`BuildSymmetryOrbit`/`ResolveSymmetryMask`"). STEP48's `Picking_UI::PickMarker` +
`Data::SpatialGrid` operate exclusively over that same `Data::PlacementInstances` buffer — a buffer
manual markers never enter. **This ticket therefore does not use STEP48's picking path to identify
a dragged manual marker at all** — see "Hit-testing a manual marker" below for the routing-around.

**Gap 6 — manual markers have no rendering consumer of any kind today**, so before this ticket even
asks "which marker did the user grab," there is nothing on the canvas to grab. Confirmed by
`STEP81`'s own out-of-scope list: *"Any rendering / overlay / compositor consumer of `markerLayers`,
`layerIndex`, `color`, or `iconScale`. [...] nothing draws with them yet, the same posture props have
had since ARCH_12_ManualPropDecalLayers.md §12."* `work_orders/SEQUENCE_PreviewOverlayLayering.md`'s Phase 5 (the phase that
wires *manual* sub-layers into real screen-space rendering) covers Props and Decals only (5.1/5.2/
5.3); no marker-domain equivalent is scheduled anywhere in that sequence — the one marker line item
in Phase 5 is the retired old `STEP60_MarkerInstanceLayer_PARAMS.md`, a PARAMS ticket, not a
rendering one. **This ticket cannot be dispatched before manually-placed markers are visible on the
canvas at all**, and no cited prerequisite makes them visible. Resolution below (not deferred to a
seventh ticket) — see "A minimal, deliberately non-Phase-5 manual-marker draw."

Both gaps are resolved inside this ticket's own scope, not punted, because the fix for each is small
and this ticket needed equivalent machinery anyway (see below). They are surfaced this prominently
so nobody dispatches this ticket believing STEP47/48/49/68 alone make it buildable.

## Required reading (in addition to STEP47/48/49/68, already summarized above)
- `work_orders/DESIGN_MarkerLayerSymmetry_R2.md` §1 (the gesture-start identity proof, read in
  full — see "The gesture-start proof, and how it is represented here" below) and §2 (the Spawn/Army
  orphan ruling — not re-litigated here, only its "refuse the drag" half is this ticket's concern).
- `work_orders/STEP81_MarkersTabManualLayers_UI.md` — sibling ticket for the Manual Marker Layers
  tab and the per-instance Layer picker. This ticket does not duplicate its scope (layer add/
  reorder/delete/symmetry-authoring, the layer picker) and consumes its output type
  (`Params::MarkerInstanceLayer`, `recipe.markerLayers`) by the shape STEP81 specifies.
- `ARCH_16_05_MarkerTransformFields.md` — `symmetryGroupIdentifier` (sentinel `0` = ungrouped) and
  `layerIndex` on `MarkerTransform`, the two fields this ticket's grouping/mask-resolution logic
  keys off.
- `ARCH_16_08_SpawnArmyShrink.md` — the orphan-never-auto-delete ruling this ticket's Spawn-refusal
  behavior must not contradict (refusing the drag is a stricter, earlier-stage guard than the
  orphan ruling; the two do not conflict — see "Spawn refusal" below).
- `ARCH_14_08_DirtyFlagTiers.md` §14.8 — the C2 tier this whole interaction runs at.

## Known pre-existing defect this ticket must route around, not inherit
`ResolvedPlacementSymmetryMask` (`src/ui/PlacementRuleSections_UI.h:56-61`) ANDs a mask against only
a 4-entry axis table (`MirrorAcrossX`/`MirrorAcrossZ`/`RotateHalfTurn`/`QuarterTurns`), silently
dropping `Params::SymmetryAxis::Radial` (bit 4, `Symmetry_PARAMS.h:23`). It is a real, already-
flagged (`STEP81`) shared-widget defect, not fixed by either ticket. **This ticket's live-recompute
math never calls it.** The mask this ticket needs is already a fully-resolved, already-validated
integer — `layer.symmetry.symmetryMask` (per STEP81's `Params::MarkerInstanceLayer` shape) or
`recipe.globalSymmetryMask` — read directly and passed straight into STEP68's
`Pipeline::BuildWorldSymmetryOrbit`, which forwards unmodified to `Proc::BuildSymmetryOrbit`, the
real bit-complete implementation (`Symmetry_PARAMS.h`'s own comment: *"bit-check order tracks
ascending bit value... `Radial` runs last"*). `ResolvedPlacementSymmetryMask` exists only to legalize
a mask **for display in the 4-checkbox widget**; it is never a step in this ticket's data path, so
the Radial-clearing bug is structurally unreachable here rather than merely avoided by discipline.
Do not add a call to it anywhere in this ticket's files — that would reintroduce the exact defect
this section exists to keep out.

## The gesture-start proof, and how it is represented here
R2 §1's key fact, proven by the Generator Expert (group theory, cross-checked against
`Placement_Symmetry_PROC_Test.cpp`'s hardest cases — **a proof, not a heuristic**): `BuildSymmetryOrbit`
always writes the seed point to output slot 0 before running any transform, so regenerating the
orbit from a member's **pre-drag** position reproduces every sibling's pre-drag position exactly
(same physical point set, same existing `duplicateEpsilon`, no new tolerance). R1's abandoned
nearest-point matching was unsafe only for a **rotated** point cloud (a drag already in progress);
matching two identical, unmoved clouds is exact-value equality, not a heuristic — this is why R2
could drop `bSymmetryAnchor`/`symmetryOrbitIndex` entirely (ARCH_16_05_MarkerTransformFields.md §16.5 confirms the drop).

**Represented here** as a one-shot, gesture-scoped correspondence table, built exactly once per
gesture and never rebuilt mid-drag:

```cpp
// MarkerDragGesture_UI.h — pure, imgui-free (same posture as MarkerLayerIndexRepair_UI.h,
// testable with no window). Layer: UI.
struct MarkerOrbitCorrespondence {
    int orbitSlot        = 0;   // index into BuildWorldSymmetryOrbit's output for this gesture
    int transformIndex   = -1;  // index into the dragged member's MarkerInstanceGroup::transforms
};

struct MarkerDragGestureState {
    bool bActive                 = false;
    int  groupIndex               = -1;   // recipe.markers[groupIndex] — the dragged member's group
    int  draggedTransformIndex    = -1;   // recipe.markers[groupIndex].transforms[...]
    int  symmetryGroupIdentifier  = 0;    // snapshot at gesture-start; frozen for the gesture
    int  effectiveSymmetryMask    = 0;    // snapshot — see "Mask resolution" below
    int  effectiveRadialRepeatCount = 0;  // snapshot
    std::vector<MarkerOrbitCorrespondence> correspondence;  // built once, at gesture-start
    int  lastValidOrbitCount      = 0;    // cardinality at gesture-start / last accepted frame
    Pipeline::WorldSymmetryOrbitPoint lastValidDraggedWorldPoint;  // for Spawn-refusal freeze
    bool bSpawnCardinalityRefused = false; // this frame's UI-feedback flag
};
```

At mouse-down on a manual marker whose `symmetryGroupIdentifier != 0`: call
`Pipeline::BuildWorldSymmetryOrbit(geometry, effectiveSymmetryMask, effectiveRadialRepeatCount,
draggedMember.transform.transform.positionX, draggedMember.transform.transform.positionZ,
scratchPoints, symmetryOrbitMaximum)` (STEP68's specified signature) using the member's **pre-drag**
position; walk `recipe.markers[groupIndex].transforms` for every other entry sharing the same
`symmetryGroupIdentifier`, and match each to the orbit output slot whose world point equals (within
`Placement_Symmetry_PROC.h`'s existing `symmetryDuplicateEpsilon`, per STEP68's own wrapper
definition) that sibling's **current** (== pre-drag, nothing has moved yet) position. Store
`{slot, transformIndex}`. The dragged member itself is never in `correspondence` — its own position
is just wherever the cursor says, orbit slot 0 by the seed-point convention.

A member with `symmetryGroupIdentifier == 0` (ungrouped) is draggable too — ordinary, unrestricted
repositioning with an empty `correspondence` table and no orbit call at all. This ticket's gesture
machinery is a strict superset of "drag any marker"; ungrouped markers just skip §1 entirely.

## Mask resolution (and why it is not `ResolveSymmetryMask`)
STEP68's `BuildWorldSymmetryOrbit` wrapper takes an already-resolved `symmetryMask`/
`radialSymmetryRepeatCount` — it does not do global-vs-local resolution itself (that PROC-side
`ResolveSymmetryMask`/`ResolveRadialSymmetryRepeatCount` pair, `Placement_RuleBuild_PROC.h`, is not
reachable from UI, and STEP68's wrapper deliberately does not wrap it — see STEP68's own scope). This
ticket's UI-side code resolves it itself, as a plain two-line ternary, sourced from the dragged
member's layer (per STEP81's shape):
```cpp
const Params::MarkerInstanceLayer& layer = markerLayers[transform.layerIndex];  // STEP81/STEP60 shape
const int  effectiveMask  = layer.symmetry.bSymmetryUseGlobal ? recipe.globalSymmetryMask
                                                                : layer.symmetry.symmetryMask;
const int  effectiveCount = layer.symmetry.bSymmetryUseGlobal ? recipe.radialSymmetryRepeatCount
                                                                : layer.symmetry.radialSymmetryRepeatCount;
```
This is new, ticket-local logic (not a shared helper renamed from somewhere) — it is intentionally
NOT unified with `DrawPlacementSymmetryAxes`'s widget-side resolution, because that path is exactly
where `ResolvedPlacementSymmetryMask`'s defect lives (see above). A future shared-widget fix that
corrects the defect is welcome to absorb this ternary too; this ticket does not wait for that fix
and does not need it, since the raw fields are already valid integers with no repair required.

## Hit-testing a manual marker (routes around Gap 5)
STEP48's `Picking_UI::PickMarker`/`Data::SpatialGrid` is **not used here** — see Gap 5. Instead, a
new, small, PARAMS-direct linear scan: for each `MarkerTransform` across every `recipe.markers[i]`,
project its `(positionX, positionZ)` through STEP47's `PreviewComposite::WorldToPreviewPixel`, then
`MapCanvasView::ProjectPreviewPixelToRegionLocal` (composing both halves STEP47 built, exactly as
that ticket anticipated for "Phase 3's screen-space icon draw pass" — used here ad hoc, at gesture-
start only, not per frame), and accept the nearest one within `pickRadiusScreenPixels` (the same
named setting STEP48 introduces, reused rather than duplicated). A linear O(manual marker count)
scan is legitimate and not a §14.9-class violation: `STEP49`'s own sizing note governs — *"hand-
placed counts are tens, not tens of thousands."* This is a genuinely different performance regime
from the 100k+-instance procedural picking STEP48 exists to fix; reusing SpatialGrid machinery here
would be solving a problem this domain doesn't have. Ordinary single-click selection of *procedural*
markers is untouched — it keeps using STEP48's path exactly as drafted; this ticket adds a second,
narrower hit-test scoped only to manual markers, tried first at press-time (see "Wiring into
`MapCanvas`" below).

## A minimal, deliberately non-Phase-5 manual-marker draw (routes around Gap 6)
This ticket already needs to draw the live drag's ghost preview points every frame (see below); it
ships the smallest extension of that same draw call to also render every **at-rest** manual marker,
so there is something to click in the first place. Deliberately minimal, so this ticket does not
silently absorb Phase 5's real scope:
- Plain `ImDrawList::AddCircleFilled` dots (or the codebase's existing simplest filled-marker
  primitive — no icon atlas, no `templateIdentifier` lookup, no thumbnail/strategic LOD switch), one
  per `MarkerTransform`, projected the same way as the hit-test above.
- No `OverlayLayer_UI`/View-toolbar participation (`ARCH_14_PreviewOverlayLayering.md` §14's system,
  STEP51/53/54) — that is real icon rendering at 100k+-instance scale with atlas paging and budget
  decimation; manual markers are tens of instances and do not need any of it.
- Lives in its own new imgui-including file (see "Files touched"), not grown into
  `MapCanvas_Draw_UI.cpp`, so `MapCanvas_Draw_UI.cpp` keeps its single job (pan/zoom/click routing)
  and stays small.
- **This is intentionally a stopgap.** A future ticket that wires manual markers into the real
  overlay/icon system (the marker-domain analogue of Phase 5's Props/Decals work) supersedes this
  draw call outright; this ticket's interaction/PARAMS logic does not change when that happens — only
  the draw call is swapped. Flag this stopgap to the human when dispatching, same posture STEP81 used
  for its own flagged-not-fixed items.

## The drag gesture state machine
1. **Press** (`ImGui::IsItemActivated()`, inside `MapCanvas::ApplyPointerInput`, STEP47/48's file):
   before falling into the existing pan-vs-click disambiguation, try the manual-marker hit-test
   above. A hit on a member whose `symmetryGroupIdentifier != 0` builds the correspondence table
   (§1's mechanism); a hit on an ungrouped member starts a drag with an empty table; a miss falls
   through to the existing pan/click path unchanged.
2. **Drag** (`ImGui::IsItemActive()` + nonzero `MouseDelta`, same frame loop STEP47/48 leaves
   unmodified for the non-marker-drag case): convert the new cursor position to world via STEP47's
   `PreviewComposite::PreviewPixelToWorld`; if `correspondence` is non-empty, call
   `BuildWorldSymmetryOrbit` again from that new world position; for every matched sibling, write
   `positionX`/`positionZ` straight into `recipe.markers[groupIndex].transforms[transformIndex]`
   **every frame** — see "Live-write vs. commit-on-release, reconciled" below for why this is
   per-frame, not deferred. `positionY` (height) is never touched on any member — STEP47's mapping
   is explicitly the two horizontal axes only, and manual markers carry no height-sampling (STEP49's
   own out-of-scope).
3. **Cardinality check, every drag frame**, before the write in step 2: if the new orbit's returned
   count differs from `correspondence.size() + 1` (the dragged member itself), see "Cardinality-
   change handling" below instead of writing straight through.
4. **Release** (`ImGui::IsItemDeactivated()`): if a structural (create/delete) change is pending from
   step 3, materialize/cascade-delete it now (reusing R1's existing machinery, unchanged by this
   ticket). Otherwise there is nothing left to commit — ordinary repositioning already landed in
   `recipe.markers` live, per-frame, during step 2. Clear `MarkerDragGestureState`.

### Live-write vs. commit-on-release, reconciled
R2 §1's own text is explicit that **ordinary repositioning writes live, every frame**: *"Live drag:
re-run the orbit from M's current position every frame, write results straight into the matched
existing instances."* Only **structural** changes (a member appearing/disappearing) wait for
release: *"Structural changes (real create/delete) commit only at mouse-up."* This ticket follows
R2's ratified text exactly. Per-frame PARAMS writes are cheap here specifically because
`recipe.markers` feeds no PROC stage (STEP49's confirmed finding, restated above) — nothing observes
these writes until the user next opens the manual-marker tab or exports, so there is no regen storm
and no `PreviewDriver` notification to gate (this ticket's canvas draw calls read `recipe.markers`
directly every frame regardless, the same "manual sub-layers read `MapRecipe` directly" posture
STEP49/STEP81 already establish). If a future reviewer reads this ticket's own summary line loosely
as "commit only at release" for *everything*, this section is the correction: only §2's structural/
cardinality case defers to release; ordinary position updates do not.

## Cardinality-change handling (exactly as R2 specifies)
A drag can move a point onto/off a mirror axis or the map center, changing the orbit's stabilizer
and therefore its point count, independent of the mask (R1's Generator-Expert-flagged finding,
carried forward unmodified by R2). Per drag frame, compare the new orbit count against the cached
gesture-start count:
- **Count increased** (new points appear): the extra points are **never written into `recipe.markers`**
  — screen-space-only ghost circles (distinct visual treatment from the at-rest draw, e.g. a lighter/
  dashed tint — exact styling is a coder-level polish call, not specified numerically here), drawn by
  the same lightweight draw call as the stopgap above. Zero PARAMS write for these slots.
- **Count decreased** (a point collapses onto another, e.g. landing exactly on a mirror axis): the
  now-unmatched existing `MarkerTransform` is **soft-hidden** — an ephemeral, gesture-local flag (not
  a new PARAMS field; lives in `MarkerDragGestureState`, discarded with the gesture), never erased,
  never written to `recipe.markers`. The instance's real data is untouched until release.
- **At release**, if the final frame's count still differs from the gesture-start count, materialize
  the ghost points into real new `MarkerTransform` entries (new `symmetryGroupIdentifier` matching
  the group, position from the ghost, `layerIndex` copied from the dragged member, `name`/`alias`
  auto-generated same convention STEP49's roster "Add" uses) and/or cascade-delete the soft-hidden
  ones, reusing R1's already-existing materialize/cascade-delete machinery (carried forward
  unmodified by R2, not re-derived here). If the final count matches the start count (the drag passed
  through a collapse and came back out), nothing structural happens — only the per-frame position
  writes from the ordinary path apply.

## Spawn-group resize refusal
A named constant for the Army-keyed marker group name — `constexpr const char* kArmyKeyedMarkerGroupName = "Spawn";` in this ticket's new header — per R2 §3's recommendation and
`ARCH_16_09_NonArchItems.md`'s confirmation that this is UI-internal naming hygiene needing no ARCH
ratification. If `recipe.markers[groupIndex].name == kArmyKeyedMarkerGroupName`, the cardinality-
change path above is **refused outright**, not silently clamped:
- The moment a drag frame's orbit count would differ from the gesture-start count, the position write
  for **every** member of the group (dragged member included) is skipped for that frame — the whole
  group freezes at `lastValidDraggedWorldPoint`, the last position where cardinality matched.
  `bSpawnCardinalityRefused = true` for that frame.
- **UI feedback while refused**: the frozen ghost/at-rest dots for this group render in a distinct
  "refused" tint (e.g. red/orange, contrasted with the ordinary drag tint), and a short status line
  is drawn near the cursor (`ImGui::SetTooltip` or the canvas's existing status-text convention —
  coder's call which), e.g. "Spawn count is fixed — drag limited." No modal, no confirmation dialog —
  matches R2 §2's own low-friction posture for the orphan ruling ("no new confirmation dialog
  either").
- **On release while refused**: nothing commits beyond the frozen position already live in
  `recipe.markers` from the last valid frame — there is no separate "snap back" step, because the
  refusal already prevented the write from ever advancing past the last valid frame. This is
  cheaper and simpler than an explicit revert, and produces the same observable result R2 asks for.
- Ordinary repositioning of a Spawn group (no cardinality change) is exactly as unrestricted as any
  other type — only the resize-via-drag path is blocked, per R2 §1's explicit carve-out. This refusal
  is orthogonal to, and strictly earlier-stage than, `ARCH_16_08_SpawnArmyShrink.md`'s orphan ruling:
  that ruling governs a shrink that *does* happen (via the layer-mask/roster path, not drag); this
  ticket's refusal exists specifically so a drag-triggered shrink never reaches that point.

## The roster-slider counterpart (R2 §4 — coordination note, not built here)
R2 §4 restates that "every roster row's position fields stay live, going through the same §1
mechanism as a canvas drag" — i.e., STEP49's per-instance Position sliders, when editing a grouped
member, must also trigger the live-recompute-and-write behavior above, not a raw single-field write.
This ticket exposes the mechanism as a reusable, non-canvas-specific entry point precisely so
STEP49's file can call it without depending on `MapCanvas` at all:
```cpp
// MarkerDragGesture_UI.h — callable from a slider's on-edit path with no canvas/gesture-state
// dependency; the canvas gesture above is one caller of this, STEP49's sliders are the other.
bool RepositionSymmetryGroupMember(std::vector<Params::MarkerInstanceGroup>& markers,
                                   const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                   const Params::Geometry& geometry, int globalSymmetryMask,
                                   int globalRadialRepeatCount, int groupIndex,
                                   int movedTransformIndex, float newWorldX, float newWorldZ);
```
Whichever of STEP49/STEP94 lands second wires the actual call site (one line in
`MarkersTab_Manual_UI.cpp`'s Position-slider block, after the existing slider write, gated on
`transform.symmetryGroupIdentifier != 0`). Not speculatively pre-wired into STEP49's not-yet-existing
file here — that file does not exist to edit yet, and STEP49's own ticket does not list this call, so
whichever lands second must add it. Flagging this explicitly so the coordination is not lost between
the two tickets, same posture STEP81 used for its STEP80 coordination section.

## Files touched
**New** (kept comfortably under the ARCH_01_05_FileSizeCeilings.md §1.5 soft-100 ceiling by
composition, not by cramming
`MapCanvas_UI.h` — that file is only 86 lines today and stays near that size; all new state and logic
lives in these new files instead, the same "large class splits its method definitions... behind one
small header" pattern ARCH_01_05_FileSizeCeilings.md §1.5 prescribes, applied here as "new interaction gets its own composed
type" rather than growing `MapCanvas` directly):
- `src/ui/MarkerDragGesture_UI.h` / `.cpp` — pure, imgui-free gesture state, correspondence-table
  build/match, live recompute, cardinality/Spawn-refusal logic, `RepositionSymmetryGroupMember`.
  Estimated ~90-120 lines split across the two files given the state struct + ~4 functions above;
  report real counts at implementation time and split further (e.g. a separate
  `MarkerOrbitCorrespondence_UI.h` for just the match-building step) if the header alone would
  exceed the soft-100 ceiling.
- `src/ui/MapCanvas_MarkerDrag_UI.h` / `.cpp` — the one imgui-including translation unit for this
  ticket: the at-rest manual-marker draw, the ghost/refused-tint draw, and the linear hit-test. Kept
  separate from `MapCanvas_Draw_UI.cpp` (pan/zoom/click) and from `MarkerDragGesture_UI` (pure logic)
  so each file has exactly one job, matching this file family's existing `Prepare`/`Cpu`/`Gpu`/
  `GpuBuffers`/`GpuProgram`-style split precedent (`PreviewComposite_*_UI.cpp`).

**Modified:**
- `src/ui/MapCanvas_UI.h` — one new composed member (`MarkerDragGesture_UI`'s state type), one setter
  (`SetManualMarkerDragSource(std::vector<Params::MarkerInstanceGroup>* markers, const
  std::vector<Params::MarkerInstanceLayer>* markerLayers, const Params::Geometry* geometry, const
  Params::MapRecipe* recipeForGlobalSymmetry)` — mutable `markers` pointer, everything else
  read-only, injected per STEP48's own specified setter shape for `SetPreviewComposite`/
  `SetMarkerPickingSource` (neither landed yet as of this ticket's dispatch, same as this one)
  (STEP48's own ruling: an injected pointer beats a callback re-pushing derived state, "exactly the
  ...second copy... anti-pattern" — cited directly as precedent for this identical shape, not
  re-argued from scratch), a couple of forwarding method declarations.
- `src/ui/MapCanvas_UI.cpp` / `MapCanvas_Draw_UI.cpp` — `ApplyPointerInput` tries the manual-marker
  hit-test at press-time before falling into the existing pan-vs-click path (see "The drag gesture
  state machine" step 1); drag-time and release-time branches route to the gesture machinery when a
  drag is active, otherwise fall through to today's pan/click behavior unchanged.
- `src/ui/Application_UI.cpp` — `WireCallbacks()` gains one `SetManualMarkerDragSource(...)` call,
  alongside STEP48's `SetMarkerPickingSource`/`SetPreviewComposite` wiring.
- `src/ui/MarkersTab_Manual_UI.cpp` (STEP49's file) — **not edited by this ticket**; see the
  roster-slider coordination note above. Whichever ticket lands second adds the one-line call.
- The relevant `CMakeLists.txt` — register the two new `.cpp`s and their test targets.

## Layer & accuracy class
UI (interaction/state machine) + one call into a PIPELINE passthrough (STEP68) whose own declared
accuracy class is Exact. The interaction itself is Visual — a refused/collapsed frame is a UI
presentation choice, not a numerical-tolerance question.

## Backend policy
CPU only, synchronous, on the UI thread — same posture STEP68 already establishes for the wrapper
this ticket calls. No `Dispatch_SYS` involvement; this authors no PROC stage and no GPU kernel.

## ARCH rules invoked
- `ARCH_16_03_ModuleBoundaryChain.md` §16.3 / `ARCH_14_08_DirtyFlagTiers.md` §14.8 — the PIPELINE
  passthrough this ticket calls every drag frame, and the C2 tier it runs at (§14.8 names this exact
  interaction as C2's motivating case).
- `ARCH_16_05_MarkerTransformFields.md` §16.5 — `symmetryGroupIdentifier`/`layerIndex`, and the R1→R2
  field-drop this ticket's correspondence-table design directly implements.
- `ARCH_16_08_SpawnArmyShrink.md` §16.8 — confirms this ticket's Spawn refusal does not need a new
  PARAMS field/flag; the association is inferred by name, same as the orphan ruling already relies on.
- Constitution §1 — UI sets PARAMS, never simulates; every write in this ticket lands in
  `recipe.markers` via a plain field assignment, never in `Data::PlacementInstances`.
- Constitution §6 — a picked/dragged index is always range-checked before dereference (mirrors the
  discipline STEP48/STEP81 already apply to their own picks/combo binds).
- `ARCH_01_05_FileSizeCeilings.md` — drives the proactive multi-file split above.

## Explicit out-of-scope
- **Layer management** (add/reorder/delete a `MarkerInstanceLayer`, the layer-level symmetry
  authoring control) — `STEP81`'s job entirely.
- **The procedural rules tab / procedural marker symmetry** — `STEP80`'s job; procedural markers are
  select/highlight-only on the canvas (R1 §3, unmodified by R2), never draggable — this ticket adds
  no drag behavior for them.
- **The real overlay/icon rendering system** (atlas paging, LOD, View-toolbar participation,
  `OverlayLayer_UI`) for manual markers — this ticket ships only the minimal stopgap draw described
  above; superseding it with real icons is a future ticket's job.
- **`Data::PlacementInstances` resolution / `manualLayerId`-style correlation column for manual
  markers** — the marker-domain analogue of `ARCH_14_13_OpenItems.md` item 3's Work-Order B, which
  this ticket deliberately routes around (Gap 5) rather than requires.
- **STEP49's per-instance editor itself** (alias/position field widgets, Spawn→Army combo, delete) —
  unmodified except for the one coordination call noted above, which is not wired by this ticket.
- **"Break Symmetry Link" and cascade-delete-on-any-member-delete** — R2 §1 carries these over from
  R1 unchanged; they are STEP49/roster-editor affordances (a button/menu item setting
  `symmetryGroupIdentifier = 0` or removing a roster row), not drag-gesture behavior, and are not
  built here.
- **Fixing `ResolvedPlacementSymmetryMask`'s Radial-bit defect** — flagged, routed around, not
  touched, same posture `STEP81` already established; a shared-widget fix should serve both tickets
  and neither should land a private version.
- **Rotation/scale editing, terrain-height snapping** — out of scope from STEP49, unchanged here.

## Acceptance test
Not dispatchable until STEP47, STEP48, STEP49, STEP68, and STEP81 have all landed for real (STEP81
because this ticket reads `Params::MarkerInstanceLayer`/`recipe.markerLayers`), and until this
ticket's own "minimal manual-marker draw" has been built (it is this ticket's own scope, not an
external blocker). Once buildable:
1. Two markers sharing a `symmetryGroupIdentifier`, mask = `MirrorAcrossX`. Drag one on the canvas;
   confirm the other's position updates every frame (not only at release) to the exact mirrored
   point, and that its `alias`/`name` are untouched.
2. Drag a member onto the mirror axis (cardinality collapses 2→1); confirm the sibling soft-hides
   (ghost/ephemeral, not erased from `recipe.markers`) during the drag, and confirm dragging back off
   the axis restores it live. Release exactly on-axis; confirm the collapse commits (sibling
   removed) only at that release, using the existing cascade-delete machinery.
3. Drag a member off a mask combination that grows the orbit (e.g. adding an off-axis position under
   `MirrorAcrossX|MirrorAcrossZ`); confirm the new point renders as a ghost with zero write to
   `recipe.markers` until release, and that release materializes a real new `MarkerTransform` sharing
   the group's `symmetryGroupIdentifier` and the dragged member's `layerIndex`.
4. A `"Spawn"`-named group: attempt a drag that would change cardinality; confirm the group freezes
   at the last valid position, the refused-tint feedback renders, and after release `recipe.markers`
   still holds exactly the pre-attempt count for that group. A same-count reposition of a Spawn group
   is confirmed unrestricted (same as test 1).
5. An ungrouped marker (`symmetryGroupIdentifier == 0`) drags freely with no orbit call and no effect
   on any other marker.
6. Grep confirms zero calls to `ResolvedPlacementSymmetryMask`/`DrawPlacementSymmetryAxes` from any
   file this ticket creates, and zero calls to `Picking_UI::PickMarker`/`Data::SpatialGrid` from the
   manual-marker hit-test path specifically (procedural picking elsewhere is untouched).
7. Full `SanGenV2` build stays clean; every existing test continues to pass; new unit tests cover
   `MarkerDragGesture_UI`'s pure functions headlessly (correspondence-table build/match, cardinality
   comparison, Spawn-refusal freeze) with no window, mirroring `MarkerLayerIndexRepair_UI_Test.cpp`'s
   posture.

## Landing order
`STEP47` → `STEP48` → `STEP68` (parallel-safe with 47/48) → `STEP49` → `STEP81` → **this ticket**.
STEP81 must precede this ticket specifically because this ticket reads `Params::MarkerInstanceLayer`/
`recipe.markerLayers`, which STEP81 (following STEP60/68's PARAMS work) is what actually lands them.
If schedule pressure reorders STEP49/STEP81 relative to each other, either order is fine for this
ticket as long as both precede it — this ticket does not otherwise depend on their relative order.
