// AreaDragGesture_UI_Test.cpp — pure-logic acceptance for AreaDragGesture_UI.h/.cpp (ARCH §21.8).
// Matches AreasTab_UI_Test.cpp's own style/rigor for every function that needs no screen-space
// projection at all (BeginAreaDragGesture, UpdateAreaDragGesture, EndAreaDragGesture,
// IsWorldPointInsideArea — every one of these takes plain world-space floats, no
// PreviewComposite/MapCanvasView argument). HitTestAreaHandles is the one function that needs a
// PreviewComposite/MapCanvasView to project through: built via PreviewComposite_TestScene_UI.h's
// BuildPreviewTestScene/ConfigurePreviewSettings and composite.ComposeOnCpu() — the SAME no-GL,
// no-imgui-frame technique MapCanvas_Picking_UI_Test.cpp already established for testing coordinate
// math headlessly. No ImGui::NewFrame()/Draw() call anywhere in this file.
#include "AreaDragGesture_UI.h"
#include "MapCanvasView_UI.h"
#include "PreviewComposite_TestScene_UI.h"
#include <cstdio>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

Params::MapArea MakeRect(float originX, float originZ, float width, float length) {
    Params::MapArea area;
    area.originX = originX; area.originZ = originZ; area.width = width; area.length = length;
    return area;
}

constexpr int   kPreviewResolution = 64;
constexpr float kRegionSidePixels  = 256.0f;

// A composite/view configured so 1 world unit is several preview/region pixels — enough
// separation that kAreaHandleScreenRadiusPixels circles do not overlap (mirrors
// MapCanvas_Picking_UI_Test.cpp's own ComposeClickableScene fixture technique). `scene` is built
// BEFORE `composite` is constructed, matching MapCanvas_Picking_UI_Test.cpp's own ordering, since
// PreviewComposite's constructor only binds references — it does not itself require the scene to be
// pre-populated, but building it first keeps this file's ordering unsurprising to read.
void ComposeHitTestScene(PreviewTestScene& scene, PreviewComposite& composite, MapCanvasView& view) {
    BuildPreviewTestScene(scene);
    ConfigurePreviewSettings(composite.Settings());
    composite.Settings().previewResolution = kPreviewResolution;
    composite.ComposeOnCpu();
    view.SetPreviewResolution(kPreviewResolution);
    view.SetRegionSide(kRegionSidePixels);
}

RegionLocalPoint ToRegionLocal(const PreviewComposite& composite, const MapCanvasView& view,
                               float worldX, float worldZ) {
    const PreviewComposite::PreviewPixelPoint pixel = composite.WorldToPreviewPixel(worldX, worldZ);
    return view.ProjectPreviewPixelToRegionLocal(pixel.pixelX, pixel.pixelY);
}

void RunHitTestChecks() {
    PreviewTestScene scene;
    PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.areas, scene.fields,
                               scene.instances, scene.entityIdentifiers);
    MapCanvasView view;
    ComposeHitTestScene(scene, composite, view);

    const Params::MapArea area = MakeRect(0.0f, 0.0f, 100.0f, 100.0f);

    const RegionLocalPoint nwPoint = ToRegionLocal(composite, view, area.originX, area.originZ);
    Check(HitTestAreaHandles(area, composite, view, nwPoint.regionLocalX, nwPoint.regionLocalY)
              == AreaHandle_UI::NW,
          "a region-local point exactly at the projected NW corner resolves NW, not Center/N/W");

    const RegionLocalPoint nMidPoint =
        ToRegionLocal(composite, view, area.originX + area.width * 0.5f, area.originZ);
    Check(HitTestAreaHandles(area, composite, view, nMidPoint.regionLocalX, nMidPoint.regionLocalY)
              == AreaHandle_UI::N,
          "a region-local point exactly at the projected N-edge midpoint resolves N");

    const RegionLocalPoint centerPoint =
        ToRegionLocal(composite, view, area.originX + area.width * 0.5f, area.originZ + area.length * 0.5f);
    Check(HitTestAreaHandles(area, composite, view, centerPoint.regionLocalX, centerPoint.regionLocalY)
              == AreaHandle_UI::Center,
          "a region-local point well inside the body but outside every handle radius resolves Center");

    const RegionLocalPoint farPoint =
        ToRegionLocal(composite, view, area.originX - 10000.0f, area.originZ - 10000.0f);
    Check(HitTestAreaHandles(area, composite, view, farPoint.regionLocalX, farPoint.regionLocalY)
              == AreaHandle_UI::None,
          "a region-local point well outside the rectangle entirely resolves None");

    Check(IsWorldPointInsideArea(area, 50.0f, 50.0f), "a point inside the rect is true");
    Check(IsWorldPointInsideArea(area, 0.0f, 0.0f), "a point on the boundary is true (inclusive)");
    Check(IsWorldPointInsideArea(area, 100.0f, 100.0f), "the far boundary is also true (inclusive)");
    Check(!IsWorldPointInsideArea(area, 150.0f, 50.0f), "a point outside the rect is false");
}

