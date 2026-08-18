// Mask_Purity_PROC_Test.cpp — the two defects ARCH §7.2 exists to prevent, asserted directly:
//   1. SINGLE WRITER — the stage must leave `materialProportions` byte-identical. Gating the
//      physical field in place is what let a renormalizing sim undo the gate.
//   2. IDEMPOTENCE — running the stage twice from the same inputs must give the same output,
//      because the dirty-hash conductor is entitled to re-run Mask alone (ARCH §3.4.2). An
//      in-place read-modify-write would apply its gate/remap a second time.
// Both are run with a settings mix that exercises the gate, both merge modes and a non-identity
// gate strength, so a re-applied transform could not hide behind an identity case.
#include "Mask_TestSupport_PROC.h"
#include "Mask_PROC.h"
#include <cstring>
#include <vector>

namespace SanmapGen {
namespace MaskTest {
namespace {

constexpr int kMapSize = 48;

std::vector<Params::Stratum> MakePurityStrata() {
    std::vector<Params::Stratum> strata(Data::MapFields::stratumCount);
    for (int index = 0; index < Data::MapFields::stratumCount; ++index) {
        Params::Stratum& stratum = strata[index];
        stratum.bSlopeUseGlobal     = false;   // exercise every stratum's OWN window, not slopeDefaults
        stratum.bSlopeGateEnabled   = true;
        stratum.minimumSlopeDegrees = 4.0f * static_cast<float>(index);
        stratum.maximumSlopeDegrees = 25.0f + 4.0f * static_cast<float>(index);
        stratum.bUseSmoothstep      = (index % 2) == 1;
        stratum.slopeFeatherDegreesLow  = 2.0f;
        stratum.slopeFeatherDegreesHigh = 3.0f;
    }
    strata[1].importedMaskMode = Params::ImportedMaskMode::ProceduralStart;
    strata[4].importedMaskMode = Params::ImportedMaskMode::StaticOverride;
    strata[6].slopeGateStrength = 0.35f;   // a non-identity partial gate: applying it twice would show
    return strata;
}

std::vector<Data::StratumArt> MakePurityArt() {
    std::vector<Data::StratumArt> stratumArt = NoStratumArt();
    const int side = 21;
    std::vector<float> pixels(static_cast<std::size_t>(side) * side);
    for (int index = 0; index < side * side; ++index)
        pixels[index] = static_cast<float>((index * 13) % 97) * 0.0103f;
    SetImportedMask(stratumArt[1], pixels.data(), side, side);
    SetImportedMask(stratumArt[4], pixels.data(), side, side);
    return stratumArt;
}

void BuildInputs(Data::MapFields& fields, int vertexSize) {
    fields.Resize(vertexSize);
    FillTestHeightfield(fields, vertexSize);
    FillTestMaterialProportions(fields, vertexSize);
}

// M5-0c: the baked slope field, checked against the analytic gradient of a heightfield whose
// slope is known by hand. A plane of rise `risePerCell` per cell in x and `2*risePerCell` in y
// has |grad(height * terrainMaxHeight)| = terrainMaxHeight * risePerCell * sqrt(5) everywhere,
// edges included (the one-sided difference of a plane is the central one).
void CheckSlopeFieldMatchesAnalyticGradient() {
    Params::Geometry geometry;
    geometry.mapSize = 16;
    geometry.terrainMaxHeight = 64.0f;
    const int vertexSize = geometry.VertexSize();
    const float risePerCell = 0.003f;
    Data::MapFields fields;
    fields.Resize(vertexSize);
    for (int y = 0; y < vertexSize; ++y)
        for (int x = 0; x < vertexSize; ++x)
            fields.heightfield.Set(x, y, risePerCell * static_cast<float>(x + 2 * y));

    const std::vector<Params::Stratum> strata(Data::MapFields::stratumCount);
    const std::vector<Data::StratumArt> stratumArt = NoStratumArt();
    const Params::SlopeDefaults slopeDefaults;
    Proc::MaskStage stage(geometry, strata, stratumArt, fields, slopeDefaults);
    stage.RunOnCpu();

    const float expectedSlope = geometry.terrainMaxHeight * risePerCell * std::sqrt(5.0f);
    CheckNear(fields.slope.Get(9, 6), expectedSlope, 1e-5f, "baked slope at a spot cell is the analytic gradient");
    CheckNear(fields.slope.Get(0, 0), expectedSlope, 1e-5f, "the one-sided edge cell matches it too");
}

} // namespace

void RunPurityTests() {
    CheckSlopeFieldMatchesAnalyticGradient();
    Params::Geometry geometry;
    geometry.mapSize = kMapSize;
    const int vertexSize = geometry.VertexSize();
    const std::vector<Params::Stratum> strata = MakePurityStrata();
    const std::vector<Data::StratumArt> stratumArt = MakePurityArt();
    const Params::SlopeDefaults slopeDefaults;

    Data::MapFields fields;
    BuildInputs(fields, vertexSize);
    const Data::MapFields inputSnapshot = fields;   // proportions + heightfield before the run

    Proc::MaskStage stage(geometry, strata, stratumArt, fields, slopeDefaults);
    stage.RunOnCpu();

    // 1. Single writer: only surfaceStratumWeights moved.
    CheckProportionsUntouched(fields, inputSnapshot,
                              "Mask leaves materialProportions byte-identical (single writer)");
    Check(FieldsAreByteIdentical(fields.heightfield, inputSnapshot.heightfield),
          "Mask leaves the heightfield byte-identical");

    // Guard against a trivially-true result: the gate/merge/remap must really have done work.
    CheckWeightsAreResolved(fields, vertexSize);

    // 2. Idempotence: a second lone run (exactly what the dirty-hash conductor may do) must
    // reproduce the first result bit for bit.
    const Data::MapFields firstRun = fields;
    stage.RunOnCpu();
    bool bIdempotent = true;
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
        if (!FieldsAreByteIdentical(fields.surfaceStratumWeights[stratum],
                                    firstRun.surfaceStratumWeights[stratum]))
            bIdempotent = false;
    Check(bIdempotent, "running Mask twice gives the identical surfaceStratumWeights (idempotent)");
    Check(FieldsAreByteIdentical(fields.slope, firstRun.slope),
          "running Mask twice gives the identical slope field (idempotent)");

    // 3. A fresh stage over the same inputs lands on the same answer — the stage carries no
    // state that would make a re-run from a clean conductor differ.
    Data::MapFields replayFields;
    BuildInputs(replayFields, vertexSize);
    Proc::MaskStage replayStage(geometry, strata, stratumArt, replayFields, slopeDefaults);
    replayStage.RunOnCpu();
    bool bReplayMatches = true;
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
        if (!FieldsAreByteIdentical(replayFields.surfaceStratumWeights[stratum],
                                    firstRun.surfaceStratumWeights[stratum]))
            bReplayMatches = false;
    Check(bReplayMatches, "a fresh stage over the same inputs reproduces the same weights");
}

} // namespace MaskTest
} // namespace SanmapGen
