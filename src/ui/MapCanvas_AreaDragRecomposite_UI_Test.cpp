// MapCanvas_AreaDragRecomposite_UI_Test.cpp — ARCH §14.18 Piece C acceptance (supersedes the
// retired MapCanvas_AreaDragSuppression_UI_Test.cpp — its entire premise, "exactly two
// recomposites per gesture," is now the opposite of law): TryBeginAreaDrag fires ZERO refresh
// requests (a begin changes no composite input — selection is not a composite input);
// ContinueAreaDrag fires exactly one refresh per frame the dragged rectangle actually moved and
// ZERO on a held-but-motionless frame; EndAreaDrag's refresh is unconditional, always fires
// exactly once, regardless of throttle state or whether the final frame moved; CreateAreaFromDrag's
// single request is unchanged. GL-backed (mirrors the retired test's own technique exactly)
// because TryBeginAreaDrag/ContinueAreaDrag/EndAreaDrag/CreateAreaFromDrag are MapCanvas-private —
// the only way to exercise them is through a real MapCanvas::Draw() press/drag/release sequence.
// The composite's own real Compose() cost, measured on this test's tiny fixture scene, stays well
// under kAreaRecompositeCostBudgetMillis for the two-move sequence this file drives, so the
// watchdog never engages here regardless of the exact (machine-dependent) measured value — see the
// per-frame walkthrough in this ticket's own text for why that is true independent of the number.
// AreaRecompositeThrottle_UI_Test.cpp is the throttle's own dedicated, fully-deterministic,
// GPU-free coverage of the watchdog's arithmetic in isolation.
// STEP227/ARCH §14.19 — Case 3 additionally proves CreateAreaFromDrag's own insertion now routes
// through Params::InsertMapAreaSortedBySize: a freshly drag-created area lands by SIZE rank, not
// unconditionally appended to the back.
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
    ImGui::Begin("AreaDragRecompositeTestWindow");
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

void RunMapCanvasAreaDragRecompositeChecks(Sys::GpuResourceManager& manager) {
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
    std::vector<AreaLockEntry>  areaLocks;
    // STEP212's per-area lock table defaults a first-touch name to LOCKED — pre-seed "Existing"
    // UNLOCKED explicitly, mirroring the retired test's own established precedent.
    ResolveAreaLocked(areaLocks, existingArea.name, /*bDefaultLocked=*/false);
    int  selectedAreaIndex = -1;
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
    canvas.SetManualAreaDragSource(&areas, &areaColors, &areaLocks, &selectedAreaIndex);
    canvas.SetAreaCompositeRefreshCallback([&] { ++refreshCount; });

    // --- Case 1: create-by-drag on empty canvas space fires exactly ONE refresh (unchanged) ---
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

    // --- Case 2: a body-move on the pre-existing area fires ZERO refreshes at begin, ONE per
    // moved frame, ZERO on a held-but-motionless frame, and exactly ONE unconditional refresh at
    // release ---
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
    check(refreshCount == refreshCountBeforeMove,
          "TryBeginAreaDrag fires NO refresh request — a begin changes no composite input");

    // A moving frame: the rectangle actually changes (originX/originZ), so exactly one refresh
    // fires.
    io.AddMousePosEvent(bodyPressPosition.x + 20.0f, bodyPressPosition.y);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    check(refreshCount == refreshCountBeforeMove + 1,
          "ContinueAreaDrag requests exactly one recomposite on a frame the rectangle moved");

    // A held-but-motionless frame (mouse position unchanged since the last frame): zero refreshes.
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    check(refreshCount == refreshCountBeforeMove + 1,
          "ContinueAreaDrag requests ZERO recomposites on a held-but-motionless frame");

    // Another moving frame: one more refresh.
    io.AddMousePosEvent(bodyPressPosition.x + 30.0f, bodyPressPosition.y);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    check(refreshCount == refreshCountBeforeMove + 2,
          "a second moved frame requests a second recomposite");

    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    check(refreshCount == refreshCountBeforeMove + 3,
          "EndAreaDrag's refresh is unconditional and always fires, exactly once, at release");

    // --- Case 3 (STEP227/ARCH §14.19): CreateAreaFromDrag now inserts through
    // Params::InsertMapAreaSortedBySize, not an unconditional push_back-to-the-back. A pre-seeded
    // large area (size 4) plus a freshly drag-created one (clamped to the 1x1 minimum extent, size
    // 1) must land the SMALL new area BEFORE the large pre-existing one. ---
    areas.clear();
    Params::MapArea bigArea;
    bigArea.name = "Big"; bigArea.originX = 0.0f; bigArea.originZ = 0.0f;
    bigArea.width = 2.0f; bigArea.length = 2.0f;   // size 4, occupies world (0,0)-(2,2)
    areas.push_back(bigArea);
    ResolveAreaLocked(areaLocks, bigArea.name, /*bDefaultLocked=*/false);
    selectedAreaIndex = -1;

    const ImVec2 emptySpotPressPosition = ScreenPositionForWorld(canvas, composite, 3.0f, 3.0f);
    io.AddMousePosEvent(emptySpotPressPosition.x, emptySpotPressPosition.y);
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    io.AddMouseButtonEvent(0, true);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    io.AddMousePosEvent(emptySpotPressPosition.x + 60.0f, emptySpotPressPosition.y + 60.0f);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame(); DrawOneFrame(canvas);

    check(areas.size() == 2u, "the second press-drag-release on empty canvas space creates a new area");
    check(areas[0].name != "Big" && areas[1].name == "Big",
          "CreateAreaFromDrag's own InsertMapAreaSortedBySize call lands the SMALL new area (clamped "
          "to the 1x1 minimum extent, size 1) BEFORE the pre-existing Big area (size 4) — not "
          "appended to the back the old push_back would have used");
    check(selectedAreaIndex == 0, "and selectedAreaIndex tracks the new area's ACTUAL landing index, "
                                  "not size()-1");

    ImGui::DestroyContext();
}

} // namespace Ui
} // namespace SanmapGen
