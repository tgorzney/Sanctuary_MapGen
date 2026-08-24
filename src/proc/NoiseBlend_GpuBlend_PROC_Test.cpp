// NoiseBlend_GpuBlend_PROC_Test.cpp — the Gpu half of the config-divergence proof (work-order
// M3-1). The old GPU layer block omitted blendMode, the noise type, the fractal type, ping-pong
// and cellular jitter, so the Gpu could not reproduce the Cpu for any non-default layer. These
// checks walk EVERY one of those fields on the Gpu and hold it against the Cpu, plus the
// dispatch routing that decides which backend runs at all.
#include "NoiseBlend_PROC.h"
#include "NoiseBlend_TestStacks_PROC.h"
#include "NoiseBlend_TestSupport_PROC.h"
#include "../data/BakedLayerImage_DATA.h"
#include "../sys/GpuResource_SYS.h"
#include <cmath>

using namespace SanmapGen;

void NoiseBlendCheck(bool bPassed, const char* label);   // NoiseBlend_PROC_Test.cpp

// Most checks below carry no baked layer at all.
const std::vector<Data::BakedLayerImage> noBakedLayerImages;

// Every HeightBlendMode reproduced exactly on the Gpu — constant inputs and an integer-selected
// branch, so this is an equality check, not a tolerance one.
void CheckGpuBlendModes(Sys::GpuResourceManager& manager) {
    Params::Geometry geometry;
    geometry.mapSize = 31;
    for (const Proc::BlendModeExpectation& expected : Proc::BlendModeExpectations()) {
        Params::LayerStack stack = Proc::MakeConstantTwoLayerStack(expected.mode, expected.opacity);
        Data::MapFields fields;
        Proc::NoiseBlendStage stage(geometry, stack, fields, noBakedLayerImages);
        stage.SetGpuResourceManager(&manager);
        stage.RunOnGpu();
        NoiseBlendCheck(stage.LastBackend() == Sys::ComputeBackend::Gpu
                        && std::fabs(fields.heightfield.Get(3, 5) - expected.height) < 1e-6f,
                        expected.label);
    }
}

// Every NoiseType against every FractalType, one layer at a time, Cpu versus Gpu. This is what
// proves the GLSL FastNoiseLite port (simplex, smooth simplex, cellular, perlin, value,
// value-cubic + the three fractal loops) matches the Cpu library it mirrors.
void CheckNoiseTypeParity(Sys::GpuResourceManager& manager) {
    static const Params::NoiseType noiseTypes[] = {
        Params::NoiseType::OpenSimplex2, Params::NoiseType::OpenSimplex2Smooth,
        Params::NoiseType::Cellular,     Params::NoiseType::Perlin,
        Params::NoiseType::ValueCubic,   Params::NoiseType::Value };
    static const Params::FractalType fractalTypes[] = {
        Params::FractalType::None,   Params::FractalType::FractionalBrownian,
        Params::FractalType::Ridged, Params::FractalType::PingPong };
    Params::Geometry geometry;
    geometry.mapSize = 127;
    geometry.seed = 99u;
    for (Params::NoiseType noiseType : noiseTypes) {
        for (Params::FractalType fractalType : fractalTypes) {
            Params::LayerStack stack = Proc::MakeSingleNoiseStack(noiseType, fractalType);
            Data::MapFields cpuFields;
            Data::MapFields gpuFields;
            Proc::NoiseBlendStage cpuStage(geometry, stack, cpuFields, noBakedLayerImages);
            Proc::NoiseBlendStage gpuStage(geometry, stack, gpuFields, noBakedLayerImages);
            gpuStage.SetGpuResourceManager(&manager);
            cpuStage.RunOnCpu();
            gpuStage.RunOnGpu();
            const char* label = Proc::NoiseCombinationLabel(noiseType, fractalType);
            Proc::CheckFieldHasSignal(cpuFields.heightfield, label, NoiseBlendCheck);
            Proc::CompareFields(cpuFields.heightfield, gpuFields.heightfield, label, NoiseBlendCheck);
        }
    }
}

// Routing: the DispatchPolicy alone decides the backend; the stage never reads a "use GPU"
// bool (ARCH §4.2 defaults for the Noise stage are Gpu on Preview, Cpu on Output).
void CheckDispatchRouting(Sys::GpuResourceManager& manager) {
    Params::Geometry geometry;
    geometry.mapSize = 63;
    Params::LayerStack stack = Proc::MakeRepresentativeStack();
    Data::MapFields fields;
    Proc::NoiseBlendStage stage(geometry, stack, fields, noBakedLayerImages);
    stage.SetGpuResourceManager(&manager);
    stage.SetDispatchPolicy(Sys::DispatchPolicy{});
    stage.SetGenerationContext(Sys::GenerationContext::Preview);
    NoiseBlendCheck(stage.Run() == Sys::ComputeBackend::Gpu, "Preview context routes to the Gpu");
    stage.SetGenerationContext(Sys::GenerationContext::Output);
    NoiseBlendCheck(stage.Run() == Sys::ComputeBackend::Cpu, "Output context routes to the Cpu");
    // No GL manager at all: the Gpu path must fall back to the Cpu, not produce nothing.
    Data::MapFields fallbackFields;
    Proc::NoiseBlendStage fallbackStage(geometry, stack, fallbackFields, noBakedLayerImages);
    fallbackStage.RunOnGpu();
    NoiseBlendCheck(fallbackStage.LastBackend() == Sys::ComputeBackend::Cpu,
                    "no GL manager falls the Gpu path back to the Cpu");
}

// STEP100: a baked layer forces the Cpu path even with a fully working Gpu program — the Gpu
// kernel twin is explicit out-of-scope, so RunOnGpu() must refuse and fall back rather than
// silently regenerating live noise on the Gpu for a layer that has none.
void CheckBakedLayerForcesCpuFallback(Sys::GpuResourceManager& manager) {
    Params::Geometry geometry;
    geometry.mapSize = 7;
    Params::Layer layer;
    layer.bBaked = true;
    layer.layerIdentifier = 0;
    Params::GeoLayer group;
    group.layers.push_back(layer);
    Params::LayerStack stack;
    stack.geoLayers.push_back(group);

    std::vector<Data::BakedLayerImage> bakedImages(1);
    bakedImages[0].layerIdentifier = 0;
    bakedImages[0].image = Data::FloatField(geometry.VertexSize(), geometry.VertexSize(), 0.42f);

    Data::MapFields fields;
    Proc::NoiseBlendStage stage(geometry, stack, fields, bakedImages);
    stage.SetGpuResourceManager(&manager);
    stage.RunOnGpu();
    NoiseBlendCheck(stage.LastBackend() == Sys::ComputeBackend::Cpu,
                    "a baked layer forces the Gpu path back to the Cpu even with a working GL program");
    NoiseBlendCheck(std::fabs(fields.heightfield.Get(3, 3) - 0.42f) < 1e-6f,
                    "and the fallback Cpu run actually reads the baked pixels");
}
