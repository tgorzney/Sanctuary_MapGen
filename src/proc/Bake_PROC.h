// Bake_PROC.h — the bake stage: composite the stratum textures weighted by the material
// masks into the output texture set. Layer: PROC — the LAST stage of generation; it reads
// what every upstream stage wrote and produces the `Textures/` payload
// (GAMEDATA_LAYOUT_SPEC, PREVIEW_COMPOSITING_SPEC).
// Accuracy class: Visual on BOTH contexts (ARCH §4.2 "Bake · Albedo · preview color =
// Gpu/Visual, Gpu/Visual") — decorative, determinism-exempt (DETERMINISM_SPEC), never on
// the gameplay-authoritative path. The CPU twin exists as the parity reference and as the
// fallback when no GL context is present.
// The stage never picks a backend: it hands itself to Sys::Dispatch with the policy
// PIPELINE gave it (ARCH §4).
#pragma once
#include <cstddef>
#include <vector>
#include "Bake_Kernel_PROC.h"
#include "../data/MapFields_DATA.h"
#include "../params/Geometry_PARAMS.h"
#include "../sys/Dispatch_SYS.h"

namespace SanmapGen {
namespace Sys { class GpuResourceManager; class ThreadPool; }

namespace Proc {

class BakeStage {
public:
    BakeStage(const Params::Geometry& geometrySettings, const Data::MapFields& inputFields,
              BakedTextureSet& outputTextures);

    // Configuration (all optional; ARCH §4.2 Gpu/Visual defaults are set in the constructor).
    BakeConstants& Constants() { return constants; }
    const BakeConstants& Constants() const { return constants; }
    StratumBakeSource& Stratum(int stratumIndex) { return stratumSources[stratumIndex]; }
    const StratumBakeSource& Stratum(int stratumIndex) const { return stratumSources[stratumIndex]; }
    void SetDispatchPolicy(const Sys::DispatchPolicy& policy) { dispatchPolicy = policy; }
    void SetGenerationContext(Sys::GenerationContext context) { generationContext = context; }
    void SetGlobalBackend(Sys::ComputeBackend backend) { globalBackend = backend; }
    void SetGpuResourceManager(Sys::GpuResourceManager* manager) { gpuResourceManager = manager; }
    // Optional: the Cpu twin bakes its output rows through this pool (rows are independent,
    // so the partition — and the result — do not depend on scheduling).
    void SetThreadPool(Sys::ThreadPool* pool) { threadPool = pool; }

    // Hash of everything this stage consumes from PARAMS: the output geometry, the stage
    // constants, and every stratum's bake settings. The mask CONTENT is deliberately absent
    // — upstream stages mix their own hashes in ahead of this one (Generation_PIPELINE), so
    // a mask change already dirties the bake.
    std::size_t ComputeParameterHash() const;

    // Resolves the backend through Sys::Dispatch and runs there. Returns the backend the
    // work actually ran on (Gpu resolves back to Cpu when no GL resources are available).
    Sys::ComputeBackend Run();

    // The two backends (public because Sys::Dispatch calls them; PIPELINE uses Run()).
    void RunOnCpu();
    void RunOnGpu();

    // Observability for tests/telemetry.
    Sys::ComputeBackend LastBackend() const { return lastBackend; }
    bool IsGpuAvailable() const { return bGpuProgramReady; }
    int  OutputResolution() const;
    const std::vector<StratumKernelConfiguration>& StratumConfigurations() const {
        return stratumConfigurations;
    }

private:
    // Flattens the stratum sources into StratumKernelConfiguration records, sizes the output
    // set, and packs the shared texel buffer both backends read.
    void PrepareRun();
    bool EnsureGpuResources();                 // Bake_Gpu_PROC.cpp
    void CompositeCpu();                       // Bake_Composite_PROC.cpp

    const Params::Geometry& geometry;
    const Data::MapFields&  mapFields;
    BakedTextureSet&        bakedTextures;
    BakeConstants           constants;
    StratumBakeSource       stratumSources[Data::MapFields::stratumCount];

    Sys::DispatchPolicy      dispatchPolicy;
    Sys::GenerationContext   generationContext  = Sys::GenerationContext::Output;
    Sys::ComputeBackend      globalBackend      = Sys::ComputeBackend::Gpu;
    Sys::GpuResourceManager* gpuResourceManager = nullptr;
    Sys::ThreadPool*         threadPool         = nullptr;

    std::vector<StratumKernelConfiguration> stratumConfigurations;
    std::vector<unsigned int>               packedAlbedoTexels;   // all stratum textures, concatenated
    std::vector<float>                      packedMaskValues;     // the 9 mask fields, concatenated

    Sys::ComputeBackend lastBackend      = Sys::ComputeBackend::Cpu;
    bool                bLastRunUsedGpu  = false;
    bool                bGpuProgramReady = false;
    int                 gpuProgramIndex  = -1;
};

} // namespace Proc
} // namespace SanmapGen
