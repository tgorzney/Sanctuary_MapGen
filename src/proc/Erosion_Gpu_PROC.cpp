// Erosion_Gpu_PROC.cpp — the Gpu speed path's host side (ARCH §4.2 Erosion → Preview/Visual).
// Layer: PROC. Owns no GL: it asks GpuResource_SYS for the compile-once program and the
// persistent buffers, so no GL handle crosses into PROC (ARCH §3.2) and the shader path is
// resolved under the configured shader directory, never the old hardcoded "D:/Projects/...".
// Rain, spawn sampling and the DATA round-trip stay on the Cpu — only the droplet trace goes
// wide — so both backends start from an identical spawn list and the parity check is honest.
#include "Erosion_PROC.h"
#include "../sys/GpuResource_SYS.h"
#include <string>

namespace SanmapGen {
namespace Proc {
namespace {

const char* const dropletKernelFileName = "Erosion_PROC.glsl";
const char* const columnKernelFileName  = "Erosion_Column_PROC.glsl";
const char* const splatKernelFileName   = "Erosion_Splat_PROC.glsl";
const char* const configurationBufferName = "erosionConfiguration";
const char* const thicknessBufferName     = "erosionThickness";
const char* const spawnBufferName         = "erosionSpawns";
const char* const physicsBufferName       = "erosionMaterialPhysics";

// The shader's compile-time constants are built from the C++ ones — one source of truth.
std::string BuildShaderDefinitions() {
    return "#define EROSION_WORKGROUP_SIZE " + std::to_string(Sys::WorkgroupSize::kDropletLinear) + "\n"
           "#define EROSION_ATOMIC_RETRY_LIMIT 64\n"
           "#define MATERIAL_PHYSICS_STRIDE " + std::to_string(materialPhysicsStride) + "\n"
           "#define MATERIAL_PHYSICS_HARDNESS_OFFSET " + std::to_string(materialPhysicsHardnessOffset) + "\n"
           "#define MATERIAL_PHYSICS_FRICTION_OFFSET " + std::to_string(materialPhysicsFrictionOffset) + "\n"
           "#define MATERIAL_PHYSICS_CAPACITY_OFFSET " + std::to_string(materialPhysicsCapacityOffset) + "\n"
           "#define MATERIAL_PHYSICS_ABSORPTION_OFFSET " + std::to_string(materialPhysicsAbsorptionOffset) + "\n"
           "#define MATERIAL_PHYSICS_ERODABLE_OFFSET " + std::to_string(materialPhysicsErodableOffset);
}

} // namespace

bool ErosionStage::EnsureGpuResources() {
    if (bGpuProgramReady) return true;
    if (gpuResourceManager == nullptr || !gpuResourceManager->IsInitialized()) return false;
    const Sys::GpuProgramHandle program = gpuResourceManager->GetOrCompileProgramFromParts(
        { dropletKernelFileName, columnKernelFileName, splatKernelFileName }, BuildShaderDefinitions());
    if (!program.IsValid()) return false;
    gpuProgramIndex = program.programIndex;
    bGpuProgramReady = true;
    return true;
}

bool ErosionStage::RunLayerPassOnGpu(const ErosionKernelConfiguration& configuration) {
    if (configuration.dropletCount <= 0) return true;
    const Sys::GpuProgramHandle program{ gpuProgramIndex };
    const std::size_t thicknessBytes = thicknessFixedPoint.size() * sizeof(int);
    const std::size_t spawnBytes = dropletSpawns.size() * sizeof(float);

    gpuResourceManager->EnsureBuffer(configurationBufferName, sizeof(ErosionKernelConfiguration));
    gpuResourceManager->EnsureBuffer(thicknessBufferName, thicknessBytes);
    gpuResourceManager->EnsureBuffer(spawnBufferName, spawnBytes);
    gpuResourceManager->EnsureBuffer(physicsBufferName, materialPhysicsBuffer.size() * sizeof(float));

    gpuResourceManager->UploadBuffer(configurationBufferName, &configuration, sizeof(ErosionKernelConfiguration));
    gpuResourceManager->UploadBuffer(thicknessBufferName, thicknessFixedPoint.data(), thicknessBytes);
    gpuResourceManager->UploadBuffer(spawnBufferName, dropletSpawns.data(), spawnBytes);
    gpuResourceManager->UploadBuffer(physicsBufferName, materialPhysicsBuffer.data(),
                                     materialPhysicsBuffer.size() * sizeof(float));
    gpuResourceManager->BindBuffer(configurationBufferName, 0);
    gpuResourceManager->BindBuffer(thicknessBufferName, 1);
    gpuResourceManager->BindBuffer(spawnBufferName, 2);
    gpuResourceManager->BindBuffer(physicsBufferName, 3);

    const int workgroupSize = Sys::WorkgroupSize::kDropletLinear;
    const unsigned groupCount = static_cast<unsigned>((configuration.dropletCount + workgroupSize - 1) / workgroupSize);
    gpuResourceManager->Dispatch(program, groupCount, 1, 1);
    gpuResourceManager->ReadbackBuffer(thicknessBufferName, thicknessFixedPoint.data(), thicknessBytes);
    return true;
}

void ErosionStage::RunOnGpu() {
    PrepareRun();
    if (!EnsureGpuResources()) {   // no GL context / shader: fall back to the accuracy path
        for (int stratum = 0; stratum < stratumCount; ++stratum) {
            if (!layerSettings[stratum].bEnabled) continue;
            BuildRainMap(stratum);
            BuildDropletSpawns(stratum);
            TraceDropletsCpu(BuildConfiguration(stratum));
            ++processedLayerCount;
            lastDropletCount += static_cast<int>(dropletSpawns.size() / 2);
        }
        WriteThicknessToFields();
        return;
    }
    for (int stratum = 0; stratum < stratumCount; ++stratum) {
        if (!layerSettings[stratum].bEnabled) continue;
        BuildRainMap(stratum);
        BuildDropletSpawns(stratum);
        RunLayerPassOnGpu(BuildConfiguration(stratum));
        ++processedLayerCount;
        lastDropletCount += static_cast<int>(dropletSpawns.size() / 2);
    }
    WriteThicknessToFields();
}

} // namespace Proc
} // namespace SanmapGen
