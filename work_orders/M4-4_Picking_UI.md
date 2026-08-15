# Work-Order M4-4 — `Picking_UI` (O(1) selection)

*Constitution §7. Milestone M4. **BATCH 2 (parallel, after Batch 0 + Batch 1).** Depends
on the M4-1 (`EntityIdBuffer_DATA`) and M4-0b (`SpatialGrid_DATA`) headers, read-only.
Own files, independent of M4-3. Executor: SanGen Coder.*

## Title
Resolve the entity under the cursor in O(1) from the entity-ID buffer + spatial grid.

## Root problem
`UI_FRAMEWORK_SPEC`: selection must not test 100k items. Two O(1) mechanisms: read the
`EntityIdBuffer` pixel under the cursor for rendered entities, and hash into the spatial
grid chunk for interactive markers.

## Target files
- `src/ui/Picking_UI.h` / `.cpp` (+ `_Test.cpp`).

## Layer & accuracy
`UI`. Reads DATA; no GL (consumes the already-read-back `EntityIdBuffer`).

## Solution
- `PickEntity(const Data::EntityIdBuffer&, int cursorX, int cursorY) -> std::uint32_t` —
  returns the ID under the cursor or `Data::EntityIdBuffer::emptySentinel`.
- `PickMarker(const Data::SpatialGrid& grid, const Data::PlacementInstances& instances,
  float worldX, float worldY, float pickRadius) -> std::int32_t` — calls
  `grid.CellIndexAt(worldX, worldY)`, walks **only** that chunk's `[begin, end)` index
  range, and returns the nearest instance index within `pickRadius`, or `-1`.

**Type note (ARCH §8.3, binding):** the parameter is `const Data::SpatialGrid&` from
`src/data/SpatialGrid_DATA.h`. The legacy name `MarkerSpatialGrid` (v1
`core/Parameters.h`) must not appear in `src/`, and `Proc::SpacingGrid`
(`src/proc/Placement_SpacingGrid_PROC.h`) is a **different structure** — a transient
Poisson min-spacing accelerator — and must not be substituted. Do **not** re-derive the
world→cell mapping here: call `grid.CellIndexAt`, which is the single source of that
arithmetic.

Both functions are pure functions of their inputs — trivially testable, no GL, no
mutation of the DATA they read.

## Acceptance
A buffer with a known ID at (x,y) returns that ID; empty space returns `emptySentinel`;
an out-of-bounds cursor is safe (returns `emptySentinel`, no read past the end). Marker
pick: a marker in a chunk is found; a click in an empty chunk returns `-1`; a marker just
outside `pickRadius` is rejected; and **only that chunk's entries are tested** (instrument
the visited count and assert it equals that bucket's size). ALL PASS. Files within §1.5
ceilings.

## Out of scope
Reading the buffer back from the GPU (composite/SYS side, M4-3); **building** the spatial
grid (`SpatialGrid::Build` is M4-0b; calling it after Placement is M4-5); selection state /
highlight UI (M5). If `src/data/SpatialGrid_DATA.h` is absent, **stop and report** — do not
create it here and do not fall back to a `core/` type (ARCH §8.4).
