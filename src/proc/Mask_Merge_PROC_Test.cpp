// Mask_Merge_PROC_Test.cpp — the stored-art half of the M3-2 acceptance test. A 3x3 map and a
// 2x2 stored art make every expected value hand-computable: the art's sample scale is 0.5, so
// the middle row/column lands exactly between two texels and bilinear must read 0.5 there —
// a nearest-neighbour resampler could only ever return 0.0 or 1.0 (MASKING_SPEC 1.8).
#include "Mask_TestSupport_PROC.h"
#include "Mask_PROC.h"
#include <vector>

namespace SanmapGen {
namespace MaskTest {
namespace {

constexpr int kMapSize = 2;          // 3x3 vertices
constexpr float kProportionValue = 0.25f;

// Stored art: a 2x2 checker. Hand-resampled onto the 3x3 vertex grid it is
//   0.0 0.5 1.0 / 0.5 0.5 0.5 / 1.0 0.5 0.0
const float kStoredArt[4] = { 0.0f, 1.0f, 1.0f, 0.0f };
const float kExpectedStoredSample[9] = { 0.0f, 0.5f, 1.0f, 0.5f, 0.5f, 0.5f, 1.0f, 0.5f, 0.0f };

std::vector<Params::Stratum> MakeMergeSettings(Params::ImportedMaskMode mode) {
    Params::Stratum stratum;
    stratum.importedMaskMode = mode;
    return std::vector<Params::Stratum>(Data::MapFields::stratumCount, stratum);
}

std::vector<Data::StratumArt> MakeCheckerArt() {
    std::vector<Data::StratumArt> stratumArt = NoStratumArt();
    for (Data::StratumArt& art : stratumArt) SetImportedMask(art, kStoredArt, 2, 2);
    return stratumArt;
}

// Flat terrain (gradient 0) so the merge is observed without any slope influence.
std::vector<float> RunMerge(const std::vector<Params::Stratum>& strata) {
    Params::Geometry geometry;
    geometry.mapSize = kMapSize;
    Data::MapFields fields;
    fields.Resize(geometry.VertexSize(), 0.5f);
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
        fields.materialProportions[stratum].Fill(kProportionValue);
    const std::vector<Data::StratumArt> stratumArt = MakeCheckerArt();
    const Params::SlopeDefaults slopeDefaults;
    Proc::MaskStage stage(geometry, strata, stratumArt, fields, slopeDefaults);
    stage.RunOnCpu();
    std::vector<float> result;
    for (int y = 0; y < geometry.VertexSize(); ++y)
        for (int x = 0; x < geometry.VertexSize(); ++x)
            result.push_back(fields.surfaceStratumWeights[0].Get(x, y));
    return result;
}

// 1-3. Each merge mode on hand-computed values.
void CheckMergeModes() {
    // Disabled ignores the stored art entirely.
    std::vector<float> result = RunMerge(MakeMergeSettings(Params::ImportedMaskMode::Disabled));
    bool bUntouched = true;
    for (float value : result) if (std::fabs(value - kProportionValue) > 1e-6f) bUntouched = false;
    Check(bUntouched, "Disabled emits the ungated procedural weight");

    // 2. StaticOverride replaces with the bilinear-resampled art, exactly.
    result = RunMerge(MakeMergeSettings(Params::ImportedMaskMode::StaticOverride));
    for (int index = 0; index < 9; ++index)
        CheckNear(result[index], kExpectedStoredSample[index], 1e-6f, "StaticOverride == resampled stored art");
    Check(std::fabs(result[4] - 0.5f) < 1e-6f, "resampler is bilinear, not nearest (mid-texel reads 0.5)");

    // 3. ProceduralStart is additive and clamped: 0.25 + stored.
    result = RunMerge(MakeMergeSettings(Params::ImportedMaskMode::ProceduralStart));
    for (int index = 0; index < 9; ++index) {
        float expected = kProportionValue + kExpectedStoredSample[index];
        if (expected > 1.0f) expected = 1.0f;
        CheckNear(result[index], expected, 1e-6f, "ProceduralStart == clamp(procedural + stored)");
    }
}

// 4. A gate that rejects every cell zeroes the procedural part but must NOT touch the stored art
// under StaticOverride (that mode is locked to what the artist shipped).
void CheckGateInteraction() {
    std::vector<Params::Stratum> settings = MakeMergeSettings(Params::ImportedMaskMode::StaticOverride);
    for (Params::Stratum& stratum : settings) {
        stratum.bSlopeUseGlobal = false;   // exercise THIS stratum's own window, not slopeDefaults
        stratum.bSlopeGateEnabled = true;
        stratum.minimumSlopeDegrees = 80.0f;
        stratum.maximumSlopeDegrees = 89.0f;
    }
    std::vector<float> result = RunMerge(settings);
    for (int index = 0; index < 9; ++index)
        CheckNear(result[index], kExpectedStoredSample[index], 1e-6f, "StaticOverride is not slope-gated");

    settings = MakeMergeSettings(Params::ImportedMaskMode::ProceduralStart);
    for (Params::Stratum& stratum : settings) {
        stratum.bSlopeUseGlobal = false;   // exercise THIS stratum's own window, not slopeDefaults
        stratum.bSlopeGateEnabled = true;
        stratum.minimumSlopeDegrees = 80.0f;
        stratum.maximumSlopeDegrees = 89.0f;
    }
    result = RunMerge(settings);
    for (int index = 0; index < 9; ++index)
        CheckNear(result[index], kExpectedStoredSample[index], 1e-6f,
                  "ProceduralStart adds onto a fully gated-out procedural weight");
}

// 5. `maskRemapMinimum`/`maskRemapMaximum` is per-stratum material/appearance pass-through
// data, NOT a Mask-stage input (Generator Expert ruling): the merge output stays the plain
// clamped merge, with no per-stratum window applied, regardless of what that field holds.
void CheckMergeOutputIgnoresAppearanceRemapField() {
    std::vector<Params::Stratum> settings = MakeMergeSettings(Params::ImportedMaskMode::StaticOverride);
    for (Params::Stratum& stratum : settings) {
        for (int channel = 0; channel < Params::kStratumColorChannelCount; ++channel) {
            stratum.maskRemapMinimum[channel] = 0.25f;
            stratum.maskRemapMaximum[channel] = 0.75f;
        }
    }
    const std::vector<float> result = RunMerge(settings);
    for (int index = 0; index < 9; ++index)
        CheckNear(result[index], kExpectedStoredSample[index], 1e-6f,
                  "Mask output is unaffected by maskRemapMinimum/Maximum (not a Mask input)");
}

// 6. A merge mode with no imported art degrades to the gated procedural weight rather than
// erasing the stratum (the pixels are DATA and may simply be absent).
void CheckMissingArtDegradesSafely() {
    Params::Geometry geometry;
    geometry.mapSize = kMapSize;
    Data::MapFields fields;
    fields.Resize(geometry.VertexSize(), 0.5f);
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
        fields.materialProportions[stratum].Fill(kProportionValue);
    const std::vector<Params::Stratum> settings = MakeMergeSettings(Params::ImportedMaskMode::StaticOverride);
    const std::vector<Data::StratumArt> emptyArt = NoStratumArt();
    const Params::SlopeDefaults slopeDefaults;
    Proc::MaskStage stage(geometry, settings, emptyArt, fields, slopeDefaults);
    stage.RunOnCpu();
    CheckNear(fields.surfaceStratumWeights[0].Get(1, 1), kProportionValue, 1e-6f,
              "StaticOverride with no loaded art falls back to the procedural weight");
}

} // namespace

void RunMergeTests() {
    CheckMergeModes();
    CheckGateInteraction();
    CheckMergeOutputIgnoresAppearanceRemapField();
    CheckMissingArtDegradesSafely();
}

} // namespace MaskTest
} // namespace SanmapGen
