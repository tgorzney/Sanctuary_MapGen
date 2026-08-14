// FlowAccumulation_Accumulate_PROC.cpp — drainage accumulation in proper DAG order (CPU).
// drainageOrder is ascending drainage height, so walking it BACKWARDS visits every cell only
// after every cell that drains into it: one ordered sweep is the exact answer, with no
// iteration budget, no atomics and no convergence question. Conservation is structural —
// each cell contributes cellWeight exactly once and passes its running total downstream, so
// the accumulation summed over the terminal sinks equals cellCount * cellWeight.
#include "FlowAccumulation_PROC.h"

namespace SanmapGen {
namespace Proc {

void FlowAccumulationStage::AccumulateDrainageCpu() {
    const int side = vertexSize;
    float* accumulation = mapFields.accumulation.Data();
    const std::size_t cellCount = static_cast<std::size_t>(side) * side;
    for (std::size_t index = 0; index < cellCount; ++index) accumulation[index] = constants.cellWeight;

    for (std::size_t position = drainageOrder.size(); position-- > 0; ) {
        const int cellIndex = drainageOrder[position];
        const int direction = flowDirections[cellIndex];
        if (direction == flowSinkDirection) continue;
        const int targetX = cellIndex % side + flowNeighbourOffsetX[direction];
        const int targetY = cellIndex / side + flowNeighbourOffsetY[direction];
        accumulation[static_cast<std::size_t>(targetY) * side + targetX] += accumulation[cellIndex];
    }
}

void FlowAccumulationStage::CountSinks() {
    sinkCount = 0;
    for (int direction : flowDirections)
        if (direction == flowSinkDirection) ++sinkCount;
}

// Optional presentation scaling. Off by default: the Exact/Output class reports raw drainage
// counts, because normalizing destroys the conservation property downstream stages rely on.
void FlowAccumulationStage::NormalizeAccumulation() {
    if (!constants.bNormalizeAccumulation) return;
    float* accumulation = mapFields.accumulation.Data();
    const std::size_t cellCount = mapFields.accumulation.CellCount();
    float largest = 0.0f;
    for (std::size_t index = 0; index < cellCount; ++index)
        if (accumulation[index] > largest) largest = accumulation[index];
    if (largest <= 0.0f) return;
    const float inverseLargest = 1.0f / largest;   // one reciprocal, then multiply in the loop
    for (std::size_t index = 0; index < cellCount; ++index) accumulation[index] *= inverseLargest;
}

} // namespace Proc
} // namespace SanmapGen
