// NoiseBlend_GpuBuffers_PROC.cpp — the stage's persistent GPU storage: the layer-configuration
// block both passes read, the per-layer raw-noise cache that makes the two-level dirty hash
// work on the Gpu too, and the heightfield / material-proportion outputs read into MapFields.
// Buffers are owned by GpuResource_SYS and reallocated ONLY on a size change; a reallocation
// throws away the cached noise, so it also invalidates the Gpu structural hashes.
// No GL handle appears here — this file only ever names buffers (Constitution §1, ARCH §3.2).
#include "NoiseBlend_PROC.h"
#include "../sys/GpuResource_SYS.h"
#include <string>

namespace SanmapGen {
namespace Proc {
namespace {

const std::string layerBufferName  = "noiseBlendLayerConfigurations";
const std::string noiseBufferName  = "noiseBlendRawNoise";
const std::string heightBufferName = "noiseBlendHeightField";
const std::string proportionBufferName   = "noiseBlendMaterialProportions";
const std::string thicknessBufferName = "noiseBlendLayerThickness";

} // namespace

bool NoiseBlendStage::EnsureGpuBuffers(std::size_t cellCount) {
    const std::size_t layerCount = layerConfigurations.size();
    bool bReallocated = gpuResourceManager->EnsureBuffer(
        layerBufferName, layerCount * sizeof(LayerKernelConfiguration));
    bReallocated |= gpuResourceManager->EnsureBuffer(noiseBufferName, layerCount * cellCount * sizeof(float));
    bReallocated |= gpuResourceManager->EnsureBuffer(heightBufferName, cellCount * sizeof(float));
    bReallocated |= gpuResourceManager->EnsureBuffer(
        proportionBufferName, static_cast<std::size_t>(Data::MapFields::stratumCount) * cellCount * sizeof(float));
    // Blend-pass scratch: sized by the ACTUAL layer count, so it is never the worst case.
    gpuResourceManager->EnsureBuffer(thicknessBufferName, layerCount * cellCount * sizeof(float));

    gpuResourceManager->BindBuffer(layerBufferName,     NoiseBlendBinding::layerConfigurations);
    gpuResourceManager->BindBuffer(noiseBufferName,     NoiseBlendBinding::rawNoise);
    gpuResourceManager->BindBuffer(heightBufferName,    NoiseBlendBinding::heightField);
    gpuResourceManager->BindBuffer(proportionBufferName,      NoiseBlendBinding::materialProportions);
    gpuResourceManager->BindBuffer(thicknessBufferName, NoiseBlendBinding::layerThickness);
    return bReallocated;
}

void NoiseBlendStage::UploadLayerConfigurationsGpu() {
    gpuResourceManager->UploadBuffer(layerBufferName, layerConfigurations.data(),
                                     layerConfigurations.size() * sizeof(LayerKernelConfiguration));
}

// One readback per output field. The proportion buffer is stratum-major, so it comes back
// whole through the staging vector and is scattered into the nine proportion fields.
void NoiseBlendStage::ReadbackFieldsGpu(std::size_t cellCount) {
    gpuResourceManager->ReadbackBuffer(heightBufferName, mapFields.heightfield.Data(),
                                       cellCount * sizeof(float));
    const std::size_t proportionValueCount = static_cast<std::size_t>(Data::MapFields::stratumCount) * cellCount;
    if (gpuTransferBuffer.size() != proportionValueCount) gpuTransferBuffer.assign(proportionValueCount, 0.0f);
    gpuResourceManager->ReadbackBuffer(proportionBufferName, gpuTransferBuffer.data(),
                                       proportionValueCount * sizeof(float));
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum) {
        float* destination = mapFields.materialProportions[stratum].Data();
        const float* source = gpuTransferBuffer.data() + static_cast<std::size_t>(stratum) * cellCount;
        for (std::size_t cell = 0; cell < cellCount; ++cell) destination[cell] = source[cell];
    }
}

} // namespace Proc
} // namespace SanmapGen
