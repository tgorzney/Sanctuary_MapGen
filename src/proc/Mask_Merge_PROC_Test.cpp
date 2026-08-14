// Mask_Merge_PROC_Test.cpp — the stored-mask half of the M3-2 acceptance test. A 3x3 map and a
// 2x2 stored art make every expected value hand-computable: the art's sample scale is 0.5, so
// the middle row/column lands exactly between two texels and bilinear must read 0.5 there —
// a nearest-neighbour resampler could only ever return 0.0 or 1.0 (MASKING_SPEC resample fix).
#include "Mask_TestSupport_PROC.h"
#include "Mask_PROC.h"
#include <vector>

namespace SanmapGen {
namespace MaskTest {
namespace {

constexpr int kMapSize = 2;          // 3x3 vertices
constexpr float kProceduralValue = 0.25f;

// Stored art: a 2x2 checker. Hand-resampled onto the 3x3 vertex grid it is
//   0.0 0.5 1.0 / 0.5 0.5 0.5 / 1.0 0.5 0.0
const float kStoredArt[4] = { 0.0f, 1.0f, 1.0f, 0.0f };
const float kExpectedStoredSample[9] = { 0.0f, 0.5f, 1.0f, 0.5f, 0.5f, 0.5f, 1.0f, 0.5f, 0.0f };

std::vector<Params::StratumMask> MakeMergeSettings(Params::ImportedMaskMode mode) {
    Params::StratumMask stratumMask;
    stratumMask.importedMaskMode = mode;
    stratumMask.importedMaskWidth = 2;
    stratumMask.importedMaskHeight = 2;
    stratumMask.importedMaskData.assign(kStoredArt, kStoredArt + 4);
    return std::vector<Params::StratumMask>(Data::MapFields::stratumCount, stratumMask);
}

// Flat terrain (gradient 0) so the merge is observed without any slope influence.
std::vector<float> RunMerge(const std::vector<Params::StratumMask>& stratumMasks) {
    Params::Geometry geometry;
    geometry.mapSize = kMapSize;
    Data::MapFields fields;
    fields.Resize(geometry.VertexSize(), 0.5f);
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
        fields.materialMasks[stratum].Fill(kProceduralValue);
    Proc::MaskStage stage(geometry, stratumMasks, fields);
    stage.RunOnCpu();
    std::vector<float> result;
    for (int y = 0; y < geometry.VertexSize(); ++y)
        for (int x = 0; x < geometry.VertexSize(); ++x) result.push_back(fields.materialMasks[0].Get(x, y));
    return result;
}

// 1-3. Each merge mode on hand-computed values.
void CheckMergeModes() {
    // Disabled ignores the stored art entirely.
    std::vector<float> result = RunMerge(MakeMergeSettings(Params::ImportedMaskMode::Disabled));
    bool bUntouched = true;
    for (float value : result) if (std::fabs(value - kProceduralValue) > 1e-6f) bUntouched = false;
    Check(bUntouched, "Disabled keeps the procedural mask untouched");

    // 2. StaticOverride replaces with the bilinear-resampled art, exactly.
    result = RunMerge(MakeMergeSettings(Params::ImportedMaskMode::StaticOverride));
    for (int index = 0; index < 9; ++index)
        CheckNear(result[index], kExpectedStoredSample[index], 1e-6f, "StaticOverride == resampled stored art");
    Check(std::fabs(result[4] - 0.5f) < 1e-6f, "resampler is bilinear, not nearest (mid-texel reads 0.5)");

    // 3. ProceduralStart is additive and clamped: 0.25 + stored.
    result = RunMerge(MakeMergeSettings(Params::ImportedMaskMode::ProceduralStart));
    for (int index = 0; index < 9; ++index) {
        float expected = kProceduralValue + kExpectedStoredSample[index];
        if (expected > 1.0f) expected = 1.0f;
        CheckNear(result[index], expected, 1e-6f, "ProceduralStart == clamp(procedural + stored)");
    }
}

// 4. A gate that rejects every cell zeroes the procedural part but must NOT touch the stored art
// under StaticOverride (that mode is locked to what the artist shipped).
void CheckGateInteraction() {
    std::vector<Params::StratumMask> settings = MakeMergeSettings(Params::ImportedMaskMode::StaticOverride);
    for (Params::StratumMask& stratumMask : settings) {
        stratumMask.bSlopeGateEnabled = true;
        stratumMask.minimumSlopeDegrees = 80.0f;
        stratumMask.maximumSlopeDegrees = 89.0f;
    }
    std::vector<float> result = RunMerge(settings);
    for (int index = 0; index < 9; ++index)
        CheckNear(result[index], kExpectedStoredSample[index], 1e-6f, "StaticOverride is not slope-gated");

    settings = MakeMergeSettings(Params::ImportedMaskMode::ProceduralStart);
    for (Params::StratumMask& stratumMask : settings) {
        stratumMask.bSlopeGateEnabled = true;
        stratumMask.minimumSlopeDegrees = 80.0f;
        stratumMask.maximumSlopeDegrees = 89.0f;
    }
    result = RunMerge(settings);
    for (int index = 0; index < 9; ++index)
        CheckNear(result[index], kExpectedStoredSample[index], 1e-6f,
                  "ProceduralStart adds onto a fully gated-out procedural mask");
}

// 5. The per-stratum remap is applied last: [0.25,0.75] stretches the stored art.
void CheckRemap() {
    std::vector<Params::StratumMask> settings = MakeMergeSettings(Params::ImportedMaskMode::StaticOverride);
    for (Params::StratumMask& stratumMask : settings) {
        stratumMask.maskRemapMinimum = 0.25f;
        stratumMask.maskRemapMaximum = 0.75f;
    }
    const std::vector<float> result = RunMerge(settings);
    CheckNear(result[0], 0.0f, 1e-6f, "remap clamps 0.0 to the floor");
    CheckNear(result[4], 0.5f, 1e-6f, "remap maps the mid value to 0.5");
    CheckNear(result[2], 1.0f, 1e-6f, "remap clamps 1.0 to the ceiling");
}

} // namespace

void RunMergeTests() {
    CheckMergeModes();
    CheckGateInteraction();
    CheckRemap();
}

} // namespace MaskTest
} // namespace SanmapGen
