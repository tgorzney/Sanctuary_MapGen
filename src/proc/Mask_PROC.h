// Mask_PROC.h — the Mask stage: resolve `surfaceStratumWeights` from `materialProportions`,
// the slope gate, and the stored .sanmap stratum art (ARCH §7.2, MASKING_SPEC Part 1).
// Layer: PROC. Runs AFTER every sim (ARCH §7.4), so the gate sees the FINAL slope and the
// FINAL proportions, and before Placement/Bake, which consume the resolved weights.
//
//   gate_s        = SlopeGateWeight(slopeGradient, stratum_s)
//   procedural_s  = materialProportions[s] * gate_s
//   merged_s      = Merge(procedural_s, storedArt_s, importedMaskMode_s)
//   surfaceStratumWeights[s] = Remap_s(merged_s)
//
// Single-writer + purity (ARCH §3.4): the ONLY fields this stage writes are
// `surfaceStratumWeights` and `slope` — the gradient magnitude the gate already needs, baked so
// Placement and the preview composite SAMPLE one slope instead of each deriving their own
// (M5-0c). It never writes `materialProportions` — gating the physical field in place let a
// renormalizing sim undo the gate and made the stage a non-idempotent read-modify-write. Inputs
// are read-only, so the dirty-hash conductor may re-run Mask alone, twice, and get the same
// answer.
//
// APPROXIMATION IN FORCE (MASKING_SPEC 1.9 / ARCH §7.5): `materialProportions` is a volume
// fraction, not surface exposure; until the persistent ordered thickness stack lands (M6) it
// stands in for exposure. The kernel does not change when that lands — only its input binding.
//
// One kernel, two backends: RunOnCpu() is the accuracy path, RunOnGpu() the speed path, both
// driven from the identical MaskStratumConfiguration records. The stage never picks a backend:
// it hands itself to Sys::Dispatch with the policy PIPELINE gave it (ARCH §4).
#pragma once
#include <cstddef>
#include <vector>
#include "Mask_Kernel_PROC.h"
#include "../data/MapFields_DATA.h"
#include "../data/StratumArt_DATA.h"
#include "../params/Geometry_PARAMS.h"
#include "../params/SlopeDefaults_PARAMS.h"
#include "../params/Stratum_PARAMS.h"
#include "../sys/Dispatch_SYS.h"

namespace SanmapGen {
namespace Sys { class GpuResourceManager; class ThreadPool; }

namespace Proc {

class MaskStage {
public:
    MaskStage(const Params::Geometry& geometrySettings,
              const std::vector<Params::Stratum>& stratumSettings,
              const std::vector<Data::StratumArt>& stratumArtInput,
              Data::MapFields& outputFields,
              const Params::SlopeDefaults& slopeDefaultSettings);

    // Configuration (all optional; ARCH §4.2 Mask defaults = Gpu/Visual preview, Cpu/Accurate
    // output — and Mask sits in the Output Exact chain because Placement consumes it, §4.6).
    MaskConstants& Constants() { return constants; }
    const MaskConstants& Constants() const { return constants; }
    void SetDispatchPolicy(const Sys::DispatchPolicy& policy) { dispatchPolicy = policy; }
    const Sys::DispatchPolicy& ActiveDispatchPolicy() const { return dispatchPolicy; }
    void SetGenerationContext(Sys::GenerationContext context) { generationContext = context; }
    void SetGlobalBackend(Sys::ComputeBackend backend) { globalBackend = backend; }
    void SetGpuResourceManager(Sys::GpuResourceManager* manager) { gpuResourceManager = manager; }
    void SetThreadPool(Sys::ThreadPool* pool) { threadPool = pool; }

    // Hash of everything this stage consumes that is its OWN: geometry scale, the stage
    // constants, the shared `slopeDefaults` record, every per-stratum mask setting (including
    // which SOURCE record — its own fields or `slopeDefaults` — `bSlopeUseGlobal` selects), and
    // the stored art's content. The heightfield and the proportions this stage reads belong to
    // UPSTREAM stages, so they are deliberately not hashed here — Generation_PIPELINE mixes the
    // upstream hash forward, which is what makes "upstream changed => mask re-runs" work without
    // this stage knowing the pipeline shape.
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

    const Params::Geometry&               geometry;
    const std::vector<Params::Stratum>&   strata;
    const std::vector<Data::StratumArt>&  stratumArt;
    Data::MapFields&                      mapFields;
    const Params::SlopeDefaults&          slopeDefaults;
    MaskConstants                         constants;

    Sys::DispatchPolicy      dispatchPolicy;                                   // ARCH §4.2 defaults
    Sys::GenerationContext   generationContext  = Sys::GenerationContext::Output;
    Sys::ComputeBackend      globalBackend      = Sys::ComputeBackend::Cpu;
    Sys::GpuResourceManager* gpuResourceManager = nullptr;
    Sys::ThreadPool*         threadPool         = nullptr;

    std::vector<MaskStratumConfiguration> stratumConfigurations;
    std::vector<float>                    packedStoredMaskValues;   // every stratum's art, one buffer
    std::vector<float>                    gpuProportionBuffer;      // upload staging (read-only input)
    std::vector<float>                    gpuSurfaceWeightBuffer;   // readback staging (the output)

    Sys::ComputeBackend lastBackend      = Sys::ComputeBackend::Cpu;
    bool                bGpuProgramReady = false;
    int                 gpuProgramIndex  = -1;
};

} // namespace Proc
} // namespace SanmapGen
