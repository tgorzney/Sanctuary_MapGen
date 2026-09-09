// MarkersTab_TypeSectionBody_UI.cpp — see MarkersTab_TypeSectionBody_UI.h.
#include "MarkersTab_TypeSectionBody_UI.h"
#include "MarkersTab_BaseInstanceList_UI.h"
#include "MarkersTab_Links_UI.h"
#include "MarkersTab_TypeSectionAddActions_UI.h"
#include "MarkersTab_TypeSectionHeaderButtons_UI.h"
#include "MarkersTab_TypeSectionPendingApply_UI.h"
#include "MarkersTab_UI.h"
#include "PlacementRuleSections_UI.h"
#include "imgui.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../pipeline/PreviewDriver_PIPELINE.h"

namespace SanmapGen {
namespace Ui {

void DrawMarkerTypeSection(Params::MapRecipe& recipe, MarkersTabState& state,
                           Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest,
                           const IconAtlasPairingLookup* pairingLookup,
                           const Data::PlacementInstances* placedMarkers, int rowIndex, const char* typeName,
                           const std::function<void(int clickedInstanceIdentifier,
                                                    const std::vector<int>& selectedInstanceIdentifiers)>&
                               selectManualMarkerInstanceCallback,
                           const std::function<void(int, bool bCtrlHeld, bool bShiftHeld)>&
                               selectProceduralMarkerInstanceCallback) {
    ImGui::PushID(typeName);
    // STEP133 — a right-aligned Hide/Unhide button, per Type-section header, toggling that
    // Type's markers off the map preview entirely (both manual and procedural). STEP135 widens
    // the same reserved-right-zone mechanism to the "+ Instance"/"+ Group"/"+ Layer" buttons the
    // human asked for, in that order, immediately to the LEFT of Hide/Unhide. STEP136 widens it
    // again, one tier further left, for the relocated per-Type marker-settings row (icon / icon
    // color / select color / global scale), formerly the Global section's own stacked rows.
    const bool bHidden = state.markerTypeVisibility.IsHidden(typeName);
    if (DrawSectionBegin(typeName, state.typeSections.stateByTypeName[typeName].outerSection,
                         HeaderButtonsSectionOptions(state.globals, bHidden))) {
        ImGui::SameLine();
        const TypeSectionHeaderButtons_UI buttons = DrawRightAlignedTypeSectionHeaderButtons(
            state.globals, rowIndex, recipe.globalMarkerSettings, iconManifest, pairingLookup, bHidden,
            !state.selectedManualInstanceIdentifiers.empty());

        if (buttons.bAddInstanceClicked)
            ApplyAddInstanceButtonAction(recipe, state, typeName);
        if (buttons.bAddGroupClicked)
            ApplyAddGroupButtonAction(recipe, state, typeName);
        if (buttons.bAddManualLayerClicked)
            ApplyAddManualLayerButtonAction(recipe, state, typeName);
        // STEP239 — "+Link": always the WHOLE tab-wide selection (§3.6), never scoped to this
        // one Type-section's own copy of the button. ApplyAddLinkAction is the pure, directly
        // testable composed action (MarkersTab_Links_UI.h) — mint the Link, partition the
        // selection by type, create one root-scoped Group+Layer per represented type (both
        // tagged with the new Link's identifier), reassign that type's instances onto it.
        if (buttons.bAddLinkClicked)
            ApplyAddLinkAction(recipe, state.selectedManualInstanceIdentifiers);
        bool bRecipeMoved = false;
        if (buttons.bAddProceduralLayerClicked)
            bRecipeMoved = ApplyAddProceduralLayerButtonAction(recipe, state, typeName);

        // STEP138 — the actual Group -> Layer -> Instance hierarchy the header's own buttons now
        // populate: the Bundle (Group) tree first (each Group's own Layers and their Instances
        // draw nested/indented inside it, DrawMarkerLayerBundleTree/DrawLayerRowBody), then this
        // Type's own UNGROUPED Layers (root `parentBundleIdentifier == -1`), then the base-section
        // Instance list — the "no Layer at all" case (see DrawBaseSectionManualInstanceList).
        DrawMarkerLayerBundleTree(recipe.markerLayerBundles, recipe.markerRuleLayers, recipe.markerLayers,
                                  recipe.markers, recipe.geometry, recipe.globalSymmetryMask,
                                  recipe.radialSymmetryRepeatCount, recipe.markerSymmetryFixSettings,
                                  state.bundles, state, previewDriver, iconManifest, typeName,
                                  selectManualMarkerInstanceCallback, recipe.markerLinks);

        // STEP140 — the tree's own header-extra "X" only RECORDS a pending choice (mutating
        // bundles/ruleLayers/instanceLayers mid-walk would desync the walk's own position-based
        // lookups for the rest of this frame); apply it now the walk above is fully done.
        ApplyPendingBundleDelete(state.bundles, recipe.markerLayerBundles, recipe.markerRuleLayers,
                                 recipe.markerLayers, recipe.markers);
        ApplyPendingManualLayerDelete(state.bundles, state.manualLayers.selectedLayerIndex,
                                      recipe.markerLayers, recipe.markers);
        bRecipeMoved = ApplyPendingProceduralLayerDelete(state.bundles, state.selectedRuleLayerIndex,
                                                         state.selectedRuleIndex, recipe.markerRuleLayers)
                     || bRecipeMoved;
        // STEP148 correction (human's own correction — "I thought I told you to have it create a
        // new layer if one did not exist") — a Group's own drop target records this instead of
        // reassigning immediately whenever it has no Manual Layer yet (structural push_back to
        // recipe.markerLayers, unsafe mid-walk, same reasoning as the pending-deletes above);
        // create the Layer AND reassign the recorded instances in one atomic step, now the walk
        // is fully done.
        ApplyPendingCreateLayerForBundleIfRequested(state.bundles, recipe.markerLayers, recipe.markers);

        ImGui::Separator();
        bRecipeMoved = DrawRuleLayerListBody(recipe.markerRuleLayers, state, previewDriver, iconManifest,
                                             typeName, placedMarkers, selectProceduralMarkerInstanceCallback)
                     || bRecipeMoved;
        DrawManualMarkerLayerListBody(state.manualLayers, recipe.markerLayers, recipe.markers,
                                      recipe.geometry, recipe.globalSymmetryMask,
                                      recipe.radialSymmetryRepeatCount, recipe.markerSymmetryFixSettings,
                                      typeName, state.selectedManualInstanceIdentifier,
                                      state.selectedManualInstanceIdentifiers,
                                      state.manualInstanceSelectionAnchorIdentifier,
                                      selectManualMarkerInstanceCallback, recipe.markerLinks);
        ImGui::Separator();
        DrawBaseSectionManualInstanceList(recipe.markers, recipe.markerLayers, typeName,
                                          state.selectedManualInstanceIdentifier,
                                          state.selectedManualInstanceIdentifiers,
                                          state.manualInstanceSelectionAnchorIdentifier,
                                          selectManualMarkerInstanceCallback);

        NotifyPlacementChange(bRecipeMoved, previewDriver);

        if (buttons.bHideToggleClicked)
            state.markerTypeVisibility.SetHidden(typeName, !bHidden);
        DrawSectionEnd();
    }
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
