// FlowAccumulation_Gpu_PROC.cpp — the GPU speed path's resources and hand-off: compile the
// three passes of FlowAccumulation_PROC.glsl once (keyed by their pass define), keep the
// persistent SSBOs, upload, then read the results back into the same DATA fields the CPU path
// writes. The relaxation loops themselves live in FlowAccumulation_GpuRelax_PROC.cpp.
// GL never appears above this seam — the stage stores program indices only (ARCH §3.2).
#include "FlowAccumulation_PROC.h"
#include "../sys/GpuResource_SYS.h"
#include <string>
#include <vector>

namespace SanmapGen {
namespace Proc {
namespace {

const char* const flowShaderFileName = "FlowAccumulation_PROC.glsl";

// The workgroup size and the unfilled seed height are shared with the C++ dispatch math, so
// the shader never hardcodes either (DISPATCH_INTERFACE_SPEC §3).
std::string CommonShaderDefinitions() {
    return "#define WORKGROUP_TILE_WIDTH "    + std::to_string(Sys::WorkgroupSize::kFieldTileWidth)
         + "\n#define WORKGROUP_TILE_HEIGHT " + std::to_string(Sys::WorkgroupSize::kFieldTileHeight)
         + "\n#define FLOW_UNFILLED_SURFACE_HEIGHT 1.0e30";
}

// Border cells keep their own height (they drain off the map); the interior starts above every
// terrain height so the relaxation converges DOWN onto the priority-flood surface. Unfilled
// mode simply routes on the raw terrain.
void BuildSeedSurface(int side, const float* terrainHeight, bool bFillDepressions,
                      std::vector<float>& seedSurface) {
    seedSurface.resize(static_cast<std::size_t>(side) * side);
    for (int cellY = 0; cellY < side; ++cellY)
        for (int cellX = 0; cellX < side; ++cellX) {
            const std::size_t index = static_cast<std::size_t>(cellY) * side + cellX;
            const bool bBorder = cellX == 0 || cellY == 0 || cellX == side - 1 || cellY == side - 1;
            seedSurface[index] = (!bFillDepressions || bBorder) ? terrainHeight[index]
                                                                : flowUnfilledSurfaceHeight;
        }
}

} // namespace

bool FlowAccumulationStage::EnsureGpuResources() {
    if (gpuResourceManager == nullptr) return false;
    if (!gpuResourceManager->Initialize()) return false;
    if (bGpuProgramsReady) return true;
    const std::string common = CommonShaderDefinitions();
    const Sys::GpuProgramHandle fill =
        gpuResourceManager->GetOrCompileProgram(flowShaderFileName, common + "\n#define FLOW_PASS_FILL 1");
    const Sys::GpuProgramHandle direction =
        gpuResourceManager->GetOrCompileProgram(flowShaderFileName, common + "\n#define FLOW_PASS_DIRECTION 1");
    const Sys::GpuProgramHandle accumulation =
        gpuResourceManager->GetOrCompileProgram(flowShaderFileName, common + "\n#define FLOW_PASS_ACCUMULATION 1");
    if (!fill.IsValid() || !direction.IsValid() || !accumulation.IsValid()) return false;
    gpuFillProgramIndex         = fill.programIndex;
    gpuDirectionProgramIndex    = direction.programIndex;
    gpuAccumulationProgramIndex = accumulation.programIndex;
    bGpuProgramsReady = true;
    return true;
}

void FlowAccumulationStage::UploadGpuInputs() {
    Sys::GpuResourceManager& manager = *gpuResourceManager;
    const std::size_t cellCount = static_cast<std::size_t>(vertexSize) * vertexSize;
    const std::size_t floatByteSize = cellCount * sizeof(float);
    manager.EnsureBuffer("flowHeight", floatByteSize);
    manager.EnsureBuffer("flowSurfaceFirst", floatByteSize);
    manager.EnsureBuffer("flowSurfaceSecond", floatByteSize);
    manager.EnsureBuffer("flowDirection", cellCount * sizeof(int));
    manager.EnsureBuffer("flowMagnitude", floatByteSize);
    manager.EnsureBuffer("flowAccumulationFirst", floatByteSize);
    manager.EnsureBuffer("flowAccumulationSecond", floatByteSize);
    manager.EnsureBuffer("flowConstants", sizeof(FlowAccumulationKernelRecord));
    manager.EnsureBuffer("flowConvergence", sizeof(int));

    FlowAccumulationKernelRecord record;
    record.cellWeight              = constants.cellWeight;
    record.flowNoiseImpact         = constants.flowNoiseImpact;
    record.depressionFillEpsilon   = constants.depressionFillEpsilon;
    record.cardinalInverseDistance = constants.cardinalInverseDistance;
    record.diagonalInverseDistance = constants.diagonalInverseDistance;
    record.flowMagnitudeScale      = constants.flowMagnitudeScale;
    manager.UploadBuffer("flowConstants", &record, sizeof(record));
    manager.UploadBuffer("flowHeight", mapFields.heightfield.Data(), floatByteSize);

    BuildSeedSurface(vertexSize, mapFields.heightfield.Data(), constants.bFillDepressions,
                     gpuTransferBuffer);
    manager.UploadBuffer("flowSurfaceFirst", gpuTransferBuffer.data(), floatByteSize);
    bSurfaceResultInFirstBuffer      = true;
    bAccumulationResultInFirstBuffer = true;
}

void FlowAccumulationStage::ReadbackGpuOutputs() {
    Sys::GpuResourceManager& manager = *gpuResourceManager;
    const std::size_t cellCount = static_cast<std::size_t>(vertexSize) * vertexSize;
    const std::size_t floatByteSize = cellCount * sizeof(float);
    manager.ReadbackBuffer("flowDirection", flowDirections.data(), cellCount * sizeof(int));
    manager.ReadbackBuffer("flowMagnitude", mapFields.flow.Data(), floatByteSize);
    manager.ReadbackBuffer(bAccumulationResultInFirstBuffer ? "flowAccumulationFirst"
                                                            : "flowAccumulationSecond",
                           mapFields.accumulation.Data(), floatByteSize);
    manager.ReadbackBuffer(bSurfaceResultInFirstBuffer ? "flowSurfaceFirst" : "flowSurfaceSecond",
                           drainageSurface.data(), floatByteSize);
}

// The speed path. If no GPU is reachable the stage still has to produce its fields, so it
// falls back to the accuracy path and says so (WasGpuFallbackUsed) — a reported capability
// fallback, never a rival backend toggle.
void FlowAccumulationStage::RunOnGpu() {
    PrepareRun();
    if (vertexSize <= 0) return;
    if (!EnsureGpuResources()) {
        RunOnCpu();
        bGpuFallbackUsed = true;
        return;
    }
    UploadGpuInputs();
    RelaxDrainageSurfaceGpu();
    RouteFlowDirectionsGpu();
    RelaxAccumulationGpu();
    ReadbackGpuOutputs();
    CountSinks();
    NormalizeAccumulation();
}

} // namespace Proc
} // namespace SanmapGen
