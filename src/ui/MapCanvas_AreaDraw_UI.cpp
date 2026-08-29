// MapCanvas_AreaDraw_UI.cpp — MapCanvas::DrawAreaOverlayPass. Fill/border/handles per
// ARCH_21_08_AreaCanvasGesture.md's 2026-08-29 amendment / ARCH_14_17_MapAreaFieldLayer.md §14.17
// item 12 (STEP211, unchanged by this ticket): the FILL is the composite's own job in the steady
// state (PreviewLayerKind::MapAreas) — this pass draws the fill ONLY for the one area currently
// suppressed from the composite (the one being dragged/resized/moved). The BORDER draws only when
// the MapAreas layer is enabled AND this is the suppressed area AND it is selected. The 8 handles
// (selected-area only) are unchanged. STEP212 — the hover-only cursor-shape feedback now gates on
// the SELECTED area's own per-area lock (MapCanvas::IsAreaLocked, MapCanvas_AreaDragDispatch_UI.cpp):
// while locked, no cursor override at all — the pre-STEP212 version hit-tested handles and set a
// resize cursor purely from proximity, regardless of lock, which is the exact "cursor lies" bug this
// ticket fixes (a locked area's handles showed a resize cursor even though TryBeginAreaDrag silently
// refused the click).
#include "MapCanvas_UI.h"
#include "AreasTab_List_UI.h"       // ResolveAreaColor
#include "PreviewComposite_UI.h"
#include <imgui.h>

namespace SanmapGen {
namespace Ui {
namespace {

// §14.17 item 12's own preferred path: read (a) through the canvas's existing `const
// PreviewComposite*` — it needs no new plumbing at all, since `Settings()` already has a const
// overload. A plain linear scan (not `PreviewFieldLayerOfKind`, which has no const overload) since
// this is the one place a const settings reference is available.
bool IsMapAreasLayerEnabled(const PreviewCompositeSettings& settings) {
    for (const PreviewFieldLayer& layer : settings.fieldLayers)
        if (layer.kind == PreviewLayerKind::MapAreas) return layer.bEnabled;
    return false;
}

} // namespace

void MapCanvas::DrawAreaOverlayPass(float regionOriginX, float regionOriginY) {
    if (composite == nullptr || manualAreaDrag.areas == nullptr) return;
    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    const std::vector<Params::MapArea>& areas = *manualAreaDrag.areas;
    const int selectedIndex = manualAreaDrag.selectedAreaIndex != nullptr ? *manualAreaDrag.selectedAreaIndex : -1;
    const int suppressedIndex = manualAreaDrag.mapAreaSuppressedIndex != nullptr
                              ? *manualAreaDrag.mapAreaSuppressedIndex : -1;

    auto ToScreen = [&](float worldX, float worldZ) {
        const PreviewComposite::PreviewPixelPoint pixel = composite->WorldToPreviewPixel(worldX, worldZ);
        const RegionLocalPoint local = view.ProjectPreviewPixelToRegionLocal(pixel.pixelX, pixel.pixelY);
        return ImVec2(regionOriginX + local.regionLocalX, regionOriginY + local.regionLocalY);
    };

    // ARCH §14.17 item 12 — fill + (conditional) border, ONLY for the suppressed area.
    if (suppressedIndex >= 0 && suppressedIndex < static_cast<int>(areas.size())) {
        const Params::MapArea& area = areas[static_cast<std::size_t>(suppressedIndex)];
        const ImVec2 nwScreen = ToScreen(area.originX, area.originZ);
        const ImVec2 seScreen = ToScreen(area.originX + area.width, area.originZ + area.length);

        const float* const color = manualAreaDrag.areaColors != nullptr
            ? ResolveAreaColor(*manualAreaDrag.areaColors, area.name) : nullptr;
        const ImU32 fillColor = color != nullptr
            ? ImGui::ColorConvertFloat4ToU32(ImVec4(color[0], color[1], color[2], color[3]))
            : ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 0.35f));
        drawList->AddRectFilled(nwScreen, seScreen, fillColor);

        if (IsMapAreasLayerEnabled(composite->Settings()) && suppressedIndex == selectedIndex)
            drawList->AddRect(nwScreen, seScreen, ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)));
    }

    // The 8 handles — selected-area-only. Unchanged.
    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(areas.size())) {
        const Params::MapArea& area = areas[static_cast<std::size_t>(selectedIndex)];
        AreaHandleWorldPoint_UI handlePoints[8];
        ComputeAreaHandleWorldPoints(area, handlePoints);
        const ImU32 handleColor = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        for (const AreaHandleWorldPoint_UI& handlePoint : handlePoints)
            drawList->AddCircleFilled(ToScreen(handlePoint.worldX, handlePoint.worldZ),
                                      kAreaHandleScreenRadiusPixels, handleColor);
    }

    // Cursor-shape feedback — hover-only, re-hit-tested fresh against the CURRENT cursor position —
    // gated on the cursor being within the canvas region (view.RegionSidePixels(), not
    // ImGui::IsItemHovered(), since this pass runs before this frame's InvisibleButton is declared).
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(areas.size())) return;
    // STEP212 fix — a LOCKED selected area shows no drag-affordance cursor at all: falls through to
    // imgui's own default arrow (no ImGuiMouseCursor_NotAllowed substitution — see this ticket's own
    // "Interpretation calls made," item 3, for why). Reuses IsAreaLocked, the SAME query
    // TryBeginAreaDrag itself gates on (MapCanvas_AreaDragDispatch_UI.cpp) — one lock check, not a
    // second, independently-maintained one.
    if (IsAreaLocked(selectedIndex)) return;
    const ImGuiIO& io = ImGui::GetIO();
    const float hoverRegionLocalX = io.MousePos.x - regionOriginX;
    const float hoverRegionLocalY = io.MousePos.y - regionOriginY;
    const float regionSide = view.RegionSidePixels();
    if (hoverRegionLocalX < 0.0f || hoverRegionLocalY < 0.0f
        || hoverRegionLocalX > regionSide || hoverRegionLocalY > regionSide) return;

    const AreaHandle_UI hoveredHandle = HitTestAreaHandles(areas[static_cast<std::size_t>(selectedIndex)],
                                                           *composite, view, hoverRegionLocalX, hoverRegionLocalY);
    switch (hoveredHandle) {
        case AreaHandle_UI::N: case AreaHandle_UI::S: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS); break;
        case AreaHandle_UI::E: case AreaHandle_UI::W: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW); break;
        case AreaHandle_UI::NE: case AreaHandle_UI::SW: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW); break;
        case AreaHandle_UI::NW: case AreaHandle_UI::SE: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE); break;
        case AreaHandle_UI::Center: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll); break;
        default: break;
    }
}

} // namespace Ui
} // namespace SanmapGen
