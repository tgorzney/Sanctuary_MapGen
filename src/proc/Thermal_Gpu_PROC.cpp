// Thermal_Gpu_PROC.cpp — the Gpu speed path: one compile-once program, persistent ping-pong
// buffers, and the per-sweep prepare/apply dispatch pair mirroring the Cpu sweep.
// Every GL handle stays behind GpuResource_SYS (ARCH §3.2) — this file only names buffers and
// counts workgroups. With no program available (no context, compile failure) the stage falls
// back to the Cpu path and reports Cpu as the backend it actually ran on.
#include "Thermal_PROC.h"
#include "../sys/GpuResource_SYS.h"

namespace SanmapGen {
namespace Proc {
namespace {

constexpr int thermalPassPrepare = 0;
constexpr int thermalPassApply   = 1;
const char* const thermalShaderFileName = "Thermal_PROC.glsl";

std::string ShaderDefinition(const char* name, int value) {
    return std::string("#define ") + name + " " + std::to_string(value) + "\n";
}

// Every literal the shader needs is built from the C++ constants, so the pair cannot drift.
std::string BuildShaderDefinitions() {
    return ShaderDefinition("THERMAL_TILE_WIDTH",  Sys::WorkgroupSize::kFieldTileWidth)
         + ShaderDefinition("THERMAL_TILE_HEIGHT", Sys::WorkgroupSize::kFieldTileHeight)
         + ShaderDefinition("THERMAL_STRATUM_COUNT",   Data::MapFields::stratumCount)
         + ShaderDefinition("THERMAL_NEIGHBOUR_COUNT", thermalNeighbourCount)
         + ShaderDefinition("THERMAL_PASS_PREPARE", thermalPassPrepare)
         + ShaderDefinition("THERMAL_PASS_APPLY",   thermalPassApply)
         + ShaderDefinition("THERMAL_SLOT_SPREAD_FACTOR",    ThermalConstantSlot::spreadFactorActive)
         + ShaderDefinition("THERMAL_SLOT_MOVEMENT_EPSILON", ThermalConstantSlot::movementEpsilon)
         + ShaderDefinition("THERMAL_SLOT_MINIMUM_COLUMN_DEPTH", ThermalConstantSlot::minimumColumnDepth)
         + ShaderDefinition("THERMAL_SLOT_PROPORTION_EPSILON", ThermalConstantSlot::proportionWeightEpsilon)
         + ShaderDefinition("THERMAL_SLOT_TALUS_BASE",   ThermalConstantSlot::talusThresholdBase)
         + ShaderDefinition("THERMAL_CONSTANT_COUNT",    ThermalConstantSlot::totalCount);
}

} // namespace

bool ThermalStage::EnsureGpuResources() {
    bGpuProgramReady = false;
    if (gpuResourceManager == nullptr) return false;
    if (!gpuResourceManager->Initialize()) return false;
    const Sys::GpuProgramHandle program =
        gpuResourceManager->GetOrCompileProgram(thermalShaderFileName, BuildShaderDefinitions());
    if (!program.IsValid()) return false;
    gpuProgramIndex = program.programIndex;
    bGpuProgramReady = true;
    return true;
}

// Persistent SSBOs: allocated on first use, reallocated only on resize (GpuResource_SYS).
void ThermalStage::UploadFieldsToGpu() {
    const std::size_t cellCount = mapFields.heightfield.CellCount();
    const std::size_t heightBytes = cellCount * sizeof(float);
    const std::size_t proportionBytes = heightBytes * Data::MapFields::stratumCount;
    const std::size_t constantBytes = kernelConstantBlock.size() * sizeof(float);
    gpuResourceManager->EnsureBuffer("thermalHeightA", heightBytes);
    gpuResourceManager->EnsureBuffer("thermalHeightB", heightBytes);
    gpuResourceManager->EnsureBuffer("thermalProportionA", proportionBytes);
    gpuResourceManager->EnsureBuffer("thermalProportionB", proportionBytes);
    gpuResourceManager->EnsureBuffer("thermalSpreadFactor", heightBytes);
    gpuResourceManager->EnsureBuffer("thermalTalusThreshold", heightBytes);
    gpuResourceManager->EnsureBuffer("thermalConstants", constantBytes);

    gpuTransferBuffer.assign(cellCount * Data::MapFields::stratumCount, 0.0f);
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum) {
        const float* source = mapFields.materialProportions[stratum].Data();
        for (std::size_t cell = 0; cell < cellCount; ++cell)
            gpuTransferBuffer[stratum * cellCount + cell] = source[cell];
    }
    gpuResourceManager->UploadBuffer("thermalHeightA", mapFields.heightfield.Data(), heightBytes);
    gpuResourceManager->UploadBuffer("thermalProportionA", gpuTransferBuffer.data(), proportionBytes);
    gpuResourceManager->UploadBuffer("thermalConstants", kernelConstantBlock.data(), constantBytes);
}

// One sweep = prepare dispatch, apply dispatch, ping-pong swap. Returns the buffer names that
// hold the newest fields.
void ThermalStage::RunGpuSweeps(std::string& heightReadName, std::string& proportionReadName) {
    const int vertexSize = mapFields.VertexSize();
    const bool bTransport = constants.bTransportMaterialProportions;
    const Sys::GpuProgramHandle program{ gpuProgramIndex };
    const unsigned groupsX = static_cast<unsigned>((vertexSize + Sys::WorkgroupSize::kFieldTileWidth - 1)
                                                   / Sys::WorkgroupSize::kFieldTileWidth);
    const unsigned groupsY = static_cast<unsigned>((vertexSize + Sys::WorkgroupSize::kFieldTileHeight - 1)
                                                   / Sys::WorkgroupSize::kFieldTileHeight);
    heightReadName = "thermalHeightA";
    proportionReadName = "thermalProportionA";
    std::string heightWriteName = "thermalHeightB";
    std::string proportionWriteName = "thermalProportionB";
    gpuResourceManager->SetUniformInt(program, "vertexSize", vertexSize);
    gpuResourceManager->SetUniformInt(program, "transportMaterialProportions", bTransport ? 1 : 0);
    gpuResourceManager->BindBuffer("thermalSpreadFactor", 4);
    gpuResourceManager->BindBuffer("thermalTalusThreshold", 5);
    gpuResourceManager->BindBuffer("thermalConstants", 6);
    for (int iteration = 0; iteration < constants.iterationCount; ++iteration) {
        gpuResourceManager->BindBuffer(heightReadName, 0);
        gpuResourceManager->BindBuffer(heightWriteName, 1);
        gpuResourceManager->BindBuffer(proportionReadName, 2);
        gpuResourceManager->BindBuffer(proportionWriteName, 3);
        gpuResourceManager->SetUniformInt(program, "passMode", thermalPassPrepare);
        gpuResourceManager->Dispatch(program, groupsX, groupsY, 1);
        gpuResourceManager->SetUniformInt(program, "passMode", thermalPassApply);
        gpuResourceManager->Dispatch(program, groupsX, groupsY, 1);
        heightReadName.swap(heightWriteName);
        if (bTransport) proportionReadName.swap(proportionWriteName);
        ++completedIterationCount;
    }
}

void ThermalStage::ReadbackFieldsFromGpu(const std::string& heightReadName, const std::string& proportionReadName) {
    const std::size_t cellCount = mapFields.heightfield.CellCount();
    const std::size_t heightBytes = cellCount * sizeof(float);
    gpuResourceManager->ReadbackBuffer(heightReadName, mapFields.heightfield.Data(), heightBytes);
    if (!constants.bTransportMaterialProportions) return;
    gpuResourceManager->ReadbackBuffer(proportionReadName, gpuTransferBuffer.data(),
                                       heightBytes * Data::MapFields::stratumCount);
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum) {
        float* destination = mapFields.materialProportions[stratum].Data();
        for (std::size_t cell = 0; cell < cellCount; ++cell)
            destination[cell] = gpuTransferBuffer[stratum * cellCount + cell];
    }
}

void ThermalStage::RunOnGpu() {
    PrepareRun();
    completedIterationCount = 0;
    if (!mapFields.IsSized()) { lastBackend = Sys::ComputeBackend::Gpu; return; }
    if (!EnsureGpuResources()) { RunOnCpu(); return; }
    UploadFieldsToGpu();
    std::string heightReadName;
    std::string proportionReadName;
    RunGpuSweeps(heightReadName, proportionReadName);
    ReadbackFieldsFromGpu(heightReadName, proportionReadName);
    lastBackend = Sys::ComputeBackend::Gpu;
}

} // namespace Proc
} // namespace SanmapGen