void RunMoveChecks() {
    std::vector<Params::MapArea> areas;
    areas.push_back(MakeRect(10.0f, 10.0f, 20.0f, 20.0f));

    AreaDragGestureState state;
    Check(BeginAreaDragGesture(state, areas, 0, AreaHandle_UI::Center, 5.0f, 5.0f),
          "Begin succeeds on a valid area index and a real handle");
    UpdateAreaDragGesture(state, areas, 8.0f, 9.0f, false, false);
    Check(areas[0].originX == 13.0f && areas[0].originZ == 14.0f,
          "Center drag translates the origin by the exact world delta");
    Check(areas[0].width == 20.0f && areas[0].length == 20.0f, "Center drag never changes width/length");

    AreaDragGestureState outOfRangeState;
    Check(!BeginAreaDragGesture(outOfRangeState, areas, static_cast<int>(areas.size()), AreaHandle_UI::Center,
                                0.0f, 0.0f),
          "Begin refuses an out-of-range areaIndex");
    Check(!outOfRangeState.bActive, "and leaves state.bActive false");

    AreaDragGestureState noHandleState;
    Check(!BeginAreaDragGesture(noHandleState, areas, 0, AreaHandle_UI::None, 0.0f, 0.0f),
          "Begin refuses handle == None");
}

void RunResizeChecks() {
    {
        std::vector<Params::MapArea> areas;
        areas.push_back(MakeRect(0.0f, 0.0f, 20.0f, 10.0f));
        AreaDragGestureState state;
        BeginAreaDragGesture(state, areas, 0, AreaHandle_UI::E, 0.0f, 0.0f);
        UpdateAreaDragGesture(state, areas, 5.0f, 0.0f, false, false);
        Check(areas[0].width == 25.0f, "E handle grows width by the drag delta");
        Check(areas[0].length == 10.0f, "E handle never touches length");
        Check(areas[0].originX == 0.0f && areas[0].originZ == 0.0f,
              "E handle keeps the West edge (origin) fixed");
    }
    {
        std::vector<Params::MapArea> areas;
        areas.push_back(MakeRect(0.0f, 0.0f, 20.0f, 10.0f));
        AreaDragGestureState state;
        BeginAreaDragGesture(state, areas, 0, AreaHandle_UI::W, 0.0f, 0.0f);
        UpdateAreaDragGesture(state, areas, -5.0f, 0.0f, false, false);
        Check(areas[0].width == 25.0f, "W handle grows width when dragged further west");
        Check(areas[0].originX == -5.0f, "W handle's origin decreases by exactly the drag magnitude");
    }
    {
        std::vector<Params::MapArea> areas;
        areas.push_back(MakeRect(0.0f, 0.0f, 20.0f, 10.0f));
        AreaDragGestureState state;
        BeginAreaDragGesture(state, areas, 0, AreaHandle_UI::N, 0.0f, 0.0f);
        UpdateAreaDragGesture(state, areas, 0.0f, -4.0f, false, false);
        Check(areas[0].length == 14.0f, "N handle grows length when dragged north");
        Check(areas[0].originZ == -4.0f, "N handle's origin decreases (South edge fixed)");
        Check(areas[0].width == 20.0f && areas[0].originX == 0.0f, "N handle never touches the X axis");
    }
    {
        std::vector<Params::MapArea> areas;
        areas.push_back(MakeRect(0.0f, 0.0f, 20.0f, 10.0f));
        AreaDragGestureState state;
        BeginAreaDragGesture(state, areas, 0, AreaHandle_UI::NE, 0.0f, 0.0f);
        UpdateAreaDragGesture(state, areas, 6.0f, -3.0f, false, false);
        Check(areas[0].width == 26.0f, "NE handle grows width via the East edge");
        Check(areas[0].length == 13.0f, "NE handle grows length via the North edge");
        Check(areas[0].originX == 0.0f, "NE handle leaves originX unchanged");
        Check(areas[0].originZ == -3.0f, "NE handle decreases originZ by the North delta");
    }
}

