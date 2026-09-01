// MapCanvas_MarkerRosterDraw_UI.cpp — DrawManualMarkerRoster and its own draw-time helpers, split out
// of MapCanvas_MarkerDrag_UI.cpp (STEP126) for the same ceiling reason as MapCanvas_MarkerHitTest_UI.cpp.
#include "MapCanvas_MarkerDrag_UI.h"
#include "MapCanvas_UI.h"
#include "MarkersTab_MarkerLinkInstanceResolvers_UI.h"
#include "PreviewComposite_UI.h"
#include "../params/GlobalMarkerSettings_PARAMS.h"
#include "../params/MarkerLink_PARAMS.h"
#include <imgui.h>

namespace SanmapGen {
namespace Ui {
namespace {

constexpr float kManualMarkerBaseDotRadiusScreenPixels = 6.0f;

// STEP122: replaces the hardcoded kManualMarkerDotRadiusScreenPixels — composes Global × effective
// per-instance Icon Scale into the roster dot's radius, mirroring ManualMarkerTint's exact shape/
// posture (same anonymous namespace, same signature family, reusing the same globalMarkerSettings
// parameter STEP116 already threads through this file).
// STEP246, ARCH §19.33/§21.9: widened to take the owning `transform` + `links` (was a bare
// layerIndex) — resolves instance-tier-first, THEN Layer-tier (EffectiveManualMarkerInstanceIconScale).
float ManualMarkerDotRadius(const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                            const Params::MarkerTransform& transform,
                            const std::vector<Params::MarkerLink>& links,
                            const std::string& groupName, const Params::GlobalMarkerSettings& globalMarkerSettings) {
    const int layerIndex = transform.layerIndex;
    const float layerIconScale = (layerIndex >= 0 && layerIndex < static_cast<int>(markerLayers.size()))
        ? EffectiveManualMarkerInstanceIconScale(transform, markerLayers[static_cast<std::size_t>(layerIndex)], links)
        : 1.0f;
    return kManualMarkerBaseDotRadiusScreenPixels
         * Params::ResolveMarkerGroupTypeScale(groupName, globalMarkerSettings) * layerIconScale;
}

ImVec2 ProjectWorldToScreen(const PreviewComposite& composite, const MapCanvasView& view,
                            float worldX, float worldZ, float regionOriginX, float regionOriginY) {
    const PreviewComposite::PreviewPixelPoint previewPixel = composite.WorldToPreviewPixel(worldX, worldZ);
    const RegionLocalPoint regionLocal = view.ProjectPreviewPixelToRegionLocal(previewPixel.pixelX, previewPixel.pixelY);
    return ImVec2(regionOriginX + regionLocal.regionLocalX, regionOriginY + regionLocal.regionLocalY);
}

// STEP246, ARCH §19.33/§21.9: widened to take the owning `transform` + `links` (was a bare
// layerIndex) — resolves instance-tier-first, THEN Layer-tier (EffectiveManualMarkerInstance*
// ColorOverrideEnabled/Color), closing the pre-existing bug this ticket is FOR: neither this nor
// any other render consumer ever actually reached a Link's own bColorOverrideEnabled/color before.
ImU32 ManualMarkerTint(const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                       const Params::MarkerTransform& transform, const std::vector<Params::MarkerLink>& links,
                       const std::string& groupName, const Params::GlobalMarkerSettings& globalMarkerSettings) {
    const int layerIndex = transform.layerIndex;
    const bool bInRange = layerIndex >= 0 && layerIndex < static_cast<int>(markerLayers.size());
    // A missing/out-of-range layer (the common case — MarkerTransform::layerIndex defaults to 0, and
    // recipe.markerLayers is empty until a Manual Layer is actually authored) has no override to
    // apply — same as an IN-range layer/instance with the override resolved off, both fall through
    // to the Type's own configured tint, never a hardcoded grey.
    const bool bHasOverride = bInRange && EffectiveManualMarkerInstanceColorOverrideEnabled(
        transform, markerLayers[static_cast<std::size_t>(layerIndex)], links);
    if (bHasOverride) {
        const float* const color = EffectiveManualMarkerInstanceColor(
            transform, markerLayers[static_cast<std::size_t>(layerIndex)], links);
        return ImGui::ColorConvertFloat4ToU32(ImVec4(color[0], color[1], color[2], color[3]));
    }
    float typeRed = 1.0f, typeGreen = 1.0f, typeBlue = 1.0f;
    Params::ResolveMarkerGroupTypeTintColor(groupName, globalMarkerSettings, typeRed, typeGreen, typeBlue);
    const float alpha = bInRange ? markerLayers[static_cast<std::size_t>(layerIndex)].color[3] : 1.0f;
    return ImGui::ColorConvertFloat4ToU32(ImVec4(typeRed, typeGreen, typeBlue, alpha));
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

// STEP126 — true when `instanceIdentifier` is in this frame's computed highlight set
// (ComputeManualMarkerSelectionHighlight). Linear scan — the design doc's own "small per-frame
// vector" posture, authoring scale.
bool IsInstanceHighlighted(const std::vector<int>& selectedHighlightInstanceIdentifiers, int instanceIdentifier) {
    if (instanceIdentifier < 0) return false;
    for (int highlighted : selectedHighlightInstanceIdentifiers)
        if (highlighted == instanceIdentifier) return true;
    return false;
}

} // namespace

void DrawManualMarkerRoster(const std::vector<Params::MarkerInstanceGroup>& markers,
                            const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                            const std::vector<Params::Army>& armies,
                            const Params::GlobalMarkerSettings& globalMarkerSettings,
                            const MarkerDragGestureState& dragState, const PreviewComposite& composite,
                            const MapCanvasView& view, float regionOriginX, float regionOriginY,
                            const std::vector<int>& selectedHighlightInstanceIdentifiers,   // NEW — STEP126
                            const std::vector<Params::MarkerLink>& markerLinks,   // NEW — STEP246
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
            // ARCH §19.18 — canonical priority, highest to lowest:
            if (bThisGroupDragging && dragState.bSpawnCardinalityRefused) {
                tint = refusedTint;
            } else if (IsInstanceHighlighted(selectedHighlightInstanceIdentifiers, transform.instanceIdentifier)) {
                // NEW — full fill replacement, opaque. ResolveMarkerGroupSelectTintColor's own
                // ratified signature returns RGB only (mirroring ResolveMarkerGroupTypeTintColor's 3-
                // out-param shape); this ticket's own call: alpha = 1.0f (fully opaque), the strongest,
                // most unambiguous "selected" signal — not layer.color[3] (that alpha belongs to the
                // UNSELECTED type/layer-color path) and not a selectColor*[3] alpha component
                // (the resolver never exposes it). Flagged as this ticket's own judgment call, not an
                // ARCH-specified value.
                float selectRed, selectGreen, selectBlue;
                Params::ResolveMarkerGroupSelectTintColor(group.name, globalMarkerSettings, selectRed, selectGreen, selectBlue);
                tint = ImGui::ColorConvertFloat4ToU32(ImVec4(selectRed, selectGreen, selectBlue, 1.0f));
            } else if (IsSpawnMarkerGroup(group)) {
                tint = ManualSpawnArmyTint(armies, transform.name,
                                           ManualMarkerTint(markerLayers, transform, markerLinks, group.name, globalMarkerSettings));
            } else {
                tint = ManualMarkerTint(markerLayers, transform, markerLinks, group.name, globalMarkerSettings);
            }
            drawList.AddCircleFilled(screenCenter,
                                     ManualMarkerDotRadius(markerLayers, transform, markerLinks, group.name, globalMarkerSettings),
                                     tint);
        }
        if (bThisGroupDragging) {
            // STEP122: the ghost points belong to the same dragged transform (not the last transform
            // iterated above, which is out of scope here) — resolves the drag-group's own dot size,
            // consistent with the ghost being that same group's sibling orbit slots. A missing dragged
            // transform (out-of-range index) falls back to a synthetic layerIndex=-1 transform, the
            // exact "out of range" shape ManualMarkerDotRadius already treats as scale 1.0.
            const Params::MarkerTransform* const draggedTransform = (dragState.draggedTransformIndex >= 0
                && static_cast<std::size_t>(dragState.draggedTransformIndex) < group.transforms.size())
                ? &group.transforms[static_cast<std::size_t>(dragState.draggedTransformIndex)] : nullptr;
            Params::MarkerTransform noDraggedTransformFallback;
            noDraggedTransformFallback.layerIndex = -1;
            const float ghostDotRadius = ManualMarkerDotRadius(markerLayers,
                draggedTransform != nullptr ? *draggedTransform : noDraggedTransformFallback,
                markerLinks, group.name, globalMarkerSettings);
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

} // namespace Ui
} // namespace SanmapGen
