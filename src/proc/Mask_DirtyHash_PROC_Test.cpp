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
    Params::SlopeDefaults slopeDefaults;
    Proc::MaskStage stage;
    Pipeline::GenerationPipeline pipeline;
    std::size_t upstreamHeightParameter = 1;
    int maskRunCount = 0;

    DirtyHashHarness()
        : strata(Data::MapFields::stratumCount), stratumArt(Data::MapFields::stratumCount),
          stage(geometry, strata, stratumArt, fields, slopeDefaults) {
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

// STEP10_SlopeDefaults_Mechanism acceptance test, part 1: the resolution is a pure substitution.
// A stratum with `bSlopeUseGlobal == true` reading a populated `slopeDefaults` must resolve to
// the IDENTICAL `MaskStratumConfiguration` as the same stratum with `bSlopeUseGlobal == false`
// and its own 7 fields manually set to match `slopeDefaults`'s values.
void CheckPureSubstitution() {
    Params::SlopeDefaults slopeDefaults;
    slopeDefaults.bSlopeGateEnabled       = true;
    slopeDefaults.minimumSlopeDegrees     = 12.0f;
    slopeDefaults.maximumSlopeDegrees     = 47.0f;
    slopeDefaults.slopeFeatherDegreesLow  = 3.0f;
    slopeDefaults.slopeFeatherDegreesHigh = 6.0f;
    slopeDefaults.bUseSmoothstep          = true;
    slopeDefaults.bInvertSlopeGate        = true;
    slopeDefaults.slopeGateStrength       = 0.6f;

    Params::Geometry geometry;
    geometry.mapSize = 8;
    Data::MapFields fieldsGlobal, fieldsExplicit;
    fieldsGlobal.Resize(geometry.VertexSize());
    fieldsExplicit.Resize(geometry.VertexSize());
    FillTestHeightfield(fieldsGlobal, geometry.VertexSize());
    FillTestHeightfield(fieldsExplicit, geometry.VertexSize());
    FillTestMaterialProportions(fieldsGlobal, geometry.VertexSize());
    FillTestMaterialProportions(fieldsExplicit, geometry.VertexSize());
    const std::vector<Data::StratumArt> stratumArt = NoStratumArt();

    std::vector<Params::Stratum> globalStrata(Data::MapFields::stratumCount);
    globalStrata[0].bSlopeUseGlobal = true;   // reads slopeDefaults; own 7 fields stay defaulted

    std::vector<Params::Stratum> explicitStrata(Data::MapFields::stratumCount);
    explicitStrata[0].bSlopeUseGlobal         = false;
    explicitStrata[0].bSlopeGateEnabled       = slopeDefaults.bSlopeGateEnabled;
    explicitStrata[0].minimumSlopeDegrees     = slopeDefaults.minimumSlopeDegrees;
    explicitStrata[0].maximumSlopeDegrees     = slopeDefaults.maximumSlopeDegrees;
    explicitStrata[0].slopeFeatherDegreesLow  = slopeDefaults.slopeFeatherDegreesLow;
    explicitStrata[0].slopeFeatherDegreesHigh = slopeDefaults.slopeFeatherDegreesHigh;
    explicitStrata[0].bUseSmoothstep          = slopeDefaults.bUseSmoothstep;
    explicitStrata[0].bInvertSlopeGate        = slopeDefaults.bInvertSlopeGate;
    explicitStrata[0].slopeGateStrength       = slopeDefaults.slopeGateStrength;

    const Params::SlopeDefaults emptyDefaults;   // explicitStrata never consults this
    Proc::MaskStage globalStage(geometry, globalStrata, stratumArt, fieldsGlobal, slopeDefaults);
    Proc::MaskStage explicitStage(geometry, explicitStrata, stratumArt, fieldsExplicit, emptyDefaults);
    globalStage.RunOnCpu();
    explicitStage.RunOnCpu();

    const Proc::MaskStratumConfiguration& fromGlobal   = globalStage.StratumConfigurations()[0];
    const Proc::MaskStratumConfiguration& fromExplicit = explicitStage.StratumConfigurations()[0];
    Check(fromGlobal.slopeGradientLow   == fromExplicit.slopeGradientLow
       && fromGlobal.slopeGradientHigh  == fromExplicit.slopeGradientHigh
       && fromGlobal.inverseFeatherLow  == fromExplicit.inverseFeatherLow
       && fromGlobal.inverseFeatherHigh == fromExplicit.inverseFeatherHigh
       && fromGlobal.bSmoothstepEnabled == fromExplicit.bSmoothstepEnabled
       && fromGlobal.bInvertEnabled     == fromExplicit.bInvertEnabled
       && fromGlobal.gateStrength       == fromExplicit.gateStrength,
          "bSlopeUseGlobal is a pure config-source substitution, bit for bit");
    Check(FieldsAreByteIdentical(fieldsGlobal.surfaceStratumWeights[0], fieldsExplicit.surfaceStratumWeights[0]),
          "the resolved surface weights match too, not just the flattened configuration");
}

// STEP10_SlopeDefaults_Mechanism acceptance test, part 2: the Generator Expert's hash ruling.
// Ruling item 1 (implemented verbatim in Mask_PROC.cpp) is explicit that `HashSlopeDefaults` is
// folded into `ComputeParameterHash()` UNCONDITIONALLY — "not gated on whether any stratum
// currently has bSlopeUseGlobal == true" — the same posture `HashConstants` already has for
// per-stratum constants that are not "currently live" for every stratum. Under that ruling, a
// `slopeDefaults`-only edit therefore dirties EVERY MaskStage's hash, regardless of whether any
// of ITS OWN strata currently opt into `slopeDefaults` — there is no way to hash it
// unconditionally (as ruled) AND have it leave an all-`bSlopeUseGlobal==false` stage's hash
// untouched; those two properties are mutually exclusive for a single aggregate stage hash. This
// test verifies the ruling as written: BOTH stages below see the hash move. (NOTE: this differs
// from this ticket's own acceptance-test prose, which additionally asked for the opaque stage's
// hash to stay put — that specific sentence cannot hold simultaneously with ruling item 1's
// "unconditional" instruction; flagged for the human/Generator Expert rather than silently
// re-deriving a conditional hash the ruling explicitly rejected.)
void CheckSlopeDefaultsHashSensitivity() {
    Params::Geometry geometry;
    Data::MapFields fields;
    fields.Resize(geometry.VertexSize());
    const std::vector<Data::StratumArt> stratumArt = NoStratumArt();
    Params::SlopeDefaults slopeDefaults;

    std::vector<Params::Stratum> usesGlobalStrata(Data::MapFields::stratumCount);
    usesGlobalStrata[0].bSlopeUseGlobal = true;
    Proc::MaskStage usesGlobalStage(geometry, usesGlobalStrata, stratumArt, fields, slopeDefaults);
    const std::size_t hashBeforeGlobalEdit = usesGlobalStage.ComputeParameterHash();

    std::vector<Params::Stratum> opaqueStrata(Data::MapFields::stratumCount);
    opaqueStrata[0].bSlopeUseGlobal = false;
    Proc::MaskStage opaqueStage(geometry, opaqueStrata, stratumArt, fields, slopeDefaults);
    const std::size_t hashBeforeOpaqueEdit = opaqueStage.ComputeParameterHash();

    slopeDefaults.maximumSlopeDegrees = 61.0f;   // the whole edit: nothing per-stratum touched
    Check(usesGlobalStage.ComputeParameterHash() != hashBeforeGlobalEdit,
          "a slopeDefaults-only edit dirties a stratum with bSlopeUseGlobal == true");
    Check(opaqueStage.ComputeParameterHash() != hashBeforeOpaqueEdit,
          "ruling item 1's UNCONDITIONAL HashSlopeDefaults call also dirties an all-opaque stage "
          "(this is the ruled behavior, not a bug — see the function header comment)");
}

} // namespace

void RunDirtyHashTests() {
    CheckPureSubstitution();
    CheckSlopeDefaultsHashSensitivity();
    DirtyHashHarness harness;
    CheckSettingsDirtying(harness);
    CheckStoredArtDirtying(harness);
}

} // namespace MaskTest
} // namespace SanmapGen
