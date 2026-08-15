// Placement_Gpu_PROC.cpp — the Gpu speed path: the preview density gate only.
// Placement_PROC.glsl evaluates one rule's per-cell gate weight; the identical Cpu
// acceptance then samples that field, so the preview shows the SAME placement the bake
// produces (no shadow re-filtering, ARCH §3.2). Gpu resources are owned by GpuResource_SYS —
// no GL handle crosses into PROC (ARCH §3.2). Any failure (no manager, no context, compile
// error) returns false and the caller falls back to the Cpu twin.
#include "Placement_PROC.h"
#include "../sys/GpuResource_SYS.h"
#include <string>

namespace SanmapGen {
namespace Proc {
namespace {

const char* const gateProgramFileName    = "Placement_PROC.glsl";
const char* const heightBufferName       = "PlacementHeightField";
const char* const slopeBufferName        = "PlacementSlopeField";   // the BAKED gradient magnitude
const char* const maskBufferName         = "PlacementMaskField";
const char* const obstacleBufferName     = "PlacementObstacleField";
const char* const gateBufferName         = "PlacementGateWeightField";
const char* const configurationBufferName = "PlacementRuleConfiguration";

// Per-RUN uploads (the fields every rule shares) — not per-rule, so a 40-rule recipe pays
// one heightfield transfer, not forty.
void UploadSharedFields(Sys::GpuResourceManager& manager, const float* heightValues,
                        const float* slopeValues, const float* obstacleValues, std::size_t fieldBytes) {
    manager.EnsureBuffer(heightBufferName, fieldBytes);
    manager.UploadBuffer(heightBufferName, heightValues, fieldBytes);
    manager.EnsureBuffer(slopeBufferName, fieldBytes);
    if (slopeValues != nullptr) manager.UploadBuffer(slopeBufferName, slopeValues, fieldBytes);
    manager.EnsureBuffer(obstacleBufferName, fieldBytes);
    if (obstacleValues != nullptr) manager.UploadBuffer(obstacleBufferName, obstacleValues, fieldBytes);
    manager.EnsureBuffer(maskBufferName, fieldBytes);
    manager.EnsureBuffer(gateBufferName, fieldBytes);
}

void BindGateBuffers(Sys::GpuResourceManager& manager) {
    manager.BindBuffer(configurationBufferName, 0);
    manager.BindBuffer(heightBufferName, 1);
    manager.BindBuffer(slopeBufferName, 2);
    manager.BindBuffer(maskBufferName, 3);
    manager.BindBuffer(obstacleBufferName, 4);
    manager.BindBuffer(gateBufferName, 5);
}

void DispatchGateProgram(Sys::GpuResourceManager& manager, Sys::GpuProgramHandle program,
                         const PlacementConstants& constants, int vertexSize,
                         bool bMaskPresent, bool bObstaclePresent) {
    manager.SetUniformInt(program, "vertexSize", vertexSize);
    manager.SetUniformInt(program, "bMaskFieldPresent", bMaskPresent ? 1 : 0);
    manager.SetUniformInt(program, "bObstacleFieldPresent", bObstaclePresent ? 1 : 0);
    const unsigned groupsX = static_cast<unsigned>((vertexSize + constants.scatterTileWidth - 1)
                                                   / constants.scatterTileWidth);
    const unsigned groupsY = static_cast<unsigned>((vertexSize + constants.scatterTileHeight - 1)
                                                   / constants.scatterTileHeight);
    manager.Dispatch(program, groupsX, groupsY, 1);
}

std::string BuildShaderDefinitions(const PlacementConstants& constants) {
    return "#define PLACEMENT_TILE_WIDTH " + std::to_string(constants.scatterTileWidth) + "\n"
         + "#define PLACEMENT_TILE_HEIGHT " + std::to_string(constants.scatterTileHeight) + "\n"
         + "#define PLACEMENT_FLAG_AVOID_WATER " + std::to_string(ScatterSelectionFlag::AvoidWater) + "\n"
         + "#define PLACEMENT_FLAG_NEAR_CLIFFS " + std::to_string(ScatterSelectionFlag::NearCliffs) + "\n"
         + "#define PLACEMENT_OBSTACLE_DISTANCE_DEFAULT "
         + std::to_string(constants.obstacleDistanceMaximum);
}

} // namespace

bool PlacementStage::EnsureGpuResources() {
    if (bGpuProgramReady) return true;
    if (gpuResourceManager == nullptr) return false;
    if (!gpuResourceManager->Initialize()) return false;
    const Sys::GpuProgramHandle program =
        gpuResourceManager->GetOrCompileProgram(gateProgramFileName, BuildShaderDefinitions(constants));
    if (!program.IsValid()) return false;
    gpuProgramIndex  = program.programIndex;
    bGpuProgramReady = true;
    return true;
}

bool PlacementStage::BuildGateFieldGpu(std::size_t configurationIndex) {
    if (!EnsureGpuResources()) return false;
    const ScatterRuleConfiguration& configuration = ruleConfigurations[configurationIndex];
    const int vertexSize = mapFields.VertexSize();
    const std::size_t cellCount = static_cast<std::size_t>(vertexSize) * vertexSize;
    const std::size_t fieldBytes = cellCount * sizeof(float);

    if (!bGpuFieldsUploaded) {
        // The Mask stage's baked slope goes up verbatim; the shader squares it at its read site,
        // exactly as the Cpu twin does (M5-0c). A run with no baked slope uploads nothing and the
        // shader reads the zero-initialized buffer — the flat-terrain answer the Cpu twin gives.
        UploadSharedFields(*gpuResourceManager, mapFields.heightfield.Data(),
                           bSlopeFieldAvailable ? mapFields.slope.Data() : nullptr,
                           bObstacleFieldBuilt ? obstacleDistanceField.Data() : nullptr, fieldBytes);
        bGpuFieldsUploaded = true;
    }

    // Same visibility gate as the Cpu twin: the surface weights, not the proportions (§7.2.6).
    const bool bMaskPresent = configuration.maskStratumIndex >= 0
                           && configuration.maskStratumIndex < Data::MapFields::stratumCount
                           && !mapFields.surfaceStratumWeights[configuration.maskStratumIndex].IsEmpty();
    if (bMaskPresent)
        gpuResourceManager->UploadBuffer(maskBufferName,
                                         mapFields.surfaceStratumWeights[configuration.maskStratumIndex].Data(),
                                         fieldBytes);
    gpuResourceManager->EnsureBuffer(configurationBufferName, sizeof(ScatterRuleConfiguration));
    gpuResourceManager->UploadBuffer(configurationBufferName, &configuration,
                                     sizeof(ScatterRuleConfiguration));

    BindGateBuffers(*gpuResourceManager);
    DispatchGateProgram(*gpuResourceManager, Sys::GpuProgramHandle{ gpuProgramIndex }, constants,
                        vertexSize, bMaskPresent, bObstacleFieldBuilt);

    gpuGateTransferBuffer.resize(cellCount);
    gpuResourceManager->ReadbackBuffer(gateBufferName, gpuGateTransferBuffer.data(), fieldBytes);
    float* gateValues = gateWeightField.Data();   // already sized by BuildDerivedFields
    for (std::size_t index = 0; index < cellCount; ++index) gateValues[index] = gpuGateTransferBuffer[index];
    return true;
}

} // namespace Proc
} // namespace SanmapGen
