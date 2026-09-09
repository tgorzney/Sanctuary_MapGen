// MarkersTab_TypeSectionAddActions_UI.cpp — see MarkersTab_TypeSectionAddActions_UI.h. The 3
// pure-mechanics helpers these 4 handlers share now live in the sibling
// MarkersTab_TypeSectionAddHelpers_UI.h/.cpp (this ticket's own §1 fallback split).
#include "MarkersTab_TypeSectionAddActions_UI.h"
#include "MarkerInstanceCreateSymmetric_UI.h"
#include "MarkerLayerId_UI.h"
#include "MarkersTab_ManualInstanceSelection_UI.h"
#include "MarkersTab_ManualLayerHelpers_UI.h"
#include "MarkersTab_TypeSectionAddHelpers_UI.h"
#include "MarkersTab_UI.h"
#include "../params/Geometry_PARAMS.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Ui {

void ApplyAddInstanceButtonAction(Params::MapRecipe& recipe, MarkersTabState& state,
                                  const std::string& typeName) {
    // Human's own bug report — "When creating an Instance, symmetry needs to be checked
    // and duplicates created for proper symmetry": a plain single push_back never
    // consulted symmetry at all. CreateSymmetricManualMarkerInstances resolves the target
    // Layer's own effective mask/count and materializes every resulting orbit point (a
    // 1-point orbit — symmetry off — still creates exactly the one instance, unchanged
    // behavior for that case).
    //
    // Human's own follow-up report — "symmetry duplicates are not created": the map's own
    // dead CENTER (MapCenterWorldUnits, this button's own default spawn X/Z) is a FIXED
    // POINT under every one of the engine's symmetry kinds that passes through center —
    // RotateHalfTurn (the recipe's own default global mask), MirrorAcrossX, MirrorAcrossZ,
    // and Radial all map the center to itself — so the orbit collapsed to 1 point
    // regardless of the target layer's own symmetry settings. A small diagonal nudge off
    // BOTH axes keeps the default spawn point off every one of those fixed points at once
    // (kNewInstanceCenterOffsetWorldUnits: small enough to still read as "the center",
    // and a multiple of the default grid-snap size so a grid-snapped layer lands clean).
    constexpr float kNewInstanceCenterOffsetWorldUnits = 4.0f;
    Params::MarkerInstanceGroup& group =
        FindOrCreateMarkerInstanceGroupByName(recipe.markers, typeName);
    const float mapCenter = MapCenterWorldUnits(recipe.geometry);
    const float spawnCoordinate = mapCenter + kNewInstanceCenterOffsetWorldUnits;
    const int layerIndex = ResolveAddInstanceLayerIndex(
        recipe.markerLayers, state.manualLayers.selectedLayerIndex, typeName);
    state.selectedManualInstanceIdentifier = CreateSymmetricManualMarkerInstances(
        group, recipe.markers, recipe.markerLayers, recipe.geometry, recipe.globalSymmetryMask,
        recipe.radialSymmetryRepeatCount, layerIndex, spawnCoordinate, 0.0f, spawnCoordinate);
}

void ApplyAddGroupButtonAction(Params::MapRecipe& recipe, MarkersTabState& state,
                               const std::string& typeName) {
    Params::MarkerLayerBundle bundle;
    bundle.identifier = NextMarkerLayerBundleId(recipe.markerLayerBundles);
    bundle.markerTypeName = typeName;
    // Groups stay nestable (human's own confirmation) — a Group added while another
    // Group of this Type is selected nests under it, same "+ Layer" targeting rule below.
    bundle.parentBundleIdentifier = ResolveSelectedParentBundleIdentifier(
        recipe.markerLayerBundles, state.bundles.selectedBundleIdentifier, typeName);
    recipe.markerLayerBundles.push_back(bundle);
    state.bundles.selectedBundleIdentifier = bundle.identifier;
    // STEP235 — a same-type selection moves into the new Group's own first Manual Layer,
    // the SAME "mint a Layer + reassign" convention a drop onto a bare Group's own header
    // already uses (ApplyPendingCreateLayerForBundle, MarkersTab_BundleHeaderExtras_UI.cpp's
    // own pending-create path) — reused here, not reinvented. A mixed-type or empty
    // selection leaves the Group exactly as created above: empty, no Layer, no move
    // (DESIGN_MarkerLink_R1.md §2).
    if (IsManualInstanceSelectionEntirelyType(recipe.markers,
                                              state.selectedManualInstanceIdentifiers, typeName)) {
        ApplyPendingCreateLayerForBundle(bundle.identifier, typeName,
                                         state.selectedManualInstanceIdentifiers,
                                         recipe.markerLayers, recipe.markers);
    }
}

void ApplyAddManualLayerButtonAction(Params::MapRecipe& recipe, MarkersTabState& state,
                                     const std::string& typeName) {
    Params::MarkerInstanceLayer layer;
    layer.name     = NextMarkerLayerName(static_cast<int>(recipe.markerLayers.size()));
    layer.layerId  = NextMarkerLayerId(recipe.markerLayers);
    layer.parentBundleIdentifier = ResolveSelectedParentBundleIdentifier(
        recipe.markerLayerBundles, state.bundles.selectedBundleIdentifier, typeName);
    layer.markerTypeName = typeName;
    recipe.markerLayers.push_back(layer);
    state.manualLayers.selectedLayerIndex = static_cast<int>(recipe.markerLayers.size()) - 1;
    // STEP235 — a same-type selection reassigns directly onto the new Layer; mixed-type or
    // empty leaves it empty, exactly as created above (DESIGN_MarkerLink_R1.md §2).
    if (IsManualInstanceSelectionEntirelyType(recipe.markers,
                                              state.selectedManualInstanceIdentifiers, typeName)) {
        ReassignManualInstanceLayers(recipe.markers, state.selectedManualInstanceIdentifiers,
                                     state.manualLayers.selectedLayerIndex);
    }
}

bool ApplyAddProceduralLayerButtonAction(Params::MapRecipe& recipe, MarkersTabState& state,
                                         const std::string& typeName) {
    Params::MarkerRuleLayer layer;
    layer.parentBundleIdentifier = ResolveSelectedParentBundleIdentifier(
        recipe.markerLayerBundles, state.bundles.selectedBundleIdentifier, typeName);
    layer.markerTypeName = typeName;
    // STEP208 — without an initial rule, the newly created layer's row has zero rules and
    // renders no settings at all (DrawRuleLayerBody's DraggableList<MarkerRule> has nothing
    // to draw): seed the same plain default-constructed rule DrawMarkerRuleButtons's own
    // "Add Rule" button already pushes, so the layer is immediately editable.
    layer.rules.push_back(Params::MarkerRule());
    recipe.markerRuleLayers.push_back(layer);
    state.selectedRuleLayerIndex = static_cast<int>(recipe.markerRuleLayers.size()) - 1;
    state.selectedRuleIndex      = 0;
    return true;
}

} // namespace Ui
} // namespace SanmapGen
