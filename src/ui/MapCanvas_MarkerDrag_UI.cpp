// MapCanvas_MarkerDrag_UI.cpp — see MapCanvas_MarkerDrag_UI.h for the file's rationale. Also
// defines MapCanvas's own three gesture-lifecycle methods (declared in MapCanvas_UI.h, defined
// here rather than MapCanvas_UI.cpp/MapCanvas_Draw_UI.cpp so those two keep their existing single
// job — pan/zoom/pick state and imgui pointer routing respectively).
#include "MapCanvas_MarkerDrag_UI.h"
#include "MapCanvas_UI.h"
#include "PreviewComposite_UI.h"
#include "../params/MapRecipe_PARAMS.h"
#include <imgui.h>

namespace SanmapGen {
namespace Ui {
namespace {

constexpr float kManualMarkerBaseDotRadiusScreenPixels = 6.0f;

// STEP122: replaces the hardcoded kManualMarkerDotRadiusScreenPixels — composes Global × per-layer
// Icon Scale into the roster dot's radius, mirroring ManualMarkerTint's exact shape/posture
// (same anonymous namespace, same signature family, reusing the same globalMarkerSettings
// parameter STEP116 already threads through this file).
float ManualMarkerDotRadius(const std::vector<Params::MarkerInstanceLayer>& markerLayers, int layerIndex,
                            const std::string& groupName, const Params::GlobalMarkerSettings& globalMarkerSettings) {
    const float layerIconScale = (layerIndex >= 0 && layerIndex < static_cast<int>(markerLayers.size()))
        ? markerLayers[static_cast<std::size_t>(layerIndex)].iconScale : 1.0f;
    return kManualMarkerBaseDotRadiusScreenPixels
         * Params::ResolveMarkerGroupTypeScale(groupName, globalMarkerSettings) * layerIconScale;
}

ImVec2 ProjectWorldToScreen(const PreviewComposite& composite, const MapCanvasView& view,
                            float worldX, float worldZ, float regionOriginX, float regionOriginY) {
    const PreviewComposite::PreviewPixelPoint previewPixel = composite.WorldToPreviewPixel(worldX, worldZ);
    const RegionLocalPoint regionLocal = view.ProjectPreviewPixelToRegionLocal(previewPixel.pixelX, previewPixel.pixelY);
    return ImVec2(regionOriginX + regionLocal.regionLocalX, regionOriginY + regionLocal.regionLocalY);
}

ImU32 ManualMarkerTint(const std::vector<Params::MarkerInstanceLayer>& markerLayers, int layerIndex,
                       const std::string& groupName, const Params::GlobalMarkerSettings& globalMarkerSettings) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(markerLayers.size()))
        return IM_COL32(220, 220, 220, 255);
    const Params::MarkerInstanceLayer& layer = markerLayers[static_cast<std::size_t>(layerIndex)];
    if (layer.bColorOverrideEnabled)
        return ImGui::ColorConvertFloat4ToU32(ImVec4(layer.color[0], layer.color[1], layer.color[2], layer.color[3]));
    float typeRed = 1.0f, typeGreen = 1.0f, typeBlue = 1.0f;
    Params::ResolveMarkerGroupTypeTintColor(groupName, globalMarkerSettings, typeRed, typeGreen, typeBlue);
    return ImGui::ColorConvertFloat4ToU32(ImVec4(typeRed, typeGreen, typeBlue, layer.color[3]));
}

// Resolves a Spawn-group transform's render tint to its matching army's real color — the ratified
// match rule, ARCH_16_08_SpawnArmyShrink.md §16.8: Army::name == MarkerTransform::name,
// byte-for-byte, NEVER MarkerTransform::alias. An orphaned Spawn slot (no army carries this name —
// already "a legal, unremarkable state," ARCH_16_08) falls back to `fallback`, the caller's own
// already-resolved layer-color tint — never a crash, never a hardcoded literal color.
ImU32 ManualSpawnArmyTint(const std::vector<Params::Army>& armies, const std::string& transformName,
                          ImU32 fallback) {
    for (const Params::Army& army : armies)
        if (army.name == transformName)
            return ImGui::ColorConvertFloat4ToU32(ImVec4(army.armyColor[0], army.armyColor[1],
                                                          army.armyColor[2], army.armyColor[3]));
    return fallback;
}

} // namespace

