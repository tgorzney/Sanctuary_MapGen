// Picking_UI_Test.cpp — acceptance test for M4-4 (Picking_UI). No GL, standalone.
//   cl /std:c++17 /EHsc /I src src\ui\Picking_UI.cpp src\ui\Picking_UI_Test.cpp
#include "Picking_UI.h"
#include <cstdio>
#include <cstdint>

using namespace SanmapGen;

static int failures = 0;
static void check(bool ok, const char* label) { if (!ok) { std::printf("FAIL: %s\n", label); ++failures; } }

// Horizontal pair is positionX/positionZ; positionY is terrain HEIGHT and is parked far away
// so that any code path reading it as a horizontal coordinate fails every radius test.
static void AppendMarker(Data::PlacementInstances& instances, float x, float z, int ruleIndex) {
    Data::PlacementInstance instance;
    instance.positionX = x; instance.positionY = 4321.0f; instance.positionZ = z;
    instance.ruleIndex = ruleIndex;
    instances.Append(instance);
}

static void TestPickEntity() {
    Data::EntityIdBuffer entityIdBuffer(8, 4);
    entityIdBuffer.Set(3, 2, 77u);
    entityIdBuffer.Set(7, 3, 5u);
    check(Ui::PickEntity(entityIdBuffer, 3, 2) == 77u, "known id at (x,y)");
    check(Ui::PickEntity(entityIdBuffer, 7, 3) == 5u, "known id in the last cell");
    check(Ui::PickEntity(entityIdBuffer, 4, 2) == Data::EntityIdBuffer::emptySentinel, "empty space");
    check(Ui::PickEntity(entityIdBuffer, 3, 1) == Data::EntityIdBuffer::emptySentinel, "empty row");
    // Out of bounds on every side, plus the empty buffer: safe, no read past the end.
    check(Ui::PickEntity(entityIdBuffer, -1, 2) == Data::EntityIdBuffer::emptySentinel, "cursor x < 0");
    check(Ui::PickEntity(entityIdBuffer, 3, -1) == Data::EntityIdBuffer::emptySentinel, "cursor y < 0");
    check(Ui::PickEntity(entityIdBuffer, 8, 2) == Data::EntityIdBuffer::emptySentinel, "cursor x == width");
    check(Ui::PickEntity(entityIdBuffer, 3, 4) == Data::EntityIdBuffer::emptySentinel, "cursor y == height");
    check(Ui::PickEntity(entityIdBuffer, 1 << 20, 1 << 20) == Data::EntityIdBuffer::emptySentinel,
          "huge cursor safe");
    const Data::EntityIdBuffer emptyBuffer;
    check(Ui::PickEntity(emptyBuffer, 0, 0) == Data::EntityIdBuffer::emptySentinel, "empty buffer safe");
}

int main() {
    TestPickEntity();

    // 1024-unit map, 8x8 chunks (128 units per chunk). Chunks: 0 -> {0,1}, 21 -> {2,3}, 63 -> {4}.
    Data::PlacementInstances instances;
    AppendMarker(instances, 100.0f, 100.0f, 0);
    AppendMarker(instances, 110.0f, 100.0f, 1);
    AppendMarker(instances, 700.0f, 300.0f, 2);
    AppendMarker(instances, 712.0f, 300.0f, 3);
    AppendMarker(instances, 900.0f, 900.0f, 4);
    Data::SpatialGrid grid;
    grid.Configure(1024.0f, 8);
    grid.Build(instances.positionX.data(), instances.positionZ.data(),
               static_cast<std::int32_t>(instances.Count()));
    check(grid.EntryCount() == 5, "grid built from the horizontal columns");

    std::int32_t visitedEntryCount = -1;
    // ---- A marker in a chunk is found, and ONLY that chunk's entries are tested.
    const int occupiedCell = grid.CellIndexAt(705.0f, 302.0f);
    const std::int32_t occupiedBucketSize = grid.BucketEnd(occupiedCell) - grid.BucketBegin(occupiedCell);
    check(occupiedCell == 21 && occupiedBucketSize == 2, "cursor chunk holds two markers");
    check(Ui::PickMarker(grid, instances, 705.0f, 302.0f, 20.0f, &visitedEntryCount) == 2,
          "nearest marker in the chunk found");
    check(visitedEntryCount == occupiedBucketSize, "visited count == that bucket's size");
    // The other marker of the same chunk wins when the cursor moves to it — nearest, not first.
    check(Ui::PickMarker(grid, instances, 714.0f, 300.0f, 20.0f, &visitedEntryCount) == 3
          && visitedEntryCount == occupiedBucketSize, "nearest of two in one chunk");
    // A far chunk's markers are never distance-tested: 5 instances exist, 2 were visited.
    check(occupiedBucketSize < grid.EntryCount(), "walk is a strict subset of the index");

    // ---- A click in an empty chunk returns -1 and tests nothing at all.
    const int emptyCell = grid.CellIndexAt(100.0f, 900.0f);
    check(emptyCell == 56 && grid.BucketEnd(emptyCell) == grid.BucketBegin(emptyCell), "chunk 56 empty");
    check(Ui::PickMarker(grid, instances, 100.0f, 900.0f, 500.0f, &visitedEntryCount) == Ui::kNoMarkerPicked,
          "click in an empty chunk returns -1");
    check(visitedEntryCount == 0, "empty chunk tests nothing");

    // ---- A marker just outside pickRadius is rejected; just inside is accepted. Distance is 5.
    check(Ui::PickMarker(grid, instances, 905.0f, 900.0f, 4.9f, &visitedEntryCount) == Ui::kNoMarkerPicked,
          "marker just outside pickRadius rejected");
    check(visitedEntryCount == 1, "the rejected marker was the only entry tested");
    check(Ui::PickMarker(grid, instances, 905.0f, 900.0f, 5.1f, &visitedEntryCount) == 4,
          "marker just inside pickRadius accepted");
    check(Ui::PickMarker(grid, instances, 905.0f, 900.0f, 5.0f) == 4, "marker exactly at pickRadius accepted");

    // ---- Chunk 0 resolves to the SoA columns, and the cursor's own chunk is the one walked.
    const int firstCell = grid.CellIndexAt(109.0f, 100.0f);
    const std::int32_t picked = Ui::PickMarker(grid, instances, 109.0f, 100.0f, 50.0f, &visitedEntryCount);
    check(picked == 1 && instances.ruleIndex[static_cast<std::size_t>(picked)] == 1, "index resolves into the SoA");
    check(visitedEntryCount == grid.BucketEnd(firstCell) - grid.BucketBegin(firstCell), "chunk 0 bucket size");

    // ---- Degenerate input is safe (Constitution §6), never a crash.
    check(Ui::PickMarker(grid, instances, 705.0f, 302.0f, 0.0f, &visitedEntryCount) == Ui::kNoMarkerPicked
          && visitedEntryCount == 0, "non-positive pickRadius picks nothing");
    check(Ui::PickMarker(grid, instances, -9999.0f, -9999.0f, 1.0f) == Ui::kNoMarkerPicked,
          "cursor far off the map clamps and picks nothing");
    const Data::SpatialGrid emptyGrid;
    check(Ui::PickMarker(emptyGrid, instances, 100.0f, 100.0f, 500.0f) == Ui::kNoMarkerPicked, "empty grid safe");
    const Data::PlacementInstances emptyInstances;
    check(Ui::PickMarker(grid, emptyInstances, 705.0f, 302.0f, 20.0f) == Ui::kNoMarkerPicked,
          "empty instance buffer safe");

    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}
