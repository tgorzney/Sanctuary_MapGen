// MapCanvas_AreaDraw_UI.cpp — MapCanvas::DrawAreaOverlayPass (ARCH §21.8): fill+border every area
// every frame (a deliberate v2 simplification over v1's drag-only fill, §21.8's own note — the
// always-on fill re-reads the same live, possibly-dragged rect every frame for free), the 8 handles
// for the selected area only, and hover-only cursor-shape feedback (cosmetic; never threaded through
// AreaDragGestureState).
#include "MapCanvas_UI.h"
#include "AreasTab_List_UI.h"       // ResolveAreaColor
#include "PreviewComposite_UI.h"
#include <imgui.h>

namespace SanmapGen {
namespace Ui {

void MapCanvas::DrawAreaOverlayPass(float regionOriginX, float regionOriginY) {
    if (composite == nullptr || manualAreaDrag.areas == nullptr) return;
    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    const std::vector<Params::MapArea>& areas = *manualAreaDrag.areas;
    const int selectedIndex = manualAreaDrag.selectedAreaIndex != nullptr ? *manualAreaDrag.selectedAreaIndex : -1;

    auto ToScreen = [&](float worldX, float worldZ) {
        const PreviewComposite::PreviewPixelPoint pixel = composite->WorldToPreviewPixel(worldX, worldZ);
        const RegionLocalPoint local = view.ProjectPreviewPixelToRegionLocal(pixel.pixelX, pixel.pixelY);
        return ImVec2(regionOriginX + local.regionLocalX, regionOriginY + local.regionLocalY);
    };

    for (int index = 0; index < static_cast<int>(areas.size()); ++index) {
        const Params::MapArea& area = areas[static_cast<std::size_t>(index)];
        const ImVec2 nwScreen = ToScreen(area.originX, area.originZ);
        const ImVec2 seScreen = ToScreen(area.originX + area.width, area.originZ + area.length);

        const float* const color = manualAreaDrag.areaColors != nullptr
            ? ResolveAreaColor(*manualAreaDrag.areaColors, area.name) : nullptr;
        const ImU32 fillColor = color != nullptr
            ? ImGui::ColorConvertFloat4ToU32(ImVec4(color[0], color[1], color[2], color[3]))
            : ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 0.35f));
        drawList->AddRectFilled(nwScreen, seScreen, fillColor);
        drawList->AddRect(nwScreen, seScreen, ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)));

        if (index != selectedIndex) continue;
        AreaHandleWorldPoint_UI handlePoints[8];
        ComputeAreaHandleWorldPoints(area, handlePoints);
        const ImU32 handleColor = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        for (const AreaHandleWorldPoint_UI& handlePoint : handlePoints)
            drawList->AddCircleFilled(ToScreen(handlePoint.worldX, handlePoint.worldZ),
                                      kAreaHandleScreenRadiusPixels, handleColor);
    }

    // Cursor-shape feedback: hover-only, re-hit-tested fresh against the CURRENT cursor position —
    // gated on the cursor being within the canvas region (view.RegionSidePixels(), not
    // ImGui::IsItemHovered(), since this pass runs before this frame's InvisibleButton is declared).
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(areas.size())) return;
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