bool HitTestManualMarkers(const std::vector<Params::MarkerInstanceGroup>& markers,
                          const PreviewComposite& composite, const MapCanvasView& view,
                          float regionLocalX, float regionLocalY, float pickRadiusScreenPixels,
                          int& outGroupIndex, int& outTransformIndex) {
    outGroupIndex = -1; outTransformIndex = -1;
    if (composite.PixelsPerPreviewCell() <= 0.0f) return false;
    const float radiusSquared = pickRadiusScreenPixels * pickRadiusScreenPixels;
    float bestDistanceSquared = radiusSquared;
    for (std::size_t groupIndex = 0; groupIndex < markers.size(); ++groupIndex) {
        const std::vector<Params::MarkerTransform>& transforms = markers[groupIndex].transforms;
        for (std::size_t transformIndex = 0; transformIndex < transforms.size(); ++transformIndex) {
            const Params::MarkerTransform& transform = transforms[transformIndex];
            const PreviewComposite::PreviewPixelPoint previewPixel =
                composite.WorldToPreviewPixel(transform.transform.positionX, transform.transform.positionZ);
            const RegionLocalPoint screenPoint =
                view.ProjectPreviewPixelToRegionLocal(previewPixel.pixelX, previewPixel.pixelY);
            const float deltaX = screenPoint.regionLocalX - regionLocalX;
            const float deltaY = screenPoint.regionLocalY - regionLocalY;
            const float distanceSquared = deltaX * deltaX + deltaY * deltaY;
            // Strict `<` once a candidate is already held, mirroring Picking_UI::PickMarker's own
            // tie convention exactly: the FIRST (lowest group, then lowest transform) marker within
            // radius wins a tie, never a later one silently overwriting it. `distanceSquared <=
            // radiusSquared` (not `<`) still admits a marker sitting exactly on the pick radius.
            if (distanceSquared <= radiusSquared
                && (outGroupIndex < 0 || distanceSquared < bestDistanceSquared)) {
                bestDistanceSquared = distanceSquared;
                outGroupIndex = static_cast<int>(groupIndex);
                outTransformIndex = static_cast<int>(transformIndex);
            }
        }
    }
    return outGroupIndex >= 0;
}

void DrawManualMarkerRoster(const std::vector<Params::MarkerInstanceGroup>& markers,
                            const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                            const std::vector<Params::Army>& armies,
                            const Params::GlobalMarkerSettings& globalMarkerSettings,
                            const MarkerDragGestureState& dragState, const PreviewComposite& composite,
                            const MapCanvasView& view, float regionOriginX, float regionOriginY,
                            ImDrawList& drawList) {
    if (composite.PixelsPerPreviewCell() <= 0.0f) return;
    const ImU32 refusedTint = IM_COL32(220, 60, 40, 255);
    const ImU32 ghostTint   = IM_COL32(200, 200, 200, 130);

    for (std::size_t groupIndex = 0; groupIndex < markers.size(); ++groupIndex) {
        const Params::MarkerInstanceGroup& group = markers[groupIndex];
        const bool bThisGroupDragging = dragState.bActive && dragState.groupIndex == static_cast<int>(groupIndex);
        for (std::size_t transformIndex = 0; transformIndex < group.transforms.size(); ++transformIndex) {
            if (bThisGroupDragging
                && IsMarkerSoftHiddenThisFrame(dragState, static_cast<int>(groupIndex), static_cast<int>(transformIndex)))
                continue;
            const Params::MarkerTransform& transform = group.transforms[transformIndex];
            const ImVec2 screenCenter = ProjectWorldToScreen(composite, view, transform.transform.positionX,
                                                             transform.transform.positionZ, regionOriginX, regionOriginY);
            ImU32 tint;
            if (bThisGroupDragging && dragState.bSpawnCardinalityRefused) {
                tint = refusedTint;
            } else if (IsSpawnMarkerGroup(group)) {
                tint = ManualSpawnArmyTint(armies, transform.name,
                                           ManualMarkerTint(markerLayers, transform.layerIndex, group.name, globalMarkerSettings));
            } else {
                tint = ManualMarkerTint(markerLayers, transform.layerIndex, group.name, globalMarkerSettings);
            }
            drawList.AddCircleFilled(screenCenter,
                                     ManualMarkerDotRadius(markerLayers, transform.layerIndex, group.name, globalMarkerSettings),
                                     tint);
        }
        if (bThisGroupDragging) {
            // STEP122: the ghost points belong to the same dragged transform — its own layerIndex
            // (not the last transform iterated above, which is out of scope here) resolves the
            // drag-group's own dot size, consistent with the ghost being that same group's sibling
            // orbit slots.
            const int draggedLayerIndex = (dragState.draggedTransformIndex >= 0
                && static_cast<std::size_t>(dragState.draggedTransformIndex) < group.transforms.size())
                ? group.transforms[static_cast<std::size_t>(dragState.draggedTransformIndex)].layerIndex : -1;
            const float ghostDotRadius = ManualMarkerDotRadius(markerLayers, draggedLayerIndex, group.name, globalMarkerSettings);
            for (const Pipeline::WorldSymmetryOrbitPoint& ghost : dragState.currentGhostPoints) {
                const ImVec2 screenCenter = ProjectWorldToScreen(composite, view, ghost.worldPositionX,
                                                                 ghost.worldPositionZ, regionOriginX, regionOriginY);
                drawList.AddCircle(screenCenter, ghostDotRadius, ghostTint, 0, 2.0f);
            }
        }
    }
    if (dragState.bActive && dragState.bSpawnCardinalityRefused)
        ImGui::SetTooltip("Spawn count is fixed - drag limited.");
}

