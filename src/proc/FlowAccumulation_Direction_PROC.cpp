// FlowAccumulation_Direction_PROC.cpp — stochastic single-flow-direction routing (CPU).
// Each cell routes to the neighbour of maximum WEIGHTED drop (slope + unitNoise * impact),
// the SIM_ALGORITHMS_SPEC rule, evaluated on the depression-resolved drainage surface.
// Candidates are restricted to STRICTLY lower neighbours: that is what guarantees the routed
// graph is a DAG, which in turn is what lets the accumulation run as one ordered sweep.
// MapFields.flow receives the path slope (the noise is a tie-breaker, never part of the
// reported magnitude); the routed neighbour index lives in flowDirections beside it.
#include "FlowAccumulation_PROC.h"
#include "../sys/ThreadPool_SYS.h"

namespace SanmapGen {
namespace Proc {
namespace {

struct RoutedCell {
    int   directionIndex;
    float pathSlope;
};

// The one routing rule; the GLSL direction pass repeats these expressions in this order, so
// the two backends differ only in float evaluation (Constitution §4).
RoutedCell RouteCell(const float* surface, int side, int cellX, int cellY,
                     const FlowAccumulationConstants& constants) {
    const float cellHeight = surface[static_cast<std::size_t>(cellY) * side + cellX];
    RoutedCell routed{ flowSinkDirection, 0.0f };
    float bestScore = 0.0f;
    for (int neighbour = 0; neighbour < flowNeighbourCount; ++neighbour) {
        const int neighbourX = cellX + flowNeighbourOffsetX[neighbour];
        const int neighbourY = cellY + flowNeighbourOffsetY[neighbour];
        if (neighbourX < 0 || neighbourY < 0 || neighbourX >= side || neighbourY >= side) continue;
        const float drop = cellHeight - surface[static_cast<std::size_t>(neighbourY) * side + neighbourX];
        if (drop <= 0.0f) continue;
        const float inverseDistance = bFlowNeighbourIsDiagonal[neighbour]
                                    ? constants.diagonalInverseDistance
                                    : constants.cardinalInverseDistance;
        const float slope = drop * inverseDistance;
        const float score = slope + FlowNoiseUnit(static_cast<unsigned int>(cellX),
                                                  static_cast<unsigned int>(cellY),
                                                  static_cast<unsigned int>(neighbour),
                                                  constants.flowNoiseSeed) * constants.flowNoiseImpact;
        if (score > bestScore) {
            bestScore = score;
            routed.directionIndex = neighbour;
            routed.pathSlope = slope;
        }
    }
    return routed;
}

} // namespace

void FlowAccumulationStage::BuildFlowDirectionsCpu() {
    const int side = vertexSize;
    const float* surface = drainageSurface.data();
    float* flowMagnitude = mapFields.flow.Data();
    // Per-cell independent, so the row partition is order-free and stays deterministic.
    auto routeRow = [this, side, surface, flowMagnitude](int cellY) {
        for (int cellX = 0; cellX < side; ++cellX) {
            const std::size_t index = static_cast<std::size_t>(cellY) * side + cellX;
            const RoutedCell routed = RouteCell(surface, side, cellX, cellY, constants);
            flowDirections[index] = routed.directionIndex;
            flowMagnitude[index] = routed.pathSlope * constants.flowMagnitudeScale;
        }
    };
    if (threadPool != nullptr) threadPool->ParallelFor(0, side, routeRow);
    else for (int cellY = 0; cellY < side; ++cellY) routeRow(cellY);
}

} // namespace Proc
} // namespace SanmapGen
