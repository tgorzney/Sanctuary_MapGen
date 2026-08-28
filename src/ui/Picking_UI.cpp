// Picking_UI.cpp — O(1) cursor resolution. Layer: UI, pure CPU, no GL, no mutation.
// The marker path compares SQUARED distances so the inner loop has no square root, and it
// walks one chunk's flat CSR range — contiguous std::int32_t entries, one cache line per
// query (ARCH §8.3).
#include "Picking_UI.h"
#include <cstddef>

namespace SanmapGen {
namespace Ui {
namespace {

// How many instances both horizontal columns actually address. A short column is malformed
// input, not a reason to read past its end (Constitution §6).
std::int32_t HorizontalInstanceCount(const Data::PlacementInstances& instances) {
    const std::size_t horizontalCount = instances.positionX.size() < instances.positionZ.size()
                                      ? instances.positionX.size() : instances.positionZ.size();
    return static_cast<std::int32_t>(horizontalCount);
}

} // namespace

std::uint32_t PickEntity(const Data::EntityIdBuffer& entityIdBuffer, int cursorX, int cursorY) {
    if (cursorX < 0 || cursorY < 0) return Data::EntityIdBuffer::emptySentinel;
    if (cursorX >= entityIdBuffer.Width() || cursorY >= entityIdBuffer.Height())
        return Data::EntityIdBuffer::emptySentinel;   // also covers the empty (0x0) buffer
    return entityIdBuffer.Get(cursorX, cursorY);
}

std::int32_t PickMarker(const Data::SpatialGrid& grid, const Data::PlacementInstances& instances,
                        float worldX, float worldY, float pickRadius,
                        std::int32_t* visitedEntryCount) {
    if (visitedEntryCount != nullptr) *visitedEntryCount = 0;
    const std::int32_t instanceCount = HorizontalInstanceCount(instances);
    // The !(pickRadius > 0) form also traps a NaN radius, which would accept nothing anyway.
    if (instanceCount <= 0 || !(pickRadius > 0.0f)) return kNoMarkerPicked;

    const int cellIndex = grid.CellIndexAt(worldX, worldY);   // the one world->cell mapping
    const std::int32_t bucketBegin = grid.BucketBegin(cellIndex);
    const std::int32_t bucketEnd = grid.BucketEnd(cellIndex);
    const std::int32_t entryCount = grid.EntryCount();
    const std::int32_t walkEnd = bucketEnd < entryCount ? bucketEnd : entryCount;

    const float radiusSquared = pickRadius * pickRadius;
    float nearestDistanceSquared = radiusSquared;
    std::int32_t nearestInstance = kNoMarkerPicked;
    std::int32_t visitedEntries = 0;
    for (std::int32_t position = bucketBegin; position < walkEnd; ++position) {
        ++visitedEntries;
        const std::int32_t instance = grid.InstanceIndexAt(position);
        if (instance < 0 || instance >= instanceCount) continue;
        const std::size_t column = static_cast<std::size_t>(instance);
        const float offsetX = instances.positionX[column] - worldX;   // positionZ, not positionY:
        const float offsetY = instances.positionZ[column] - worldY;   // positionY is height.
        const float distanceSquared = offsetX * offsetX + offsetY * offsetY;
        if (distanceSquared > radiusSquared) continue;
        if (nearestInstance < 0 || distanceSquared < nearestDistanceSquared) {
            nearestDistanceSquared = distanceSquared;
            nearestInstance = instance;
        }
    }
    if (visitedEntryCount != nullptr) *visitedEntryCount = visitedEntries;
    return nearestInstance;
}

void PickInstancesInRegion(const Data::SpatialGrid& grid, const Data::PlacementInstances& instances,
                           float worldMinX, float worldMinY, float worldMaxX, float worldMaxY,
                           std::vector<std::int32_t>& outInstanceIndices) {
    outInstanceIndices.clear();
    const std::int32_t instanceCount = HorizontalInstanceCount(instances);
    // A degenerate/NaN box (min > max, or either bound non-finite in a way that breaks the
    // exact-position test below) answers empty rather than silently scanning the whole grid.
    if (instanceCount <= 0 || !(worldMaxX >= worldMinX) || !(worldMaxY >= worldMinY)) return;

    const int minCellX = grid.CellCoordinateXAt(worldMinX);
    const int maxCellX = grid.CellCoordinateXAt(worldMaxX);
    const int minCellY = grid.CellCoordinateYAt(worldMinY);
    const int maxCellY = grid.CellCoordinateYAt(worldMaxY);
    const std::int32_t entryCount = grid.EntryCount();

    for (int cellY = minCellY; cellY <= maxCellY; ++cellY) {
        for (int cellX = minCellX; cellX <= maxCellX; ++cellX) {
            const int cellIndex = grid.CellIndexAtCoordinate(cellX, cellY);
            const std::int32_t bucketBegin = grid.BucketBegin(cellIndex);
            const std::int32_t bucketEnd = grid.BucketEnd(cellIndex);
            const std::int32_t walkEnd = bucketEnd < entryCount ? bucketEnd : entryCount;
            for (std::int32_t position = bucketBegin; position < walkEnd; ++position) {
                const std::int32_t instance = grid.InstanceIndexAt(position);
                if (instance < 0 || instance >= instanceCount) continue;
                const std::size_t column = static_cast<std::size_t>(instance);
                const float positionX = instances.positionX[column];
                const float positionY = instances.positionZ[column];   // positionZ, not positionY: see PickMarker's own comment
                if (positionX < worldMinX || positionX > worldMaxX
                    || positionY < worldMinY || positionY > worldMaxY)
                    continue;   // this cell straddles the box edge; this candidate falls outside it
                outInstanceIndices.push_back(instance);
            }
        }
    }
}

} // namespace Ui
} // namespace SanmapGen
