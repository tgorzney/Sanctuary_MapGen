// Application_DeleteKey_UI.cpp — the global Delete-key shortcut (STEP234,
// DESIGN_MarkerLink_R1.md §1.3). `Application::ApplyGlobalDeleteShortcut` itself is a thin
// imgui/Application-state glue over the pure logic in Application_DeleteKey_UI.h.
#include "Application_UI.h"
#include "Application_DeleteKey_UI.h"
#include "ManualInstanceDelete_UI.h"
#include <imgui.h>

namespace SanmapGen {
namespace Ui {

bool DeleteSelectedManualInstancesAcrossDomains(
    const OverlayInstanceKeySet_UI& selected,
    std::vector<Params::MarkerInstanceGroup>& markers,
    const std::vector<Params::MarkerInstanceLayer>& markerLayers,
    const std::vector<Params::MarkerLink>& markerLinks,
    std::vector<Params::PropInstanceGroup>& props,
    const std::vector<Params::PropInstanceLayer>& propLayers,
    std::vector<Params::DecalInstanceGroup>& decals,
    const std::vector<Params::DecalInstanceLayer>& decalLayers) {
    std::vector<int> markerIdentifiers, propIdentifiers, decalIdentifiers;
    for (const OverlayInstanceKey_UI& key : selected.keys) {
        if (!key.bManual) continue;   // procedural instances have no persisted identity — §0's corollary
        switch (key.collection) {
            case PlacementCollectionKind_UI::Markers: markerIdentifiers.push_back(key.instanceIndex); break;
            case PlacementCollectionKind_UI::Props:   propIdentifiers.push_back(key.instanceIndex);   break;
            case PlacementCollectionKind_UI::Decals:  decalIdentifiers.push_back(key.instanceIndex);  break;
            default: break;   // Units — out of scope, mirrors §21's own closing note
        }
    }
    bool bAnyDeleted = false;
    if (!markerIdentifiers.empty())
        bAnyDeleted |= DeleteSelectedManualMarkerInstances(markers, markerIdentifiers, markerLayers, markerLinks) > 0;
    if (!propIdentifiers.empty())
        bAnyDeleted |= DeleteSelectedManualPropInstances(props, propIdentifiers, propLayers) > 0;
    if (!decalIdentifiers.empty())
        bAnyDeleted |= DeleteSelectedManualDecalInstances(decals, decalIdentifiers, decalLayers) > 0;
    return bAnyDeleted;
}

void Application::ApplyGlobalDeleteShortcut() {
    if (!ShouldApplyGlobalDeleteShortcut(ImGui::GetIO().WantTextInput, scenarioEditMode.IsActive(),
                                         ImGui::IsKeyPressed(ImGuiKey_Delete)))
        return;

    const bool bAnyDeleted = DeleteSelectedManualInstancesAcrossDomains(
        canvas.SelectedInstanceKeys(), recipe.markers, recipe.markerLayers, recipe.markerLinks,
        recipe.props, recipe.propLayers, recipe.decals, recipe.decalLayers);
    if (!bAnyDeleted) return;

    canvas.ClearSelection();   // the staleness hazard DESIGN_Assembly_R1.md §2 already flagged for a
                               // held index-based selection surviving a structural mutation underneath it
    tabState.markers.selectedManualInstanceIdentifier = -1;             // Markers' own tab-local
    tabState.markers.selectedManualInstanceIdentifiers.clear();         // mirror — Props/Decals have
    tabState.markers.manualInstanceSelectionAnchorIdentifier = -1;      // no such mirror to clear
    previewDriver.NotifyParametersChanged();
}

} // namespace Ui
} // namespace SanmapGen
