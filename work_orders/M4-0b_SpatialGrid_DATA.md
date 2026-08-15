# Work-Order M4-0b — `SpatialGrid_DATA` (chunked hit-test index)

*Constitution §7. Milestone M4. **BATCH 0 (parallel with M4-0a, M4-1). Must land before
M4-4 starts.** Own files, no dependencies beyond the existing `PlacementInstances_DATA`
header (read-only). Executor: SanGen Coder.*

## Title
The persistent chunk grid that turns "what did I click?" into a one-chunk test.

## Root problem
ARCH §5.1 lists `SpatialGrid_DATA` as the v2 replacement for
`GenerationParams::MarkerSpatialGrid` (the v1 32×32 `MarkerChunk` grid in
`core/Parameters.h`), but it does not exist in `src/data/` yet, so M4-4's `PickMarker`
has nothing legal to take. `Proc::SpacingGrid` (`src/proc/Placement_SpacingGrid_PROC.h`)
is **not** this structure and must not be reused for it — it is a transient Poisson
min-spacing accelerator whose cell size is the rule's spacing radius. Ruled in
**ARCH §8.3** (table of the two structures).

## Target files
- `src/data/SpatialGrid_DATA.h` (+ `_Test.cpp`).

## Layer & accuracy
`DATA`. Computed index over `Data::PlacementInstances`. No GPU handles, no PARAMS
dependency, no sim logic.

## Solution (shapes are ARCH §8.3 rulings, not coder choices)
`class Data::SpatialGrid` — a uniform chunk grid over the square map:
- Configuration: `chunkResolution` (tweakable, default **32** → 32×32 chunks) and
  `mapWorldSize`; both stored, neither hardcoded at a use site.
- **Flat CSR buckets**, two contiguous arrays — `std::vector<std::int32_t> bucketStart`
  (size `chunkResolution² + 1`) and `std::vector<std::int32_t> instanceIndex` (size =
  total entries). No `vector<vector<>>`.
- **Indices, not string keys.** Entries are `std::int32_t` indices into
  `Data::PlacementInstances` (the resolved SoA). The v1 `std::vector<std::string>
  MarkerKeys` is retired.
- `int CellIndexAt(float worldX, float worldY) const` — the **single** world→cell
  mapping, clamped to range, shared by the builder and the picker (ARCH §8.3: two copies
  of this arithmetic is how a picker drifts from its index).
- Bucket read accessors returning the `[begin, end)` range of `instanceIndex` for a cell.
- `void Build(const float* positionX, const float* positionY, std::int32_t count)` —
  mechanical two-pass counting fill (count per cell → prefix sum → scatter), exactly as
  `EntityIdBuffer_DATA` exposes `Set`. **Single writer (ARCH §3.4.1 / §8.3):
  `Generation_PIPELINE`, immediately after the Placement stage — it is the only caller.**
- `Clear()`; empty grid is valid and answers every query with an empty range.

## Performance basis
Cycle-counted, not measured: a pick becomes `CellIndexAt` (2 multiplies + 2 clamps) plus
one contiguous span of ~`count / 1024` indices, versus the linear 100k-instance scan it
replaces. Build is O(count) with two passes over two contiguous arrays.

## Acceptance
Build over a known scatter puts each instance in the chunk its position maps to; the
`bucketStart` prefix sum is monotonic with `bucketStart[cellCount] == count`;
`CellIndexAt` clamps out-of-range coordinates instead of indexing out of bounds; an empty
grid and a `count == 0` build are safe; rebuilding over the same object replaces (does not
accumulate). ASan/UBSan clean. Files within §1.5 ceilings.

## Out of scope
The pick query itself (M4-4). Wiring `Build` into the pipeline after Placement (M4-5).
Any 3D / multi-level grid, any dynamic insert-remove — rebuild is the only mutation path.
