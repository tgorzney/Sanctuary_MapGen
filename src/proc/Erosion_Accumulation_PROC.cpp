// Erosion_Accumulation_PROC.cpp — the ordered spillover DAG (SIM_ALGORITHMS_SPEC's
// "AccurateSimultaneousAccumulation", Cpu-only). Layer: PROC.
// Droplets pile sediment where they die; this pass walks the columns from highest to lowest
// and lets an over-deep pile spill into its lowest neighbour, turning those spikes into filled
// valleys. It is a DAG because material only ever moves DOWNHILL and the descending walk
// finalises a cell before anything it feeds. Cpu-only by design: the traversal order IS the
// algorithm — exactly the ordered reduction a Gpu cannot promise, so the Visual-class Gpu pass
// skips it (and the parity test runs with it off).
#include "Erosion_PROC.h"
#include <algorithm>

namespace SanmapGen {
namespace Proc {
namespace {

constexpr int neighbourOffsetX[8] = { -1, 1,  0, 0, -1,  1, -1, 1 };
constexpr int neighbourOffsetY[8] = {  0, 0, -1, 1, -1, -1,  1, 1 };

// Descending height, ties broken by cell index, so the order is a pure function of the state.
std::vector<int> BuildDescendingCellOrder(const std::vector<int>& columnTotals) {
    std::vector<int> ordered(columnTotals.size());
    for (std::size_t index = 0; index < ordered.size(); ++index) ordered[index] = static_cast<int>(index);
    std::sort(ordered.begin(), ordered.end(), [&columnTotals](int first, int second) {
        if (columnTotals[first] != columnTotals[second]) return columnTotals[first] > columnTotals[second];
        return first < second;
    });
    return ordered;
}

int FindLowestNeighbour(const std::vector<int>& columnTotals, int vertexSize, int cellX, int cellY,
                        int& outLowestTotal) {
    int lowestCellIndex = -1;
    for (int neighbour = 0; neighbour < 8; ++neighbour) {
        const int neighbourIndex = (cellY + neighbourOffsetY[neighbour]) * vertexSize
                                 + (cellX + neighbourOffsetX[neighbour]);
        if (columnTotals[neighbourIndex] < outLowestTotal) {
            outLowestTotal = columnTotals[neighbourIndex];
            lowestCellIndex = neighbourIndex;
        }
    }
    return lowestCellIndex;
}

} // namespace

void ErosionStage::ApplyAccumulationDagCpu(int stratumIndex) {
    const ErosionLayerSettings& settings = layerSettings[stratumIndex];
    const int spilloverThresholdTicks = HeightToFixedPoint(settings.spilloverThreshold,
                                                          constants.heightFixedPointScale);
    int* depositLayer = thicknessFixedPoint.data() + static_cast<std::size_t>(stratumIndex) * cellCount;

    std::vector<int> columnTotals(static_cast<std::size_t>(cellCount));
    for (int cellIndex = 0; cellIndex < cellCount; ++cellIndex)
        columnTotals[cellIndex] = ColumnTotalFixedPointAt(cellIndex);
    const std::vector<int> sortedCellIndices = BuildDescendingCellOrder(columnTotals);

    for (int ordinal = 0; ordinal < cellCount; ++ordinal) {
        const int cellIndex = sortedCellIndices[ordinal];
        const int cellX = cellIndex % vertexSize;
        const int cellY = cellIndex / vertexSize;
        if (cellX <= 0 || cellX >= vertexSize - 1 || cellY <= 0 || cellY >= vertexSize - 1) continue;
        const int spillableTicks = depositLayer[cellIndex] - spilloverThresholdTicks;
        if (spillableTicks <= 0) continue;

        int lowestTotal = columnTotals[cellIndex];
        const int lowestCellIndex = FindLowestNeighbour(columnTotals, vertexSize, cellX, cellY, lowestTotal);
        if (lowestCellIndex < 0) continue;

        // Never pass on more than half the height difference: the pair cannot swap ranking, so
        // the descending walk stays a DAG and one sweep is enough.
        const int levellingTicks = static_cast<int>(static_cast<float>(columnTotals[cellIndex] - lowestTotal)
                                                    * settings.spilloverShare);
        const int transferTicks = spillableTicks < levellingTicks ? spillableTicks : levellingTicks;
        if (transferTicks <= 0) continue;
        depositLayer[cellIndex] -= transferTicks;
        depositLayer[lowestCellIndex] += transferTicks;
        columnTotals[cellIndex] -= transferTicks;
        columnTotals[lowestCellIndex] += transferTicks;
    }
}

} // namespace Proc
} // namespace SanmapGen
