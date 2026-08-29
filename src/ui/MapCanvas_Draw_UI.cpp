// MapCanvas_Draw_UI.cpp — the canvas's imgui frame: one image draw of the composite texture and
// one invisible hit-test surface over it (UI_FRAMEWORK_SPEC §1, the bypass pattern). Layer: UI.
// This is the ONLY translation unit of the canvas that includes imgui, and it contains no math:
// it reads imgui's pointer state and calls the gestures in MapCanvas_UI.cpp, so what a click
// means is defined in exactly one place. The 100k entities are NOT drawn here — the composite
// already rasterized them into the texture and into the entity-id buffer, which is why a click
// costs one buffer read instead of an imgui loop over every instance.
#include "MapCanvas_UI.h"
#include "MarkerTypeVisibility_UI.h"
#include "../params/MapRecipe_PARAMS.h"
#include <algorithm>
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
    // STEP207 — the marquee's own rubber-band rectangle, on top of the normal overlay stack.
    DrawMarqueeRectanglePass(regionOrigin.x, regionOrigin.y);
    // ARCH §21.8 — every Area's fill+border every frame, handles for the selected one only.
    DrawAreaOverlayPass(regionOrigin.x, regionOrigin.y);
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
    // STEP133 — null-safe: no shell has wired SetMarkerTypeVisibilitySource means null/0, i.e.
    // today's exact unfiltered behavior.
    iconLayerInput.markerTypeVisibility = markerTypeVisibilitySource;
    iconLayerInput.markerTypeVisibilityRevision =
        markerTypeVisibilitySource != nullptr ? markerTypeVisibilitySource->revision : 0;
    // ARCH §19.25 — `selectedInstanceKey` IS the canonical key now (procedural or manual, correctly
    // tagged `bManual`); no longer reconstructed from a bare entity id, which could only ever
    // represent the procedural case. ARCH §21.1 — the draw pass still highlights only the PRIMARY;
    // widening it to the whole multi-select set is a visual-language ticket of its own, not this one.
    if (HasSelection())
        iconLayerInput.selectedInstanceKey = PrimaryOfSelectionSet(selectedInstanceKeys);
    DrawOverlayIconLayers(iconLayerInput, overlayLayerAabbCache, overlayIconLayerFrameCache,
                         *ImGui::GetWindowDrawList());
}

// STEP207 — the marquee's own visual feedback: press-start to live-cursor rubber-band rectangle.
// Collection-agnostic (Markers/Props/Decals all share ApplyPointerInput/ApplyMarqueeGesture, ARCH
// §21.2/§21.6) — drawn once, keyed only on `bPressActive`/no-drag-active, never per collection. The
// selection LOGIC this feeds is untouched (STEP207's own out-of-scope note); this is visual only.
void MapCanvas::DrawMarqueeRectanglePass(float regionOriginX, float regionOriginY) {
    // A manual-instance drag, once active, owns the whole press exclusively (ARCH §21.2) — never
    // show a marquee box mid-drag.
    const bool bManualDragActive = bManualMarkerDragActive || bManualPropDragActive || bManualDecalDragActive;
    if (!bPressActive || bManualDragActive || bAreaDragActive) return;

    const ImGuiIO& io = ImGui::GetIO();
    const float currentRegionLocalX = io.MousePos.x - regionOriginX;
    const float currentRegionLocalY = io.MousePos.y - regionOriginY;
    const ImVec2 pressCorner(regionOriginX + pressStartRegionLocalX, regionOriginY + pressStartRegionLocalY);
    const ImVec2 currentCorner(regionOriginX + currentRegionLocalX, regionOriginY + currentRegionLocalY);
    // A press that hasn't moved at all yet has an exactly zero-area box — nothing to draw. This is
    // NOT the click/drag tolerance threshold (no "pop-in" at that boundary, per the work-order's own
    // UX note) — it only skips the true zero-pixel case, which is imperceptible regardless.
    if (pressCorner.x == currentCorner.x && pressCorner.y == currentCorner.y) return;

    const ImVec2 minCorner(std::min(pressCorner.x, currentCorner.x), std::min(pressCorner.y, currentCorner.y));
    const ImVec2 maxCorner(std::max(pressCorner.x, currentCorner.x), std::max(pressCorner.y, currentCorner.y));
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(minCorner, maxCorner, ImGui::GetColorU32(ImGuiCol_ButtonActive, 0.15f));
    drawList->AddRect(minCorner, maxCorner, ImGui::GetColorU32(ImGuiCol_ButtonActive));
}

