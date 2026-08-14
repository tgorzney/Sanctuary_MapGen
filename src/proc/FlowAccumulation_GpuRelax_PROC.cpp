// FlowAccumulation_GpuRelax_PROC.cpp — the GPU's two relaxation loops and the routing pass.
// Both loops are ping-pong (read one buffer, write the other; never read-modify-write), which
// is how the GPU twin reaches the CPU answer without the old shader's float scatter race.
// Each loop carries a budget (a declared Visual-class cost bound, Constitution §8) plus a
// cheap integer convergence probe: when a probe reports zero changed cells the fixed point is
// reached and the result is the exact CPU answer, not an approximation.
#include "FlowAccumulation_PROC.h"
#include "../sys/GpuResource_SYS.h"

namespace SanmapGen {
namespace Proc {
namespace {

unsigned TileGroupCount(int vertexSize, int tileSize) {
    return static_cast<unsigned>((vertexSize + tileSize - 1) / tileSize);
}

} // namespace

// Planchon-Darboux: surface = max(height, min(neighbours) + epsilon), iterated to its fixed
// point — the same surface the CPU priority-flood builds in one pass.
void FlowAccumulationStage::RelaxDrainageSurfaceGpu() {
    Sys::GpuResourceManager& manager = *gpuResourceManager;
    gpuFillIterationsUsed = 0;
    bGpuConverged = true;
    bSurfaceResultInFirstBuffer = true;
    if (!constants.bFillDepressions) return;   // the seed already IS the routing surface

    const Sys::GpuProgramHandle program{ gpuFillProgramIndex };
    manager.BindBuffer("flowHeight", 0);
    manager.BindBuffer("flowConstants", 7);
    manager.BindBuffer("flowConvergence", 8);
    manager.SetUniformInt(program, "vertexSize", vertexSize);
    const unsigned groupsX = TileGroupCount(vertexSize, Sys::WorkgroupSize::kFieldTileWidth);
    const unsigned groupsY = TileGroupCount(vertexSize, Sys::WorkgroupSize::kFieldTileHeight);
    const int iterationLimit = ResolvedFillIterationLimit();
    const int probeInterval = constants.gpuConvergenceCheckInterval > 0
                            ? constants.gpuConvergenceCheckInterval : 1;
    bGpuConverged = false;
    for (int iteration = 0; iteration < iterationLimit; ++iteration) {
        const bool bProbe = ((iteration + 1) % probeInterval) == 0 || iteration + 1 == iterationLimit;
        const int zero = 0;
        if (bProbe) manager.UploadBuffer("flowConvergence", &zero, sizeof(int));
        manager.BindBuffer(bSurfaceResultInFirstBuffer ? "flowSurfaceFirst" : "flowSurfaceSecond", 1);
        manager.BindBuffer(bSurfaceResultInFirstBuffer ? "flowSurfaceSecond" : "flowSurfaceFirst", 2);
        manager.Dispatch(program, groupsX, groupsY, 1);
        bSurfaceResultInFirstBuffer = !bSurfaceResultInFirstBuffer;
        ++gpuFillIterationsUsed;
        if (!bProbe) continue;
        int changedCellCount = 0;
        manager.ReadbackBuffer("flowConvergence", &changedCellCount, sizeof(int));
        if (changedCellCount == 0) { bGpuConverged = true; break; }
    }
}

// One pass: the stochastic single-flow-direction rule, plus the accumulation seed.
void FlowAccumulationStage::RouteFlowDirectionsGpu() {
    Sys::GpuResourceManager& manager = *gpuResourceManager;
    const Sys::GpuProgramHandle program{ gpuDirectionProgramIndex };
    manager.BindBuffer(bSurfaceResultInFirstBuffer ? "flowSurfaceFirst" : "flowSurfaceSecond", 1);
    manager.BindBuffer("flowDirection", 3);
    manager.BindBuffer("flowMagnitude", 4);
    manager.BindBuffer("flowAccumulationFirst", 5);
    manager.BindBuffer("flowConstants", 7);
    manager.SetUniformInt(program, "vertexSize", vertexSize);
    manager.SetUniformInt(program, "flowNoiseSeed", static_cast<int>(constants.flowNoiseSeed));
    manager.Dispatch(program, TileGroupCount(vertexSize, Sys::WorkgroupSize::kFieldTileWidth),
                     TileGroupCount(vertexSize, Sys::WorkgroupSize::kFieldTileHeight), 1);
    bAccumulationResultInFirstBuffer = true;
}

// Gather relaxation: accumulation = weight + sum of the neighbours routing INTO this cell.
// Monotone from the seed, so it climbs to the exact drainage totals in as many iterations as
// the longest flow path is long — the DAG order the CPU walks, expressed in parallel.
void FlowAccumulationStage::RelaxAccumulationGpu() {
    Sys::GpuResourceManager& manager = *gpuResourceManager;
    const Sys::GpuProgramHandle program{ gpuAccumulationProgramIndex };
    manager.BindBuffer("flowDirection", 3);
    manager.BindBuffer("flowConstants", 7);
    manager.BindBuffer("flowConvergence", 8);
    manager.SetUniformInt(program, "vertexSize", vertexSize);
    const unsigned groupsX = TileGroupCount(vertexSize, Sys::WorkgroupSize::kFieldTileWidth);
    const unsigned groupsY = TileGroupCount(vertexSize, Sys::WorkgroupSize::kFieldTileHeight);
    const int iterationLimit = ResolvedAccumulationIterationLimit();
    const int probeInterval = constants.gpuConvergenceCheckInterval > 0
                            ? constants.gpuConvergenceCheckInterval : 1;
    gpuAccumulationIterationsUsed = 0;
    bool bAccumulationConverged = false;
    for (int iteration = 0; iteration < iterationLimit; ++iteration) {
        const bool bProbe = ((iteration + 1) % probeInterval) == 0 || iteration + 1 == iterationLimit;
        const int zero = 0;
        if (bProbe) manager.UploadBuffer("flowConvergence", &zero, sizeof(int));
        manager.BindBuffer(bAccumulationResultInFirstBuffer ? "flowAccumulationFirst"
                                                            : "flowAccumulationSecond", 5);
        manager.BindBuffer(bAccumulationResultInFirstBuffer ? "flowAccumulationSecond"
                                                            : "flowAccumulationFirst", 6);
        manager.Dispatch(program, groupsX, groupsY, 1);
        bAccumulationResultInFirstBuffer = !bAccumulationResultInFirstBuffer;
        ++gpuAccumulationIterationsUsed;
        if (!bProbe) continue;
        int changedCellCount = 0;
        manager.ReadbackBuffer("flowConvergence", &changedCellCount, sizeof(int));
        if (changedCellCount == 0) { bAccumulationConverged = true; break; }
    }
    bGpuConverged = bGpuConverged && bAccumulationConverged;
}

} // namespace Proc
} // namespace SanmapGen
