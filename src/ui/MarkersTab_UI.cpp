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

// The whole procedural stack: the two-level list (MarkersTab_RuleLayers_UI.h) and the selected
// rule's detail sections. STEP80 moved the list mechanics, the buttons, and the layer-level
// symmetry out to that file — the ONE thing this file lost is the deleted per-rule
// DrawPlacementSymmetryAxes call, now `layer.symmetry.*` in the Selected Layer settings block.
void DrawRuleStack(Params::MapRecipe& recipe, MarkersTabState& state,
                   Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest) {
    if (!DrawSectionBegin("Procedural Rules", state.ruleStackSection)) return;
    DrawMarkerRuleLayerList(recipe.markerRuleLayers, state, previewDriver);
    ImGui::Separator();
    Params::MarkerRule* const rule = SelectedMarkerRule(recipe.markerRuleLayers, state);
    if (rule == nullptr) {
        ImGui::TextUnformatted("Select a marker rule to edit it.");
        DrawSectionEnd();
        return;
    }
    if (!state.slopeToggle.IsCommitDeferred() && !state.heightToggle.IsCommitDeferred()
        && !state.countToggle.IsCommitDeferred()) LoadMarkerRuleValues(*rule, state);
    DrawMarkerRuleGates(*rule, state, previewDriver);
    DrawMarkerRuleQuantity(*rule, state, previewDriver);
    DrawMarkerRuleArea(*rule, state.ruleDetail, previewDriver);
    DrawMarkerRuleFocus(*rule, state.ruleDetail, previewDriver);
    DrawPlacementGateSection(rule->maskStratumIndex, rule->maskWeightMinimum, rule->mapEdgePadding,
                             state.gate, previewDriver);
    DrawPlacementTransformSection(rule->transform, state.transform, previewDriver);
    DrawPlacementTemplatePicker(rule->transform, state.iconGridState, state.iconGridHeight,
                                iconManifest, previewDriver);
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
    // STEP49: the hand-authored roster. `DrawManualMarkers` takes no map-size parameter, so the
    // caller resolves the X/Z slider bounds from `recipe.geometry.mapSize` into the state each
    // frame (MarkersTab_Manual_UI.h).
    state.manual.positionHorizontalRange = MarkerPositionHorizontalSliderRange(recipe.geometry.mapSize);
    DrawManualMarkers(recipe.markers, recipe.armies, state.manual);
    DrawPlacedMarkerList(placedMarkers, state.placedList);
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
