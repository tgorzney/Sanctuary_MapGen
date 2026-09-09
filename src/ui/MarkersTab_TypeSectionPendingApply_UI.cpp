// MarkersTab_TypeSectionPendingApply_UI.cpp — see MarkersTab_TypeSectionPendingApply_UI.h.
#include "MarkersTab_TypeSectionPendingApply_UI.h"
#include "MarkersTab_BundleDelete_UI.h"
#include "MarkersTab_Bundles_UI.h"

namespace SanmapGen {
namespace Ui {

void ApplyPendingBundleDelete(MarkerLayerBundlesState& bundlesState,
                              std::vector<Params::MarkerLayerBundle>& bundles,
                              std::vector<Params::MarkerRuleLayer>& ruleLayers,
                              std::vector<Params::MarkerInstanceLayer>& markerLayers,
                              std::vector<Params::MarkerInstanceGroup>& markers) {
    // STEP140 — the tree's own header-extra "X" only RECORDS a pending choice (mutating
    // bundles/ruleLayers/instanceLayers mid-walk would desync the walk's own position-based
    // lookups for the rest of this frame); apply it now the walk above is fully done.
    if (bundlesState.pendingDeleteBundleIdentifier >= 0) {
        const int deletedBundleIdentifier = bundlesState.pendingDeleteBundleIdentifier;
        if (bundlesState.bPendingDeleteBundleCascade)
            DeleteMarkerLayerBundleCascade(deletedBundleIdentifier, bundles,
                                           ruleLayers, markerLayers, markers);
        else
            DeleteMarkerLayerBundleGroupOnly(deletedBundleIdentifier, bundles,
                                             ruleLayers, markerLayers);
        if (bundlesState.selectedBundleIdentifier == deletedBundleIdentifier)
            bundlesState.selectedBundleIdentifier = -1;
        bundlesState.pendingDeleteBundleIdentifier = -1;
    }
}

void ApplyPendingManualLayerDelete(MarkerLayerBundlesState& bundlesState, int& selectedManualLayerIndex,
                                   std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                   std::vector<Params::MarkerInstanceGroup>& markers) {
    if (bundlesState.pendingDeleteManualLayerIndex >= 0) {
        const int deletedLayerIndex = bundlesState.pendingDeleteManualLayerIndex;
        if (bundlesState.bPendingDeleteManualLayerCascade)
            DeleteMarkerInstanceLayerCascade(deletedLayerIndex, markerLayers, markers);
        else
            DeleteMarkerInstanceLayerOnly(deletedLayerIndex, markerLayers, markers);
        if (selectedManualLayerIndex == deletedLayerIndex)
            selectedManualLayerIndex = -1;
        else if (selectedManualLayerIndex > deletedLayerIndex)
            --selectedManualLayerIndex;
        bundlesState.pendingDeleteManualLayerIndex = -1;
    }
}

bool ApplyPendingProceduralLayerDelete(MarkerLayerBundlesState& bundlesState, int& selectedRuleLayerIndex,
                                       int& selectedRuleIndex,
                                       std::vector<Params::MarkerRuleLayer>& ruleLayers) {
    if (bundlesState.pendingDeleteProceduralLayerIndex >= 0) {
        const int deletedLayerIndex = bundlesState.pendingDeleteProceduralLayerIndex;
        DeleteMarkerRuleLayer(deletedLayerIndex, ruleLayers);
        if (selectedRuleLayerIndex == deletedLayerIndex) {
            selectedRuleLayerIndex = -1;
            selectedRuleIndex      = 0;
        } else if (selectedRuleLayerIndex > deletedLayerIndex) {
            --selectedRuleLayerIndex;
        }
        bundlesState.pendingDeleteProceduralLayerIndex = -1;
        return true;
    }
    return false;
}

void ApplyPendingCreateLayerForBundleIfRequested(MarkerLayerBundlesState& bundlesState,
                                                 std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                                 std::vector<Params::MarkerInstanceGroup>& markers) {
    // STEP148 correction (human's own correction — "I thought I told you to have it create a
    // new layer if one did not exist") — a Group's own drop target records this instead of
    // reassigning immediately whenever it has no Manual Layer yet (structural push_back to
    // recipe.markerLayers, unsafe mid-walk, same reasoning as the pending-deletes above);
    // create the Layer AND reassign the recorded instances in one atomic step, now the walk
    // is fully done.
    if (bundlesState.pendingCreateLayerForBundleIdentifier >= 0) {
        ApplyPendingCreateLayerForBundle(bundlesState.pendingCreateLayerForBundleIdentifier,
                                         bundlesState.pendingCreateLayerMarkerTypeName,
                                         bundlesState.pendingCreateLayerInstanceIdentifiers,
                                         markerLayers, markers);
        bundlesState.pendingCreateLayerForBundleIdentifier = -1;
        bundlesState.pendingCreateLayerMarkerTypeName.clear();
        bundlesState.pendingCreateLayerInstanceIdentifiers.clear();
    }
}

} // namespace Ui
} // namespace SanmapGen
