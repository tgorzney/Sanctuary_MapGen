// FlowAccumulation_PROC.h — the flow-direction + drainage-accumulation stage.
// Layer: PROC. Reads MapFields.heightfield; writes MapFields.flow (path magnitude) and
// MapFields.accumulation (drainage area). Preview Visual (Gpu) / Output Exact (Cpu):
// accumulation drives river shading and, later, fluvial erosion and pathing, so the CPU
// pass is the authority (SIM_ALGORITHMS_SPEC "Flow & accumulation"). This replaces the old
// UseGPUFlowMap bool, which merely SKIPPED the CPU flow and left an empty map.
// Both backends run the same kernel contract (FlowAccumulation_Kernel_PROC.h) in three steps:
//   1. a depression-resolved drainage surface — CPU priority-flood (+epsilon), GPU the
//      Planchon-Darboux relaxation that converges to the identical fixed point;
//   2. stochastic single-flow-direction routing among STRICTLY lower neighbours, which is
//      what keeps the flow graph a DAG;
//   3. drainage accumulated in proper DAG order — CPU one ordered sweep back down the
//      priority-flood order, GPU a race-free gather relaxation (never a float scatter).
// The stage never picks a backend: it hands itself to Sys::Dispatch with the policy the
// PIPELINE gave it (ARCH §4).
#pragma once
#include <cstddef>
#include <vector>
#include "FlowAccumulation_Kernel_PROC.h"
#include "../data/MapFields_DATA.h"
#include "../params/Geometry_PARAMS.h"
#include "../sys/Dispatch_SYS.h"

namespace SanmapGen {
namespace Sys { class GpuResourceManager; class ThreadPool; }

namespace Proc {

class FlowAccumulationStage {
public:
    FlowAccumulationStage(const Params::Geometry& geometrySettings, Data::MapFields& outputFields);

    FlowAccumulationConstants& Constants() { return constants; }
    const FlowAccumulationConstants& Constants() const { return constants; }
    void SetDispatchPolicy(const Sys::DispatchPolicy& policy) { dispatchPolicy = policy; }
    void SetGenerationContext(Sys::GenerationContext context) { generationContext = context; }
    void SetGlobalBackend(Sys::ComputeBackend backend) { globalBackend = backend; }
    void SetGpuResourceManager(Sys::GpuResourceManager* manager) { gpuResourceManager = manager; }
    void SetThreadPool(Sys::ThreadPool* pool) { threadPool = pool; }

    // Hash of everything this stage consumes on its own (geometry + every constant). PIPELINE
    // registers it as the stage's computeParamHash; upstream changes arrive through the
    // pipeline's forward-mixed hash, so the heightfield itself is not hashed here.
    std::size_t ComputeParameterHash() const;

    // Resolves the backend through Sys::Dispatch and runs there. Returns the backend used.
    Sys::ComputeBackend Run();

    // The two backends (public because Sys::Dispatch calls them; PIPELINE uses Run()).
    void RunOnCpu();
    void RunOnGpu();

    // Observability for tests/telemetry. Direction is a companion of MapFields.flow: the
    // field carries the magnitude, this array the routed neighbour index (-1 = sink).
    Sys::ComputeBackend LastBackend() const { return lastBackend; }
    const std::vector<int>& FlowDirections() const { return flowDirections; }
    const std::vector<float>& DrainageSurface() const { return drainageSurface; }
    int  VertexSize() const { return vertexSize; }
    int  SinkCount() const { return sinkCount; }
    bool WasGpuFallbackUsed() const { return bGpuFallbackUsed; }
    bool WasGpuConverged() const { return bGpuConverged; }
    int  GpuFillIterationsUsed() const { return gpuFillIterationsUsed; }
    int  GpuAccumulationIterationsUsed() const { return gpuAccumulationIterationsUsed; }

private:
    void PrepareRun();
    void BuildDrainageSurfaceCpu();        // FlowAccumulation_Fill_PROC.cpp
    void BuildDrainageOrderFromSurface();  // FlowAccumulation_Fill_PROC.cpp
    void BuildFlowDirectionsCpu();         // FlowAccumulation_Direction_PROC.cpp
    void AccumulateDrainageCpu();          // FlowAccumulation_Accumulate_PROC.cpp
    void NormalizeAccumulation();          // FlowAccumulation_Accumulate_PROC.cpp
    void CountSinks();                     // FlowAccumulation_Accumulate_PROC.cpp
    bool EnsureGpuResources();             // FlowAccumulation_Gpu_PROC.cpp
    void UploadGpuInputs();                // FlowAccumulation_Gpu_PROC.cpp
    void ReadbackGpuOutputs();             // FlowAccumulation_Gpu_PROC.cpp
    void RelaxDrainageSurfaceGpu();        // FlowAccumulation_GpuRelax_PROC.cpp
    void RouteFlowDirectionsGpu();         // FlowAccumulation_GpuRelax_PROC.cpp
    void RelaxAccumulationGpu();           // FlowAccumulation_GpuRelax_PROC.cpp
    int  ResolvedFillIterationLimit() const;
    int  ResolvedAccumulationIterationLimit() const;

    const Params::Geometry&   geometry;
    Data::MapFields&          mapFields;
    FlowAccumulationConstants constants;

    Sys::DispatchPolicy      dispatchPolicy;
    Sys::GenerationContext   generationContext  = Sys::GenerationContext::Output;
    Sys::ComputeBackend      globalBackend      = Sys::ComputeBackend::Cpu;
    Sys::GpuResourceManager* gpuResourceManager = nullptr;
    Sys::ThreadPool*         threadPool         = nullptr;

    std::vector<float> drainageSurface;   // depression-resolved surface the routing ran on
    std::vector<int>   flowDirections;    // neighbour index 0..7, flowSinkDirection = terminal
    std::vector<int>   drainageOrder;     // ascending drainage height = topological order
    std::vector<float> gpuTransferBuffer;

    Sys::ComputeBackend lastBackend = Sys::ComputeBackend::Cpu;
    int  vertexSize                    = 0;
    int  sinkCount                     = 0;
    int  gpuFillIterationsUsed         = 0;
    int  gpuAccumulationIterationsUsed = 0;
    int  gpuFillProgramIndex           = -1;   // opaque program handles stay in SYS; the stage
    int  gpuDirectionProgramIndex      = -1;   // keeps only their indices so this header needs
    int  gpuAccumulationProgramIndex   = -1;   // no GL/SYS resource include (ARCH §3.2)
    bool bSurfaceResultInFirstBuffer      = true;
    bool bAccumulationResultInFirstBuffer = true;
    bool bGpuProgramsReady = false;
    bool bGpuFallbackUsed  = false;
    bool bGpuConverged     = false;
};

} // namespace Proc
} // namespace SanmapGen
