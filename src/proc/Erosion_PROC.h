// Erosion_PROC.h — the hydraulic (droplet) erosion stage: per-layer, per-material physics.
// Layer: PROC. Accuracy: Preview Visual (Gpu) / Output Exact (Cpu) — erosion shapes terrain
// AND pathing, so the output pass is gameplay-authoritative (ARCH §4.2, DETERMINISM_SPEC).
// One kernel, two backends: RunOnCpu() is the accuracy path (sequential droplets, full
// droplet-to-droplet feedback), RunOnGpu() the speed path (one droplet per invocation,
// atomic fixed-point scatter — NOT the old non-atomic float RMW race). Both carry the same
// configuration, the same rates and the same meander/divergence term; the erosion state is
// integer ticks on both sides, so accumulation is exact and order-independent.
// The stage never picks a backend: it hands itself to Sys::Dispatch (ARCH §4).
#pragma once
#include <cstddef>
#include <vector>
#include "Erosion_Column_PROC.h"
#include "Erosion_Kernel_PROC.h"
#include "Erosion_Physics_PROC.h"
#include "Erosion_Settings_PROC.h"
#include "../data/MapFields_DATA.h"
#include "../params/Geometry_PARAMS.h"
#include "../sys/Dispatch_SYS.h"

namespace SanmapGen {
namespace Sys { class GpuResourceManager; }

namespace Proc {

class ErosionStage {
public:
    static constexpr int stratumCount = Data::MapFields::stratumCount;

    ErosionStage(const Params::Geometry& geometrySettings, Data::MapFields& fields);

    // Configuration (Constitution §8 — everything is reachable).
    ErosionConstants& Constants() { return constants; }
    const ErosionConstants& Constants() const { return constants; }
    MaterialPhysics& Material(int stratumIndex) { return materials[stratumIndex]; }
    const MaterialPhysics& Material(int stratumIndex) const { return materials[stratumIndex]; }
    ErosionLayerSettings& LayerSettings(int stratumIndex) { return layerSettings[stratumIndex]; }
    const ErosionLayerSettings& LayerSettings(int stratumIndex) const { return layerSettings[stratumIndex]; }

    void SetDispatchPolicy(const Sys::DispatchPolicy& policy) { dispatchPolicy = policy; }
    const Sys::DispatchPolicy& ActiveDispatchPolicy() const { return dispatchPolicy; }
    void SetGenerationContext(Sys::GenerationContext context) { generationContext = context; }
    void SetGlobalBackend(Sys::ComputeBackend backend) { globalBackend = backend; }
    void SetGpuResourceManager(Sys::GpuResourceManager* manager) { gpuResourceManager = manager; }

    // Hash of everything the stage consumes — geometry, every material's physics and every
    // enabled layer's settings. PIPELINE registers this as the stage's computeParamHash, so
    // changing one rate re-runs erosion and everything downstream (M2-1 dirty hash).
    std::size_t ComputeParameterHash() const;

    // Resolves the backend through Sys::Dispatch and runs there. Returns the backend used.
    Sys::ComputeBackend Run();

    // The two backends (public because Sys::Dispatch calls them; PIPELINE uses Run()).
    void RunOnCpu();
    void RunOnGpu();

    // Observability for tests/telemetry.
    Sys::ComputeBackend LastBackend() const { return lastBackend; }
    int ProcessedLayerCount() const { return processedLayerCount; }
    int LastDropletCount() const { return lastDropletCount; }
    bool IsGpuAvailable() const { return bGpuProgramReady; }
    // Total column volume in fixed-point ticks — the conservation check reads this.
    long long TotalVolumeFixedPoint() const;
    const std::vector<int>& ThicknessFixedPoint() const { return thicknessFixedPoint; }

private:
    void PrepareRun();                                     // Erosion_PROC.cpp
    ErosionKernelConfiguration BuildConfiguration(int stratumIndex) const;  // Erosion_PROC.cpp
    void BuildMaterialPhysicsBuffer();                     // Erosion_PROC.cpp
    void ReadThicknessFromFields();                        // Erosion_Field_PROC.cpp
    void WriteThicknessToFields();                         // Erosion_Field_PROC.cpp
    int  ColumnTotalFixedPointAt(int cellIndex) const;     // Erosion_Field_PROC.cpp
    void BuildRainMap(int stratumIndex);                   // Erosion_Rain_PROC.cpp
    void BuildDropletSpawns(int stratumIndex);             // Erosion_Spawn_PROC.cpp
    void TraceDropletsCpu(const ErosionKernelConfiguration& configuration);  // Erosion_Droplet_PROC.cpp
    void ApplyAccumulationDagCpu(int stratumIndex);        // Erosion_Accumulation_PROC.cpp
    bool EnsureGpuResources();                             // Erosion_Gpu_PROC.cpp
    bool RunLayerPassOnGpu(const ErosionKernelConfiguration& configuration);  // Erosion_Gpu_PROC.cpp

    const Params::Geometry& geometry;
    Data::MapFields&        mapFields;
    ErosionConstants        constants;
    MaterialPhysics         materials[stratumCount];
    ErosionLayerSettings    layerSettings[stratumCount];

    // ARCH §4.2 defaults for Erosion: Preview Gpu/Visual, Output Cpu/Exact.
    Sys::DispatchPolicy      dispatchPolicy{ Sys::ComputeBackend::Gpu, Sys::ComputeBackend::Cpu,
                                             Sys::AccuracyClass::Visual, Sys::AccuracyClass::Exact, false };
    Sys::GenerationContext   generationContext  = Sys::GenerationContext::Output;
    Sys::ComputeBackend      globalBackend      = Sys::ComputeBackend::Cpu;
    Sys::GpuResourceManager* gpuResourceManager = nullptr;

    std::vector<int>   thicknessFixedPoint;   // stratum-major erosion state (the sim's truth)
    std::vector<float> materialPhysicsBuffer; // stratumCount * materialPhysicsStride floats
    std::vector<float> rainMap;
    std::vector<float> dropletSpawns;         // interleaved x,y
    std::vector<int>   gpuTransferBuffer;

    int vertexSize = 0;
    int cellCount  = 0;
    Sys::ComputeBackend lastBackend = Sys::ComputeBackend::Cpu;
    int  processedLayerCount = 0;
    int  lastDropletCount    = 0;
    bool bGpuProgramReady    = false;
    int  gpuProgramIndex     = -1;
};

} // namespace Proc
} // namespace SanmapGen
