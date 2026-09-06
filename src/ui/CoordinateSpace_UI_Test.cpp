// CoordinateSpace_UI_Test.cpp — acceptance test for CoordinateSpace_UI.h's six conversions.
// Own binary (this file's own main()), mirroring every other stand-alone UI acceptance test in
// this directory. Runs the Cpu twin of PreviewComposite (PreviewTestScene_UI.h's shared fixture),
// so it needs no GL context.
#include "CoordinateSpace_UI.h"
#include "PreviewComposite_TestScene_UI.h"

using namespace SanmapGen;

namespace {

void check(bool bCondition, const char* label) { Ui::CheckPreviewExpectation(bCondition, label); }

bool NearlyEqual(float value, float expected, float tolerance = 0.01f) {
    const float difference = value - expected;
    return difference < tolerance && difference > -tolerance;
}

// --- WorldToGrid/GridToWorld: pure math, no view/composite needed. -----------------------------

void CheckGridRoundTripAlwaysLandsOnACellCenter() {
    // Cell size 10 (the "worldUnitsPerCell = 10" game the work-order's own header names).
    const Ui::GridPoint gridA = Ui::WorldToGrid(10.0f, Ui::WorldPoint{25.0f, -5.0f});
    check(NearlyEqual(gridA.gridX, 2.0f) && NearlyEqual(gridA.gridZ, -1.0f),
          "WorldToGrid floors toward the owning cell, including a negative coordinate");
    const Ui::WorldPoint centerA = Ui::GridToWorld(10.0f, gridA);
    check(NearlyEqual(centerA.worldX, 25.0f) && NearlyEqual(centerA.worldZ, -5.0f),
          "GridToWorld answers the CENTER of the cell (25/-5 sit exactly mid-cell already)");

    // An off-center world position still resolves to that same cell's center, not its own value.
    const Ui::WorldPoint snappedOffCenter =
        Ui::GridToWorld(10.0f, Ui::WorldToGrid(10.0f, Ui::WorldPoint{21.0f, -9.9f}));
    check(NearlyEqual(snappedOffCenter.worldX, 25.0f) && NearlyEqual(snappedOffCenter.worldZ, -5.0f),
          "an off-center position snaps to its owning cell's exact center");
}

// A whole-number cell-size MULTIPLIER (Part 3 of the work-order) must produce evenly-spaced
// centers with no lopsided block at the origin — the exact defect the work-order's own text calls
// out as the first draft's bug (mixing round() and floor()).
void CheckMultiCellSnapIsEvenlySpacedFromOrigin() {
    const float cellSize = 20.0f;   // a 2-cell multiplier over a 10-world-unit terrain cell
    const float expectedCenters[] = {-10.0f, 10.0f, 30.0f, 50.0f};
    const float sampleWorldPositions[] = {-15.0f, 1.0f, 20.001f, 49.9f};
    for (int i = 0; i < 4; ++i) {
        const Ui::WorldPoint snapped =
            Ui::GridToWorld(cellSize, Ui::WorldToGrid(cellSize, Ui::WorldPoint{sampleWorldPositions[i], 0.0f}));
        check(NearlyEqual(snapped.worldX, expectedCenters[i]),
              "a multi-cell snap block is evenly spaced from the origin, not lopsided");
    }
}

void CheckNonPositiveCellSizeGuardsWithoutCrashing() {
    const Ui::GridPoint grid = Ui::WorldToGrid(0.0f, Ui::WorldPoint{12.5f, -3.0f});
    check(NearlyEqual(grid.gridX, 12.5f) && NearlyEqual(grid.gridZ, -3.0f),
          "a non-positive cell size answers the raw input unchanged, never a divide by zero");
    const Ui::WorldPoint world = Ui::GridToWorld(-1.0f, Ui::GridPoint{4.0f, 4.0f});
    check(NearlyEqual(world.worldX, 4.0f) && NearlyEqual(world.worldZ, 4.0f),
          "GridToWorld guards the same non-positive cell size the same way");
}

// --- ScreenToWorld/WorldToScreen: needs a real (baked) view + composite. -----------------------

void CheckScreenWorldRoundTrip() {
    Ui::PreviewTestScene scene;
    Ui::BuildPreviewTestScene(scene);   // mapSize=4 -> a 5x5 field, worldUnitsPerCell=1 (Geometry default)
    Ui::PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.areas, scene.fields,
                                   scene.instances, scene.entityIdentifiers);
    Ui::ConfigurePreviewSettings(composite.Settings());   // previewResolution = 4
    composite.Compose();

