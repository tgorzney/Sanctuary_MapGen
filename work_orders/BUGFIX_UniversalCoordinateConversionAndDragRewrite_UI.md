# Fix marker/prop/decal drag: universal coordinate conversions, group move, cell-center grid snap

**Layer:** UI. **Executor:** SanGen Coder. Replaces the two prior drafts for this same bug report
(now deleted) — this version is simpler because it fixes the actual gap instead of working around
it: there is currently no single, reusable way to convert between screen space, world space, and
grid space. Each caller reinvents a fragment of that math inline, and that's the real reason the
old drag/snap code (and the first draft of this fix) got complicated. Fix the gap first; drag and
snap become simple once it exists.

## Part 1 — six universal conversion functions (new file, `src/ui/CoordinateSpace_UI.h`)

Units: 1 world unit = the base unit (what the game stores positions in). 1 grid cell =
`Params::Geometry::worldUnitsPerCell` world units (10, for this game). Screen space = region-local
pixels (top-left of the canvas widget), the same space every existing gesture already works in.

```cpp
struct WorldPoint  { float worldX = 0.0f;  float worldZ = 0.0f; };
struct ScreenPoint { float screenX = 0.0f; float screenY = 0.0f; };
struct GridPoint   { float gridX = 0.0f;   float gridZ = 0.0f; };   // float, not int — a mid-cell
                                                                    // position stays meaningful
                                                                    // (only snapping rounds it)

// screen <-> world. Continuous — NEVER floors/rounds internally (that was the root cause of the
// "large increment" drag bug: today's code floors to an integer preview-texel before converting).
WorldPoint  ScreenToWorld(const MapCanvasView& view, const PreviewComposite& composite, ScreenPoint screen);
ScreenPoint WorldToScreen(const MapCanvasView& view, const PreviewComposite& composite, WorldPoint world);

// world <-> grid, at a given cell size in world units (pass geometry.worldUnitsPerCell for "the
// real terrain grid"; pass a multiple of it for a coarser snap grid — see Part 3). GridToWorld
// always returns the CENTER of the cell, never a corner/vertex — that's what makes every caller
// downstream automatically correct, with no separate "+half a cell" step anywhere else.
GridPoint  WorldToGrid(float cellSizeWorldUnits, WorldPoint world);
WorldPoint GridToWorld(float cellSizeWorldUnits, GridPoint grid);

// screen <-> grid — pure composition of the four above, provided for convenience/discoverability.
GridPoint   ScreenToGrid(const MapCanvasView& view, const PreviewComposite& composite,
                         float cellSizeWorldUnits, ScreenPoint screen);
ScreenPoint GridToScreen(const MapCanvasView& view, const PreviewComposite& composite,
                         float cellSizeWorldUnits, GridPoint grid);
```

Implementation notes:
- `ScreenToWorld`/`WorldToScreen` are literally today's `MapCanvasView::ResolvePreviewPixel` +
  `PreviewComposite::PreviewPixelToWorld` / `PreviewComposite::WorldToPreviewPixel` +
  `MapCanvasView::ProjectPreviewPixelToRegionLocal`, composed — MINUS the integer floor
  `ResolvePreviewPixel` currently applies. Do not change `ResolvePreviewPixel`'s own existing
  int-returning form or any of its current callers (picking/hit-testing still wants the discrete
  texel) — add the continuous composition as new code alongside it.
- `WorldToGrid`: `{ floor(world.worldX / cellSizeWorldUnits), floor(world.worldZ / cellSizeWorldUnits) }`.
- `GridToWorld`: `{ (grid.gridX + 0.5f) * cellSizeWorldUnits, (grid.gridZ + 0.5f) * cellSizeWorldUnits }`.
- Both guard `cellSizeWorldUnits <= 0` (return input unchanged) — Constitution §6, never a divide by
  zero.
- `MapCanvas_MarkerRosterDraw_UI.cpp`'s existing `ProjectWorldToScreen` becomes a one-line call to
  `WorldToScreen` (same output, just routed through the new shared function instead of a
  file-private one-off).

## Part 2 — drag: delta-based, one code path for one instance or many

Today, single-instance drag snaps the instance directly onto the cursor's world position every
frame (loses whatever offset you originally clicked at inside the icon), and multi-select drag
doesn't move the other selected instances at all. Replace both with one rule:

