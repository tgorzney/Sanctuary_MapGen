// Bake_Weights_PROC_Test.cpp — the half of the bake acceptance test that pins the ARCH §7.2.5
// consumption contract: the bake reads `surfaceStratumWeights` VERBATIM (it has no remap of its
// own — two live remaps double-remapped any .sanmap that set both), and its dirty hash covers
// its own appearance settings and nothing that belongs to the Mask stage.
#include "Bake_TestScene_PROC.h"
#include "../pipeline/Generation_PIPELINE.h"
#include <cstdio>

using namespace SanmapGen;

namespace {

using Proc::AllTexelsEqual;
using Proc::BakeSceneInputs;
using Proc::BuildTwoStratumScene;
using Proc::CpuVisualPolicy;
using Proc::ExpectedTexel;
void check(bool bCondition, const char* label) { Proc::CheckBakeExpectation(bCondition, label); }

// The weight the Mask stage produced is what the bake composites and what it packs, byte for
// byte. `maskRemapMinimum`/`maskRemapMaximum` is per-stratum material/appearance pass-through
// data (Generator Expert ruling) — neither a Mask nor a Bake input — so no stage in the current
// pipeline consumes it; the bake must ignore it just like every other stage does today.
void TestNoRivalRemap() {
    Params::Geometry geometry; geometry.mapSize = 4;
    Data::MapFields fields;
    BakeSceneInputs scene;
    Proc::BakedTextureSet textures;
    Proc::BakeStage stage(geometry, scene.strata, scene.stratumArt, fields, textures);
    BuildTwoStratumScene(geometry, fields, scene, stage);
    stage.SetDispatchPolicy(CpuVisualPolicy());

    scene.strata[1].maskRemapMinimum[0] = 0.5f;
    scene.strata[1].maskRemapMaximum[0] = 1.0f;
    stage.Run();
    check(AllTexelsEqual(textures.compositeAlbedo, ExpectedTexel(64, 0, 191, 255)),
          "the bake ignores the stratum appearance remap field (pure pass-through, no consumer yet)");
    check(AllTexelsEqual(textures.stratumMaskLow, ExpectedTexel(191, 0, 0, 0)),
          "the packed mask texture stores the surface weight verbatim");

    // Re-resolving the weight (what Mask would write for that window) is what moves the output:
    // 0.75 remapped onto [0.5, 1.0] = 0.5, so the blend becomes (85, 0, 170).
    fields.surfaceStratumWeights[1].Fill(0.5f);
    stage.Run();
    check(AllTexelsEqual(textures.compositeAlbedo, ExpectedTexel(85, 0, 170, 255)),
          "a changed surface weight — not a bake remap — is what re-weights the blend");
}

// Dirty hash through the pipeline: first Run bakes, an unchanged Run skips, a changed
// appearance setting re-runs, and the appearance remap field never enters this stage's hash.
void TestDirtyHashSkipAndReRun() {
    Params::Geometry geometry; geometry.mapSize = 4;
    Data::MapFields fields;
    BakeSceneInputs scene;
    Proc::BakedTextureSet textures;
    Proc::BakeStage stage(geometry, scene.strata, scene.stratumArt, fields, textures);
    BuildTwoStratumScene(geometry, fields, scene, stage);
    stage.SetDispatchPolicy(CpuVisualPolicy());

    Pipeline::GenerationPipeline pipeline;
    int runCount = 0;
    pipeline.AddStage("Bake", [&stage]() { return stage.ComputeParameterHash(); },
                      [&stage, &runCount]() { stage.Run(); ++runCount; });
    check(pipeline.Run().size() == 1 && runCount == 1, "first pipeline run bakes the stage");
    check(pipeline.Run().empty() && runCount == 1, "unchanged parameters skip the stage");
    scene.strata[1].tintGreen = 1.0f;            // the blue stratum becomes cyan
    check(pipeline.Run().size() == 1 && runCount == 2, "changing a stratum tint re-runs the stage");
    check(AllTexelsEqual(textures.compositeAlbedo, ExpectedTexel(64, 191, 191, 255)),
          "the re-run picked up the new tint");

    const std::size_t hashBeforeAppearanceEdit = stage.ComputeParameterHash();
    scene.strata[1].maskRemapMinimum[0] = 0.3f;
    check(stage.ComputeParameterHash() == hashBeforeAppearanceEdit,
          "the appearance remap field (not a Mask or Bake input) does not enter the bake's parameter hash");
}

} // namespace

void RunBakeWeightChecks() {
    TestNoRivalRemap();
    TestDirtyHashSkipAndReRun();
}