void RunCtrlCenterResizeChecks() {
    std::vector<Params::MapArea> areas;
    areas.push_back(MakeRect(0.0f, 0.0f, 20.0f, 10.0f));   // center X = 10
    AreaDragGestureState state;
    BeginAreaDragGesture(state, areas, 0, AreaHandle_UI::E, 0.0f, 0.0f);
    UpdateAreaDragGesture(state, areas, 4.0f, 0.0f, false, true);
    Check(areas[0].width == 28.0f, "Ctrl doubles the extent delta (28, not 24)");
    Check(areas[0].originX == -4.0f, "the rect stays centered on X=10 (originX == 10 - 28/2 == -4)");
}

void RunShiftAspectLockChecks() {
    {
        std::vector<Params::MapArea> areas;
        areas.push_back(MakeRect(0.0f, 0.0f, 20.0f, 10.0f));   // aspect 2.0
        AreaDragGestureState state;
        BeginAreaDragGesture(state, areas, 0, AreaHandle_UI::E, 0.0f, 0.0f);
        UpdateAreaDragGesture(state, areas, 10.0f, 0.0f, true, false);
        Check(areas[0].width == 30.0f, "E handle with Shift: width grows by the raw delta");
        Check(areas[0].length == 15.0f, "length is locked to the frozen 2.0 aspect from the new width");
    }
    {
        std::vector<Params::MapArea> areas;
        areas.push_back(MakeRect(0.0f, 0.0f, 20.0f, 10.0f));
        AreaDragGestureState state;
        BeginAreaDragGesture(state, areas, 0, AreaHandle_UI::N, 0.0f, 0.0f);
        UpdateAreaDragGesture(state, areas, 0.0f, -5.0f, true, false);
        Check(areas[0].length == 15.0f, "N handle with Shift: length grows by the raw delta");
        Check(areas[0].width == 30.0f, "width is derived from the new length, consistent with the frozen aspect");
    }
    {
        std::vector<Params::MapArea> areas;
        areas.push_back(MakeRect(0.0f, 0.0f, 20.0f, 10.0f));
        AreaDragGestureState state;
        BeginAreaDragGesture(state, areas, 0, AreaHandle_UI::NE, 0.0f, 0.0f);
        UpdateAreaDragGesture(state, areas, 10.0f, -1.0f, true, false);
        Check(areas[0].width == 30.0f, "NE corner with Shift: the larger-magnitude delta (X) leads");
        Check(areas[0].length == 15.0f, "length is derived from the leading width, not the reverse");
    }
}

void RunMinimumFloorChecks() {
    {
        std::vector<Params::MapArea> areas;
        areas.push_back(MakeRect(0.0f, 0.0f, 5.0f, 5.0f));
        AreaDragGestureState state;
        BeginAreaDragGesture(state, areas, 0, AreaHandle_UI::W, 0.0f, 0.0f);
        UpdateAreaDragGesture(state, areas, 20.0f, 0.0f, false, false);
        Check(areas[0].width == kAreaMinimumExtentWorldUnits, "width floors at exactly the minimum, never below");
        Check(areas[0].originX == 5.0f - kAreaMinimumExtentWorldUnits,
              "originX is computed from the FLOORED width (West-edge-fixed, post-floor)");
    }
    {
        std::vector<Params::MapArea> areas;
        areas.push_back(MakeRect(0.0f, 0.0f, 5.0f, 5.0f));
        AreaDragGestureState state;
        BeginAreaDragGesture(state, areas, 0, AreaHandle_UI::N, 0.0f, 0.0f);
        UpdateAreaDragGesture(state, areas, 0.0f, 20.0f, false, false);
        Check(areas[0].length == kAreaMinimumExtentWorldUnits,
              "the floor applies per-axis independently on the length axis too");
    }
}

void RunEndChecks() {
    std::vector<Params::MapArea> areas;
    areas.push_back(MakeRect(0.0f, 0.0f, 20.0f, 10.0f));
    AreaDragGestureState state;
    BeginAreaDragGesture(state, areas, 0, AreaHandle_UI::Center, 0.0f, 0.0f);
    EndAreaDragGesture(state);
    Check(!state.bActive, "EndAreaDragGesture clears bActive");
    Check(state.areaIndex == -1, "and resets areaIndex to its default-constructed value");
    Check(state.handle == AreaHandle_UI::None, "and resets handle to its default-constructed value");
    Check(areas[0].originX == 0.0f && areas[0].originZ == 0.0f
              && areas[0].width == 20.0f && areas[0].length == 10.0f,
          "a Begin+End with no Update in between leaves the area vector completely untouched");
}

} // namespace

int main() {
    RunHitTestChecks();
    RunMoveChecks();
    RunResizeChecks();
    RunCtrlCenterResizeChecks();
    RunShiftAspectLockChecks();
    RunMinimumFloorChecks();
    RunEndChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
