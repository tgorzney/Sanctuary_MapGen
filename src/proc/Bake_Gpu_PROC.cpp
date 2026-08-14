// Bake_Gpu_PROC.cpp — the Gpu speed path of the bake stage (its default backend in BOTH
// contexts, ARCH §4.2). Packs the stage inputs into the persistent SSBOs GpuResource_SYS
// owns, dispatches Bake_PROC.glsl once over the output texture, waits on a fence (never a
// blocking map on the hot path) and reads the baked texture set back.
// GL handles never appear here — only the opaque handles the SYS seam exposes (ARCH §3.2).
// If no GL context/manager is available the stage falls back to the Cpu twin and reports
// Cpu as the backend actually used, rather than silently producing nothing.
#include "Bake_PROC.h"
#include "Bake_Sampling_PROC.h"
#include "../sys/GpuResource_SYS.h"
#include <cstring>
#include <string>
#include <thread>

namespace SanmapGen {
namespace Proc {
namespace {

const char* const bakeShaderFileName       = "Bake_PROC.glsl";
const char* const configurationsBufferName = "bakeStratumConfigurations";
const char* const maskBufferName           = "bakeMaterialMasks";
const char* const albedoBufferName         = "bakeAlbedoTexels";
const char* const compositeBufferName      = "bakeCompositeAlbedo";
const char* const maskLowBufferName        = "bakeStratumMaskLow";
const char* const maskHighBufferName       = "bakeStratumMaskHigh";
// The fence poll is an early-out, not a correctness requirement (the readback below is
// ordered after the dispatch by GL itself), so it yields instead of burning a core: a busy
// spin here costs ~300 ms of pure CPU when other threads are competing for the machine.
constexpr int fencePollLimit = 100000;

void PackMaskValues(const Data::MapFields& mapFields, std::vector<float>& packed) {
    const std::size_t cellCount = mapFields.materialMasks[0].CellCount();
    packed.resize(cellCount * Data::MapFields::stratumCount);
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
        std::memcpy(packed.data() + static_cast<std::size_t>(stratum) * cellCount,
                    mapFields.materialMasks[stratum].Data(), cellCount * sizeof(float));
}

// Concatenates the stratum albedo textures in the exact order PrepareRun assigned the
// offsets. Never empty: GL will not allocate a zero-byte buffer.
void PackAlbedoTexels(const StratumBakeSource* sources,
                      const std::vector<StratumKernelConfiguration>& configurations,
                      std::vector<unsigned int>& packed) {
    packed.clear();
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum) {
        const StratumKernelConfiguration& configuration = configurations[stratum];
        if (configuration.albedoWidth <= 0 || configuration.albedoHeight <= 0) continue;
        const std::size_t texelCount = static_cast<std::size_t>(configuration.albedoWidth)
                                     * configuration.albedoHeight;
        packed.insert(packed.end(), sources[stratum].albedoPixels,
                      sources[stratum].albedoPixels + texelCount);
    }
    if (packed.empty()) packed.push_back(0u);
}

void EnsureAndBind(Sys::GpuResourceManager& manager, const char* bufferName,
                   const void* data, std::size_t byteSize, unsigned bindingIndex) {
    manager.EnsureBuffer(bufferName, byteSize);
    if (data != nullptr) manager.UploadBuffer(bufferName, data, byteSize);
    manager.BindBuffer(bufferName, bindingIndex);
}

// Three inputs (configurations, masks, albedo texels) and three outputs, on the binding
// indices Bake_PROC.glsl declares.
void BindBakeBuffers(Sys::GpuResourceManager& manager,
                     const std::vector<StratumKernelConfiguration>& configurations,
                     const std::vector<float>& maskValues, const std::vector<unsigned int>& albedoTexels,
                     std::size_t outputByteSize) {
    EnsureAndBind(manager, configurationsBufferName, configurations.data(),
                  configurations.size() * sizeof(StratumKernelConfiguration), 0);
    EnsureAndBind(manager, maskBufferName, maskValues.data(), maskValues.size() * sizeof(float), 1);
    EnsureAndBind(manager, albedoBufferName, albedoTexels.data(),
                  albedoTexels.size() * sizeof(unsigned int), 2);
    EnsureAndBind(manager, compositeBufferName, nullptr, outputByteSize, 3);
    EnsureAndBind(manager, maskLowBufferName, nullptr, outputByteSize, 4);
    EnsureAndBind(manager, maskHighBufferName, nullptr, outputByteSize, 5);
}

// The SYS seam exposes int uniforms only, so the one float constant travels pre-quantized.
void SetBakeUniforms(Sys::GpuResourceManager& manager, Sys::GpuProgramHandle program, int vertexSize,
                     int resolution, int baseStratumIndex, float compositeAlphaValue) {
    float alphaValue = compositeAlphaValue < 0.0f ? 0.0f : (compositeAlphaValue > 1.0f ? 1.0f : compositeAlphaValue);
    manager.SetUniformInt(program, "vertexSize", vertexSize);
    manager.SetUniformInt(program, "outputResolution", resolution);
    manager.SetUniformInt(program, "baseStratumIndex", baseStratumIndex);
    manager.SetUniformInt(program, "compositeAlphaByte", static_cast<int>(alphaValue * bakeByteScale + 0.5f));
}

void DispatchAndWait(Sys::GpuResourceManager& manager, Sys::GpuProgramHandle program, int resolution) {
    const unsigned groupsX = (resolution + Sys::WorkgroupSize::kFieldTileWidth - 1)
                           / Sys::WorkgroupSize::kFieldTileWidth;
    const unsigned groupsY = (resolution + Sys::WorkgroupSize::kFieldTileHeight - 1)
                           / Sys::WorkgroupSize::kFieldTileHeight;
    manager.Dispatch(program, groupsX, groupsY, 1);
    const Sys::GpuFenceHandle fence = manager.InsertFence();
    for (int poll = 0; poll < fencePollLimit && !manager.IsFenceSignaled(fence); ++poll)
        std::this_thread::yield();
    manager.DeleteFence(fence);
}

} // namespace

bool BakeStage::EnsureGpuResources() {
    if (gpuResourceManager == nullptr) return false;
    if (!gpuResourceManager->IsInitialized() && !gpuResourceManager->Initialize()) return false;
    if (bGpuProgramReady) return true;
    const std::string definitions =
        "#define BAKE_TILE_WIDTH " + std::to_string(Sys::WorkgroupSize::kFieldTileWidth) +
        "\n#define BAKE_TILE_HEIGHT " + std::to_string(Sys::WorkgroupSize::kFieldTileHeight) +
        "\n#define BAKE_STRATUM_COUNT " + std::to_string(Data::MapFields::stratumCount);
    const Sys::GpuProgramHandle program =
        gpuResourceManager->GetOrCompileProgram(bakeShaderFileName, definitions);
    if (!program.IsValid()) return false;
    gpuProgramIndex = program.programIndex;
    bGpuProgramReady = true;
    return true;
}

void BakeStage::RunOnGpu() {
    PrepareRun();
    const int resolution = bakedTextures.resolution;
    if (resolution <= 0 || !mapFields.IsSized()) return;
    if (!EnsureGpuResources()) { CompositeCpu(); return; }   // no GL -> the Cpu twin

    Sys::GpuResourceManager& manager = *gpuResourceManager;
    const Sys::GpuProgramHandle program{ gpuProgramIndex };
    const std::size_t outputByteSize = static_cast<std::size_t>(resolution) * resolution
                                     * sizeof(unsigned int);
    PackMaskValues(mapFields, packedMaskValues);
    PackAlbedoTexels(stratumSources, stratumConfigurations, packedAlbedoTexels);
    BindBakeBuffers(manager, stratumConfigurations, packedMaskValues, packedAlbedoTexels, outputByteSize);

    const int baseStratum = constants.baseStratumIndex < 0
                         || constants.baseStratumIndex >= Data::MapFields::stratumCount
                          ? 0 : constants.baseStratumIndex;
    SetBakeUniforms(manager, program, mapFields.VertexSize(), resolution, baseStratum,
                    constants.compositeAlphaValue);
    DispatchAndWait(manager, program, resolution);

    manager.ReadbackBuffer(compositeBufferName, bakedTextures.compositeAlbedo.data(), outputByteSize);
    manager.ReadbackBuffer(maskLowBufferName, bakedTextures.stratumMaskLow.data(), outputByteSize);
    manager.ReadbackBuffer(maskHighBufferName, bakedTextures.stratumMaskHigh.data(), outputByteSize);
    bLastRunUsedGpu = true;
}

} // namespace Proc
} // namespace SanmapGen
