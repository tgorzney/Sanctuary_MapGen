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

// STEP133 — the gap between the Type-section header's own drawn content and the Hide/Unhide button
// composed into its reserved right edge (Constitution §8: a named constant, never a bare literal at
// the call site), mirroring HeightmapTab_UI.cpp's kGeoLayerAddButtonSpacingPixels precedent exactly.
constexpr float kHideToggleButtonSpacingPixels = 8.0f;

// The reserved-right-width `SectionOptions` for one Type-section header: exactly the CURRENT label's
// ("Hide" or the wider "Unhide") own measured width plus the fixed spacing above, mirroring
// HeightmapTab_UI.cpp's GeoLayerSectionOptions exactly.
SectionOptions HideToggleSectionOptions(bool bHidden) {
    SectionOptions options;
    const float buttonWidth = ImGui::CalcTextSize(bHidden ? "Unhide" : "Hide").x
                             + ImGui::GetStyle().FramePadding.x * 2.0f;
    options.reservedRightWidth = buttonWidth + kHideToggleButtonSpacingPixels;
    return options;
}

// Right-aligns the Hide/Unhide SmallButton within the header's own reserved right zone: its right
// edge lands at the same X regardless of which label ("Hide" vs. the wider "Unhide") is current,
// rather than sitting flush-left of the reserved zone the way SameLine() alone would leave it
// (HeightmapTab_UI.cpp's single-label "Add GeoLayer" precedent never needed this). Must be called
// immediately after ImGui::SameLine(), with the reserved zone the ONLY content still ahead of the
// cursor on this line — GetContentRegionAvail().x is exactly that zone's remaining width, and
// DrawSectionBegin sized `barWidth` so this button's own right edge always lands at the header's own
// full right edge, independent of which label reserved the zone this frame.
bool DrawRightAlignedHideToggleButton(bool bHidden) {
    const char* label = bHidden ? "Unhide" : "Hide";
    const float buttonWidth = ImGui::CalcTextSize(label).x + ImGui::GetStyle().FramePadding.x * 2.0f;
    const float availableWidth = ImGui::GetContentRegionAvail().x;
    if (availableWidth > buttonWidth)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availableWidth - buttonWidth);
    return ImGui::SmallButton(label);
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
        // STEP133 — a right-aligned Hide/Unhide button, per Type-section header, toggling that
        // Type's markers off the map preview entirely (both manual and procedural).
        const bool bHidden = state.markerTypeVisibility.IsHidden(typeName);
        if (DrawSectionBegin(typeName, state.typeSections.stateByTypeName[typeName].outerSection,
                             HideToggleSectionOptions(bHidden))) {
            ImGui::SameLine();
            if (DrawRightAlignedHideToggleButton(bHidden))
                state.markerTypeVisibility.SetHidden(typeName, !bHidden);
            DrawSectionEnd();
        }
        ImGui::PopID();
    }
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
