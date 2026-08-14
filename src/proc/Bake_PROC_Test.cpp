// Bake_PROC_Test.cpp — acceptance test for the bake stage (M3-7): the two-stratum weighted
// blend (flat tints and a real tiled texture), the maskRemapMin/Max window, and the
// dirty-hash skip/re-run through Generation_PIPELINE. The Gpu half (Gpu path + Cpu/Gpu
// parity) lives in Bake_Gpu_PROC_Test.cpp — it needs a GL context.
// Build (MSVC): both *_Test.cpp + Bake_PROC.cpp + Bake_Composite_PROC.cpp + Bake_Gpu_PROC.cpp
// + the four sys/Gpu*.cpp units, linking opengl32/gdi32/user32.
// Optional argv[1] = the directory holding Bake_PROC.glsl (defaults to ".").
#include "Bake_TestScene_PROC.h"
#include "../pipeline/Generation_PIPELINE.h"
#include "../sys/ThreadPool_SYS.h"
#include <cstdio>
#include <string>

using namespace SanmapGen;

int RunBakeGpuAcceptance(const std::string& shaderDirectory);   // Bake_Gpu_PROC_Test.cpp

namespace {

using Proc::AllTexelsEqual;
using Proc::BuildTwoStratumScene;
using Proc::CpuVisualPolicy;
using Proc::ExpectedTexel;
void check(bool bCondition, const char* label) { Proc::CheckBakeExpectation(bCondition, label); }

// 1. Two-stratum blend: 0.25 * red + 0.75 * blue = (64, 0, 191), and strata 1..4 pack the
//    remapped weights (0.75 -> 191) for the export mask texture.
void TestTwoStratumBlend() {
    Params::Geometry geometry; geometry.mapSize = 4;
    Data::MapFields fields;
    Proc::BakedTextureSet textures;
    Proc::BakeStage stage(geometry, fields, textures);
    BuildTwoStratumScene(geometry, fields, stage);
    stage.SetDispatchPolicy(CpuVisualPolicy());

    check(stage.Run() == Sys::ComputeBackend::Cpu, "Cpu policy resolves to the Cpu twin");
    check(textures.resolution == 4, "output texture sized from mapSize * multiplier");
    check(AllTexelsEqual(textures.compositeAlbedo, ExpectedTexel(64, 0, 191, 255)),
          "two-stratum mask blends to the expected weighted colour (0.25 red + 0.75 blue)");
    check(AllTexelsEqual(textures.stratumMaskLow, ExpectedTexel(191, 0, 0, 0)),
          "stratum 1 weight lands in the packed stratums_1_4 texture");
    check(AllTexelsEqual(textures.stratumMaskHigh, 0u), "strata 5..8 stay empty");
}

// 2. The same blend with a real 2x2 tiled albedo texture on the blue stratum: the texture
//    (pure green) replaces the tint, so the composite becomes 0.25 red + 0.75 green.
void TestTexturedBlend() {
    Params::Geometry geometry; geometry.mapSize = 4;
    Data::MapFields fields;
    Proc::BakedTextureSet textures;
    Proc::BakeStage stage(geometry, fields, textures);
    BuildTwoStratumScene(geometry, fields, stage);
    stage.SetDispatchPolicy(CpuVisualPolicy());
    const unsigned int greenTexels[4] = { ExpectedTexel(0, 255, 0, 255), ExpectedTexel(0, 255, 0, 255),
                                          ExpectedTexel(0, 255, 0, 255), ExpectedTexel(0, 255, 0, 255) };
    stage.Stratum(1).albedoPixels = greenTexels;
    stage.Stratum(1).albedoWidth = 2;
    stage.Stratum(1).albedoHeight = 2;
    stage.Stratum(1).tileCount = 2.0f;
    stage.Stratum(1).tintRed = 1.0f; stage.Stratum(1).tintGreen = 1.0f; stage.Stratum(1).tintBlue = 1.0f;

    stage.Run();
    check(AllTexelsEqual(textures.compositeAlbedo, ExpectedTexel(64, 191, 0, 255)),
          "tiled stratum texture is composited at its mask weight");
}

// 3. maskRemapMin/Max: remapping stratum 1 to [0.5, 1.0] halves its 0.75 weight to 0.5, so
//    the normalized blend moves from (64, 0, 191) to (85, 0, 170).
void TestMaskRemapRange() {
    Params::Geometry geometry; geometry.mapSize = 4;
    Data::MapFields fields;
    Proc::BakedTextureSet textures;
    Proc::BakeStage stage(geometry, fields, textures);
    BuildTwoStratumScene(geometry, fields, stage);
    stage.SetDispatchPolicy(CpuVisualPolicy());
    stage.Stratum(1).maskRemapMinimum = 0.5f;
    stage.Stratum(1).maskRemapMaximum = 1.0f;

    stage.Run();
    check(AllTexelsEqual(textures.compositeAlbedo, ExpectedTexel(85, 0, 170, 255)),
          "maskRemapMin/Max re-weights the blend");
    check(AllTexelsEqual(textures.stratumMaskLow, ExpectedTexel(128, 0, 0, 0)),
          "the packed mask texture stores the remapped weight");

    stage.Stratum(1).maskRemapMinimum = 0.75f;   // window starts at the mask value -> weight 0
    stage.Run();
    check(AllTexelsEqual(textures.compositeAlbedo, ExpectedTexel(255, 0, 0, 255)),
          "a window above the mask value drops the stratum entirely");
}

// 4. Dirty hash through the pipeline: first Run bakes, an unchanged Run skips, a changed
//    remap window re-runs.
void TestDirtyHashSkipAndReRun() {
    Params::Geometry geometry; geometry.mapSize = 4;
    Data::MapFields fields;
    Proc::BakedTextureSet textures;
    Proc::BakeStage stage(geometry, fields, textures);
    BuildTwoStratumScene(geometry, fields, stage);
    stage.SetDispatchPolicy(CpuVisualPolicy());

    Pipeline::GenerationPipeline pipeline;
    int runCount = 0;
    pipeline.AddStage("Bake", [&stage]() { return stage.ComputeParameterHash(); },
                      [&stage, &runCount]() { stage.Run(); ++runCount; });
    check(pipeline.Run().size() == 1 && runCount == 1, "first pipeline run bakes the stage");
    check(pipeline.Run().empty() && runCount == 1, "unchanged parameters skip the stage");
    stage.Stratum(1).maskRemapMaximum = 0.75f;   // 0.75 mask now saturates the window
    check(pipeline.Run().size() == 1 && runCount == 2, "changing maskRemapMax re-runs the stage");
    check(AllTexelsEqual(textures.compositeAlbedo, ExpectedTexel(51, 0, 204, 255)),
          "the re-run picked up the new remap window");
}

// 5. The threaded Cpu bake is texel-identical to the serial one (rows are independent).
void TestThreadedCpuBakeMatchesSerial() {
    Params::Geometry geometry; geometry.mapSize = 64;
    Data::MapFields serialFields, threadedFields;
    Proc::BakedTextureSet serialTextures, threadedTextures;
    Proc::BakeStage serialStage(geometry, serialFields, serialTextures);
    Proc::BakeStage threadedStage(geometry, threadedFields, threadedTextures);
    BuildTwoStratumScene(geometry, serialFields, serialStage);
    BuildTwoStratumScene(geometry, threadedFields, threadedStage);
    serialStage.SetDispatchPolicy(CpuVisualPolicy());
    threadedStage.SetDispatchPolicy(CpuVisualPolicy());
    Sys::ThreadPool threadPool;
    threadedStage.SetThreadPool(&threadPool);

    serialStage.Run();
    threadedStage.Run();
    check(serialTextures.compositeAlbedo == threadedTextures.compositeAlbedo
       && serialTextures.stratumMaskLow == threadedTextures.stratumMaskLow,
          "the thread-pooled Cpu bake is identical to the serial one");
}

} // namespace

int main(int argumentCount, char** argumentValues) {
    const std::string shaderDirectory = argumentCount > 1 ? argumentValues[1] : ".";
    TestTwoStratumBlend();
    TestTexturedBlend();
    TestMaskRemapRange();
    TestDirtyHashSkipAndReRun();
    TestThreadedCpuBakeMatchesSerial();
    const int gpuResult = RunBakeGpuAcceptance(shaderDirectory);
    const int failures = Proc::bakeTestFailures;

    if (failures == 0 && gpuResult == 2) { std::printf("CPU PASS, GPU SKIPPED (no GL context)\n"); return 2; }
    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}
