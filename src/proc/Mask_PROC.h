// Mask_PROC.h — the mask stage: slope masking + the stored-stratum-mask merge into the one
// MaterialMasks weight field. Layer: PROC — runs after noise/blend, which owns the top-down
// occlusion fill of MaterialMasks (M3-1); this stage GATES that field by slope and merges the
// imported art into the SAME field (MASKING_SPEC "Stored stratum masks are the SAME system").
// One kernel, two backends: RunOnCpu() is the accuracy path, RunOnGpu() the speed path, both
// driven from the identical MaskStratumConfiguration records. The stage never picks a backend:
// it hands itself to Sys::Dispatch with the policy PIPELINE gave it (ARCH §4).
#pragma once
#include <cstddef>
#include <vector>
#include "Mask_Kernel_PROC.h"
#include "../data/MapFields_DATA.h"
#include "../params/Geometry_PARAMS.h"
#include "../params/StratumMask_PARAMS.h"
#include "../sys/Dispatch_SYS.h"

namespace SanmapGen {
namespace Sys { class GpuResourceManager; class ThreadPool; }

namespace Proc {

class MaskStage {
public:
    MaskStage(const Params::Geometry& geometrySettings,
              const std::vector<Params::StratumMask>& stratumMaskSettings,
              Data::MapFields& outputFields);

    // Configuration (all optional; ARCH §4.2 Mask defaults = Gpu/Visual preview, Cpu/Accurate output).
    MaskConstants& Constants() { return constants; }
    const MaskConstants& Constants() const { return constants; }
    void SetDispatchPolicy(const Sys::DispatchPolicy& policy) { dispatchPolicy = policy; }
    void SetGenerationContext(Sys::GenerationContext context) { generationContext = context; }
    void SetGlobalBackend(Sys::ComputeBackend backend) { globalBackend = backend; }
    void SetGpuResourceManager(Sys::GpuResourceManager* manager) { gpuResourceManager = manager; }
    void SetThreadPool(Sys::ThreadPool* pool) { threadPool = pool; }

    // Hash of everything this stage consumes that is its OWN: geometry scale, the stage
    // constants, and every per-stratum mask setting including the stored art's content. The
    // heightfield this stage reads belongs to the UPSTREAM stage, so it is deliberately not
    // hashed here — Generation_PIPELINE mixes the upstream hash forward, which is what makes
    // "upstream changed => mask re-runs" work without this stage knowing the pipeline shape.
    std::size_t ComputeParameterHash() const;

    // Resolves the backend through Sys::Dispatch and runs there. Returns the backend used.
    Sys::ComputeBackend Run();

    // The two backends (public because Sys::Dispatch calls them; PIPELINE uses Run()).
    void RunOnCpu();
    void RunOnGpu();

    // Observability for tests/telemetry.
    Sys::ComputeBackend LastBackend() const { return lastBackend; }
    bool IsGpuAvailable() const { return bGpuProgramReady; }
    const std::vector<MaskStratumConfiguration>& StratumConfigurations() const { return stratumConfigurations; }

private:
    // Flattens the per-stratum settings into MaskStratumConfiguration records (degrees ->
    // gradient, feathers -> reciprocals) and packs the stored art both backends sample.
    void PrepareRun();
    bool EnsureGpuResources();   // Mask_Gpu_PROC.cpp

    const Params::Geometry&                  geometry;
    const std::vector<Params::StratumMask>&  stratumMasks;
    Data::MapFields&                         mapFields;
    MaskConstants                            constants;

    Sys::DispatchPolicy      dispatchPolicy;                                   // ARCH §4.2 defaults
    Sys::GenerationContext   generationContext  = Sys::GenerationContext::Output;
    Sys::ComputeBackend      globalBackend      = Sys::ComputeBackend::Cpu;
    Sys::GpuResourceManager* gpuResourceManager = nullptr;
    Sys::ThreadPool*         threadPool         = nullptr;

    std::vector<MaskStratumConfiguration> stratumConfigurations;
    std::vector<float>                    packedStoredMaskValues;   // every stratum's art, one buffer
    std::vector<float>                    gpuTransferBuffer;

    Sys::ComputeBackend lastBackend      = Sys::ComputeBackend::Cpu;
    bool                bGpuProgramReady = false;
    int                 gpuProgramIndex  = -1;
};

} // namespace Proc
} // namespace SanmapGen
