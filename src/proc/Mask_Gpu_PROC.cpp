// Mask_Gpu_PROC.cpp — the GPU speed path: compile the mask program once, keep persistent
// SSBOs, dispatch one invocation per vertex, fence, read back. Same algorithm and the same
// full configuration record as the CPU path — the backends differ only by accuracy class
// (DISPATCH_INTERFACE_SPEC §1/§3). If no GL resource manager is available the stage falls back
// to the CPU accuracy path rather than silently producing nothing (the old UseGPUFlowMap bug).
#include "Mask_PROC.h"
#include "Mask_Merge_PROC.h"
#include "../sys/GpuResource_SYS.h"
#include <cstring>
#include <string>

namespace SanmapGen {
namespace Proc {
namespace {

const char* const kConfigurationBuffer = "maskStratumConfigurations";
const char* const kHeightBuffer        = "maskHeightField";
const char* const kMaterialMaskBuffer  = "maskMaterialMasks";
const char* const kStoredMaskBuffer    = "maskStoredMasks";

// Every literal the shader needs comes from the C++ side — nothing is hardcoded in GLSL (§8).
std::string BuildShaderDefinitions() {
    return "#define MASK_TILE_WIDTH "   + std::to_string(Sys::WorkgroupSize::kFieldTileWidth)
         + "\n#define MASK_TILE_HEIGHT " + std::to_string(Sys::WorkgroupSize::kFieldTileHeight)
         + "\n#define MASK_STRATUM_COUNT " + std::to_string(Data::MapFields::stratumCount)
         + "\n#define MASK_MERGE_DISABLED " + std::to_string(kMergeModeDisabled)
         + "\n#define MASK_MERGE_PROCEDURAL_START " + std::to_string(kMergeModeProceduralStart)
         + "\n#define MASK_MERGE_STATIC_OVERRIDE " + std::to_string(kMergeModeStaticOverride);
}

// The 9 mask fields are packed stratum-major into one buffer, matching the shader's
// `stratum * cellCount + cellIndex` addressing.
void PackMaterialMasks(const Data::MapFields& fields, std::vector<float>& transfer, std::size_t cellCount) {
    transfer.resize(cellCount * Data::MapFields::stratumCount);
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
        std::memcpy(transfer.data() + stratum * cellCount, fields.materialMasks[stratum].Data(),
                    cellCount * sizeof(float));
}

void UnpackMaterialMasks(const std::vector<float>& transfer, Data::MapFields& fields, std::size_t cellCount) {
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
        std::memcpy(fields.materialMasks[stratum].Data(), transfer.data() + stratum * cellCount,
                    cellCount * sizeof(float));
}

} // namespace

bool MaskStage::EnsureGpuResources() {
    if (gpuResourceManager == nullptr) return false;
    if (!gpuResourceManager->IsInitialized() && !gpuResourceManager->Initialize()) return false;
    if (bGpuProgramReady) return true;
    const Sys::GpuProgramHandle program = gpuResourceManager->GetOrCompileProgramFromParts(
        { "Mask_PROC.glsl", "Mask_Slope_PROC.glsl", "Mask_Merge_PROC.glsl" }, BuildShaderDefinitions());
    if (!program.IsValid()) return false;
    gpuProgramIndex  = program.programIndex;
    bGpuProgramReady = true;
    return true;
}

void MaskStage::RunOnGpu() {
    PrepareRun();
    const int vertexSize = geometry.VertexSize();
    if (!mapFields.IsSized() || mapFields.VertexSize() != vertexSize) return;   // validate input
    if (!EnsureGpuResources()) { RunOnCpu(); return; }                          // accuracy fallback

    const std::size_t cellCount = static_cast<std::size_t>(vertexSize) * vertexSize;
    const std::size_t configurationBytes = stratumConfigurations.size() * sizeof(MaskStratumConfiguration);
    const std::size_t maskBytes = cellCount * Data::MapFields::stratumCount * sizeof(float);
    const std::size_t storedBytes = packedStoredMaskValues.size() * sizeof(float);
    PackMaterialMasks(mapFields, gpuTransferBuffer, cellCount);

    gpuResourceManager->EnsureBuffer(kConfigurationBuffer, configurationBytes);
    gpuResourceManager->EnsureBuffer(kHeightBuffer, cellCount * sizeof(float));
    gpuResourceManager->EnsureBuffer(kMaterialMaskBuffer, maskBytes);
    gpuResourceManager->EnsureBuffer(kStoredMaskBuffer, storedBytes);
    gpuResourceManager->UploadBuffer(kConfigurationBuffer, stratumConfigurations.data(), configurationBytes);
    gpuResourceManager->UploadBuffer(kHeightBuffer, mapFields.heightfield.Data(), cellCount * sizeof(float));
    gpuResourceManager->UploadBuffer(kMaterialMaskBuffer, gpuTransferBuffer.data(), maskBytes);
    gpuResourceManager->UploadBuffer(kStoredMaskBuffer, packedStoredMaskValues.data(), storedBytes);
    gpuResourceManager->BindBuffer(kConfigurationBuffer, 0);
    gpuResourceManager->BindBuffer(kHeightBuffer, 1);
    gpuResourceManager->BindBuffer(kMaterialMaskBuffer, 2);
    gpuResourceManager->BindBuffer(kStoredMaskBuffer, 3);

    const Sys::GpuProgramHandle program{ gpuProgramIndex };
    gpuResourceManager->SetUniformInt(program, "vertexSize", vertexSize);
    const unsigned groupsX = (vertexSize + Sys::WorkgroupSize::kFieldTileWidth - 1) / Sys::WorkgroupSize::kFieldTileWidth;
    const unsigned groupsY = (vertexSize + Sys::WorkgroupSize::kFieldTileHeight - 1) / Sys::WorkgroupSize::kFieldTileHeight;
    gpuResourceManager->Dispatch(program, groupsX, groupsY, 1);

    Sys::GpuFenceHandle fence = gpuResourceManager->InsertFence();
    for (int spin = 0; spin < 1000000 && !gpuResourceManager->IsFenceSignaled(fence); ++spin) {}
    gpuResourceManager->DeleteFence(fence);
    gpuResourceManager->ReadbackBuffer(kMaterialMaskBuffer, gpuTransferBuffer.data(), maskBytes);
    UnpackMaterialMasks(gpuTransferBuffer, mapFields, cellCount);
    lastBackend = Sys::ComputeBackend::Gpu;
}

} // namespace Proc
} // namespace SanmapGen
