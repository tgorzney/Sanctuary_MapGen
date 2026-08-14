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

// The mask stage wired behind a mock upstream stage, exactly as PIPELINE will wire it.
struct DirtyHashHarness {
    Params::Geometry geometry;
    Data::MapFields fields;
    std::vector<Params::StratumMask> stratumMasks;
    Proc::MaskStage stage;
    Pipeline::GenerationPipeline pipeline;
    std::size_t upstreamHeightParameter = 1;
    int maskRunCount = 0;

    DirtyHashHarness()
        : stratumMasks(Data::MapFields::stratumCount), stage(geometry, stratumMasks, fields) {
        geometry.mapSize = 32;
        fields.Resize(geometry.VertexSize());
        FillTestHeightfield(fields, geometry.VertexSize());
        FillTestProceduralMasks(fields, geometry.VertexSize());
        stratumMasks[0].bSlopeGateEnabled = true;
        stratumMasks[0].maximumSlopeDegrees = 30.0f;
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

    harness.stratumMasks[0].maximumSlopeDegrees = 35.0f;
    ran = harness.pipeline.Run();
    Check(ran.size() == 1 && Ran(ran, "Mask") && harness.maskRunCount == 2,
          "changing a slope-gate setting re-runs only the mask stage");

    harness.stratumMasks[3].maskRemapMaximum = 0.9f;
    ran = harness.pipeline.Run();
    Check(ran.size() == 1 && harness.maskRunCount == 3,
          "changing another stratum's remap re-runs the mask stage");

    harness.upstreamHeightParameter = 2;
    ran = harness.pipeline.Run();
    Check(ran.size() == 2 && Ran(ran, "Heightfield") && Ran(ran, "Mask") && harness.maskRunCount == 4,
          "an upstream change dirties the mask stage as well");
}

// Stored art is an input like any other: both its arrival and its pixels are part of the hash.
void CheckStoredArtDirtying(DirtyHashHarness& harness) {
    harness.stratumMasks[1].importedMaskMode = Params::ImportedMaskMode::StaticOverride;
    harness.stratumMasks[1].importedMaskWidth = 2;
    harness.stratumMasks[1].importedMaskHeight = 2;
    harness.stratumMasks[1].importedMaskData = { 0.0f, 0.25f, 0.5f, 0.75f };
    std::vector<std::string> ran = harness.pipeline.Run();
    Check(ran.size() == 1 && harness.maskRunCount == 5, "importing stored art re-runs the mask stage");

    const std::size_t hashBeforeEdit = harness.stage.ComputeParameterHash();
    harness.stratumMasks[1].importedMaskData[2] = 0.6f;
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
