// MapCanvas_AreaDragSuppression_UI_Test.cpp — ARCH §14.17 item 11 acceptance: the Area canvas
// gesture's drag-performance rule — exactly two recomposite requests per drag/resize/move gesture
// (begin + end), never one per ContinueAreaDrag frame, driven through SetAreaCompositeRefreshCallback
// and the transient mapAreaSuppressedIndex slot SetManualAreaDragSource's fifth parameter injects.
// GL-backed (mirrors MapCanvas_ActivePanelGate_UI_Test.cpp's own technique exactly) because
// TryBeginAreaDrag/ContinueAreaDrag/EndAreaDrag/CreateAreaFromDrag are MapCanvas-private — the only
// way to exercise them from a test is through a real MapCanvas::Draw() press/drag/release sequence.
#include "MapCanvas_UI.h"
#include "PreviewComposite_TestScene_UI.h"
#include <imgui.h>

namespace SanmapGen {
namespace Ui {
namespace {

void check(bool bCondition, const char* label) { CheckPreviewExpectation(bCondition, label); }

constexpr int   kPreviewResolution = 64;
constexpr float kRegionSidePixels  = 256.0f;
constexpr unsigned long long kFontAtlasIdentifier = 0xF0000005ull;

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
    ImGui::Begin("AreaDragSuppressionTestWindow");
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

void RunMapCanvasAreaDragSuppressionChecks(Sys::GpuResourceManager& manager) {
    PreviewTestScene scene;
    BuildPreviewTestScene(scene);
    PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.areas, scene.fields,
                               scene.instances, scene.entityIdentifiers);
    ConfigurePreviewSettings(composite.Settings());
    composite.Settings().previewResolution = kPreviewResolution;
    composite.SetGpuResourceManager(&manager);
    composite.Compose();

    std::vector<Params::MapArea> areas;
    Params::MapArea existingArea;
    existingArea.name = "Existing";
    existingArea.originX = 1.0f; existingArea.originZ = 1.0f;
    existingArea.width = 1.0f;   existingArea.length = 1.0f;
    areas.push_back(existingArea);
    std::vector<AreaColorEntry> areaColors;
    // STEP212 — the retired tab-wide `bool bAreasLocked` is now a per-area lock table; the
    // pre-existing "Existing" area must be resolved UNLOCKED explicitly here (mirroring the two real
    // creation call sites' own `/*bDefaultLocked=*/false`) or this test's own body-move case (below)
    // would be silently refused by the new per-area gate.
    std::vector<AreaLockEntry> areaLocks;
    ResolveAreaLocked(areaLocks, existingArea.name, /*bDefaultLocked=*/false);
    int  selectedAreaIndex = -1;
    int  mapAreaSuppressedIndex = -1;
    int  refreshCount = 0;

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
    canvas.SetManualAreaDragSource(&areas, &areaColors, &areaLocks, &selectedAreaIndex,
                                   &mapAreaSuppressedIndex);
    canvas.SetAreaCompositeRefreshCallback([&] { ++refreshCount; });

    // --- Case 1: create-by-drag on empty canvas space fires exactly ONE refresh, no suppression ---
    const ImVec2 emptyPressPosition = ScreenPositionForWorld(canvas, composite, 3.2f, 3.2f);
    io.AddMousePosEvent(emptyPressPosition.x, emptyPressPosition.y);
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    io.AddMouseButtonEvent(0, true);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    io.AddMousePosEvent(emptyPressPosition.x + 60.0f, emptyPressPosition.y + 60.0f);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame(); DrawOneFrame(canvas);

    check(areas.size() == 2u, "a press-drag-release on empty canvas space creates a new area");
    check(refreshCount == 1, "create-by-drag requests exactly one recomposite");
    check(mapAreaSuppressedIndex == -1, "create-by-drag never touches the suppression slot");

    // --- Case 2: a body-move on the pre-existing area fires exactly TWO refreshes (begin + end),
    // suppressing that area's index for the WHOLE gesture, with ZERO extra refreshes while held ---
    selectedAreaIndex = 0;   // the pre-existing "Existing" area, index 0
    const int refreshCountBeforeMove = refreshCount;
    // Dead center of the 1x1 world rect — ~32 screen px from every 8px handle circle at this zoom,
    // so step 1's handle hit-test correctly misses and step 2's body/AABB test correctly hits.
    const ImVec2 bodyPressPosition = ScreenPositionForWorld(canvas, composite, 1.5f, 1.5f);
    io.AddMousePosEvent(bodyPressPosition.x, bodyPressPosition.y);
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    io.AddMouseButtonEvent(0, true);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    check(mapAreaSuppressedIndex == 0, "TryBeginAreaDrag suppresses the dragged area immediately");
    check(refreshCount == refreshCountBeforeMove + 1, "the FIRST of exactly two recomposites fires at press");

    io.AddMousePosEvent(bodyPressPosition.x + 20.0f, bodyPressPosition.y);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    io.AddMousePosEvent(bodyPressPosition.x + 30.0f, bodyPressPosition.y);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    check(mapAreaSuppressedIndex == 0, "the suppression stays set for the whole held drag");
    check(refreshCount == refreshCountBeforeMove + 1,
          "ContinueAreaDrag requests zero recomposites, no matter how many held frames run");

    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    check(mapAreaSuppressedIndex == -1, "EndAreaDrag clears the suppression slot");
    check(refreshCount == refreshCountBeforeMove + 2,
          "the SECOND of exactly two recomposites fires at release — net two per gesture, not one per frame");

    ImGui::DestroyContext();
}

} // namespace Ui
} // namespace SanmapGen
