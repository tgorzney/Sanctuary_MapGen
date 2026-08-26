// MarkersTab_UI.cpp — the imgui composition of the marker tab. Layer: UI.
// Shared widgets only: DraggableList (via MarkersTab_RuleLayers_UI) for the procedural rule stack,
// VirtualList for the placed markers, IconGrid for the pickers, Section/Checkbox/Combo/RangeSlider/
// Dial for the scalars. No ImGui::SliderFloat / DragFloat / VSliderFloat in this file.
#include "MarkersTab_UI.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../pipeline/PreviewDriver_PIPELINE.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {

// The rule the detail controls edit: a two-index walk, both bounds-checked, null on either miss
// (STEP80, mirroring `SelectedLayer`, LayersTab_UI.cpp:120-127).
Params::MarkerRule* SelectedMarkerRule(std::vector<Params::MarkerRuleLayer>& markerRuleLayers,
                                       const MarkersTabState& state) {
    Params::MarkerRuleLayer* const layer = SelectedMarkerRuleLayer(markerRuleLayers, state);
    if (layer == nullptr) return nullptr;
    if (state.selectedRuleIndex < 0
        || state.selectedRuleIndex >= static_cast<int>(layer->rules.size())) return nullptr;
    return &layer->rules[static_cast<std::size_t>(state.selectedRuleIndex)];
}

void DrawMarkersTab(Params::MapRecipe& recipe, MarkersTabState& state,
                    Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest,
                    const IconAtlasPairingLookup* pairingLookup,
                    const Data::PlacementInstances* placedMarkers,
                    const std::function<void(int)>& selectManualMarkerInstanceCallback,
                    const std::function<void(int)>& selectProceduralMarkerInstanceCallback) {
    ImGui::PushID("markersTab");
    DrawMarkersTabGlobals(state.globals, recipe.globalMarkerSettings, iconManifest, pairingLookup);
    // STEP125: replaces the old flat DrawMarkerLayerBundleTree/DrawRuleStack/DrawManualMarkerLayers
    // trio with the dynamic Type-section outer loop (ARCH §19.14/§19.15) — one collapsible Section
    // per distinct markerTypeName, each containing its own type-filtered Bundle tree and the two
    // type-filtered "Ungrouped ..." lists (MarkersTab_TypeSections_UI.h). STEP132: also threads
    // `placedMarkers`/`selectProceduralMarkerInstanceCallback` down to the Rule layer list's own
    // per-Rule instance list (ARCH §19.27) — the SAME pointer DrawPlacedMarkerList below already reads.
    DrawMarkerTypeSections(recipe, state, previewDriver, iconManifest, selectManualMarkerInstanceCallback,
                           placedMarkers, selectProceduralMarkerInstanceCallback);
    // Human's own instruction: no separate "Manual Markers"/"Placed Markers" top-level sections --
    // Manual and Procedural are Layer TYPES within each Alloy/Plasma/Spawn Type-section, not their
    // own tab-level zones. The read-only "Placed Markers" preview (DrawPlacedMarkerList) is fully
    // superseded by STEP132's own per-Rule instance list inside the Type-section hierarchy above,
    // so it is removed outright. `DrawManualMarkers` (STEP49) is NOT removed here: it is still the
    // ONLY authoring surface for adding/deleting a manual marker, editing its position/alias/army
    // assignment, and picking its Layer -- the new per-Layer instance list (DrawLayerRowBody) is
    // view+select only, with no Add/Delete/position-edit affordance of its own. Removing this call
    // would break marker authoring entirely with nothing built yet to replace it -- flagged back to
    // the human rather than silently deleted.
    state.manual.positionHorizontalRange = MarkerPositionHorizontalSliderRange(recipe.geometry.mapSize);
    DrawManualMarkers(recipe.markers, recipe.armies, recipe.markerLayers, state.manual,
                      state.manualLayers.selectedLayerIndex, iconManifest);
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
