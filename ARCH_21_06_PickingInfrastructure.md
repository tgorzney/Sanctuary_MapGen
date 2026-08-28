[← ARCH index](ARCH.md) · [§21 ARCH_21_CanvasInteractionUnification](ARCH_21_CanvasInteractionUnification.md) · SanGen ARCH §21.6. **Only the ARCH Expert writes this file.**

### 21.6 Picking infrastructure — `Data::SpatialGridSet`, `BuildSpatialGridSet`, three new `SpatialGrid` accessors, `PickInstancesInRegion` (renamed from the design's `PickMarkersInRegion`)

**Ratified as designed, with one naming correction below** — surfaced by direct re-read of
`SpatialGrid_DATA.h`/`Picking_UI.h`/`GenerationAssembler_PIPELINE.h`/
`GenerationAssembler_Stages_PIPELINE.cpp`; nothing else in the relayed shape conflicts.

**`Data::SpatialGridSet`** (new `src/data/SpatialGridSet_DATA.h`) — a byte-identical structural
mirror of the already-shipped `Data::RuleBucketIndexSet` (`RuleBucketIndexSet_DATA.h`):
```cpp
struct SpatialGridSet {
    SpatialGrid markers, props, units, decals;
    void Clear() { markers.Clear(); props.Clear(); units.Clear(); decals.Clear(); }
};
```
`GenerationAssembler` (`GenerationAssembler_PIPELINE.h`) replaces its single
`Data::SpatialGrid markerSpatialGrid` member with `Data::SpatialGridSet spatialGridSet`. The
accessor shape mirrors `RuleBucketIndex()`'s own already-established precedent exactly — one
accessor returning the whole set by const reference, not four separate per-collection accessors
(confirmed: `RuleBucketIndex()` returns `const Data::RuleBucketIndexSet&`, and its one real caller,
`Application_UI.cpp:117`, reads `.markers`/etc. off the returned set directly):
```cpp
const Data::SpatialGridSet& SpatialGridSet() const { return spatialGridSet; }
// Thin back-compat wrapper — every existing caller (Application_UI.cpp:111) compiles unchanged.
const Data::SpatialGrid& MarkerSpatialGrid() const { return spatialGridSet.markers; }
```
`BuildMarkerSpatialGrid()` generalizes to `BuildSpatialGridSet()` — four Configure+Build call pairs
(markers/props/units/decals), one per `Data::PlacementResults` collection, same posture as the
already-four-way `BuildRuleBucketIndex()` immediately beside it (`GenerationAssembler_Stages_PIPELINE.cpp:45-66`).
Still built inside `Placement`'s registered run, immediately after `placementStage.Run()` — unchanged
single-writer lifecycle (§8.3, §3.4.1).

**Three new `Data::SpatialGrid` accessors** (additive, zero behavior change to any existing caller)
exposing the row/column split the existing PRIVATE `ClampedCellCoordinate` already computes:
```cpp
int CellCoordinateXAt(float worldX) const { return ClampedCellCoordinate(worldX); }
int CellCoordinateYAt(float worldY) const { return ClampedCellCoordinate(worldY); }
// Inputs are ALREADY-CLAMPED cell coordinates (from the two accessors above) — this does NOT
// re-clamp; it is the other half of CellIndexAt's own formula, split so a region query can compute
// its cellX/cellY span once and walk it without re-deriving CellIndexAt's multiply per cell.
int CellIndexAtCoordinate(int cellX, int cellY) const { return cellY * chunkResolution + cellX; }
```

**The region-query function — corrected name.** The relayed design proposes
`Picking_UI::PickMarkersInRegion(grid, instances, ...)`, but its own stated contract is fully
generic — "over any `(SpatialGrid, PlacementInstances)` pair (so it serves all four collections...
not duplicated per-collection)." A function whose whole point is domain-neutrality may not carry
"Markers" in its name (ARCH §1.1) — that collides with `PickMarker`'s own existing, genuinely
Marker-specific name (today's only working single-point picker, per this file's own header comment:
"Today only markers have a working picker"). **Ruled: `PickInstancesInRegion`.**
```cpp
void PickInstancesInRegion(const Data::SpatialGrid& grid, const Data::PlacementInstances& instances,
                           float worldMinX, float worldMinY, float worldMaxX, float worldMaxY,
                           std::vector<std::int32_t>& outInstanceIndices);
```
Clears `outInstanceIndices` first (a single-call contract, mirroring `SpatialGrid::Build`'s own
"replaces... never accumulates" posture) — never appends onto a caller's stale contents. Walks the
cell span the box covers (`CellCoordinateXAt`/`YAt` on the box's min/max corners, then
`CellIndexAtCoordinate` per cell in that row×column range), exact-position-tests every candidate
against the box (a chunk cell can extend past the query box at its own edges — cell-membership
alone is not sufficient), and appends every match — the same "index accelerator, then exact test"
shape `PickMarker` already uses for a single point, generalized to a rectangle.

**Consumer scope for this ratification: Markers/Props/Decals only.** `Data::SpatialGridSet` is
built 4-way (mirroring `RuleBucketIndexSet`'s own precedent of shipping full-width infrastructure
ahead of every consumer existing), but §21.1-§21.5's selection/drag work has no Units consumer — no
`UnitTransform::instanceIdentifier` field, no `UnitDragGesture`, no click/marquee UI for Units is
ratified here. `spatialGridSet.units` exists and is built correctly from day one; wiring a Units
picker onto it is future, unscoped work.
