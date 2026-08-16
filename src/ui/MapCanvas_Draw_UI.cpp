// MapCanvas_Draw_UI.cpp — the canvas's imgui frame: one image draw of the composite texture and
// one invisible hit-test surface over it (UI_FRAMEWORK_SPEC §1, the bypass pattern). Layer: UI.
// This is the ONLY translation unit of the canvas that includes imgui, and it contains no math:
// it reads imgui's pointer state and calls the gestures in MapCanvas_UI.cpp, so what a click
// means is defined in exactly one place. The 100k entities are NOT drawn here — the composite
// already rasterized them into the texture and into the entity-id buffer, which is why a click
// costs one buffer read instead of an imgui loop over every instance.
#include "MapCanvas_UI.h"
#include <cmath>
#include <imgui.h>

namespace SanmapGen {
namespace Ui {

void MapCanvas::Draw(const char* canvasIdentifier, float regionSidePixels) {
    if (regionSidePixels <= 0.0f) return;
    view.SetRegionSide(regionSidePixels);
    const ImVec2 regionOrigin = ImGui::GetCursorScreenPos();
    const ImVec2 regionSize(regionSidePixels, regionSidePixels);
    const unsigned long long presentationIdentifier = PresentationIdentifier();

    // Nothing composited yet: reserve the same rectangle and show it empty, rather than drawing a
    // stale or invalid texture (Constitution §6 — never hand the UI an unverified handle).
    if (presentationIdentifier == 0ull) {
        ImGui::GetWindowDrawList()->AddRectFilled(
            regionOrigin, ImVec2(regionOrigin.x + regionSidePixels, regionOrigin.y + regionSidePixels),
            ImGui::GetColorU32(ImGuiCol_FrameBg));
        ImGui::Dummy(regionSize);
        return;
    }

    // The visible window of the image is the view's texture-coordinate window, so zooming costs
    // no re-composite and no upload — the same texture is sampled differently.
    const MapCanvasTextureWindow window = view.TextureWindow();
    ImGui::Image(static_cast<ImTextureID>(presentationIdentifier), regionSize,
                 ImVec2(window.lowTextureCoordinateX, window.lowTextureCoordinateY),
                 ImVec2(window.highTextureCoordinateX, window.highTextureCoordinateY));
    // The hit-test surface sits exactly on the image, declared after it so it wins the hover.
    ImGui::SetCursorScreenPos(regionOrigin);
    ImGui::InvisibleButton(canvasIdentifier, regionSize);
    ApplyPointerInput(regionOrigin.x, regionOrigin.y);
}

// Wheel = zoom about the cursor; left-drag = pan; a left press that barely moved = select.
// Separating click from drag by travelled distance is what lets one button do both.
void MapCanvas::ApplyPointerInput(float regionOriginX, float regionOriginY) {
    const ImGuiIO& io = ImGui::GetIO();
    const float regionLocalX = io.MousePos.x - regionOriginX;
    const float regionLocalY = io.MousePos.y - regionOriginY;

    if (ImGui::IsItemHovered() && io.MouseWheel != 0.0f)
        ApplyScroll(regionLocalX, regionLocalY, io.MouseWheel);

    if (ImGui::IsItemActivated()) { bPressActive = true; pressTravelPixels = 0.0f; }
    if (bPressActive && ImGui::IsItemActive()) {
        pressTravelPixels += std::fabs(io.MouseDelta.x) + std::fabs(io.MouseDelta.y);
        if (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f)
            ApplyDrag(io.MouseDelta.x, io.MouseDelta.y);
    }
    if (bPressActive && ImGui::IsItemDeactivated()) {
        bPressActive = false;
        if (pressTravelPixels <= view.settings.clickDragTolerancePixels)
            ApplyClick(regionLocalX, regionLocalY);
    }
}

} // namespace Ui
} // namespace SanmapGen