// Wheel = zoom about the cursor. Left button = click-to-select / drag-a-manual-instance /
// box-marquee-select — never pans (ARCH §21.2). Right button = the independent pan tracker below,
// entirely separate from the left-button state machine. Separating click from drag-or-marquee by
// travelled distance is what lets one (left) button do both.
void MapCanvas::ApplyPointerInput(float regionOriginX, float regionOriginY) {
    const ImGuiIO& io = ImGui::GetIO();
    const float regionLocalX = io.MousePos.x - regionOriginX;
    const float regionLocalY = io.MousePos.y - regionOriginY;

    if (ImGui::IsItemHovered() && io.MouseWheel != 0.0f)
        ApplyScroll(regionLocalX, regionLocalY, io.MouseWheel);

    // STEP78 — exclusive interaction ownership: while active, drag/click/marquee/pan never reach the
    // normal path below at all (zoom above stays available; it does not conflict with editing).
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

    // ARCH §21.2 — LEFT BUTTON: press-time drag-begin-first-else-click/marquee.
    // A press that lands on a manual instance (any of Markers/Props/Decals, nearest-hit-wins, each
    // lock-gated) starts a drag gesture instead; a miss starts neither a drag nor a pan — it merely
    // lets travel accumulate (no left-drag-pans branch at all, ARCH §21.2's own instruction).
    if (ImGui::IsItemActivated()) {
        bPressActive = true; pressTravelPixels = 0.0f;
        pressStartRegionLocalX = regionLocalX; pressStartRegionLocalY = regionLocalY;
        if (!TryBeginManualInstanceDrag(regionLocalX, regionLocalY))
            TryBeginAreaDrag(regionLocalX, regionLocalY);
    }
    const bool bManualDragActive = bManualMarkerDragActive || bManualPropDragActive || bManualDecalDragActive;
    if (bPressActive && ImGui::IsItemActive()) {
        // Human's own bug report (predates §21.2, still applies) — this must accumulate regardless
        // of which branch below runs, or a gesture's own pressTravelPixels would stay frozen at its
        // activation-time 0.0f for the gesture's ENTIRE duration, and the release check below would
        // then treat every drag, however large, as a zero-travel click.
        pressTravelPixels += std::fabs(io.MouseDelta.x) + std::fabs(io.MouseDelta.y);
        if (bManualDragActive) ContinueManualInstanceDrag(regionLocalX, regionLocalY);
        else if (bAreaDragActive) ContinueAreaDrag(regionLocalX, regionLocalY, io.KeyShift, io.KeyCtrl);
    }
    if (bPressActive && ImGui::IsItemDeactivated()) {
        bPressActive = false;
        const bool bClick = pressTravelPixels <= view.settings.clickDragTolerancePixels;
        if (bManualDragActive) {
            // Settle the (no-op, if it never moved) gesture first, exactly as before §21.2.
            EndManualInstanceDrag();
            // A drag that WAS active never reaches marquee resolution — its own end-of-gesture
            // handling above is exclusive for this press (ARCH §21.2's own instruction). A gesture
            // that never actually moved is a click in disguise (human's own bug report, predates
            // §21.2) — still resolves through the real click path, with live modifier state.
            if (bClick) ApplyClickGesture(regionLocalX, regionLocalY, io.KeyCtrl, io.KeyShift);
        } else if (bAreaDragActive) {
            // ARCH §21.8 — a live Area resize/move settles here; it never falls through to
            // click/marquee resolution (Areas pre-empt that path entirely while its own panel is
            // active, ruling 5).
            EndAreaDrag();
        } else if (AreaGestureEligible()) {
            // ARCH §21.8 — the Areas panel is active and unlocked, but this press hit neither a
            // manual instance nor an existing area's handle/body: a real drag creates a new area; a
            // zero-travel click deselects. Neither ever falls through to ApplyClickGesture/
            // ApplyMarqueeGesture (ruling 5 — Areas replaces marquee-select for that panel entirely).
            if (bClick) {
                if (manualAreaDrag.selectedAreaIndex != nullptr) *manualAreaDrag.selectedAreaIndex = -1;
            } else {
                CreateAreaFromDrag(pressStartRegionLocalX, pressStartRegionLocalY, regionLocalX, regionLocalY);
            }
        } else if (bClick) {
            ApplyClickGesture(regionLocalX, regionLocalY, io.KeyCtrl, io.KeyShift);
        } else {
            // pressTravelPixels > tolerance AND no drag was active: a marquee release.
            ApplyMarqueeGesture(pressStartRegionLocalX, pressStartRegionLocalY, regionLocalX, regionLocalY,
                                io.KeyCtrl, io.KeyShift);
        }
    }

    // ARCH §21.2 — RIGHT BUTTON: the independent pan tracker, replacing left-drag-pans entirely.
    // ImGui's item-activation (IsItemActivated/IsItemActive/IsItemDeactivated) is LEFT-button-only
    // for a default-flags InvisibleButton, so this reads raw button-1 state instead — the SAME idiom
    // ScenarioEditModePointerFrame_UI::bRightClicked already uses just above in this exact file.
    if (!bRightPressActive && ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        bRightPressActive = true;
    if (bRightPressActive) {
        // Persists every frame the button stays down, independent of hover — mirroring how
        // IsItemActive() persists for the left button once a press began, even after the cursor
        // leaves the item.
        if (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f) ApplyDrag(io.MouseDelta.x, io.MouseDelta.y);
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) bRightPressActive = false;
    }
}

} // namespace Ui
} // namespace SanmapGen
