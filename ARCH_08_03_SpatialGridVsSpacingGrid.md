[← ARCH index](ARCH.md) · [§8 ARCH_08_M4Resolutions](ARCH_08_M4Resolutions.md) · SanGen ARCH §8.3. **Only the ARCH Expert writes this file.**

### 8.3 `SpatialGrid_DATA` vs `Placement_SpacingGrid_PROC` — two different structures
**Ruling: they are unrelated and both stay. `Placement_SpacingGrid_PROC`'s header comment
is correct** and is not to be "reconciled" with §5.1.

| | `Proc::SpacingGrid` (`src/proc/Placement_SpacingGrid_PROC.h`) | `Data::SpatialGrid` (`src/data/SpatialGrid_DATA.h`, new) |
| --- | --- | --- |
| Purpose | Poisson **min-spacing rejection** during scatter | **Hit-test** acceleration for the UI cursor |
| Cell size | the rule's spacing radius (varies per rule) | the map divided into a fixed chunk count |
| Lifetime | transient, inside one Placement run | persistent DATA, survives to the UI |
| Contents | accepted candidate positions | indices into the resolved instance arrays |
| Layer | PROC (a scatter accelerator) | DATA (a computed index) |

`SpatialGrid_DATA` is the §5.1 replacement for the v1 `GenerationParams::MarkerSpatialGrid`
(32×32 `MarkerChunk`s). M4-4's `PickMarker` takes **`const Data::SpatialGrid&`**; the
legacy name `MarkerSpatialGrid` does not appear anywhere in `src/`.

Binding shape decisions:
- **Indices, not string keys.** The v1 chunk stored `std::vector<std::string> MarkerKeys`;
  v2 stores `std::int32_t` indices into `Data::PlacementInstances` (the resolved SoA).
  A pick returns an index; the caller reads the SoA columns it needs. String keys in a hot
  hit-test path are a cache-coherence defect, and `PlacementInstances` is already the SoA
  of record.
- **Flat CSR buckets, not `vector<vector<>>`** — `bucketStart[cellCount + 1]` +
  `instanceIndex[total]`, two contiguous arrays. Same reason the instance buffer is a real
  SoA: one allocation, one cache line per chunk query.
- **The cell hash lives on the DATA type** as a pure accessor
  (`CellIndexAt(worldX, worldY)`), so the builder and the picker share exactly one
  world→cell mapping. Two copies of that arithmetic is how a picker silently drifts from
  its index.
- **Chunk resolution is a tweakable** (Constitution §8), defaulted to 32, carried on the
  grid — not a hardcoded 32 in two places as in v1.
- **Single writer (§3.4.1): `Generation_PIPELINE`, immediately after the Placement stage.**
  The grid is a derived index over `PlacementInstances`, not a new physical quantity, so it
  needs no PROC stage of its own; the container exposes a mechanical `Build(...)` exactly as
  `EntityIdBuffer_DATA` exposes `Set(...)`, and PIPELINE is its only caller. `Picking_UI`
  is strictly a reader.

