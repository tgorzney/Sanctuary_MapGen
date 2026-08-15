// Picking_UI.h — resolve the entity under the cursor in O(1). Layer: UI.
// Two independent O(1) mechanisms, never a linear scan of 100k items (UI_FRAMEWORK_SPEC):
// a rendered entity is read out of the already-read-back Data::EntityIdBuffer (the GPU
// readback itself is PreviewComposite_UI/SYS, M4-3 — nothing here touches GL), and an
// interactive marker is found by hashing into one Data::SpatialGrid chunk.
// Both functions are pure functions of their inputs: they mutate no DATA they read.
#pragma once
#include <cstdint>
#include "../data/EntityIdBuffer_DATA.h"
#include "../data/PlacementInstances_DATA.h"
#include "../data/SpatialGrid_DATA.h"

namespace SanmapGen {
namespace Ui {

// Returned when no marker lies within the pick radius. Distinct from the entity-buffer's
// Data::EntityIdBuffer::emptySentinel: this one is an INDEX into Data::PlacementInstances,
// so its "nothing" value is a negative index, not a sentinel id.
enum : std::int32_t { kNoMarkerPicked = -1 };

// The id rendered at pixel (cursorX, cursorY), or Data::EntityIdBuffer::emptySentinel for
// empty space. An out-of-bounds cursor — negative, past the width/height, or any cursor at
// all against an empty buffer — is safe and answers emptySentinel; it never reads past the
// end (Constitution §6: validate input rather than trust the caller's mouse).
std::uint32_t PickEntity(const Data::EntityIdBuffer& entityIdBuffer, int cursorX, int cursorY);

// The index of the marker nearest to (worldX, worldY) within pickRadius, or kNoMarkerPicked.
//
// Only the ONE chunk containing the cursor is tested: the cell comes from
// `grid.CellIndexAt(worldX, worldY)` — the single source of the world->cell arithmetic
// (ARCH §8.3; a second copy here is exactly how a picker drifts from its index) — and the
// walk covers only that cell's [BucketBegin, BucketEnd) range.
//
// (worldX, worldY) are the two HORIZONTAL world coordinates, named as the axis-agnostic grid
// names them. In Data::PlacementInstances the horizontal pair is `positionX`/`positionZ`
// (`positionY` is terrain HEIGHT), so this compares against those two columns and the grid
// must have been Built from the same two columns (Generation_PIPELINE, M4-5).
//
// A marker exactly at pickRadius is accepted; anything beyond it is rejected. Ties keep the
// earlier bucket entry, so a pick is deterministic. A non-positive or NaN pickRadius, an
// empty grid, an empty instance buffer, and an index that does not address the SoA are all
// safe and answer kNoMarkerPicked.
//
// `visitedEntryCount`, when given, receives how many bucket entries were actually distance-
// tested — the instrumentation that proves the O(1) claim (it equals the chunk's bucket size).
std::int32_t PickMarker(const Data::SpatialGrid& grid, const Data::PlacementInstances& instances,
                        float worldX, float worldY, float pickRadius,
                        std::int32_t* visitedEntryCount = nullptr);

} // namespace Ui
} // namespace SanmapGen
