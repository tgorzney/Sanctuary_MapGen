// FlowAccumulation_PROC_Test.cpp — acceptance test for the M3-5 work order. Build with MSVC:
//   cl /EHsc /std:c++17 /O2 FlowAccumulation_PROC_Test.cpp FlowAccumulation_Pipeline_PROC_Test.cpp
//      FlowAccumulation_Parity_PROC_Test.cpp FlowAccumulation_PROC.cpp
//      FlowAccumulation_Fill_PROC.cpp FlowAccumulation_Direction_PROC.cpp
//      FlowAccumulation_Accumulate_PROC.cpp FlowAccumulation_Gpu_PROC.cpp
//      FlowAccumulation_GpuRelax_PROC.cpp ..\sys\GpuResource_Program_SYS.cpp
//      ..\sys\GpuResource_ProgramParts_SYS.cpp ..\sys\GpuResource_Buffer_SYS.cpp
//      ..\sys\GpuGlFunctions_SYS.cpp opengl32.lib gdi32.lib user32.lib
// argv[1] = shader directory (defaults to this source directory).
// This file covers conservation on a closed basin and the coherent single-valley channel; the
// dirty-hash and CPU/GPU parity halves live in the two sibling test files.
#include "FlowAccumulation_TestSupport_PROC.h"

using namespace SanmapGen;
using namespace FlowAccumulationTest;

namespace {

// A closed basin must route every cell's weight into its terminal sink(s) — nothing created,
// nothing lost — whether the pit is kept (unfilled) or spilled to the border (filled).
void RunConservationChecks(int side) {
    const float cellCount = static_cast<float>(side) * static_cast<float>(side);
    const int center = side / 2;
    Params::Geometry geometry;
    geometry.mapSize = side - 1;
    Data::MapFields fields;
    BuildTiltedBowlTerrain(fields, side);

    Proc::FlowAccumulationStage stage(geometry, fields);
    stage.SetGenerationContext(Sys::GenerationContext::Output);
    stage.Constants().bFillDepressions = false;
    Check(stage.Run() == Sys::ComputeBackend::Cpu, "Output context resolves to the Cpu Exact path");
    Check(stage.SinkCount() == 1, "unfilled closed basin has exactly one terminal sink");
    Check(fields.accumulation.Get(center, center) == cellCount,
          "the pit accumulates the whole map (total accumulation == cell count)");
    Check(SinkAccumulationTotal(stage, fields) == static_cast<double>(cellCount),
          "conservation: accumulation over all sinks == cell count (unfilled)");

    stage.Constants().bFillDepressions = true;
    stage.Run();
    Check(stage.SinkCount() == 1, "priority-flood leaves exactly one outlet on the border");
    Check(fields.accumulation.Get(0, center) == cellCount,
          "the spillover outlet drains the whole basin");
    Check(SinkAccumulationTotal(stage, fields) == static_cast<double>(cellCount),
          "conservation: accumulation over all sinks == cell count (filled)");
    Check(stage.FlowDirections()[static_cast<std::size_t>(center) * side] == Proc::flowSinkDirection,
          "the outlet cell itself is the sink");
}

// One valley must produce ONE channel: the drainage maximum of every row sits on the valley
// floor, and the floor total grows by a full row of cells per step downstream.
void RunSingleValleyChannelChecks(int side) {
    Params::Geometry geometry;
    geometry.mapSize = side - 1;
    Data::MapFields fields;
    BuildSingleValleyTerrain(fields, side);
    Proc::FlowAccumulationStage stage(geometry, fields);
    stage.Run();

    const int floorColumn = side / 2;
    bool bChannelOnFloor = true;
    bool bChannelGrowsDownstream = true;
    float previousFloorTotal = 0.0f;
    for (int cellY = 0; cellY < side; ++cellY) {
        int strongestColumn = 0;
        for (int cellX = 1; cellX < side; ++cellX)
            if (fields.accumulation.Get(cellX, cellY) > fields.accumulation.Get(strongestColumn, cellY))
                strongestColumn = cellX;
        if (strongestColumn != floorColumn) bChannelOnFloor = false;
        const float floorTotal = fields.accumulation.Get(floorColumn, cellY);
        if (floorTotal != static_cast<float>((cellY + 1) * side)) bChannelGrowsDownstream = false;
        if (floorTotal <= previousFloorTotal) bChannelGrowsDownstream = false;
        previousFloorTotal = floorTotal;
    }
    Check(bChannelOnFloor, "every row's accumulation peak sits on the valley floor (one coherent channel)");
    Check(bChannelGrowsDownstream, "the channel gains exactly one row of drainage per cell downstream");
    Check(stage.SinkCount() == 1, "the valley drains through a single outlet");
    Check(fields.flow.Get(floorColumn, side / 2) > 0.0f, "the channel carries a non-zero flow magnitude");
}

// Degenerate inputs must route and conserve rather than crash (Constitution §6): a grid too
// small to have an interior, and a perfectly flat map where every drop is zero until the
// priority-flood epsilon tilts the flats out toward the border.
void RunDegenerateInputChecks() {
    for (int side = 1; side <= 3; ++side) {
        Params::Geometry geometry;
        geometry.mapSize = side - 1;
        Data::MapFields fields;
        BuildTiltedBowlTerrain(fields, side);
        Proc::FlowAccumulationStage stage(geometry, fields);
        stage.Run();
        Check(SinkAccumulationTotal(stage, fields) == static_cast<double>(side) * side,
              "conservation holds on a grid with no interior cells");
    }

    const int side = 33;
    Params::Geometry geometry;
    geometry.mapSize = side - 1;
    Data::MapFields fields;
    fields.Resize(side, 0.25f);   // perfectly flat
    Proc::FlowAccumulationStage stage(geometry, fields);
    stage.Run();
    Check(SinkAccumulationTotal(stage, fields) == static_cast<double>(side) * side,
          "conservation holds on a perfectly flat map (flats resolved, not stalled)");
    Check(stage.SinkCount() == 4 * (side - 1),
          "a flat map drains to its border ring, with no interior cell left stranded");
}

} // namespace

int main(int argc, char** argv) {
    const int side = 65;
    const char* shaderDirectory = (argc > 1) ? argv[1] : ".";

    RunConservationChecks(side);
    RunSingleValleyChannelChecks(side);
    RunDegenerateInputChecks();
    RunDirtyHashChecks(side);
    RunStochasticRoutingChecks(side);
    if (!RunGpuParityChecks(side, shaderDirectory))
        std::printf("SKIP: no GL compute context available — CPU/GPU parity not verified\n");

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
