// Mask_WorldScale_PROC_Test.cpp — M5-0d: cell world-size has exactly ONE owner,
// `Params::Geometry::worldUnitsPerCell` (ARCH §7.1). Checks that the baked slope is rise per
// WORLD unit (so the same terrain is shallower on a coarser cell), that changing the value
// dirties the stage, and — the integration half — that the Placement gate reading that baked
// slope and the world positions Placement emits are on the SAME scale, because both now read
// the one value. Expected gradients are derived by hand, never from the kernel under test.
#include "Placement_Test_Terrain.h"      // the fixture stands in for this stage; global namespace
#include "Mask_TestSupport_PROC.h"
#include "Mask_PROC.h"
#include <vector>

namespace SanmapGen {
namespace MaskTest {
namespace {

constexpr int   rampMapSize     = 8;
constexpr float rampRisePerCell = 1.0f / 128.0f;   // x terrainMaxHeight 128 = 1.0 rise per cell

// Runs the Cpu path over a pure x-ramp at one cell world-size and returns the baked slope field.
// A ramp has the same finite difference at every vertex, interior and one-sided edge alike.
Data::FloatField BakeRampSlope(float worldUnitsPerCell) {
    Params::Geometry geometry;
    geometry.mapSize           = rampMapSize;
    geometry.terrainMaxHeight  = 128.0f;
    geometry.worldUnitsPerCell = worldUnitsPerCell;
    const int vertexSize = geometry.VertexSize();
    Data::MapFields fields;
    fields.Resize(vertexSize);
    for (int y = 0; y < vertexSize; ++y)
        for (int x = 0; x < vertexSize; ++x)
            fields.heightfield.Set(x, y, static_cast<float>(x) * rampRisePerCell);
    FillTestMaterialProportions(fields, vertexSize);
    const std::vector<Params::Stratum> strata(Data::MapFields::stratumCount);
    const std::vector<Data::StratumArt> stratumArt = NoStratumArt();
    const Params::SlopeDefaults slopeDefaults;
    Proc::MaskStage stage(geometry, strata, stratumArt, fields, slopeDefaults);
    stage.RunOnCpu();
    return fields.slope;
}

void CheckEveryCell(const Data::FloatField& slope, float expectedGradient, const char* label) {
    bool bAllMatch = slope.CellCount() > 0;
    for (std::size_t index = 0; index < slope.CellCount(); ++index)
        if (std::fabs(slope.Data()[index] - expectedGradient) > 1e-5f) bAllMatch = false;
    Check(bAllMatch, label);
}

// 1. The pinned unit is rise per WORLD unit: one unit of rise over a two-unit-wide cell is half
// the gradient of the same rise over a one-unit cell. 0 is the validate-then-default case.
void CheckSlopeScalesWithCellWorldSize() {
    CheckEveryCell(BakeRampSlope(1.0f), 1.0f,
                   "worldUnitsPerCell 1: 1.0 rise across a 1.0 cell = gradient 1.0");
    CheckEveryCell(BakeRampSlope(2.0f), 0.5f,
                   "worldUnitsPerCell 2: the same terrain is exactly half as steep per world unit");
    CheckEveryCell(BakeRampSlope(0.25f), 4.0f,
                   "worldUnitsPerCell 0.25: four times as steep per world unit");
    CheckEveryCell(BakeRampSlope(0.0f), 1.0f,
                   "a non-positive cell world-size degrades to 1 instead of dividing by zero");
}

// 2. It is a stage input like any other, so the dirty-hash conductor must see it move.
void CheckWorldUnitsPerCellDirtiesTheStage() {
    Params::Geometry geometry;
    geometry.mapSize = rampMapSize;
    Data::MapFields fields;
    fields.Resize(geometry.VertexSize());
    const std::vector<Params::Stratum> strata(Data::MapFields::stratumCount);
    const std::vector<Data::StratumArt> stratumArt = NoStratumArt();
    const Params::SlopeDefaults slopeDefaults;
    Proc::MaskStage stage(geometry, strata, stratumArt, fields, slopeDefaults);
    const std::size_t hashBeforeChange = stage.ComputeParameterHash();
    geometry.worldUnitsPerCell = 2.0f;
    Check(stage.ComputeParameterHash() != hashBeforeChange,
          "changing worldUnitsPerCell dirties the mask stage");
}

// The placement fixture's cone drops 0.5 of normalized height over 18 cells; at terrainMaxHeight
// 128 that is a gradient of 3.5556 (74.3 degrees) per world unit at worldUnitsPerCell 1, and
// exactly half of it (1.7778 = 60.6 degrees) at 2. A 65-degree rule therefore rejects the whole
// flank on the fine scale and accepts it on the coarse one — the gate really reads the scale.
constexpr float coneFlankRuleDegrees = 65.0f;
constexpr float coneFlankHeightLow   = 0.55f;   // the flank annulus only: no apex, no plain
constexpr float coneFlankHeightHigh  = 0.95f;

Params::MapRecipe MakeConeFlankRecipe(float worldUnitsPerCell) {
    Params::MapRecipe recipe;
    recipe.geometry.mapSize           = PlacementTest::mapSize;
    recipe.geometry.seed              = 4242u;
    recipe.geometry.terrainMaxHeight  = PlacementTest::terrainMaxHeight;
    recipe.geometry.worldUnitsPerCell = worldUnitsPerCell;
    // STEP16_SymmetryGlobalSettings_IO audit: the default `globalSymmetryMask` changed from None
    // to RotateHalfTurn (ARCH-ratified). A whole symmetry orbit is accepted or rejected together
    // (Placement_Accept_PROC.cpp), and the cone flank here sits off-center, so its point-reflected
    // mirror mostly falls outside the gated flank annulus — a non-None mask would collapse
    // `coarseResults.props.Count()` well under this test's `> 20` bound instead of merely doubling
    // it. This test validates the world-scale gate agreement, not symmetry; pinned explicitly.
    recipe.globalSymmetryMask = Params::SymmetryAxis::None;
    Params::PropRule rule;
    rule.density         = 0.9f;
    rule.spacingMinimum  = 2.0f;
    rule.mapEdgePadding  = 4;
    rule.minHeight       = coneFlankHeightLow;
    rule.maxHeight       = coneFlankHeightHigh;
    rule.maxSlope        = coneFlankRuleDegrees;
    rule.transform       = PlacementTest::MakeTransform("edbm014", 1.0f, 1.0f);
    recipe.propRules.push_back(rule);
    return recipe;
}

// 3. Integration: Mask bakes the slope and Placement gates on it, both off the one value.
void CheckPlacementAgreesWithTheBakedScale() {
    Data::MapFields fineFields, coarseFields;
    PlacementTest::BuildTestFields(fineFields, 1.0f);
    PlacementTest::BuildTestFields(coarseFields, 2.0f);
    const Params::MapRecipe fineRecipe   = MakeConeFlankRecipe(1.0f);
    const Params::MapRecipe coarseRecipe = MakeConeFlankRecipe(2.0f);
    Data::PlacementResults fineResults, coarseResults;
    Proc::PlacementStage fineStage(fineRecipe, fineFields, fineResults);
    Proc::PlacementStage coarseStage(coarseRecipe, coarseFields, coarseResults);
    fineStage.Run();
    coarseStage.Run();

    Check(fineResults.props.Count() == 0,
          "worldUnitsPerCell 1: the 74-degree cone flank fails a 65-degree rule");
    Check(coarseResults.props.Count() > 20,
          "worldUnitsPerCell 2: the same flank is 60 degrees and scatters");
    Check(PlacementTest::AllWithinGates(coarseResults.props, coarseFields, coneFlankHeightLow,
                                        coneFlankHeightHigh, coneFlankRuleDegrees, 4, 2.0f),
          "emitted world positions map back onto cells that pass the scaled slope gate");
    Check(!PlacementTest::AllWithinGates(coarseResults.props, coarseFields, coneFlankHeightLow,
                                         coneFlankHeightHigh, coneFlankRuleDegrees, 4, 1.0f),
          "reading those positions at any other cell scale misses the gated cells");
}

} // namespace

void RunWorldScaleTests() {
    CheckSlopeScalesWithCellWorldSize();
    CheckWorldUnitsPerCellDirtiesTheStage();
    CheckPlacementAgreesWithTheBakedScale();
}

} // namespace MaskTest
} // namespace SanmapGen