bool MapCanvas::TryBeginManualMarkerDrag(float regionLocalX, float regionLocalY) {
    if (manualMarkerDragMarkers == nullptr || manualMarkerDragGeometry == nullptr
        || manualMarkerDragRecipe == nullptr || composite == nullptr) return false;
    // STEP113 — a drag may only BEGIN while the Markers panel is active. Guard-clause negated-OR
    // form, matching this function's OWN existing null-check style immediately above (not
    // DrawScenarioEditModeOverlayPass's positive "!= nullptr && ->IsActive()" gate-and-proceed
    // form) — both are the same null-safety posture, applied as the shape each call site already
    // uses. Null (no shell has wired a panel source, e.g. a test harness) refuses, never defaults
    // to permitting a drag — same null-safe-refuses posture as the existing scenarioEditModeState
    // pointer (MapCanvas_UI.h:173), not a new convention.
    if (activePanelSource == nullptr || *activePanelSource != ApplicationPanel::Markers) return false;
    int hitGroupIndex = -1, hitTransformIndex = -1;
    if (!HitTestManualMarkers(*manualMarkerDragMarkers, *composite, view, regionLocalX, regionLocalY,
                              pickRadiusScreenPixels, hitGroupIndex, hitTransformIndex))
        return false;
    static const std::vector<Params::MarkerInstanceLayer> kNoLayers;
    return BeginMarkerDragGesture(manualMarkerDragState, *manualMarkerDragMarkers,
                                  manualMarkerDragLayers != nullptr ? *manualMarkerDragLayers : kNoLayers,
                                  *manualMarkerDragGeometry, manualMarkerDragRecipe->globalSymmetryMask,
                                  manualMarkerDragRecipe->radialSymmetryRepeatCount,
                                  hitGroupIndex, hitTransformIndex);
}

void MapCanvas::ContinueManualMarkerDrag(float regionLocalX, float regionLocalY) {
    if (manualMarkerDragMarkers == nullptr || manualMarkerDragGeometry == nullptr || composite == nullptr) return;
    const PreviewPixelCoordinate previewPixel = view.ResolvePreviewPixel(regionLocalX, regionLocalY);
    const PreviewComposite::PreviewWorldPoint worldPoint = composite->PreviewPixelToWorld(
        static_cast<float>(previewPixel.pixelX), static_cast<float>(previewPixel.pixelY));
    static const std::vector<Params::MarkerInstanceLayer> kNoLayers;
    UpdateMarkerDragGesture(manualMarkerDragState, *manualMarkerDragMarkers,
                           manualMarkerDragLayers != nullptr ? *manualMarkerDragLayers : kNoLayers,
                           *manualMarkerDragGeometry, worldPoint.worldX, worldPoint.worldZ);
}

void MapCanvas::EndManualMarkerDrag() {
    if (manualMarkerDragMarkers == nullptr || manualMarkerDragGeometry == nullptr) {
        manualMarkerDragState = MarkerDragGestureState{};
        return;
    }
    EndMarkerDragGesture(manualMarkerDragState, *manualMarkerDragMarkers, *manualMarkerDragGeometry);
}

void MapCanvas::DrawManualMarkerDragPass(float regionOriginX, float regionOriginY) {
    if (composite == nullptr || manualMarkerDragMarkers == nullptr) return;
    static const std::vector<Params::MarkerInstanceLayer> kNoLayers;
    static const std::vector<Params::Army> kNoArmies;
    static const Params::GlobalMarkerSettings kDefaultGlobalMarkerSettings;
    DrawManualMarkerRoster(*manualMarkerDragMarkers, manualMarkerDragLayers != nullptr ? *manualMarkerDragLayers : kNoLayers,
                          manualMarkerDragRecipe != nullptr ? manualMarkerDragRecipe->armies : kNoArmies,
                          manualMarkerDragRecipe != nullptr ? manualMarkerDragRecipe->globalMarkerSettings : kDefaultGlobalMarkerSettings,
                          manualMarkerDragState, *composite, view, regionOriginX, regionOriginY,
                          *ImGui::GetWindowDrawList());
}

} // namespace Ui
} // namespace SanmapGen
