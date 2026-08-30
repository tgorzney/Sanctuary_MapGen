// MapCanvas_AreaOverlayPanelGate_UI_Test.cpp — STEP228 acceptance: DrawAreaOverlayPass's own new
// AreaGestureEligible() early-return gate (MapCanvas_AreaDraw_UI.cpp) suppresses its chrome — the
// selected area's border+8 handles AND the hover cursor-shape feedback — the moment a non-Areas
// panel is active, even with a perfectly valid, unchanged `selectedAreaIndex` (the exact stale-
// selection scenario the human's own bug report described). GL-backed (mirrors
// MapCanvas_AreaOverlapHitTest_UI_Test.cpp's own technique exactly) because DrawAreaOverlayPass is
// MapCanvas-private — the only way to exercise it is a real MapCanvas::Draw() frame. One
// translation unit of the MapCanvas_UI_Test binary.
#include "MapCanvas_UI.h"
#include "PreviewComposite_TestScene_UI.h"
#include <cstddef>
#include <imgui.h>

namespace SanmapGen {
namespace Ui {
namespace {

void check(bool bCondition, const char* label) { CheckPreviewExpectation(bCondition, label); }

constexpr int   kPreviewResolution = 64;
constexpr float kRegionSidePixels  = 256.0f;
constexpr unsigned long long kFontAtlasIdentifier = 0xF0000008ull;

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
    ImGui::Begin("AreaOverlayPanelGateTestWindow");
    const ImVec2 regionOrigin = ImGui::GetCursorScreenPos();
    canvas.Draw("mapCanvas", kRegionSidePixels);
    ImGui::End();
    ImGui::Render();
    return regionOrigin;
}

ImVec2 ScreenPositionFor(const ImVec2& regionOrigin, MapCanvas& canvas, const PreviewComposite& composite,
                         float worldX, float worldZ) {
    const PreviewComposite::PreviewPixelPoint previewPixel = composite.WorldToPreviewPixel(worldX, worldZ);
    const RegionLocalPoint regionLocal =
        canvas.View().ProjectPreviewPixelToRegionLocal(previewPixel.pixelX, previewPixel.pixelY);
    return ImVec2(regionOrigin.x + regionLocal.regionLocalX, regionOrigin.y + regionLocal.regionLocalY);
}

// The total vertex count across every draw list this frame — the border quad and the 8 resize-
// handle circles are pure additional geometry, so a suppressed frame must draw strictly fewer
// vertices than an otherwise-identical frame that draws them.
std::size_t TotalDrawnVertexCount(const ImDrawData& drawData) {
    std::size_t total = 0;
    for (int listIndex = 0; listIndex < drawData.CmdListsCount; ++listIndex)
        total += static_cast<std::size_t>(drawData.CmdLists[listIndex]->VtxBuffer.Size);
    return total;
}

} // namespace

void RunMapCanvasAreaOverlayPanelGateChecks(Sys::GpuResourceManager& manager) {
    PreviewTestScene scene;
    BuildPreviewTestScene(scene);
    PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.areas, scene.fields,
                               scene.instances, scene.entityIdentifiers);
    ConfigurePreviewSettings(composite.Settings());
    composite.Settings().previewResolution = kPreviewResolution;
    composite.SetGpuResourceManager(&manager);
    composite.Compose();

    std::vector<Params::MapArea> areas;
    Params::MapArea area;
    area.name = "Gated"; area.originX = 0.0f; area.originZ = 0.0f; area.width = 2.0f; area.length = 2.0f;
    areas.push_back(area);
    std::vector<AreaColorEntry> areaColors;
    std::vector<AreaLockEntry>  areaLocks;
    ResolveAreaLocked(areaLocks, area.name, /*bDefaultLocked=*/false);
    int selectedAreaIndex = 0;   // a valid, pre-existing selection — the exact stale-selection shape
                                 // the human's bug report described, unmodified by this ticket.

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(600.0f, 600.0f);
    io.IniFilename = nullptr;

    MapCanvas canvas;
    canvas.SetPreviewTexture(&manager, composite.CompositeTexture(), composite.Resolution());
    canvas.SetPreviewComposite(&composite);
    canvas.View().SetRegionSide(kRegionSidePixels);
    canvas.SetManualAreaDragSource(&areas, &areaColors, &areaLocks, &selectedAreaIndex);
    canvas.SetAreaCompositeRefreshCallback([] {});

    // Frame 0 — priming, mouse away: establishes the region origin this window layout produces
    // (mirrors every sibling GL-backed MapCanvas test's own frame-0 priming convention).
    io.AddMousePosEvent(-100.0f, -100.0f);
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame();
    const ImVec2 regionOrigin = DrawOneFrame(canvas);

    // The E handle sits at (maxX, midZ) = (2, 1) for this rect — a real handle screen position, so
    // the cursor-shape branch is genuinely exercised, not merely a miss.
    const ImVec2 handlePosition = ScreenPositionFor(regionOrigin, canvas, composite, 2.0f, 1.0f);

    // --- Case 1: a NON-Areas panel active — the stale selection's chrome must be fully suppressed:
    // strictly less chrome geometry drawn, and no resize cursor even though the mouse sits exactly
    // on the E handle's own screen position. ---
    ApplicationPanel activePanel = ApplicationPanel::Heightmap;
    canvas.SetActivePanelSource(&activePanel);
    io.AddMousePosEvent(handlePosition.x, handlePosition.y);
    BeginHeadlessFrame();
    DrawOneFrame(canvas);
    const std::size_t gatedVertexCount = TotalDrawnVertexCount(*ImGui::GetDrawData());
    const ImGuiMouseCursor gatedCursor = ImGui::GetMouseCursor();

    // --- Case 2: the Areas panel active — identical selection, identical mouse position: chrome
    // must draw exactly as before this ticket (no regression). ---
    activePanel = ApplicationPanel::Areas;   // same MapCanvas, same pointer, new value
    BeginHeadlessFrame();
    DrawOneFrame(canvas);
    const std::size_t areasActiveVertexCount = TotalDrawnVertexCount(*ImGui::GetDrawData());
    const ImGuiMouseCursor areasActiveCursor = ImGui::GetMouseCursor();

    check(gatedCursor == ImGuiMouseCursor_Arrow,
          "STEP228: a non-Areas panel active suppresses the resize cursor even directly over a "
          "stale selection's own E-handle screen position");
    check(areasActiveCursor == ImGuiMouseCursor_ResizeEW,
          "STEP228: the Areas panel active still shows the E-handle's resize cursor exactly as "
          "before this ticket - no regression");
    check(areasActiveVertexCount > gatedVertexCount,
          "STEP228: the Areas panel active draws strictly more chrome geometry (the border quad "
          "plus the 8 resize-handle circles) than the gated, non-Areas-active frame");

    ImGui::DestroyContext();
}

} // namespace Ui
} // namespace SanmapGen
