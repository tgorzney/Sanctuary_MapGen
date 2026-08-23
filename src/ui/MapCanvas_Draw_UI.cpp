// MapCanvas_Draw_UI.cpp — the canvas's imgui frame: one image draw of the composite texture and
// one invisible hit-test surface over it (UI_FRAMEWORK_SPEC §1, the bypass pattern). Layer: UI.
// This is the ONLY translation unit of the canvas that includes imgui, and it contains no math:
// it reads imgui's pointer state and calls the gestures in MapCanvas_UI.cpp, so what a click
// means is defined in exactly one place. The 100k entities are NOT drawn here — the composite
// already rasterized them into the texture and into the entity-id buffer, which is why a click
// costs one buffer read instead of an imgui loop over every instance.
#include "MapCanvas_UI.h"
#include "../params/MapRecipe_PARAMS.h"
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

    // STEP53 — the screen-space overlay icon draw pass. Composites on top of the terrain texture
    // (§14's whole redesign), before ApplyPointerInput below (ordering here is readability only,
    // not correctness — click-picking is off the draw list, STEP48 onward).
    DrawOverlayIconLayerPass(regionOrigin.x, regionOrigin.y, regionSidePixels);
    // STEP94 — Gap 6's minimal stopgap manual-marker draw, on top of the terrain/overlay stack.
    DrawManualMarkerDragPass(regionOrigin.x, regionOrigin.y);
    // STEP78 — Scenario Edit Mode's own overlay, on top of the normal overlay stack.
    DrawScenarioEditModeOverlayPass(regionOrigin.x, regionOrigin.y);

    // The hit-test surface sits exactly on the image, declared after it so it wins the hover.
    ImGui::SetCursorScreenPos(regionOrigin);
    ImGui::InvisibleButton(canvasIdentifier, regionSize);
    ApplyPointerInput(regionOrigin.x, regionOrigin.y);
    // The right-click "Remove for this scenario"/"Add Alloy Marker" popup, opened by
    // ApplyPointerInput above the SAME frame it resolves a request (bContextMenuJustRequested).
    if (scenarioEditModeState != nullptr && scenarioEditModeState->IsActive())
        DrawScenarioEditModeContextMenuPopup(*scenarioEditModeState);
}

void MapCanvas::DrawScenarioEditModeOverlayPass(float regionOriginX, float regionOriginY) {
    if (scenarioEditModeState == nullptr || !scenarioEditModeState->IsActive()) return;
    ScenarioEditModeDrawInput input;
    input.resolveInput.overlayLayerSettings = overlayLayerSettings;
    input.resolveInput.placements           = overlayPlacements;
    input.resolveInput.ruleBucketIndex      = overlayRuleBucketIndex;
    input.resolveInput.armies    = overlayRecipe != nullptr ? &overlayRecipe->armies : nullptr;
    input.composite               = composite;
    input.view                    = &view;
    input.pairingLookup           = overlayPairingLookup;
    input.atlasManifest           = overlayAtlasManifest;
    input.regionOriginX = regionOriginX; input.regionOriginY = regionOriginY;
    DrawScenarioEditModeOverlay(*scenarioEditModeState, input, *ImGui::GetWindowDrawList());
}

// Missing sources (any nullptr) draw nothing — the same posture the render-only tests already
// rely on (no overlay setters wired means no icon draw commands, not a crash).
void MapCanvas::DrawOverlayIconLayerPass(float regionOriginX, float regionOriginY, float regionSidePixels) {
    if (composite == nullptr) return;
    DrawOverlayIconLayersInput iconLayerInput;
    iconLayerInput.overlayLayerSettings = overlayLayerSettings;
    iconLayerInput.renderingSettings    = overlayRenderingSettings;
    iconLayerInput.placements           = overlayPlacements;
    iconLayerInput.ruleBucketIndex      = overlayRuleBucketIndex;
    iconLayerInput.recipe               = overlayRecipe;
    iconLayerInput.pairingLookup        = overlayPairingLookup;
    iconLayerInput.atlasManifest        = overlayAtlasManifest;
    iconLayerInput.footprintSizeTable   = worldFootprintSizeTable;
    iconLayerInput.composite            = composite;
    iconLayerInput.view                 = &view;
    iconLayerInput.regionOriginX        = regionOriginX;
    iconLayerInput.regionOriginY        = regionOriginY;
    iconLayerInput.regionSidePixels     = regionSidePixels;
    if (HasSelection())
        iconLayerInput.selectedInstanceKey = OverlayInstanceKey_UI{
            PlacementCollectionKind_UI::Markers, static_cast<std::int32_t>(selectedEntityIdentifier), true};
    DrawOverlayIconLayers(iconLayerInput, overlayLayerAabbCache, overlayIconLayerFrameCache,
                         *ImGui::GetWindowDrawList());
}

// Wheel = zoom about the cursor; left-drag = pan; a left press that barely moved = select.
// Separating click from drag by travelled distance is what lets one button do both.
void MapCanvas::ApplyPointerInput(float regionOriginX, float regionOriginY) {
    const ImGuiIO& io = ImGui::GetIO();
    const float regionLocalX = io.MousePos.x - regionOriginX;
    const float regionLocalY = io.MousePos.y - regionOriginY;

    if (ImGui::IsItemHovered() && io.MouseWheel != 0.0f)
        ApplyScroll(regionLocalX, regionLocalY, io.MouseWheel);

    // STEP78 — exclusive interaction ownership: while active, drag/click never reach the normal
    // pan/pick path below at all (zoom above stays available; it does not conflict with editing).
    if (scenarioEditModeState != nullptr && scenarioEditModeState->IsActive()) {
        if (composite != nullptr) {
            ScenarioEditModePointerFrame_UI pointerFrame;
            pointerFrame.regionLocalX = regionLocalX; pointerFrame.regionLocalY = regionLocalY;
            pointerFrame.bPressActivated = ImGui::IsItemActivated();
            pointerFrame.bPressActive    = ImGui::IsItemActive();
            pointerFrame.bRightClicked   = ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right);
            const std::vector<Params::Army> noArmies;
            ApplyScenarioEditModePointerInput(*scenarioEditModeState, view, *composite,
                                              overlayRecipe != nullptr ? overlayRecipe->armies : noArmies, pointerFrame);
        }
        return;
    }

    // STEP94 — before the existing pan-vs-click disambiguation: a press that lands on a manual
    // marker starts a drag gesture instead (a hit on an ungrouped marker still starts one, with an
    // empty correspondence table); a miss falls through to the pan/click path below unchanged.
    if (ImGui::IsItemActivated()) {
        bPressActive = true; pressTravelPixels = 0.0f;
        bManualMarkerDragActive = TryBeginManualMarkerDrag(regionLocalX, regionLocalY);
    }
    if (bPressActive && ImGui::IsItemActive()) {
        if (bManualMarkerDragActive) {
            ContinueManualMarkerDrag(regionLocalX, regionLocalY);
        } else {
            pressTravelPixels += std::fabs(io.MouseDelta.x) + std::fabs(io.MouseDelta.y);
            if (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f)
                ApplyDrag(io.MouseDelta.x, io.MouseDelta.y);
        }
    }
    if (bPressActive && ImGui::IsItemDeactivated()) {
        bPressActive = false;
        if (bManualMarkerDragActive) {
            EndManualMarkerDrag();
            bManualMarkerDragActive = false;
        } else if (pressTravelPixels <= view.settings.clickDragTolerancePixels) {
            ApplyClick(regionLocalX, regionLocalY);
        }
    }
}

} // namespace Ui
} // namespace SanmapGen
