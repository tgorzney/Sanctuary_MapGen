// NoiseBlend_PROC.h — the noise/blend stage: per-layer noise, density/levels reshape, the
// height-blend stack, and the per-stratum material proportions. Layer: PROC — the FIRST stage of
// generation; everything downstream reads what it writes (NOISE_BLEND_SPEC).
// One kernel, two backends: RunOnCpu() is the accuracy path, RunOnGpu() the speed path, and
// both carry the SAME full layer configuration (the old GPU config omitted blend/fractal/
// type — that divergence is the bug this stage kills). The stage never picks a backend: it
// hands itself to Sys::Dispatch with the policy PIPELINE gave it (ARCH §4).
// Two-level dirty hash: structural noise per layer (reshape-independent) and the blended
// result, so cheap reshaping never re-rolls the cached noise.
#pragma once
#include <cstddef>
#include <vector>
#include "NoiseBlend_Kernel_PROC.h"
#include "../data/BakedLayerImage_DATA.h"
#include "../data/MapFields_DATA.h"
#include "../params/Geometry_PARAMS.h"
#include "../params/LayerStack_PARAMS.h"
#include "../sys/Dispatch_SYS.h"

namespace SanmapGen {
namespace Sys { class GpuResourceManager; class ThreadPool; }

namespace Proc {

class NoiseBlendStage {
public:
    NoiseBlendStage(const Params::Geometry& geometrySettings, const Params::LayerStack& layerStackSettings,
                    Data::MapFields& outputFields,
                    const std::vector<Data::BakedLayerImage>& bakedLayerImagesSettings);

    // Configuration (all optional; sane defaults per ARCH §4.2 for the Noise stage).
    NoiseBlendConstants& Constants() { return constants; }
    const NoiseBlendConstants& Constants() const { return constants; }
    void SetDispatchPolicy(const Sys::DispatchPolicy& policy) { dispatchPolicy = policy; }
    const Sys::DispatchPolicy& ActiveDispatchPolicy() const { return dispatchPolicy; }
    void SetGenerationContext(Sys::GenerationContext context) { generationContext = context; }
    void SetGlobalBackend(Sys::ComputeBackend backend) { globalBackend = backend; }
    void SetGpuResourceManager(Sys::GpuResourceManager* manager) { gpuResourceManager = manager; }
    void SetThreadPool(Sys::ThreadPool* pool) { threadPool = pool; }

    // Hash of everything this stage consumes: geometry (seed + size) and every setting of
    // every enabled layer, in stack order. This is what PIPELINE registers as the stage's
    // computeParamHash — change any layer setting and the stage (plus everything downstream)
    // re-runs; change nothing and it is skipped.
    std::size_t ComputeParameterHash() const;

    // Resolves the backend through Sys::Dispatch and runs there. Returns the backend used.
    Sys::ComputeBackend Run();

    // The two backends (public because Sys::Dispatch calls them; PIPELINE uses Run()).
    void RunOnCpu();
    void RunOnGpu();

