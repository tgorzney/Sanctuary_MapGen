// SpatialGrid_DATA.h — the persistent chunk grid that turns a UI pick into a one-chunk test.
// Layer: DATA — a computed index over Data::PlacementInstances (no GPU handles, no PARAMS, no
// sim logic), replacing the v1 GenerationParams::MarkerSpatialGrid 32x32 MarkerChunk grid:
// flat CSR buckets of std::int32_t indices into the resolved SoA, never string keys, never
// vector<vector<>>. NOT Proc::SpacingGrid, a transient Poisson min-spacing accelerator sized
// by the rule's spacing radius (both rulings: ARCH §8.3). Single writer (ARCH §3.4.1):
// Generation_PIPELINE, right after Placement; rebuild is the only mutation path.
#pragma once
#include <vector>
#include <cstdint>
#include <cstddef>

namespace SanmapGen {
namespace Data {

class SpatialGrid {
public:
    static constexpr int defaultChunkResolution = 32;  // 32x32 chunks; tweakable (Constitution §8)
    SpatialGrid() { Configure(1.0f, defaultChunkResolution); }

    // Chunk count and world extent are both stored, neither hardcoded at a use site.
    // Reconfiguring drops any built index — the cell mapping just changed under it.
    void Configure(float worldSize, int resolution = defaultChunkResolution) {
        mapWorldSize = worldSize > 0.0f ? worldSize : 1.0f;
        chunkResolution = resolution > 0 ? resolution : 1;
        cellsPerWorldUnit = static_cast<float>(chunkResolution) / mapWorldSize;
        highestCellCoordinate = static_cast<float>(chunkResolution - 1);
        Clear();
    }
    float MapWorldSize() const { return mapWorldSize; }
    int ChunkResolution() const { return chunkResolution; }
    int CellCount() const { return chunkResolution * chunkResolution; }
    std::int32_t EntryCount() const { return static_cast<std::int32_t>(instanceIndex.size()); }
    bool IsEmpty() const { return instanceIndex.empty(); }

    // The SINGLE world->cell mapping, shared by Build and the picker (ARCH §8.3: two copies of
    // it is how a picker drifts). Clamped — out-of-range and NaN land in an edge cell.
    int CellIndexAt(float worldX, float worldY) const {
        return ClampedCellCoordinate(worldY) * chunkResolution + ClampedCellCoordinate(worldX);
    }

    // ARCH §21.6 — the row/column split CellIndexAt's own formula composes, exposed so a region
    // (box) query can compute its cellX/cellY span ONCE and walk it without re-deriving
    // CellIndexAt's multiply per cell. Additive; CellIndexAt itself is unchanged and still the
    // single mapping a point query uses.
    int CellCoordinateXAt(float worldX) const { return ClampedCellCoordinate(worldX); }
    int CellCoordinateYAt(float worldY) const { return ClampedCellCoordinate(worldY); }
    // Inputs are ALREADY-CLAMPED cell coordinates (from the two accessors above) — this does NOT
    // re-clamp; it is the other half of CellIndexAt's own formula.
    int CellIndexAtCoordinate(int cellX, int cellY) const { return cellY * chunkResolution + cellX; }

    // [begin, end) into InstanceIndexAt for one cell; out-of-range cells and every cell of an
    // empty grid answer with an empty range.
    std::int32_t BucketBegin(int cellIndex) const {
        return IsValidCell(cellIndex) ? bucketStart[static_cast<std::size_t>(cellIndex)] : 0;
    }
    std::int32_t BucketEnd(int cellIndex) const {
        return IsValidCell(cellIndex) ? bucketStart[static_cast<std::size_t>(cellIndex) + 1] : 0;
    }
    // The entry at a position inside a bucket range: an index into Data::PlacementInstances.
    std::int32_t InstanceIndexAt(std::int32_t position) const {
        return instanceIndex[static_cast<std::size_t>(position)];
    }
    const std::int32_t* InstanceIndexData() const { return instanceIndex.data(); }
    // Mechanical two-pass counting fill (count per cell -> prefix sum -> scatter) over the two
    // HORIZONTAL position columns of the resolved SoA — the caller decides which those are.
    // Replaces the previous contents; it never accumulates.
    void Build(const float* positionX, const float* positionY, std::int32_t count) {
        const int cellCount = CellCount();
        bucketStart.assign(static_cast<std::size_t>(cellCount) + 1, 0);
        instanceIndex.clear();
        if (count <= 0 || positionX == nullptr || positionY == nullptr) return;
        for (std::int32_t entry = 0; entry < count; ++entry)
            ++bucketStart[static_cast<std::size_t>(CellIndexAt(positionX[entry], positionY[entry])) + 1];
        for (int cell = 0; cell < cellCount; ++cell)
            bucketStart[static_cast<std::size_t>(cell) + 1] += bucketStart[static_cast<std::size_t>(cell)];
        instanceIndex.resize(static_cast<std::size_t>(count));
        std::vector<std::int32_t> writeCursor(bucketStart.begin(), bucketStart.end() - 1);
        for (std::int32_t entry = 0; entry < count; ++entry) {
            const int cell = CellIndexAt(positionX[entry], positionY[entry]);
            instanceIndex[static_cast<std::size_t>(writeCursor[static_cast<std::size_t>(cell)]++)] = entry;
        }
    }

    void Clear() {   // an empty grid is valid and answers every query with an empty range
        bucketStart.assign(static_cast<std::size_t>(CellCount()) + 1, 0);
        instanceIndex.clear();
    }

private:
    bool IsValidCell(int cellIndex) const { return cellIndex >= 0 && cellIndex < CellCount(); }
    // Precomputed reciprocal scale, then clamp in the float domain (absorbs NaN, whose
    // float->int conversion would otherwise be undefined behaviour).
    int ClampedCellCoordinate(float worldCoordinate) const {
        float cellCoordinate = worldCoordinate * cellsPerWorldUnit;
        if (!(cellCoordinate > 0.0f)) cellCoordinate = 0.0f;
        if (cellCoordinate > highestCellCoordinate) cellCoordinate = highestCellCoordinate;
        return static_cast<int>(cellCoordinate);
    }

    std::vector<std::int32_t> bucketStart;      // size chunkResolution^2 + 1, monotonic
    std::vector<std::int32_t> instanceIndex;    // size = total entries
    float mapWorldSize = 1.0f;
    float cellsPerWorldUnit = 1.0f;             // chunkResolution / mapWorldSize, precomputed
    float highestCellCoordinate = 0.0f;
    int   chunkResolution = 1;
};

} // namespace Data
} // namespace SanmapGen
