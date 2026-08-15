// Bake_PROC.h — the bake stage: composite the stratum textures weighted by the SURFACE
// STRATUM WEIGHTS into the output texture set. Layer: PROC — the LAST stage of generation; it
// reads what every upstream stage wrote and produces the `Textures/` payload
// (GAMEDATA_LAYOUT_SPEC, PREVIEW_COMPOSITING_SPEC).
// It consumes `surfaceStratumWeights` VERBATIM (ARCH §7.2.5): the one remap already happened
// in the Mask stage, so this stage has no remap of its own — having both double-remapped any
// .sanmap that set them. Per-stratum settings come from `Params::Stratum` and the albedo
// pixels from `Data::StratumArt`; the stage keeps no private per-stratum array (ARCH §7.1).
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
#include "../data/StratumArt_DATA.h"
#include "../params/Geometry_PARAMS.h"
#include "../params/Stratum_PARAMS.h"
#include "../sys/Dispatch_SYS.h"

namespace SanmapGen {
namespace Sys { class GpuResourceManager; class ThreadPool; }

namespace Proc {

class BakeStage {
public:
    BakeStage(const Params::Geometry& geometrySettings,
              const std::vector<Params::Stratum>& stratumSettings,
              const std::vector<Data::StratumArt>& stratumArtInput,
              const Data::MapFields& inputFields, BakedTextureSet& outputTextures);

    // Configuration (all optional; ARCH §4.2 Gpu/Visual defaults are set in the constructor).
    BakeConstants& Constants() { return constants; }
    const BakeConstants& Constants() const { return constants; }
    void SetDispatchPolicy(const Sys::DispatchPolicy& policy) { dispatchPolicy = policy; }
    void SetGenerationContext(Sys::GenerationContext context) { generationContext = context; }
    void SetGlobalBackend(Sys::ComputeBackend backend) { globalBackend = backend; }
    void SetGpuResourceManager(Sys::GpuResourceManager* manager) { gpuResourceManager = manager; }
    // Optional: the Cpu twin bakes its output rows through this pool (rows are independent,
    // so the partition — and the result — do not depend on scheduling).
    void SetThreadPool(Sys::ThreadPool* pool) { threadPool = pool; }

    // Hash of everything this stage consumes from PARAMS: the output geometry, the stage
    // constants, every stratum's appearance settings and its albedo version. The weight FIELD
    // content is deliberately absent — upstream stages mix their own hashes in ahead of this
    // one (Generation_PIPELINE), so a mask change already dirties the bake.
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
    // Flattens the stratum settings + art into StratumKernelConfiguration records, sizes the
    // output set, and packs the shared texel buffer both backends read.
    void PrepareRun();
    bool EnsureGpuResources();                 // Bake_Gpu_PROC.cpp
    void CompositeCpu();                       // Bake_Composite_PROC.cpp

    const Params::Geometry&              geometry;
    const std::vector<Params::Stratum>&  strata;
    const std::vector<Data::StratumArt>& stratumArt;
    const Data::MapFields&               mapFields;
    BakedTextureSet&                     bakedTextures;
    BakeConstants                        constants;

    Sys::DispatchPolicy      dispatchPolicy;
    Sys::GenerationContext   generationContext  = Sys::GenerationContext::Output;
    Sys::ComputeBackend      globalBackend      = Sys::ComputeBackend::Gpu;
    Sys::GpuResourceManager* gpuResourceManager = nullptr;
    Sys::ThreadPool*         threadPool         = nullptr;

    std::vector<StratumKernelConfiguration> stratumConfigurations;
    std::vector<unsigned int>               packedAlbedoTexels;   // all stratum textures, concatenated
    std::vector<float>                      packedSurfaceWeights; // the 9 weight fields, concatenated

    Sys::ComputeBackend lastBackend      = Sys::ComputeBackend::Cpu;
    bool                bLastRunUsedGpu  = false;
    bool                bGpuProgramReady = false;
    int                 gpuProgramIndex  = -1;
};

} // namespace Proc
} // namespace SanmapGen
