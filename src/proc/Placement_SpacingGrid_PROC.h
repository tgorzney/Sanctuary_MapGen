// Placement_SpacingGrid_PROC.h — the uniform grid that enforces the Poisson minimum spacing.
// Layer: PROC. Accepted points are bucketed by a cell size equal to the rule's spacing, so a
// "is anything within spacing of this position" test only ever reads the 3x3 neighbourhood —
// O(1) per candidate instead of O(accepted). This is what makes the min-spacing invariant
// affordable at 100k instances (the v1 MarkerSpatialGrid was a 32x32 hit-test grid for the
// UI, never a scatter accelerator — PLACEMENT_SCATTER_SPEC).
#pragma once
#include <vector>

namespace SanmapGen {
namespace Proc {

class SpacingGrid {
public:
    // `cellSize` must be >= the largest query radius for the 3x3 read to be exhaustive.
    void Configure(float cellSize, int vertexSize) {
        gridCellSize = cellSize > 0.0f ? cellSize : 1.0f;
        cellSizeReciprocal = 1.0f / gridCellSize;
        gridSide = static_cast<int>(static_cast<float>(vertexSize) * cellSizeReciprocal) + 2;
        buckets.assign(static_cast<std::size_t>(gridSide) * gridSide, std::vector<int>());
        pointX.clear();
        pointY.clear();
    }

    void Insert(float positionX, float positionY) {
        buckets[BucketIndex(positionX, positionY)].push_back(static_cast<int>(pointX.size()));
        pointX.push_back(positionX);
        pointY.push_back(positionY);
    }

    bool HasPointWithin(float positionX, float positionY, float radius) const {
        const float radiusSquared = radius * radius;
        const int centerX = ClampCell(static_cast<int>(positionX * cellSizeReciprocal));
        const int centerY = ClampCell(static_cast<int>(positionY * cellSizeReciprocal));
        for (int offsetY = -1; offsetY <= 1; ++offsetY)
            for (int offsetX = -1; offsetX <= 1; ++offsetX) {
                const int cellX = centerX + offsetX, cellY = centerY + offsetY;
                if (cellX < 0 || cellY < 0 || cellX >= gridSide || cellY >= gridSide) continue;
                const std::vector<int>& bucket = buckets[static_cast<std::size_t>(cellY) * gridSide + cellX];
                for (int pointIndex : bucket) {
                    const float deltaX = pointX[pointIndex] - positionX;
                    const float deltaY = pointY[pointIndex] - positionY;
                    if (deltaX * deltaX + deltaY * deltaY < radiusSquared) return true;
                }
            }
        return false;
    }

private:
    int ClampCell(int cell) const { return cell < 0 ? 0 : (cell >= gridSide ? gridSide - 1 : cell); }
    std::size_t BucketIndex(float positionX, float positionY) const {
        const int cellX = ClampCell(static_cast<int>(positionX * cellSizeReciprocal));
        const int cellY = ClampCell(static_cast<int>(positionY * cellSizeReciprocal));
        return static_cast<std::size_t>(cellY) * gridSide + cellX;
    }

    std::vector<std::vector<int>> buckets;
    std::vector<float> pointX;
    std::vector<float> pointY;
    float gridCellSize = 1.0f;
    float cellSizeReciprocal = 1.0f;
    int   gridSide = 1;
};

} // namespace Proc
} // namespace SanmapGen
