// Mask_Gpu_PROC.cpp — the GPU speed path: compile the mask program once, keep persistent
// SSBOs, dispatch one invocation per vertex, fence, read back. Same algorithm and the same
// full configuration record as the CPU path — the backends differ only by accuracy class
// (DISPATCH_INTERFACE_SPEC §1/§3). If no GL resource manager is available the stage falls back
// to the CPU accuracy path rather than silently producing nothing (the old UseGPUFlowMap bug).
// The proportion buffer is uploaded read-only and the surface-weight and slope buffers are
// separate, write-only outputs — the GPU twin honours the single-writer rule exactly like the
// CPU twin, and bakes the same `slope` field (M5-0c).
#include "Mask_PROC.h"
#include "Mask_Merge_PROC.h"
#include "../sys/GpuResource_SYS.h"
#include <cstring>
#include <string>

namespace SanmapGen {
namespace Proc {
namespace {

const char* const kConfigurationBuffer   = "maskStratumConfigurations";
const char* const kHeightBuffer          = "maskHeightField";
const char* const kProportionBuffer      = "maskMaterialProportions";
const char* const kSurfaceWeightBuffer   = "maskSurfaceStratumWeights";
const char* const kStoredMaskBuffer      = "maskStoredArt";
const char* const kSlopeBuffer           = "maskSlopeField";

// Every literal the shader needs comes from the C++ side — nothing is hardcoded in GLSL (§8).
std::string BuildShaderDefinitions() {
    return "#define MASK_TILE_WIDTH "   + std::to_string(Sys::WorkgroupSize::kFieldTileWidth)
         + "\n#define MASK_TILE_HEIGHT " + std::to_string(Sys::WorkgroupSize::kFieldTileHeight)
         + "\n#define MASK_STRATUM_COUNT " + std::to_string(Data::MapFields::stratumCount)
         + "\n#define MASK_MERGE_DISABLED " + std::to_string(kMergeModeDisabled)
         + "\n#define MASK_MERGE_PROCEDURAL_START " + std::to_string(kMergeModeProceduralStart)
         + "\n#define MASK_MERGE_STATIC_OVERRIDE " + std::to_string(kMergeModeStaticOverride);
}

// The 9 fields of a family are packed stratum-major into one buffer, matching the shader's
// `stratum * cellCount + cellIndex` addressing.
void PackStratumFields(const Data::FloatField* fields, std::vector<float>& transfer, std::size_t cellCount) {
    transfer.resize(cellCount * Data::MapFields::stratumCount);
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
        std::memcpy(transfer.data() + stratum * cellCount, fields[stratum].Data(),
                    cellCount * sizeof(float));
}

void UnpackStratumFields(const std::vector<float>& transfer, Data::FloatField* fields, std::size_t cellCount) {
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
        std::memcpy(fields[stratum].Data(), transfer.data() + stratum * cellCount,
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
    const std::size_t fieldBytes = cellCount * Data::MapFields::stratumCount * sizeof(float);
    const std::size_t storedBytes = packedStoredMaskValues.size() * sizeof(float);
    PackStratumFields(mapFields.materialProportions, gpuProportionBuffer, cellCount);
    gpuSurfaceWeightBuffer.assign(cellCount * Data::MapFields::stratumCount, 0.0f);

    gpuResourceManager->EnsureBuffer(kConfigurationBuffer, configurationBytes);
    gpuResourceManager->EnsureBuffer(kHeightBuffer, cellCount * sizeof(float));
    gpuResourceManager->EnsureBuffer(kProportionBuffer, fieldBytes);
    gpuResourceManager->EnsureBuffer(kSurfaceWeightBuffer, fieldBytes);
    gpuResourceManager->EnsureBuffer(kStoredMaskBuffer, storedBytes);
    gpuResourceManager->EnsureBuffer(kSlopeBuffer, cellCount * sizeof(float));
    gpuResourceManager->UploadBuffer(kConfigurationBuffer, stratumConfigurations.data(), configurationBytes);
    gpuResourceManager->UploadBuffer(kHeightBuffer, mapFields.heightfield.Data(), cellCount * sizeof(float));
    gpuResourceManager->UploadBuffer(kProportionBuffer, gpuProportionBuffer.data(), fieldBytes);
    gpuResourceManager->UploadBuffer(kStoredMaskBuffer, packedStoredMaskValues.data(), storedBytes);
    gpuResourceManager->BindBuffer(kConfigurationBuffer, 0);
    gpuResourceManager->BindBuffer(kHeightBuffer, 1);
    gpuResourceManager->BindBuffer(kProportionBuffer, 2);
    gpuResourceManager->BindBuffer(kStoredMaskBuffer, 3);
    gpuResourceManager->BindBuffer(kSurfaceWeightBuffer, 4);
    gpuResourceManager->BindBuffer(kSlopeBuffer, 5);

    const Sys::GpuProgramHandle program{ gpuProgramIndex };
    gpuResourceManager->SetUniformInt(program, "vertexSize", vertexSize);
    const unsigned groupsX = (vertexSize + Sys::WorkgroupSize::kFieldTileWidth - 1) / Sys::WorkgroupSize::kFieldTileWidth;
    const unsigned groupsY = (vertexSize + Sys::WorkgroupSize::kFieldTileHeight - 1) / Sys::WorkgroupSize::kFieldTileHeight;
    gpuResourceManager->Dispatch(program, groupsX, groupsY, 1);

    Sys::GpuFenceHandle fence = gpuResourceManager->InsertFence();
    for (int spin = 0; spin < 1000000 && !gpuResourceManager->IsFenceSignaled(fence); ++spin) {}
    gpuResourceManager->DeleteFence(fence);
    gpuResourceManager->ReadbackBuffer(kSurfaceWeightBuffer, gpuSurfaceWeightBuffer.data(), fieldBytes);
    UnpackStratumFields(gpuSurfaceWeightBuffer, mapFields.surfaceStratumWeights, cellCount);
    gpuResourceManager->ReadbackBuffer(kSlopeBuffer, mapFields.slope.Data(), cellCount * sizeof(float));
    lastBackend = Sys::ComputeBackend::Gpu;
}

} // namespace Proc
} // namespace SanmapGen
