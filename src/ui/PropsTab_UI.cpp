// PropsTab_UI.cpp — the imgui composition of the prop-scatter tab. Layer: UI.
// Shared widgets only: VirtualList for the rule list, RangeSlider/LabelledDial for every scalar,
// IconGrid for the picker. No ImGui::SliderFloat/DragFloat/VSliderFloat in this file.
#include "PropsTab_UI.h"
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

// The checkbox the shared library has no widget for. No drag to defer: it commits at once.
void DrawBooleanSetting(const char* label, bool& value, Pipeline::PreviewDriver* previewDriver) {
    bool bValue = value;
    if (!ImGui::Checkbox(label, &bValue)) return;
    value = bValue;
    NotifyChange(true, previewDriver);
}

// The rule list. Selecting a row reloads the mirrors, so the controls below always show the rule
// the list highlights.
void DrawRuleList(std::vector<Params::PropRule>& propRules, PropsTabState& state,
                  Pipeline::PreviewDriver* previewDriver) {
    char rowLabel[40] = { 0 };
    VirtualList<Params::PropRule>::Render(
        "propRules", propRules, state.ruleRowHeight, state.ruleListHeight,
        [&](int rowIndex, const Params::PropRule& rule) {
            ImGui::PushID(rowIndex);
            DrawBooleanSetting("##enabled", propRules[static_cast<std::size_t>(rowIndex)].bEnabled,
                               previewDriver);
            ImGui::SameLine();
            // %.7s: the tpId is a fixed 8-byte field whose last byte need not be a terminator.
            std::snprintf(rowLabel, sizeof(rowLabel), "%d: %.7s density %.3f", rowIndex,
                          rule.transform.templateIdentifier, rule.density);
            if (ImGui::Selectable(rowLabel, rowIndex == state.selectedRuleIndex)) {
                state.selectedRuleIndex = rowIndex;
                LoadPropRuleValues(rule, state);
            }
            ImGui::PopID();
        });
}

// Add / remove, applied AFTER the list is drawn so the vector never moves under the clipper.
void DrawRuleListButtons(std::vector<Params::PropRule>& propRules, PropsTabState& state,
                         Pipeline::PreviewDriver* previewDriver) {
    bool bRecipeMoved = false;
    if (ImGui::Button("Add Rule")) { propRules.push_back(Params::PropRule()); bRecipeMoved = true; }
    ImGui::SameLine();
    if (ImGui::Button("Remove Selected") && SelectedPropRule(propRules, state) != nullptr) {
        propRules.erase(propRules.begin() + state.selectedRuleIndex);
        bRecipeMoved = true;
    }
    if (bRecipeMoved) state.selectedRuleIndex = static_cast<int>(propRules.size()) - 1;
    NotifyChange(bRecipeMoved, previewDriver);
}

// What terrain a prop may land on, and how densely it lands.
void DrawRuleGates(Params::PropRule& rule, PropsTabState& state,
                   Pipeline::PreviewDriver* previewDriver) {
    WidgetChange change = DrawRangeSlider("Slope Gate (degrees)", state.slopeValues, state.slopeBounds,
                                          state.slopeToggle, WidgetStyle(), "%.1f");
    if (change.bValueChanged) StorePropRuleValues(state, rule);
    NotifyChange(change.bCommitted, previewDriver);
    change = DrawRangeSlider("Height Gate (normalized)", state.heightValues, state.heightBounds,
                             state.heightToggle);
    if (change.bValueChanged) StorePropRuleValues(state, rule);
    NotifyChange(change.bCommitted, previewDriver);
    NotifyChange(DrawLabelledDial("Density", rule.density, state.densityRange, state.densityToggle,
                                  WidgetStyle(), "%.4f").bCommitted, previewDriver);
    NotifyChange(DrawLabelledDial("Spacing Minimum (cells)", rule.spacingMinimum, state.spacingRange,
                                  state.spacingToggle, WidgetStyle(), "%.2f").bCommitted, previewDriver);
    NotifyChange(DrawLabelledDial("Obstacle Distance Minimum", rule.obstacleDistanceMinimum,
                                  state.obstacleDistanceRange, state.obstacleDistanceToggle,
                                  WidgetStyle(), "%.2f").bCommitted, previewDriver);
}

// The water / cliff affinities and the symmetry source.
void DrawRuleAffinities(Params::PropRule& rule, PropsTabState& state,
                        Pipeline::PreviewDriver* previewDriver) {
    DrawBooleanSetting("Avoid Water", rule.bAvoidWater, previewDriver);
    DrawBooleanSetting("Near Cliffs Only", rule.bNearCliffs, previewDriver);
    NotifyChange(DrawLabelledDial("Near Cliff Distance Maximum", rule.nearCliffDistanceMaximum,
                                  state.nearCliffDistanceRange, state.nearCliffDistanceToggle,
                                  WidgetStyle(), "%.2f").bCommitted, previewDriver);
    DrawBooleanSetting("Use Global Symmetry", rule.bSymmetryUseGlobal, previewDriver);
}

// The template (`tpId`) the rule scatters: typed here, browsed in the resident atlas beside it.
void DrawTemplatePicker(Params::PropRule& rule, PropsTabState& state,
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

Params::PropRule* SelectedPropRule(std::vector<Params::PropRule>& propRules,
                                   const PropsTabState& state) {
    if (state.selectedRuleIndex < 0 || state.selectedRuleIndex >= static_cast<int>(propRules.size()))
        return nullptr;
    return &propRules[static_cast<std::size_t>(state.selectedRuleIndex)];
}

void DrawPropsTab(Params::MapRecipe& recipe, PropsTabState& state,
                  Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest) {
    ImGui::PushID("propsTab");
    DrawRuleList(recipe.propRules, state, previewDriver);
    DrawRuleListButtons(recipe.propRules, state, previewDriver);
    ImGui::Separator();
    Params::PropRule* const rule = SelectedPropRule(recipe.propRules, state);
    if (rule == nullptr) {
        ImGui::TextUnformatted("Select a prop rule to edit its gates.");
        ImGui::PopID();
        return;
    }
    if (!state.slopeToggle.IsCommitDeferred() && !state.heightToggle.IsCommitDeferred())
        LoadPropRuleValues(*rule, state);
    DrawRuleGates(*rule, state, previewDriver);
    DrawRuleAffinities(*rule, state, previewDriver);
    DrawTemplatePicker(*rule, state, previewDriver, iconManifest);
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
