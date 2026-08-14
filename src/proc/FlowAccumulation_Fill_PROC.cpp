// FlowAccumulation_Fill_PROC.cpp — the depression-resolved drainage surface (CPU, exact).
// Barnes priority-flood +epsilon: pop cells in ascending drainage height from the map border
// inward, raising every pit and flat just enough that some neighbour is strictly lower. Two
// products for the price of one: the surface the routing runs on, and drainageOrder — the
// ascending pop order, which IS a topological order of the flow forest, so the accumulation
// sweep needs no separate sort and no ad-hoc iteration.
#include "FlowAccumulation_PROC.h"
#include <algorithm>
#include <queue>
#include <vector>

namespace SanmapGen {
namespace Proc {
namespace {

struct DrainageFrontEntry {
    float surfaceHeight;
    int   cellIndex;
};

// Min-heap on (height, index). The index tie-break makes the pop order canonical, so the fill
// and the topological order derived from it are deterministic (DETERMINISM_SPEC).
struct DrainageFrontOrder {
    bool operator()(const DrainageFrontEntry& left, const DrainageFrontEntry& right) const {
        if (left.surfaceHeight != right.surfaceHeight) return left.surfaceHeight > right.surfaceHeight;
        return left.cellIndex > right.cellIndex;
    }
};

using DrainageFront = std::priority_queue<DrainageFrontEntry, std::vector<DrainageFrontEntry>,
                                          DrainageFrontOrder>;

// Border cells drain off the map and keep their own height; the interior starts above every
// terrain height and relaxes down as the flood reaches it.
void SeedDrainageFront(int side, const float* terrainHeight, std::vector<float>& drainageSurface,
                       std::vector<unsigned char>& bVisited, DrainageFront& front) {
    for (int cellY = 0; cellY < side; ++cellY)
        for (int cellX = 0; cellX < side; ++cellX) {
            const std::size_t index = static_cast<std::size_t>(cellY) * side + cellX;
            const bool bBorder = cellX == 0 || cellY == 0 || cellX == side - 1 || cellY == side - 1;
            drainageSurface[index] = bBorder ? terrainHeight[index] : flowUnfilledSurfaceHeight;
            if (!bBorder) continue;
            bVisited[index] = 1;
            front.push(DrainageFrontEntry{ terrainHeight[index], static_cast<int>(index) });
        }
}

} // namespace

void FlowAccumulationStage::BuildDrainageSurfaceCpu() {
    const int side = vertexSize;
    const std::size_t cellCount = static_cast<std::size_t>(side) * side;
    const float* terrainHeight = mapFields.heightfield.Data();
    if (!constants.bFillDepressions) {
        for (std::size_t index = 0; index < cellCount; ++index)
            drainageSurface[index] = terrainHeight[index];
        BuildDrainageOrderFromSurface();
        return;
    }

    std::vector<unsigned char> bVisited(cellCount, 0);
    DrainageFront front;
    drainageOrder.clear();
    drainageOrder.reserve(cellCount);
    SeedDrainageFront(side, terrainHeight, drainageSurface, bVisited, front);

    while (!front.empty()) {
        const DrainageFrontEntry lowest = front.top();
        front.pop();
        drainageOrder.push_back(lowest.cellIndex);
        const int cellX = lowest.cellIndex % side;
        const int cellY = lowest.cellIndex / side;
        for (int neighbour = 0; neighbour < flowNeighbourCount; ++neighbour) {
            const int neighbourX = cellX + flowNeighbourOffsetX[neighbour];
            const int neighbourY = cellY + flowNeighbourOffsetY[neighbour];
            if (neighbourX < 0 || neighbourY < 0 || neighbourX >= side || neighbourY >= side) continue;
            const std::size_t target = static_cast<std::size_t>(neighbourY) * side + neighbourX;
            if (bVisited[target]) continue;
            bVisited[target] = 1;
            // The first neighbour to pop is by construction the lowest one, so this is exactly
            // the Planchon-Darboux fixed point max(height, min(neighbours) + epsilon) that the
            // GPU relaxation converges to — the two backends agree bit for bit.
            drainageSurface[target] = std::max(terrainHeight[target],
                                               lowest.surfaceHeight + constants.depressionFillEpsilon);
            front.push(DrainageFrontEntry{ drainageSurface[target], static_cast<int>(target) });
        }
    }
}

// Unfilled mode: pits and flats stay terminal sinks, so the topological order is simply the
// cells sorted by their own height (index tie-break keeps it canonical).
void FlowAccumulationStage::BuildDrainageOrderFromSurface() {
    const std::size_t cellCount = drainageSurface.size();
    drainageOrder.resize(cellCount);
    for (std::size_t index = 0; index < cellCount; ++index)
        drainageOrder[index] = static_cast<int>(index);
    const std::vector<float>& surface = drainageSurface;
    std::sort(drainageOrder.begin(), drainageOrder.end(), [&surface](int left, int right) {
        if (surface[left] != surface[right]) return surface[left] < surface[right];
        return left < right;
    });
}

} // namespace Proc
} // namespace SanmapGen
