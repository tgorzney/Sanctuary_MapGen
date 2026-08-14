// Thermal_PROC.h — the thermal / talus-relaxation stage: slope-limited material slumping.
// Layer: PROC (SIM_ALGORITHMS_SPEC "Thermal / avalanche — talus repose"). Where a local drop
// exceeds the material's talus angle, the excess slides downslope until the field is stable.
// One kernel, two backends: RunOnCpu() is the accuracy path, RunOnGpu() the speed path, and
// both evaluate the SAME gather formulation — a per-cell spread factor pass followed by an
// apply pass — so the legacy racy scatter (and its hardcoded "/2.0") is gone and the two
// backends agree cell for cell. The stage never picks a backend: it hands itself to
// Sys::Dispatch with the policy PIPELINE gave it (ARCH §4).
#pragma once
#include <cstddef>
#include <string>
#include <vector>
#include "Thermal_Kernel_PROC.h"
#include "../data/MapFields_DATA.h"
#include "../params/Geometry_PARAMS.h"
#include "../sys/Dispatch_SYS.h"

namespace SanmapGen {
namespace Sys { class GpuResourceManager; class ThreadPool; }

namespace Proc {

class ThermalStage {
public:
    ThermalStage(const Params::Geometry& geometrySettings, Data::MapFields& fields);

    // Configuration (all optional; ARCH §4.2 defaults for the Thermal stage).
    ThermalConstants& Constants() { return constants; }
    const ThermalConstants& Constants() const { return constants; }
    void SetDispatchPolicy(const Sys::DispatchPolicy& policy) { dispatchPolicy = policy; }
    void SetGenerationContext(Sys::GenerationContext context) { generationContext = context; }
    void SetGlobalBackend(Sys::ComputeBackend backend) { globalBackend = backend; }
    void SetGpuResourceManager(Sys::GpuResourceManager* manager) { gpuResourceManager = manager; }
    void SetThreadPool(Sys::ThreadPool* pool) { threadPool = pool; }

    // Hash of everything this stage consumes — PIPELINE registers it as the stage's
    // computeParamHash, so changing any thermal constant re-runs this stage and everything
    // downstream, and changing nothing skips it.
    std::size_t ComputeParameterHash() const;

    // Resolves the backend through Sys::Dispatch and runs there. Returns the backend used.
    Sys::ComputeBackend Run();

    // The two backends (public because Sys::Dispatch calls them; PIPELINE uses Run()).
    void RunOnCpu();
    void RunOnGpu();

    // Observability for tests/telemetry.
    Sys::ComputeBackend LastBackend() const { return lastBackend; }
    int CompletedIterationCount() const { return completedIterationCount; }
    bool IsGpuAvailable() const { return bGpuProgramReady; }
    const std::vector<float>& ResolvedTalusThresholds() const { return resolvedTalusThresholds; }

private:
    void PrepareRun();              // Thermal_PROC.cpp    — resolve thresholds, size scratch
    void PrepareIterationCpu();     // Thermal_Relax_PROC.cpp     — spread-factor pass
    void ApplyIterationCpu();       // Thermal_Transport_PROC.cpp — gather + material transport
    void CommitIterationCpu();      // Thermal_Transport_PROC.cpp — scratch -> live fields
    bool EnsureGpuResources();      // Thermal_Gpu_PROC.cpp — compile-once program
    void UploadFieldsToGpu();       // Thermal_Gpu_PROC.cpp — persistent buffers + upload
    void RunGpuSweeps(std::string& heightReadName, std::string& maskReadName);
    void ReadbackFieldsFromGpu(const std::string& heightReadName, const std::string& maskReadName);

    const Params::Geometry& geometry;
    Data::MapFields&        mapFields;
    ThermalConstants        constants;

    Sys::DispatchPolicy      dispatchPolicy;   // ARCH §4.2: Gpu/Visual preview, Cpu/Accurate output
    Sys::GenerationContext   generationContext = Sys::GenerationContext::Output;
    Sys::ComputeBackend      globalBackend     = Sys::ComputeBackend::Cpu;
    Sys::GpuResourceManager* gpuResourceManager = nullptr;
    Sys::ThreadPool*         threadPool         = nullptr;

    std::vector<float> resolvedTalusThresholds;   // per stratum: talus angle -> height units
    std::vector<float> kernelConstantBlock;       // ThermalConstantSlot layout, uploaded as-is
    std::vector<float> gpuTransferBuffer;
    Data::FloatField   cellSpreadFactor;
    Data::FloatField   cellTalusThreshold;
    Data::FloatField   heightScratch;
    Data::FloatField   materialMaskScratch[Data::MapFields::stratumCount];

    Sys::ComputeBackend lastBackend = Sys::ComputeBackend::Cpu;
    int  completedIterationCount = 0;
    bool bGpuProgramReady = false;
    int  gpuProgramIndex  = -1;
};

} // namespace Proc
} // namespace SanmapGen
