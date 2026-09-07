// MapCanvas_MarkerRosterDraw_UI.cpp — DrawManualMarkerRoster and its own draw-time helpers, split out
// of MapCanvas_MarkerDrag_UI.cpp (STEP126) for the same ceiling reason as MapCanvas_MarkerHitTest_UI.cpp.
#include "CoordinateSpace_UI.h"
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

// BUGFIX_UniversalCoordinateConversionAndDragRewrite_UI, Part 1 — a one-line call to the new
// shared CoordinateSpace_UI::WorldToScreen, same output as before (WorldToPreviewPixel +
// ProjectPreviewPixelToRegionLocal composed), just routed through the one shared function instead
// of this file's own one-off.
ImVec2 ProjectWorldToScreen(const PreviewComposite& composite, const MapCanvasView& view,
                            float worldX, float worldZ, float regionOriginX, float regionOriginY) {
    const ScreenPoint screen = WorldToScreen(view, composite, WorldPoint{worldX, worldZ});
    return ImVec2(regionOriginX + screen.screenX, regionOriginY + screen.screenY);
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

// Follow-up to BUGFIX_UniversalCoordinateConversionAndDragRewrite_UI — the four small helpers below
// fold `dragStates`' plurality into the same single-bool/single-check shape the rest of this file's
// logic already expects, so the per-instance loop and the ghost-draw pass below read exactly as they
// did for a single `dragState`, just OR'd/unioned across every entry. A single-element vector reduces
// to precisely the old single-state boolean, byte-identical.

// True when ANY drag gesture in `dragStates` is actively dragging a member of `groupIndex`.
bool AnyDragStateActiveForGroup(const std::vector<MarkerDragGestureState>& dragStates, int groupIndex) {
    for (const MarkerDragGestureState& state : dragStates)
        if (state.bActive && state.groupIndex == groupIndex) return true;
    return false;
}

// True when ANY drag gesture active on `groupIndex` is itself Spawn-cardinality-refused — the flag
// is group-wide (the whole roster's Spawn slot count is frozen), so one refused dragger is enough to
// tint every instance in that group red, exactly as the single-state code already did for its one
// dragger.
bool AnyDragStateRefusedForGroup(const std::vector<MarkerDragGestureState>& dragStates, int groupIndex) {
    for (const MarkerDragGestureState& state : dragStates)
        if (state.bActive && state.groupIndex == groupIndex && state.bSpawnCardinalityRefused) return true;
    return false;
}

// True when ANY drag gesture reports `(groupIndex, transformIndex)` as this frame's soft-hidden.
bool AnyInstanceSoftHiddenThisFrame(const std::vector<MarkerDragGestureState>& dragStates, int groupIndex,
                                    int transformIndex) {
    for (const MarkerDragGestureState& state : dragStates)
        if (IsMarkerSoftHiddenThisFrame(state, groupIndex, transformIndex)) return true;
    return false;
}

// True when ANY drag gesture in `dragStates` is both active and Spawn-cardinality-refused, regardless
// of which group — the tooltip is a single global status line, not per-group.
bool AnyDragStateActiveAndRefused(const std::vector<MarkerDragGestureState>& dragStates) {
    for (const MarkerDragGestureState& state : dragStates)
        if (state.bActive && state.bSpawnCardinalityRefused) return true;
    return false;
}

// BUGFIX_OverlayVisibilityAndPropIconFallback_R1, Part 1 — mirrors the exact domain gate
// MapCanvas_IconLayer_CullManual_UI.cpp's ResolveMarkersManual already applies via its caller's
// `if (!layer.bEnabled) continue;` (MapCanvas_IconLayer_Cull_UI.cpp's ResolveVisibleCandidates):
// `group`'s domain is Alloy unless it's the reserved Spawn group name (IsSpawnMarkerGroup, the SAME
// discriminator ResolveMarkersManual itself uses via `bWantSpawnGroups`/`bIsSpawnGroup`). A `nullptr`
// source (not wired — every pre-ticket caller, including this file's own tests) means "unfiltered."
// A domain with no configured OverlayLayer_UI row at all (should not happen once
// ConfigureDefaultOverlayLayers has run, Application_UI.cpp) defaults to visible, never a silent
// hide for a shell that has not finished configuring its overlay rows yet.
bool IsMarkerGroupDomainVisible(const OverlayLayerSettings* overlayLayerSettings,
                                const Params::MarkerInstanceGroup& group) {
    if (overlayLayerSettings == nullptr) return true;
    const OverlayDomainKind_UI targetDomain =
        IsSpawnMarkerGroup(group) ? OverlayDomainKind_UI::SpawnsArmies : OverlayDomainKind_UI::Alloy;
    for (const OverlayLayer_UI& layer : overlayLayerSettings->overlayLayers)
        if (layer.domainKind == targetDomain) return layer.bEnabled;
    return true;
}

} // namespace

