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
namespace {

// The whole procedural stack: the two-level list (MarkersTab_RuleLayers_UI.h). STEP80 moved the
// list mechanics, the buttons and the layer-level symmetry into that file; STEP110 moved this
// file's own remaining per-rule detail sections (Gates/Quantity/Area/Focus/Placement Gate/
// Transform/Template Picker, `DrawRuleSettings`) into the INNER row body too, nested inside its own
// expanded rule-layer row (MarkersTab_RuleLayerSettings_UI.cpp / MarkersTab_RuleLayers_UI.cpp) — so
// this function is now just the Section wrapper around the two-level list.
void DrawRuleStack(Params::MapRecipe& recipe, MarkersTabState& state,
                   Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest) {
    if (!DrawSectionBegin("Procedural Rules", state.ruleStackSection)) return;
    DrawMarkerRuleLayerList(recipe.markerRuleLayers, state, previewDriver, iconManifest);
    DrawSectionEnd();
}

} // namespace

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
                    const Data::PlacementInstances* placedMarkers) {
    ImGui::PushID("markersTab");
    DrawMarkersTabGlobals(state.globals, iconManifest);
    DrawRuleStack(recipe, state, previewDriver, iconManifest);
    // STEP81: the Manual Marker Layers block, drawn BEFORE the manual roster below so a layer
    // added this frame is pickable by that roster's Layer combo on the same frame (STEP60 §3's
    // read-after-populate ordering, applied to authoring rather than import).
    DrawManualMarkerLayers(state.manualLayers, recipe.markerLayers, recipe.markers, recipe.geometry,
                          recipe.globalSymmetryMask, recipe.radialSymmetryRepeatCount,
                          recipe.markerSymmetryFixSettings);
    // STEP49: the hand-authored roster. `DrawManualMarkers` takes no map-size parameter, so the
    // caller resolves the X/Z slider bounds from `recipe.geometry.mapSize` into the state each
    // frame (MarkersTab_Manual_UI.h).
    state.manual.positionHorizontalRange = MarkerPositionHorizontalSliderRange(recipe.geometry.mapSize);
    DrawManualMarkers(recipe.markers, recipe.armies, recipe.markerLayers, state.manual,
                      state.manualLayers.selectedLayerIndex);
    DrawPlacedMarkerList(placedMarkers, state.placedList);
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