    // Observability for tests/telemetry.
    Sys::ComputeBackend LastBackend() const { return lastBackend; }
    int RegeneratedLayerCount() const { return regeneratedLayerCount; }
    bool WasLastRunSkipped() const { return bLastRunSkipped; }
    bool IsGpuAvailable() const { return bGpuProgramReady; }
    const std::vector<LayerKernelConfiguration>& LayerConfigurations() const { return layerConfigurations; }
    // The stage's own cached raw noise, one FloatField per flattened layer, exposed so
    // STEP102's "Bake a live noise layer" UI action can snapshot a layer's CURRENT computed
    // output into a new Data::BakedLayerImage before flipping bBaked = true.
    const std::vector<Data::FloatField>& CachedRawNoiseCpu() const { return cachedRawNoiseCpu; }
    // The SAME flattened layer pointers PrepareRun() built `cachedRawNoiseCpu` from, in the SAME
    // call, so the two vectors are provably index-for-index in lockstep (STEP151). Lets a caller
    // find a layer's live noise slot by POINTER IDENTITY rather than recomputing
    // `layerStack.GetFlatLayers()` and trusting position, which silently reattaches to the wrong
    // layer after a stack reorder between this stage's last Run() and the lookup.
    const std::vector<const Params::Layer*>& CachedFlatLayerPointers() const {
        return cachedFlatLayerPointers;
    }

private:
    // Flattens the enabled layers into LayerKernelConfiguration records and sizes the caches.
    void PrepareRun();
    // The frequency the kernels are handed for one layer — `Params::Geometry::
    // bScaleFeaturesToMapSize` applied to the layer's own (NoiseBlend_Kernel_PROC.h). Resolved in
    // ONE place so the flatten and the dirty hash can never disagree about it.
    float EffectiveFrequencyOfLayer(const Params::Layer& layer) const;
    std::size_t ComputeStructuralNoiseHash(std::size_t layerIndex) const;
    std::size_t ComputeBlendHash() const;
    void GenerateLayerNoiseCpu(std::size_t layerIndex);   // NoiseBlend_Noise_PROC.cpp
    void BlendLayersCpu();                                // NoiseBlend_Blend_PROC.cpp
    void ClearMaterialProportions();
    bool EnsureGpuResources();                            // NoiseBlend_GpuProgram_PROC.cpp
    bool EnsureGpuBuffers(std::size_t cellCount);         // NoiseBlend_GpuBuffers_PROC.cpp
    void UploadLayerConfigurationsGpu();                  // NoiseBlend_GpuBuffers_PROC.cpp
    void ReadbackFieldsGpu(std::size_t cellCount);        // NoiseBlend_GpuBuffers_PROC.cpp
    void DispatchNoisePassGpu(int layerIndex, int vertexSize);  // NoiseBlend_Gpu_PROC.cpp
    void DispatchBlendPassGpu(int vertexSize);            // NoiseBlend_Gpu_PROC.cpp
    void RegenerateChangedLayersGpu(int vertexSize);      // NoiseBlend_Gpu_PROC.cpp
    bool WaitForGpuFence();                               // NoiseBlend_Gpu_PROC.cpp

    const Params::Geometry&   geometry;
    const Params::LayerStack& layerStack;
    Data::MapFields&          mapFields;
    const std::vector<Data::BakedLayerImage>& bakedLayerImages;
    NoiseBlendConstants       constants;

    Sys::DispatchPolicy      dispatchPolicy;                                   // ARCH §4.2 defaults
    Sys::GenerationContext   generationContext = Sys::GenerationContext::Output;
    Sys::ComputeBackend      globalBackend     = Sys::ComputeBackend::Cpu;
    Sys::GpuResourceManager* gpuResourceManager = nullptr;
    Sys::ThreadPool*         threadPool         = nullptr;

    std::vector<LayerKernelConfiguration> layerConfigurations;
    std::vector<Data::FloatField>         cachedRawNoiseCpu;
    std::vector<const Params::Layer*>     cachedFlatLayerPointers;   // lockstep with cachedRawNoiseCpu
    std::vector<std::size_t>              cachedStructuralHashesCpu;
    std::vector<std::size_t>              cachedStructuralHashesGpu;
    std::vector<float>                    gpuTransferBuffer;

    std::size_t         cachedBlendHash = 0;
    bool                bBlendCacheValid = false;
    Sys::ComputeBackend cachedBlendBackend = Sys::ComputeBackend::Cpu;
    Sys::ComputeBackend lastBackend        = Sys::ComputeBackend::Cpu;
    int                 regeneratedLayerCount = 0;
    bool                bLastRunSkipped   = false;
    bool                bGpuProgramReady  = false;
    int                 gpuProgramIndex   = -1;
};

} // namespace Proc
} // namespace SanmapGen