void DrawManualMarkerRoster(const std::vector<Params::MarkerInstanceGroup>& markers,
                            const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                            const std::vector<Params::Army>& armies,
                            const Params::GlobalMarkerSettings& globalMarkerSettings,
                            const std::vector<MarkerDragGestureState>& dragStates, const PreviewComposite& composite,
                            const MapCanvasView& view, float regionOriginX, float regionOriginY,
                            const std::vector<int>& selectedHighlightInstanceIdentifiers,   // NEW — STEP126
                            const std::vector<Params::MarkerLink>& markerLinks,   // NEW — STEP246
                            ImDrawList& drawList,
                            const OverlayLayerSettings* overlayLayerSettings) {   // NEW — see MapCanvas_MarkerDrag_UI.h
    if (composite.PixelsPerPreviewCell() <= 0.0f) return;
    const ImU32 refusedTint = IM_COL32(220, 60, 40, 255);
    const ImU32 ghostTint   = IM_COL32(200, 200, 200, 130);

    for (std::size_t groupIndex = 0; groupIndex < markers.size(); ++groupIndex) {
        const Params::MarkerInstanceGroup& group = markers[groupIndex];
        // BUGFIX_OverlayVisibilityAndPropIconFallback_R1, Part 1 — the dead-stopgap bug: this pass
        // used to draw every group unconditionally, regardless of the View toolbar's per-domain
        // OverlayLayer_UI::bEnabled toggle the gated DrawOverlayIconLayerPass already honors
        // (MapCanvas_IconLayer_CullManual_UI.cpp's ResolveMarkersManual). Narrowing to active-drag-
        // only groups was considered and rejected: MapCanvas_MarkerDrag_UI_Test.cpp's own coverage
        // proves this function's tint/radius/selection resolution is unit-tested ONLY by observing
        // its at-rest (no active drag) draw output, so removing the at-rest draw here would silently
        // delete that coverage's ability to exercise ManualMarkerTint/ManualMarkerDotRadius at all.
        // Consulting the SAME OverlayLayerSettings/bEnabled state instead — IsMarkerGroupDomainVisible,
        // this file's own anonymous namespace below — is the fix the work-order names as the fallback
        // when the narrower fix is not sufficient.
        if (!IsMarkerGroupDomainVisible(overlayLayerSettings, group)) continue;
        const bool bThisGroupDragging = AnyDragStateActiveForGroup(dragStates, static_cast<int>(groupIndex));
        const bool bThisGroupRefused = AnyDragStateRefusedForGroup(dragStates, static_cast<int>(groupIndex));
        for (std::size_t transformIndex = 0; transformIndex < group.transforms.size(); ++transformIndex) {
            if (bThisGroupDragging
                && AnyInstanceSoftHiddenThisFrame(dragStates, static_cast<int>(groupIndex), static_cast<int>(transformIndex)))
                continue;
            const Params::MarkerTransform& transform = group.transforms[transformIndex];
            const ImVec2 screenCenter = ProjectWorldToScreen(composite, view, transform.transform.positionX,
                                                             transform.transform.positionZ, regionOriginX, regionOriginY);
            ImU32 tint;
            // ARCH §19.18 — canonical priority, highest to lowest:
            if (bThisGroupDragging && bThisGroupRefused) {
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
            // Follow-up to BUGFIX_UniversalCoordinateConversionAndDragRewrite_UI — every drag gesture
            // active on THIS group draws its own ghost ring set (its own draggedTransformIndex's own
            // dot size, its own currentGhostPoints), not just the first-grabbed instance's. A
            // single-element `dragStates` reduces to exactly one iteration here, byte-identical to the
            // pre-widening single-state draw.
            for (const MarkerDragGestureState& dragState : dragStates) {
                if (!dragState.bActive || dragState.groupIndex != static_cast<int>(groupIndex)) continue;
                // STEP122: the ghost points belong to the same dragged transform (not the last
                // transform iterated above, which is out of scope here) — resolves the drag-group's
                // own dot size, consistent with the ghost being that same group's sibling orbit slots.
                // A missing dragged transform (out-of-range index) falls back to a synthetic
                // layerIndex=-1 transform, the exact "out of range" shape ManualMarkerDotRadius
                // already treats as scale 1.0.
                const Params::MarkerTransform* const draggedTransform = (dragState.draggedTransformIndex >= 0
                    && static_cast<std::size_t>(dragState.draggedTransformIndex) < group.transforms.size())
                    ? &group.transforms[static_cast<std::size_t>(dragState.draggedTransformIndex)] : nullptr;
                Params::MarkerTransform noDraggedTransformFallback;
                noDraggedTransformFallback.layerIndex = -1;
                const float ghostDotRadius = ManualMarkerDotRadius(markerLayers,
                    draggedTransform != nullptr ? *draggedTransform : noDraggedTransformFallback,
                    markerLinks, group.name, globalMarkerSettings);
                for (const Pipeline::WorldSymmetryOrbitPoint& ghost : dragState.currentGhostPoints) {
                    const ImVec2 ghostScreenCenter = ProjectWorldToScreen(composite, view, ghost.worldPositionX,
                                                                          ghost.worldPositionZ, regionOriginX, regionOriginY);
                    drawList.AddCircle(ghostScreenCenter, ghostDotRadius, ghostTint, 0, 2.0f);
                }
            }
        }
    }
    if (AnyDragStateActiveAndRefused(dragStates))
        ImGui::SetTooltip("Spawn count is fixed - drag limited.");
}

} // namespace Ui
} // namespace SanmapGen