1. **Mouse-down:** `dragAnchorWorld = ScreenToWorld(cursor)`. Record every currently-selected
   instance's own CURRENT world position (`startWorldX/Z`) — if only one instance is being dragged
   (nothing else selected, or the grabbed instance isn't part of the current selection), that set is
   just the one instance.
2. **Every frame while dragging (live, so the canvas visibly follows the cursor — not just on
   release):** `currentWorld = ScreenToWorld(cursor)`; `delta = currentWorld - dragAnchorWorld`.
   For each recorded instance: `targetPosition = instance.startWorld + delta`, then grid-snap it
   (Part 3) if that instance's layer has snap on, then write it. This is exactly today's existing
   per-instance write step (`InstanceDragGesture_UI.h`'s `UpdateInstanceDragGesture` +
   `Traits::QuantizePositionToLayerGrid`) — unchanged — just fed `startWorld + delta` instead of the
   raw cursor position.
3. **Symmetry:** unchanged existing behavior, applied per selected instance — an instance whose own
   effective symmetry is on still drags its whole orbit along with it (today's existing
   `BuildWorldSymmetryOrbit`/correspondence-matching machinery, verbatim); an instance with symmetry
   off moves alone. Two selected instances that happen to be each other's mirror copy: only apply
   the delta once per instance (skip a selected instance if an earlier selected instance in the same
   symmetry group already claims it) — otherwise its orbit-follow write and its own direct write
   would race.
4. **Mouse-up:** finalize exactly as today (existing `EndInstanceDragGesture` per instance —
   materializes/cascades any orbit cardinality change, unchanged).

Mechanically: keep one `InstanceDragGestureState` per dragged instance (today's existing, unchanged,
already-tested struct/functions) — just call `Begin`/`Update`/`End` once per selected instance
instead of once for the single grabbed one, and feed each `Update` call `startWorld + delta` instead
of the cursor's raw world position. No new state machine, no per-instance-count special casing.

## Part 3 — grid snap: setting is a whole-number cell MULTIPLIER, always snaps to a cell CENTER

`MarkerInstanceLayer::gridSnapSizeWorldUnits` (and the Props/Decals equivalents) currently store a
raw, arbitrary world-unit distance with no relationship to the terrain, and round to the nearest
multiple of it from world origin — landing on vertices, not cell centers. There is exactly ONE
source of truth for "how big is a terrain cell in world units": `Params::Geometry::worldUnitsPerCell`
(confirmed by direct read — every generation/placement/mask stage already reads this one field, and
`Mask_Prepare_PROC.cpp:21` documents it in so many words as "the ONE owner of cell world-size"). Do
NOT introduce a second field/constant that duplicates that fact. Change:

- The field now means **snap every N terrain cells** — a whole number, minimum 1 (confirmed with the
  human: yes to keeping this as an optional multiplier). Rename to `gridSnapSizeCellMultiplier` (or
  similar — Format Expert's call on the exact name) in the three PARAMS structs. Displayed/edited in
  the UI as a plain integer count, not a world-unit distance. Default `1` = snap to the map's own
  terrain grid directly, cell-centered, nothing further to configure.
- Snapping a position: `effectiveCellSize = gridSnapSizeCellMultiplier * geometry.worldUnitsPerCell`,
  then `snapped = GridToWorld(effectiveCellSize, WorldToGrid(effectiveCellSize, position))` — i.e.
  Part 1's two functions applied back-to-back, at that scaled cell size. Because `GridToWorld` always
  returns a center, this is automatically correct for every whole-number multiplier (1, 2, 3...) with
  no separate rounding/parity logic needed anywhere — the bug in the first draft (uneven block sizes
  from mixing `round()` and `floor()`) doesn't exist here because there's only one floor step, reused
  consistently.
- `QuantizeMarkerPositionToLayerGrid`/`QuantizePropPositionToLayerGrid`/`QuantizeDecalPositionToLayerGrid`
  (`MarkersTab_ManualLayerHelpers_UI.h`, `PropsTab_ManualLayerHelpers_UI.h`,
  `DecalsTab_ManualLayerHelpers_UI.h`) each gain a `const Params::Geometry&` parameter and their body
  becomes the one-line call above. `MarkerDragTraits`/`PropDragTraits`/`DecalDragTraits`'s
  `QuantizePositionToLayerGrid` wrappers (and `InstanceDragGesture_UI.h`'s existing call sites, which
  already have `geometry` in scope) forward it through — mechanical, `geometry` is already a
  parameter one level up in every case.

### Saved-map compatibility (minor, flagging not deciding)
Existing `.sanmap` files already have this field populated as a world-unit float (default `1.0`).
Reinterpreted directly as a whole-number cell multiplier, the common default case (`1.0` old ->
`1` new) happens to mean the same thing either way — every OTHER previously-customized value shifts
meaning. Given this is an authoring convenience (not gameplay data), recommend shipping as a plain
behavior change with no migration, but flag to the Format/IO Architecture Expert in case they'd
rather add a one-time conversion step for consistency with how other field reinterpretations are
normally handled in this codebase.

### Separate, optional decision: should a brand-new map default to `worldUnitsPerCell = 10`?
Not part of this bug fix — `Geometry_PARAMS.h:32`, one line (`= 1.0f` -> `= 10.0f`), the only place a
new map's default comes from. Flagging because it came up in discussion; do only if the human asks
for it explicitly, separately from this ticket.

## Files touched
**New:** `src/ui/CoordinateSpace_UI.h` (+ its own test file).
**Modified:** `MapCanvas_ManualDragDispatch_UI.cpp` (drag now delta-based, per selected instance),
`MapCanvas_UI.h` (per-instance state list instead of one single-instance state, per domain),
`MapCanvas_MarkerRosterDraw_UI.cpp` (`ProjectWorldToScreen` → calls `WorldToScreen`),
`MarkersTab_ManualLayerHelpers_UI.h`, `PropsTab_ManualLayerHelpers_UI.h`,
`DecalsTab_ManualLayerHelpers_UI.h` (geometry-aware quantize), `MarkerDragGesture_UI.h`,
`PropDragGesture_UI.h`, `DecalDragGesture_UI.h` (forward `geometry`), the Markers/Props/Decals tab UI
that draws the "Grid Size" field (relabel to grid units), plus every test file exercising the
functions above.

## Session coordination (required — this repo runs multiple concurrent Claude Code sessions)
Before editing OR reading-to-edit (which locks) **each individual file** in this ticket — not once
at the start of the ticket, every time, immediately before you touch that specific file — run
`ListAgents` and message every peer session naming that exact file, confirming none of them are
currently editing or have it open for edit. Do not treat an earlier check (even one done minutes ago
for a different file in this same ticket) as still valid for a later file — a peer session can start
touching any file at any point during a long multi-file ticket. If a peer reports they're already in
that file, negotiate (wait, or take a different file first) rather than proceeding anyway.

Separately: even with fully disjoint file lists, two coders building at the same time can still
collide on the shared `build/` directory (PDB lock, `C1041`) — this is a known, benign, transient
collision, not a real compile failure. If you hit it, check `Get-Process`/`Get-Process cl` for a
peer's build still running, wait for it to finish, and retry — do not "fix" the code in response to
it.

## Acceptance test
1. At any zoom level, dragging a marker one screen pixel moves it by that pixel's actual world-unit
   span, continuously — no frozen frames, no multi-unit jumps, confirmed at a map size where the old
   code demonstrably froze (mapSize=1024).
2. Grid snap on, size = 1 grid unit: every snapped position is an exact terrain-cell center.
3. Grid snap on, size = 2+ grid units: every snapped position is still an exact cell center of a
   correctly-sized, evenly-spaced block — no lopsided block at the origin.
4. Grid snap off: position is untouched, byte-identical to today.
5. Select 5 unrelated markers, drag one: all 5 move by the same world-space delta; nothing
   unselected moves.
6. Within that selection, a marker with symmetry on also drags its orbit; one with symmetry off
   doesn't. Both members of an existing mirror pair, if both individually selected, end up in the
   single correct place (no double-write).
7. Drag a single marker not part of any selection: behaves the same shape as before (moves, orbit
   follows if symmetric) but now preserves the click's grab-offset instead of snapping the marker's
   origin onto the cursor — call this out in the PR as an intentional, minor UX improvement, not a
   silent one.
8. Full `SanGenV2` build stays clean; existing test suites pass (with expected-value updates
   wherever old vertex-snap values are asserted).
