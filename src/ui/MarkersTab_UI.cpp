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
                    const Data::PlacementInstances* placedMarkers) {
    ImGui::PushID("markersTab");
    DrawMarkersTabGlobals(state.globals, recipe.globalMarkerSettings, iconManifest, pairingLookup);
    // STEP125: replaces the old flat DrawMarkerLayerBundleTree/DrawRuleStack/DrawManualMarkerLayers
    // trio with the dynamic Type-section outer loop (ARCH §19.14/§19.15) — one collapsible Section
    // per distinct markerTypeName, each containing its own type-filtered Bundle tree and the two
    // type-filtered "Ungrouped ..." lists (MarkersTab_TypeSections_UI.h).
    DrawMarkerTypeSections(recipe, state, previewDriver, iconManifest);
    // STEP49: the hand-authored roster. `DrawManualMarkers` takes no map-size parameter, so the
    // caller resolves the X/Z slider bounds from `recipe.geometry.mapSize` into the state each
    // frame (MarkersTab_Manual_UI.h).
    state.manual.positionHorizontalRange = MarkerPositionHorizontalSliderRange(recipe.geometry.mapSize);
    DrawManualMarkers(recipe.markers, recipe.armies, recipe.markerLayers, state.manual,
                      state.manualLayers.selectedLayerIndex, iconManifest);
    DrawPlacedMarkerList(placedMarkers, state.placedList);
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
