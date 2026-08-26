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
    (void)previewDriver; (void)placedMarkers;
    (void)selectManualMarkerInstanceCallback; (void)selectProceduralMarkerInstanceCallback;
    ImGui::PushID("markersTab");
    DrawMarkersTabGlobals(state.globals, recipe.globalMarkerSettings, iconManifest, pairingLookup);
    // Human's own explicit instruction: strip the tab down to Global plus three EMPTY collapsible
    // sections (Alloy/Plasma/Spawn), nothing else, as a clean baseline to verify before anything
    // else is rebuilt on top. No Bundle tree, no Rule stack/Add Rule/Remove Rule, no Manual Markers,
    // no Placed Markers.
    for (const char* typeName : { "Alloy", "Plasma", "Spawn" }) {
        ImGui::PushID(typeName);
        if (DrawSectionBegin(typeName, state.typeSections.stateByTypeName[typeName].outerSection))
            DrawSectionEnd();
        ImGui::PopID();
    }
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
