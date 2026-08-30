// MapCanvas_AreaOverlapHitTest_UI_Test.cpp — STEP227/ARCH §14.19 acceptance: TryBeginAreaDrag's
// step 2 body hit-test (MapCanvas_AreaDragDispatch_UI.cpp) now resolves forward iteration, FIRST
// unlocked match wins, early exit — ascending array index is Z-descending, so the first hit IS the
// topmost area. A small area at index 0 fully inside a large area at index 1: clicking inside the
// overlap must select the SMALL area, not the large one (the exact inversion this ticket ships).
// GL-backed (mirrors MapCanvas_AreaAltCenterResizeModifier_UI_Test.cpp's own technique exactly)
// because TryBeginAreaDrag is MapCanvas-private — the only way to exercise it is a real
// MapCanvas::Draw() press/release cycle. One translation unit of the MapCanvas_UI_Test binary.
#include "MapCanvas_UI.h"
#include "PreviewComposite_TestScene_UI.h"
#include <imgui.h>

namespace SanmapGen {
namespace Ui {
namespace {

void check(bool bCondition, const char* label) { CheckPreviewExpectation(bCondition, label); }

constexpr int   kPreviewResolution = 64;
constexpr float kRegionSidePixels  = 256.0f;
constexpr unsigned long long kFontAtlasIdentifier = 0xF0000006ull;

void BeginHeadlessFrame() {
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = 1.0f / 60.0f;
    unsigned char* atlasPixels = nullptr; int atlasWidth = 0, atlasHeight = 0;
    io.Fonts->GetTexDataAsRGBA32(&atlasPixels, &atlasWidth, &atlasHeight);
    io.Fonts->SetTexID(static_cast<ImTextureID>(kFontAtlasIdentifier));
    ImGui::NewFrame();
}

ImVec2 DrawOneFrame(MapCanvas& canvas) {
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(600.0f, 600.0f));
    ImGui::Begin("AreaOverlapHitTestWindow");
    const ImVec2 regionOrigin = ImGui::GetCursorScreenPos();
    canvas.Draw("mapCanvas", kRegionSidePixels);
    ImGui::End();
    ImGui::Render();
    return regionOrigin;
}

ImVec2 ScreenPositionForWorld(MapCanvas& canvas, const PreviewComposite& composite,
                              float worldX, float worldZ) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(-100.0f, -100.0f);
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame();
    const ImVec2 regionOrigin = DrawOneFrame(canvas);
    const PreviewComposite::PreviewPixelPoint previewPixel = composite.WorldToPreviewPixel(worldX, worldZ);
    const RegionLocalPoint regionLocal =
        canvas.View().ProjectPreviewPixelToRegionLocal(previewPixel.pixelX, previewPixel.pixelY);
    return ImVec2(regionOrigin.x + regionLocal.regionLocalX, regionOrigin.y + regionLocal.regionLocalY);
}

} // namespace

void RunMapCanvasAreaOverlapHitTestChecks(Sys::GpuResourceManager& manager) {
    PreviewTestScene scene;
    BuildPreviewTestScene(scene);
    PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.areas, scene.fields,
                               scene.instances, scene.entityIdentifiers);
    ConfigurePreviewSettings(composite.Settings());
    composite.Settings().previewResolution = kPreviewResolution;
    composite.SetGpuResourceManager(&manager);
    composite.Compose();

    // Small at index 0, fully inside Large at index 1 — the exact inverted-Z scenario the ticket's
    // own acceptance test names.
    std::vector<Params::MapArea> areas;
    Params::MapArea small;
    small.name = "Small"; small.originX = 1.0f; small.originZ = 1.0f; small.width = 1.0f; small.length = 1.0f;
    Params::MapArea large;
    large.name = "Large"; large.originX = 0.0f; large.originZ = 0.0f; large.width = 4.0f; large.length = 4.0f;
    areas.push_back(small);
    areas.push_back(large);
    std::vector<AreaColorEntry> areaColors;
    std::vector<AreaLockEntry>  areaLocks;
    // Both unlocked — a locked area is excluded from the step 2 scan entirely (STEP212), which
    // would make this test vacuous.
    ResolveAreaLocked(areaLocks, small.name, /*bDefaultLocked=*/false);
    ResolveAreaLocked(areaLocks, large.name, /*bDefaultLocked=*/false);
    int selectedAreaIndex = -1;   // no prior selection — step 1's own handle test is skipped

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(600.0f, 600.0f);
    io.IniFilename = nullptr;

    MapCanvas canvas;
    canvas.SetPreviewTexture(&manager, composite.CompositeTexture(), composite.Resolution());
    canvas.SetPreviewComposite(&composite);
    canvas.View().SetRegionSide(kRegionSidePixels);
    ApplicationPanel activePanel = ApplicationPanel::Areas;
    canvas.SetActivePanelSource(&activePanel);
    canvas.SetManualAreaDragSource(&areas, &areaColors, &areaLocks, &selectedAreaIndex);
    canvas.SetAreaCompositeRefreshCallback([] {});

    // A point inside BOTH rectangles, far from either area's own 8 resize handles (Small's own
    // handles sit on its 1x1 boundary at world 1/1..2/2; (1.5, 1.5) is Small's own dead center).
    const ImVec2 overlapPosition = ScreenPositionForWorld(canvas, composite, 1.5f, 1.5f);
    io.AddMousePosEvent(overlapPosition.x, overlapPosition.y);
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    io.AddMouseButtonEvent(0, true);
    BeginHeadlessFrame(); DrawOneFrame(canvas);

    check(selectedAreaIndex == 0,
          "ARCH §14.19: clicking inside the overlap of a small-at-index-0/large-at-index-1 pair "
          "selects the SMALL area (index 0), the first unlocked hit scanning forward — NOT the "
          "large one the old last-match-wins rule would have picked");

    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame(); DrawOneFrame(canvas);

    ImGui::DestroyContext();
}

} // namespace Ui
} // namespace SanmapGen
