// Bake_PROC_Test.cpp — acceptance test for the bake stage (M3-7, reworked for ARCH §7.2): the
// two-stratum weighted blend on flat tints and on a real tiled texture, and the threaded/serial
// Cpu equivalence. The §7.2.5 consumption contract (no rival remap) and the dirty hash live in
// Bake_Weights_PROC_Test.cpp; the Gpu path and Cpu/Gpu parity in Bake_Gpu_PROC_Test.cpp, which
// needs a GL context (ARCH §1.5 file ceilings — one binary, several aspect units).
// Build (MSVC): the three *_Test.cpp + Bake_PROC.cpp + Bake_Composite_PROC.cpp +
// Bake_Gpu_PROC.cpp + the four sys/Gpu*.cpp units, linking opengl32/gdi32/user32.
// Optional argv[1] = the directory holding Bake_PROC.glsl (defaults to ".").
#include "Bake_TestScene_PROC.h"
#include "../sys/ThreadPool_SYS.h"
#include <cstdio>
#include <string>

using namespace SanmapGen;

void RunBakeWeightChecks();                                     // Bake_Weights_PROC_Test.cpp
int RunBakeGpuAcceptance(const std::string& shaderDirectory);   // Bake_Gpu_PROC_Test.cpp

namespace {

using Proc::AllTexelsEqual;
using Proc::BakeSceneInputs;
using Proc::BuildTwoStratumScene;
using Proc::CpuVisualPolicy;
using Proc::ExpectedTexel;
void check(bool bCondition, const char* label) { Proc::CheckBakeExpectation(bCondition, label); }

// 1. Two-stratum blend: 0.25 * red + 0.75 * blue = (64, 0, 191), and strata 1..4 pack the
//    surface weights (0.75 -> 191) for the export mask texture.
void TestTwoStratumBlend() {
    Params::Geometry geometry; geometry.mapSize = 4;
    Data::MapFields fields;
    BakeSceneInputs scene;
    Proc::BakedTextureSet textures;
    Proc::BakeStage stage(geometry, scene.strata, scene.stratumArt, fields, textures);
    BuildTwoStratumScene(geometry, fields, scene, stage);
    stage.SetDispatchPolicy(CpuVisualPolicy());

    check(stage.Run() == Sys::ComputeBackend::Cpu, "Cpu policy resolves to the Cpu twin");
    check(textures.resolution == 4, "output texture sized from mapSize * multiplier");
    check(AllTexelsEqual(textures.compositeAlbedo, ExpectedTexel(64, 0, 191, 255)),
          "two-stratum weights blend to the expected colour (0.25 red + 0.75 blue)");
    check(AllTexelsEqual(textures.stratumMaskLow, ExpectedTexel(191, 0, 0, 0)),
          "stratum 1 weight lands in the packed stratums_1_4 texture");
    check(AllTexelsEqual(textures.stratumMaskHigh, 0u), "strata 5..8 stay empty");
}

// 2. The same blend with a real 2x2 tiled albedo texture on the blue stratum: the texture
//    (pure green) replaces the tint, so the composite becomes 0.25 red + 0.75 green.
void TestTexturedBlend() {
    Params::Geometry geometry; geometry.mapSize = 4;
    Data::MapFields fields;
    BakeSceneInputs scene;
    Proc::BakedTextureSet textures;
    Proc::BakeStage stage(geometry, scene.strata, scene.stratumArt, fields, textures);
    BuildTwoStratumScene(geometry, fields, scene, stage);
    stage.SetDispatchPolicy(CpuVisualPolicy());
    const unsigned int greenTexels[4] = { ExpectedTexel(0, 255, 0, 255), ExpectedTexel(0, 255, 0, 255),
                                          ExpectedTexel(0, 255, 0, 255), ExpectedTexel(0, 255, 0, 255) };
    scene.stratumArt[1].albedoTexels = greenTexels;
    scene.stratumArt[1].albedoWidth = 2;
    scene.stratumArt[1].albedoHeight = 2;
    scene.strata[1].tileCount = 2.0f;
    scene.strata[1].tintRed = 1.0f; scene.strata[1].tintGreen = 1.0f; scene.strata[1].tintBlue = 1.0f;

    stage.Run();
    check(AllTexelsEqual(textures.compositeAlbedo, ExpectedTexel(64, 191, 0, 255)),
          "tiled stratum texture is composited at its surface weight");
}

// 3. The threaded Cpu bake is texel-identical to the serial one (rows are independent).
void TestThreadedCpuBakeMatchesSerial() {
    Params::Geometry geometry; geometry.mapSize = 64;
    Data::MapFields serialFields, threadedFields;
    BakeSceneInputs serialScene, threadedScene;
    Proc::BakedTextureSet serialTextures, threadedTextures;
    Proc::BakeStage serialStage(geometry, serialScene.strata, serialScene.stratumArt,
                                serialFields, serialTextures);
    Proc::BakeStage threadedStage(geometry, threadedScene.strata, threadedScene.stratumArt,
                                  threadedFields, threadedTextures);
    BuildTwoStratumScene(geometry, serialFields, serialScene, serialStage);
    BuildTwoStratumScene(geometry, threadedFields, threadedScene, threadedStage);
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
    TestThreadedCpuBakeMatchesSerial();
    RunBakeWeightChecks();
    const int gpuResult = RunBakeGpuAcceptance(shaderDirectory);
    const int failures = Proc::bakeTestFailures;

    if (failures == 0 && gpuResult == 2) { std::printf("CPU PASS, GPU SKIPPED (no GL context)\n"); return 2; }
    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}
