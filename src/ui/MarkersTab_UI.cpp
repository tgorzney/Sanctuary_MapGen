// MarkersTab_UI.cpp — the imgui composition of the marker-rule tab. Layer: UI.
// Shared widgets only: VirtualList for the rule list, RangeSlider/LabelledDial for every scalar,
// IconGrid for the picker. No ImGui::SliderFloat/DragFloat/VSliderFloat in this file.
#include "MarkersTab_UI.h"
#include "VirtualListWidget_UI.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../pipeline/PreviewDriver_PIPELINE.h"
#include "imgui.h"
#include <cstdio>

namespace SanmapGen {
namespace Ui {
namespace {

// The ONE thing a tab does with a commit. WHICH tier it becomes is the driver's derivation from
// the stage parameter hashes, never this call site's decision.
void NotifyChange(bool bCommitted, Pipeline::PreviewDriver* previewDriver) {
    if (bCommitted && previewDriver != nullptr) previewDriver->NotifyParametersChanged();
}

// Checkboxes and dropdowns have no shared-library equivalent to compose from (M5-1/2/3 built
// scalars, ranges, lists, ramps, icon grids). Neither has a drag to defer: both commit at once.
void DrawBooleanSetting(const char* label, bool& value, Pipeline::PreviewDriver* previewDriver) {
    bool bValue = value;
    if (!ImGui::Checkbox(label, &bValue)) return;
    value = bValue;
    NotifyChange(true, previewDriver);
}

const char* const categoryNames[] = { "Generic", "Spawn", "Alloys", "Expansion" };

// The rule list. Selecting a row reloads the mirrors, so the detail controls below always show
// the rule the list highlights.
void DrawRuleList(std::vector<Params::MarkerRule>& markerRules, MarkersTabState& state,
                  Pipeline::PreviewDriver* previewDriver) {
    char rowLabel[40] = { 0 };
    VirtualList<Params::MarkerRule>::Render(
        "markerRules", markerRules, state.ruleRowHeight, state.ruleListHeight,
        [&](int rowIndex, const Params::MarkerRule& rule) {
            ImGui::PushID(rowIndex);
            DrawBooleanSetting("##enabled", markerRules[static_cast<std::size_t>(rowIndex)].bEnabled,
                               previewDriver);
            ImGui::SameLine();
            std::snprintf(rowLabel, sizeof(rowLabel), "%d: %s x%d", rowIndex,
                          categoryNames[static_cast<int>(rule.category)], rule.count);
            if (ImGui::Selectable(rowLabel, rowIndex == state.selectedRuleIndex)) {
                state.selectedRuleIndex = rowIndex;
                LoadMarkerRuleValues(rule, state);
            }
            ImGui::PopID();
        });
}

// Add / remove, applied AFTER the list is drawn so the vector never moves under the clipper.
void DrawRuleListButtons(std::vector<Params::MarkerRule>& markerRules, MarkersTabState& state,
                         Pipeline::PreviewDriver* previewDriver) {
    bool bRecipeMoved = false;
    if (ImGui::Button("Add Rule")) { markerRules.push_back(Params::MarkerRule()); bRecipeMoved = true; }
    ImGui::SameLine();
    if (ImGui::Button("Remove Selected") && SelectedMarkerRule(markerRules, state) != nullptr) {
        markerRules.erase(markerRules.begin() + state.selectedRuleIndex);
        bRecipeMoved = true;
    }
    if (bRecipeMoved) state.selectedRuleIndex = static_cast<int>(markerRules.size()) - 1;
    NotifyChange(bRecipeMoved, previewDriver);
}

// The gates: what terrain a marker may land on, and how many land.
void DrawRuleGates(Params::MarkerRule& rule, MarkersTabState& state,
                   Pipeline::PreviewDriver* previewDriver) {
    int categoryIndex = static_cast<int>(rule.category);
    if (ImGui::Combo("Category", &categoryIndex, categoryNames, IM_ARRAYSIZE(categoryNames))) {
        rule.category = static_cast<Params::MarkerCategory>(categoryIndex);
        NotifyChange(true, previewDriver);
    }
    DrawBooleanSetting("Hidden (still generated for clearance/fairness)", rule.bHidden, previewDriver);
    WidgetChange change = DrawRangeSlider("Slope Gate (degrees)", state.slopeValues, state.slopeBounds,
                                          state.slopeToggle, WidgetStyle(), "%.1f");
    if (change.bValueChanged) StoreMarkerRuleValues(state, rule);
    NotifyChange(change.bCommitted, previewDriver);
    change = DrawRangeSlider("Height Gate (normalized)", state.heightValues, state.heightBounds,
                             state.heightToggle);
    if (change.bValueChanged) StoreMarkerRuleValues(state, rule);
    NotifyChange(change.bCommitted, previewDriver);
    NotifyChange(DrawLabelledDial("Obstacle Distance Minimum", rule.obstacleDistanceMinimum,
                                  state.obstacleDistanceRange, state.obstacleDistanceToggle,
                                  WidgetStyle(), "%.2f").bCommitted, previewDriver);
}

void DrawRuleQuantity(Params::MarkerRule& rule, MarkersTabState& state,
                      Pipeline::PreviewDriver* previewDriver) {
    DrawBooleanSetting("Use Density (off = fixed count)", rule.bUseDensity, previewDriver);
    WidgetChange change = DrawLabelledDial("Count", state.countValue, state.countRange,
                                           state.countToggle, WidgetStyle(), "%.0f");
    if (change.bValueChanged) StoreMarkerRuleValues(state, rule);
    NotifyChange(change.bCommitted, previewDriver);
    NotifyChange(DrawLabelledDial("Density", rule.density, state.densityRange, state.densityToggle,
                                  WidgetStyle(), "%.4f").bCommitted, previewDriver);
    NotifyChange(DrawLabelledDial("Clearance Spacing", rule.clearanceSpacing, state.clearanceSpacingRange,
                                  state.clearanceSpacingToggle, WidgetStyle(), "%.2f").bCommitted, previewDriver);
    DrawBooleanSetting("Use Global Symmetry", rule.bSymmetryUseGlobal, previewDriver);
}

// The template (`tpId`) the rule spawns: typed here, browsed in the resident atlas beside it.
void DrawTemplatePicker(Params::MarkerRule& rule, MarkersTabState& state,
                        Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest) {
    // The field writes the tpId live and commits when it is left — the RT-toggle contract, in the
    // one control the library has no widget for.
    ImGui::InputText("Template Id (tpId)", rule.transform.templateIdentifier,
                     IM_ARRAYSIZE(rule.transform.templateIdentifier));
    NotifyChange(ImGui::IsItemDeactivatedAfterEdit(), previewDriver);
    if (iconManifest == nullptr) {
        ImGui::TextUnformatted("No resident icon atlas: type the template id above.");
        return;
    }
    DrawIconGrid("Template Atlas", *iconManifest, state.iconGridState, state.iconGridHeight);
    ImGui::Text("Selected icon id: %d", state.iconGridState.selectedIconId);
}

} // namespace

Params::MarkerRule* SelectedMarkerRule(std::vector<Params::MarkerRule>& markerRules,
                                       const MarkersTabState& state) {
    if (state.selectedRuleIndex < 0 || state.selectedRuleIndex >= static_cast<int>(markerRules.size()))
        return nullptr;
    return &markerRules[static_cast<std::size_t>(state.selectedRuleIndex)];
}

void DrawMarkersTab(Params::MapRecipe& recipe, MarkersTabState& state,
                    Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest) {
    ImGui::PushID("markersTab");
    DrawRuleList(recipe.markerRules, state, previewDriver);
    DrawRuleListButtons(recipe.markerRules, state, previewDriver);
    ImGui::Separator();
    Params::MarkerRule* const rule = SelectedMarkerRule(recipe.markerRules, state);
    if (rule == nullptr) {
        ImGui::TextUnformatted("Select a marker rule to edit its gates.");
        ImGui::PopID();
        return;
    }
    if (!state.slopeToggle.IsCommitDeferred() && !state.heightToggle.IsCommitDeferred()
        && !state.countToggle.IsCommitDeferred()) LoadMarkerRuleValues(*rule, state);
    DrawRuleGates(*rule, state, previewDriver);
    DrawRuleQuantity(*rule, state, previewDriver);
    DrawTemplatePicker(*rule, state, previewDriver, iconManifest);
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
