// NoiseBlend_Gpu_PROC.cpp — the Gpu speed path: the same two-level dirty hash as the Cpu
// accuracy path, one noise dispatch per structurally-changed layer, one blend dispatch, then a
// fenced readback into MapFields. Twin of NoiseBlend_PROC.glsl; the layer configurations it
// uploads are the SAME records the Cpu consumes, so a layer bakes the same shape either way
// (NOISE_BLEND_SPEC "CPU vs GPU" — the config omission this stage exists to kill).
// With no Gpu program available the stage falls back to the Cpu and says so through
// LastBackend() — it never silently produces nothing (Constitution §6).
#include "NoiseBlend_PROC.h"
#include "../sys/GpuResource_SYS.h"
#include <thread>

namespace SanmapGen {
namespace Proc {
namespace {

unsigned TileGroupCount(int vertexSize, int tileSize) {
    return static_cast<unsigned>((vertexSize + tileSize - 1) / tileSize);
}

} // namespace

void NoiseBlendStage::DispatchNoisePassGpu(int layerIndex, int vertexSize) {
    const Sys::GpuProgramHandle program{ gpuProgramIndex };
    gpuResourceManager->SetUniformInt(program, "passMode", NoiseBlendPassMode::Noise);
    gpuResourceManager->SetUniformInt(program, "activeLayerIndex", layerIndex);
    gpuResourceManager->Dispatch(program, TileGroupCount(vertexSize, Sys::WorkgroupSize::kFieldTileWidth),
                                 TileGroupCount(vertexSize, Sys::WorkgroupSize::kFieldTileHeight), 1u);
}

void NoiseBlendStage::DispatchBlendPassGpu(int vertexSize) {
    const Sys::GpuProgramHandle program{ gpuProgramIndex };
    gpuResourceManager->SetUniformInt(program, "passMode", NoiseBlendPassMode::Blend);
    gpuResourceManager->Dispatch(program, TileGroupCount(vertexSize, Sys::WorkgroupSize::kFieldTileWidth),
                                 TileGroupCount(vertexSize, Sys::WorkgroupSize::kFieldTileHeight), 1u);
}

// First hash level: only the layers whose STRUCTURAL settings changed are re-dispatched; the
// rest keep their cached raw noise in the persistent SSBO (NOISE_BLEND_SPEC "Cache").
void NoiseBlendStage::RegenerateChangedLayersGpu(int vertexSize) {
    const Sys::GpuProgramHandle program{ gpuProgramIndex };
    gpuResourceManager->SetUniformInt(program, "vertexSize", vertexSize);
    gpuResourceManager->SetUniformInt(program, "layerCount", static_cast<int>(layerConfigurations.size()));
    for (std::size_t index = 0; index < layerConfigurations.size(); ++index) {
        const std::size_t structuralHash = ComputeStructuralNoiseHash(index);
        if (cachedStructuralHashesGpu[index] == structuralHash) continue;
        DispatchNoisePassGpu(static_cast<int>(index), vertexSize);
        cachedStructuralHashesGpu[index] = structuralHash;
        ++regeneratedLayerCount;
    }
}

// Non-blocking poll that YIELDS the core between attempts. A bare spin here measurably starves
// the ThreadPool workers sharing the same cores, so the yield is mandatory, not a nicety.
bool NoiseBlendStage::WaitForGpuFence() {
    const Sys::GpuFenceHandle fence = gpuResourceManager->InsertFence();
    bool bSignaled = false;
    for (int poll = 0; poll < constants.gpuFenceMaximumPollCount && !bSignaled; ++poll) {
        bSignaled = gpuResourceManager->IsFenceSignaled(fence);
        if (!bSignaled) std::this_thread::yield();
    }
    gpuResourceManager->DeleteFence(fence);
    return bSignaled;
}

void NoiseBlendStage::RunOnGpu() {
    PrepareRun();
    // No layers, a stack deeper than the Gpu scratch bound, or no GL program at all: the Cpu
    // twin is always legal, so take it and record that it is what ran.
    const bool bStackFitsKernel =
        !layerConfigurations.empty()
        && layerConfigurations.size() <= static_cast<std::size_t>(constants.maximumGpuLayerCount);
    if (!bStackFitsKernel || !EnsureGpuResources()) { RunOnCpu(); return; }

    const std::size_t blendHash = ComputeBlendHash();
    if (bBlendCacheValid && blendHash == cachedBlendHash && cachedBlendBackend == Sys::ComputeBackend::Gpu) {
        bLastRunSkipped = true;
        regeneratedLayerCount = 0;
        return;
    }
    bLastRunSkipped = false;
    regeneratedLayerCount = 0;

    const int vertexSize = geometry.VertexSize();
    const std::size_t cellCount = static_cast<std::size_t>(vertexSize) * vertexSize;
    // A reallocation discards the cached raw noise, so every layer must be regenerated.
    if (EnsureGpuBuffers(cellCount))
        cachedStructuralHashesGpu.assign(layerConfigurations.size(), ~std::size_t(0));
    UploadLayerConfigurationsGpu();

    RegenerateChangedLayersGpu(vertexSize);
    DispatchBlendPassGpu(vertexSize);
    WaitForGpuFence();
    ReadbackFieldsGpu(cellCount);

    cachedBlendHash = blendHash;
    cachedBlendBackend = Sys::ComputeBackend::Gpu;
    bBlendCacheValid = true;
    lastBackend = Sys::ComputeBackend::Gpu;
}

} // namespace Proc
} // namespace SanmapGen
