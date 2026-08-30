// MapCanvas_AreaDraw_UI.cpp — MapCanvas::DrawAreaOverlayPass. ARCH §14.18 Part 3 (items 1/9) — an
// area's fill has EXACTLY ONE renderer, in every state including mid-gesture: the composite. This
// pass draws chrome ONLY — border (when the MapAreas field layer is enabled AND the area is
// selected) and the 8 resize handles (selected-area only), sharing one `if (selectedIndex ...)`
// scope and one pair of corner projections. There is no `AddRectFilled` anywhere in this file.
// STEP212's per-area-lock-gated cursor-shape feedback is unchanged.
#include "MapCanvas_UI.h"
#include "PreviewComposite_UI.h"
#include <imgui.h>

namespace SanmapGen {
namespace Ui {
namespace {

// ARCH §14.17 item 12's own preferred path: read through the canvas's existing `const
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
    // STEP228 — the SAME eligibility check every Area input/gesture path already trusts
    // (TryBeginAreaDrag, the click-release deselect handler): the border, the 8 resize handles, AND
    // the hover cursor-shape feedback below are all panel-scoped chrome. A stale selectedAreaIndex
    // surviving a switch to another tab must not keep drawing this area's chrome over whatever the
    // human is actually looking at now.
    if (!AreaGestureEligible()) return;
    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    const std::vector<Params::MapArea>& areas = *manualAreaDrag.areas;
    const int selectedIndex = manualAreaDrag.selectedAreaIndex != nullptr ? *manualAreaDrag.selectedAreaIndex : -1;

    auto ToScreen = [&](float worldX, float worldZ) {
        const PreviewComposite::PreviewPixelPoint pixel = composite->WorldToPreviewPixel(worldX, worldZ);
        const RegionLocalPoint local = view.ProjectPreviewPixelToRegionLocal(pixel.pixelX, pixel.pixelY);
        return ImVec2(regionOriginX + local.regionLocalX, regionOriginY + local.regionLocalY);
    };

    // ARCH §14.18 item 9 — border + the 8 handles, ONE scope, selected-area only. The border draws
    // only when the MapAreas layer is enabled; the "layer disabled => no border at all, regardless
    // of selection" rule is unchanged and still law. The handles draw unconditionally for the
    // selected area, exactly as before this ticket.
    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(areas.size())) {
        const Params::MapArea& area = areas[static_cast<std::size_t>(selectedIndex)];
        if (IsMapAreasLayerEnabled(composite->Settings())) {
            const ImVec2 nwScreen = ToScreen(area.originX, area.originZ);
            const ImVec2 seScreen = ToScreen(area.originX + area.width, area.originZ + area.length);
            drawList->AddRect(nwScreen, seScreen, ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)));
        }
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
    // imgui's own default arrow. Reuses IsAreaLocked, the SAME query TryBeginAreaDrag itself gates
    // on (MapCanvas_AreaDragDispatch_UI.cpp) — one lock check, not a second, independently-maintained
    // one.
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
