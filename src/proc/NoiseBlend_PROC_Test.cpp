// NoiseBlend_PROC_Test.cpp — acceptance test for work-order M3-1 (the noise/blend stage).
// Covers item 2 (blend correctness, hand-checked on a 2-layer stack), item 3 (the two-level
// dirty hash driven through Generation_PIPELINE) and item 4 (determinism). Item 1 (Cpu/Gpu
// parity) needs a GL context, so it lives in NoiseBlend_GpuParity_PROC_Test.cpp and is called
// from here (with NoiseBlend_GpuBlend_PROC_Test.cpp). Build with MSVC from src/proc:
//   cl /EHsc /std:c++17 /W4 /I..\..\core NoiseBlend_PROC_Test.cpp
//      NoiseBlend_GpuParity_PROC_Test.cpp NoiseBlend_GpuBlend_PROC_Test.cpp NoiseBlend_PROC.cpp
//      NoiseBlend_Prepare_PROC.cpp NoiseBlend_Noise_PROC.cpp NoiseBlend_Blend_PROC.cpp
//      NoiseBlend_Gpu_PROC.cpp NoiseBlend_GpuBuffers_PROC.cpp NoiseBlend_GpuProgram_PROC.cpp
//      ..\sys\GpuResource_Program_SYS.cpp ..\sys\GpuResource_ProgramParts_SYS.cpp
//      ..\sys\GpuResource_Buffer_SYS.cpp ..\sys\GpuGlFunctions_SYS.cpp
//      opengl32.lib gdi32.lib user32.lib
//   noiseblend_test.exe <directory holding the NoiseBlend_*.glsl units>
#include "NoiseBlend_PROC.h"
#include "NoiseBlend_TestStacks_PROC.h"
#include "NoiseBlend_TestSupport_PROC.h"
#include "../pipeline/Generation_PIPELINE.h"
#include "../sys/ThreadPool_SYS.h"
#include <cstdio>
#include <cmath>

using namespace SanmapGen;

void RunNoiseBlendGpuParityChecks(const char* shaderDirectory);  // NoiseBlend_GpuParity_PROC_Test.cpp

int noiseBlendTestFailures = 0;
void NoiseBlendCheck(bool bPassed, const char* label) {
    if (!bPassed) { std::printf("FAIL: %s\n", label); ++noiseBlendTestFailures; }
}

namespace {

// Item 2 — every HeightBlendMode on a 2-layer stack of exact constants (see the header for
// why the layers are exact). Base layer 0.6 added onto height 0, then layer 0.5 combined.
void CheckBlendModes() {
    Params::Geometry geometry;
    geometry.mapSize = 15;
    for (const Proc::BlendModeExpectation& expected : Proc::BlendModeExpectations()) {
        Params::LayerStack stack = Proc::MakeConstantTwoLayerStack(expected.mode, expected.opacity);
        Data::MapFields fields;
        Proc::NoiseBlendStage stage(geometry, stack, fields);
        stage.RunOnCpu();
        NoiseBlendCheck(std::fabs(fields.heightfield.Get(3, 5) - expected.height) < 1e-6f, expected.label);
    }
    // Masks: top-down occlusion splits coverage by each layer's own thickness (Add case:
    // 0.6 from the base stratum, the remaining 0.4 from the layer above).
    Params::LayerStack stack = Proc::MakeConstantTwoLayerStack(Params::HeightBlendMode::Add, 1.0f);
    Data::MapFields fields;
    Proc::NoiseBlendStage stage(geometry, stack, fields);
    stage.RunOnCpu();
    NoiseBlendCheck(std::fabs(fields.materialMasks[0].Get(3, 5) - 0.6f) < 1e-6f, "stratum 0 mask = 0.6");
    NoiseBlendCheck(std::fabs(fields.materialMasks[1].Get(3, 5) - 0.4f) < 1e-6f, "stratum 1 mask = 0.4");
}

// Item 3 — the stage as a PIPELINE stage: unchanged settings skip it, a structural change
// (frequency) re-rolls that layer's noise, a shaping-only change (opacity) re-blends from the
// CACHED noise. That second level is the whole point of the two-level hash.
void CheckDirtyHash() {
    Params::Geometry geometry;
    Params::LayerStack stack = Proc::MakeRepresentativeStack();
    Data::MapFields fields;
    Proc::NoiseBlendStage stage(geometry, stack, fields);
    Pipeline::GenerationPipeline pipeline;
    pipeline.AddStage("NoiseBlend", [&] { return stage.ComputeParameterHash(); }, [&] { stage.Run(); });

    NoiseBlendCheck(pipeline.Run().size() == 1, "first pipeline run executes the stage");
    NoiseBlendCheck(stage.RegeneratedLayerCount() == 3, "first run generates all three layers");
    NoiseBlendCheck(pipeline.Run().empty(), "unchanged settings skip the stage entirely");

    stack.geoLayers[0].layers[1].frequency *= 2.0f;
    NoiseBlendCheck(pipeline.Run().size() == 1, "a frequency change re-runs the stage");
    NoiseBlendCheck(stage.RegeneratedLayerCount() == 1, "only the changed layer's noise is re-rolled");

    stack.geoLayers[0].layers[1].opacity = 0.25f;
    NoiseBlendCheck(pipeline.Run().size() == 1, "an opacity change re-runs the stage");
    NoiseBlendCheck(stage.RegeneratedLayerCount() == 0, "reshaping reuses the cached raw noise");
}

// Item 4 — the Cpu accuracy path is bit-identical across runs and across thread-pool widths
// (rows are independent, so the partition cannot reach the result).
void CheckDeterminism() {
    Params::Geometry geometry;
    geometry.mapSize = 128;
    Params::LayerStack stack = Proc::MakeRepresentativeStack();
    Data::MapFields firstFields;
    Data::MapFields secondFields;
    Proc::NoiseBlendStage firstStage(geometry, stack, firstFields);
    Proc::NoiseBlendStage secondStage(geometry, stack, secondFields);
    Sys::ThreadPool pool(4);
    secondStage.SetThreadPool(&pool);
    firstStage.RunOnCpu();
    secondStage.RunOnCpu();

    bool bIdentical = true;
    for (std::size_t cell = 0; cell < firstFields.heightfield.CellCount(); ++cell)
        if (firstFields.heightfield.Data()[cell] != secondFields.heightfield.Data()[cell]) bIdentical = false;
    NoiseBlendCheck(bIdentical, "two runs give bit-identical heightfields (serial vs 4 workers)");
}

} // namespace

// argv[1] = the directory holding the NoiseBlend_*.glsl kernel units (defaults to the
// working directory) — the stage never carries a hardcoded shader path.
int main(int argc, char** argv) {
    CheckBlendModes();
    CheckDirtyHash();
    CheckDeterminism();
    RunNoiseBlendGpuParityChecks(argc > 1 ? argv[1] : ".");
    if (noiseBlendTestFailures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", noiseBlendTestFailures);
    return 1;
}
