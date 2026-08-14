// FlowAccumulation_TestSupport_PROC.h — shared fixtures for the FlowAccumulation acceptance
// tests: the two reference terrains, the pass/fail counter, and the entry points of the test
// files that cover the dirty-hash and the CPU/GPU parity halves of the work-order acceptance
// list. Test-only support; it holds no stage logic.
#pragma once
#include "FlowAccumulation_PROC.h"
#include <cstdio>
#include <cstdlib>

namespace FlowAccumulationTest {

inline int failureCount = 0;

inline void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL: %s\n", label);
    ++failureCount;
}

// A closed basin: a paraboloid bowl covering the whole grid, tilted a hair toward -x so that
// BOTH the interior pit and the lowest border cell are unique — no symmetric ties to argue
// about when the conservation totals are checked.
inline void BuildTiltedBowlTerrain(SanmapGen::Data::MapFields& fields, int side) {
    fields.Resize(side);
    const int center = side / 2;
    for (int cellY = 0; cellY < side; ++cellY)
        for (int cellX = 0; cellX < side; ++cellX) {
            const int deltaX = cellX - center;
            const int deltaY = cellY - center;
            const float radiusSquared = static_cast<float>(deltaX * deltaX + deltaY * deltaY);
            fields.heightfield.Set(cellX, cellY,
                                   radiusSquared * 0.001f + static_cast<float>(cellX) * 0.0001f);
        }
}

// A single V-shaped valley draining toward +y: cross-slope 1.0 per cell, downstream slope 0.1,
// so every cell walks to the valley floor first and then down it. The floor column must end up
// carrying the whole map's drainage.
inline void BuildSingleValleyTerrain(SanmapGen::Data::MapFields& fields, int side) {
    fields.Resize(side);
    const int floorColumn = side / 2;
    for (int cellY = 0; cellY < side; ++cellY)
        for (int cellX = 0; cellX < side; ++cellX) {
            const int distanceToFloor = cellX > floorColumn ? cellX - floorColumn : floorColumn - cellX;
            const float crossSlope = static_cast<float>(distanceToFloor) * 1.0f;
            const float downstreamDrop = static_cast<float>(side - 1 - cellY) * 0.1f;
            fields.heightfield.Set(cellX, cellY, crossSlope + downstreamDrop);
        }
}

// The conserved quantity: every cell contributes its weight exactly once, so whatever reaches
// the terminal sinks must total cellCount * cellWeight.
inline double SinkAccumulationTotal(const SanmapGen::Proc::FlowAccumulationStage& stage,
                                    const SanmapGen::Data::MapFields& fields) {
    double total = 0.0;
    const std::vector<int>& directions = stage.FlowDirections();
    for (std::size_t index = 0; index < directions.size(); ++index)
        if (directions[index] == SanmapGen::Proc::flowSinkDirection)
            total += static_cast<double>(fields.accumulation.Data()[index]);
    return total;
}

// Defined in FlowAccumulation_Pipeline_PROC_Test.cpp / FlowAccumulation_Parity_PROC_Test.cpp.
void RunDirtyHashChecks(int side);
void RunStochasticRoutingChecks(int side);
bool RunGpuParityChecks(int side, const char* shaderDirectory);   // false = skipped, no GL

} // namespace FlowAccumulationTest
