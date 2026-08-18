// Mask_DirtyHash_PROC_Test.cpp — the dirty-hash half of the M3-2 acceptance test: the stage
// registered in Generation_PIPELINE behind a mock upstream (heightfield) stage. Verifies the
// skip/re-run contract in both directions — the stage's own settings dirty only it, an upstream
// change dirties it too (the stage never inspects the pipeline shape itself, ARCH §3.2).
#include "Mask_TestSupport_PROC.h"
#include "Mask_PROC.h"
#include "../pipeline/Generation_PIPELINE.h"
#include <string>
#include <vector>

namespace SanmapGen {
namespace MaskTest {
namespace {

bool Ran(const std::vector<std::string>& stageNames, const char* name) {
    for (const std::string& stageName : stageNames) if (stageName == name) return true;
    return false;
}

// The mask stage wired behind a mock upstream stage, exactly as PIPELINE wires it.
struct DirtyHashHarness {
    Params::Geometry geometry;
    Data::MapFields fields;
    std::vector<Params::Stratum> strata;
    std::vector<Data::StratumArt> stratumArt;
    Proc::MaskStage stage;
    Pipeline::GenerationPipeline pipeline;
    std::size_t upstreamHeightParameter = 1;
    int maskRunCount = 0;

    DirtyHashHarness()
        : strata(Data::MapFields::stratumCount), stratumArt(Data::MapFields::stratumCount),
          stage(geometry, strata, stratumArt, fields) {
        geometry.mapSize = 32;
        fields.Resize(geometry.VertexSize());
        FillTestHeightfield(fields, geometry.VertexSize());
        FillTestMaterialProportions(fields, geometry.VertexSize());
        strata[0].bSlopeGateEnabled = true;
        strata[0].maximumSlopeDegrees = 30.0f;
        Sys::DispatchPolicy cpuOnlyPolicy;
        cpuOnlyPolicy.previewBackend = Sys::ComputeBackend::Cpu;
        cpuOnlyPolicy.outputBackend  = Sys::ComputeBackend::Cpu;
        stage.SetDispatchPolicy(cpuOnlyPolicy);
        pipeline.AddStage("Heightfield", [this] { return upstreamHeightParameter; }, [] {});
        pipeline.AddStage("Mask", [this] { return stage.ComputeParameterHash(); },
                                  [this] { stage.Run(); ++maskRunCount; });
    }
};

void CheckSettingsDirtying(DirtyHashHarness& harness) {
    std::vector<std::string> ran = harness.pipeline.Run();
    Check(Ran(ran, "Mask") && harness.maskRunCount == 1, "first run executes the mask stage");

    ran = harness.pipeline.Run();
    Check(ran.empty() && harness.maskRunCount == 1, "unchanged settings skip the mask stage");

    harness.strata[0].maximumSlopeDegrees = 35.0f;
    ran = harness.pipeline.Run();
    Check(ran.size() == 1 && Ran(ran, "Mask") && harness.maskRunCount == 2,
          "changing a slope-gate setting re-runs only the mask stage");

    harness.strata[3].slopeGateStrength = 0.5f;
    ran = harness.pipeline.Run();
    Check(ran.size() == 1 && harness.maskRunCount == 3,
          "changing another stratum's gate strength re-runs the mask stage");

    harness.upstreamHeightParameter = 2;
    ran = harness.pipeline.Run();
    Check(ran.size() == 2 && Ran(ran, "Heightfield") && Ran(ran, "Mask") && harness.maskRunCount == 4,
          "an upstream change dirties the mask stage as well");

    // `maskRemapMinimum`/`maskRemapMaximum` is per-stratum material/appearance pass-through
    // data, NOT a Mask-stage input (Generator Expert ruling): editing it must not move the hash
    // or re-run the stage.
    const std::size_t hashBeforeAppearanceEdit = harness.stage.ComputeParameterHash();
    harness.strata[2].maskRemapMinimum[0] = 0.4f;
    harness.strata[2].maskRemapMaximum[1] = 0.6f;
    Check(harness.stage.ComputeParameterHash() == hashBeforeAppearanceEdit,
          "maskRemapMinimum/Maximum does not change the Mask parameter hash");
    ran = harness.pipeline.Run();
    Check(ran.empty() && harness.maskRunCount == 4,
          "changing maskRemapMinimum/Maximum does not re-run the mask stage");
}

// Stored art is an input like any other: both its arrival and its pixels are part of the hash.
void CheckStoredArtDirtying(DirtyHashHarness& harness) {
    const float artPixels[4] = { 0.0f, 0.25f, 0.5f, 0.75f };
    harness.strata[1].importedMaskMode = Params::ImportedMaskMode::StaticOverride;
    SetImportedMask(harness.stratumArt[1], artPixels, 2, 2);
    std::vector<std::string> ran = harness.pipeline.Run();
    Check(ran.size() == 1 && harness.maskRunCount == 5, "importing stored art re-runs the mask stage");

    const std::size_t hashBeforeEdit = harness.stage.ComputeParameterHash();
    harness.stratumArt[1].importedMask.Set(0, 1, 0.6f);
    Check(harness.stage.ComputeParameterHash() != hashBeforeEdit, "stored-art CONTENT is part of the hash");
    ran = harness.pipeline.Run();
    Check(ran.size() == 1 && harness.maskRunCount == 6, "editing stored-art pixels re-runs the mask stage");

    ran = harness.pipeline.Run();
    Check(ran.empty() && harness.maskRunCount == 6, "the stage settles again once nothing changes");
}

} // namespace

void RunDirtyHashTests() {
    DirtyHashHarness harness;
    CheckSettingsDirtying(harness);
    CheckStoredArtDirtying(harness);
}

} // namespace MaskTest
} // namespace SanmapGen