    Ui::MapCanvasView view;
    view.SetPreviewResolution(composite.Resolution());
    view.SetRegionSide(256.0f);   // 64 screen pixels per preview pixel at zoom 1

    const Ui::WorldPoint roundTripped =
        Ui::ScreenToWorld(view, composite, Ui::WorldToScreen(view, composite, Ui::WorldPoint{2.0f, 2.0f}));
    check(NearlyEqual(roundTripped.worldX, 2.0f) && NearlyEqual(roundTripped.worldZ, 2.0f),
          "WorldToScreen -> ScreenToWorld round-trips a world position");

    // The continuity fix itself: two screen points ONE screen pixel apart must map to two
    // DIFFERENT world positions — the pre-fix code floored to an integer preview-texel first, so a
    // whole span of nearby cursor positions (many screen pixels, at high zoom) collapsed onto the
    // exact same world position (the "frozen frame" bug). At 64 screen pixels per preview pixel,
    // moving one screen pixel is a small FRACTION of one preview pixel, so ANY floored composition
    // maps it to the same texel every time; the continuous composition must not.
    const Ui::WorldPoint worldAtOnePixel  = Ui::ScreenToWorld(view, composite, Ui::ScreenPoint{100.0f, 100.0f});
    const Ui::WorldPoint worldAtNextPixel = Ui::ScreenToWorld(view, composite, Ui::ScreenPoint{101.0f, 100.0f});
    check(worldAtOnePixel.worldX != worldAtNextPixel.worldX,
          "a one-screen-pixel cursor move changes the resolved world position continuously, never freezes");
    check(NearlyEqual(worldAtNextPixel.worldX - worldAtOnePixel.worldX, 1.0f / 64.0f, 0.001f),
          "the moved amount matches that pixel's actual world-unit span, not a whole-texel jump");
}

void CheckScreenGridComposition() {
    Ui::PreviewTestScene scene;
    Ui::BuildPreviewTestScene(scene);
    Ui::PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.areas, scene.fields,
                                   scene.instances, scene.entityIdentifiers);
    Ui::ConfigurePreviewSettings(composite.Settings());
    composite.Compose();

    Ui::MapCanvasView view;
    view.SetPreviewResolution(composite.Resolution());
    view.SetRegionSide(256.0f);

    const Ui::ScreenPoint screen{130.0f, 130.0f};
    const Ui::GridPoint viaComposition = Ui::ScreenToGrid(view, composite, 1.0f, screen);
    const Ui::GridPoint viaManualSteps = Ui::WorldToGrid(1.0f, Ui::ScreenToWorld(view, composite, screen));
    check(NearlyEqual(viaComposition.gridX, viaManualSteps.gridX)
       && NearlyEqual(viaComposition.gridZ, viaManualSteps.gridZ),
          "ScreenToGrid is exactly the ScreenToWorld+WorldToGrid composition");

    const Ui::ScreenPoint backViaComposition = Ui::GridToScreen(view, composite, 1.0f, viaComposition);
    const Ui::ScreenPoint backViaManualSteps =
        Ui::WorldToScreen(view, composite, Ui::GridToWorld(1.0f, viaManualSteps));
    check(NearlyEqual(backViaComposition.screenX, backViaManualSteps.screenX)
       && NearlyEqual(backViaComposition.screenY, backViaManualSteps.screenY),
          "GridToScreen is exactly the GridToWorld+WorldToScreen composition");
}

} // namespace

int main() {
    CheckGridRoundTripAlwaysLandsOnACellCenter();
    CheckMultiCellSnapIsEvenlySpacedFromOrigin();
    CheckNonPositiveCellSizeGuardsWithoutCrashing();
    CheckScreenWorldRoundTrip();
    CheckScreenGridComposition();
    return Ui::previewTestFailureCount == 0 ? 0 : 1;
}
