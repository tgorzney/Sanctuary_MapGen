// ArmiesTab_Units_UI.cpp — the imgui composition of one army's unit rules. Layer: UI.
// Shared widgets only: VirtualList over the filtered index list, RangeSlider / SliderScalar for
// the scalars, Section for the blocks, IconGrid for the Add Units picker.
#include "ArmiesTab_Units_UI.h"
#include "VirtualListWidget_UI.h"
#include "imgui.h"
#include <cstdio>

namespace SanmapGen {
namespace Ui {
namespace {

// The filtered list. Rows are the army's rules; the payload index is the RECIPE index, so a
// selection survives the filter being rebuilt next frame.
void DrawUnitRuleRows(const std::vector<Params::UnitRule>& unitRules, ArmyUnitListState& state) {
    char rowLabel[56] = { 0 };
    RenderVirtualRows("armyUnitRules", static_cast<int>(state.armyRuleIndices.size()),
                      state.rowHeight, state.listHeight, [&](int rowIndex) {
        const int ruleIndex = state.armyRuleIndices[static_cast<std::size_t>(rowIndex)];
        const Params::UnitRule& rule = unitRules[static_cast<std::size_t>(ruleIndex)];
        // %.7s: the tpId is a fixed 8-byte field whose last byte need not be a terminator.
        std::snprintf(rowLabel, sizeof(rowLabel), "%d: %.7s x%d", ruleIndex,
                      rule.transform.templateIdentifier, rule.count);
        if (ImGui::Selectable(rowLabel, ruleIndex == state.selectedRuleIndex))
            state.selectedRuleIndex = ruleIndex;
    });
}

// The Add Units picker. It appends ONE rule per confirm: `Ui::IconGridState` carries a single
// selected id, so the plan's multi-select is not something the shared grid can express today —
// adding it is a widget work-order this one does not own (ARCH §8.4).
void DrawAddUnitsPicker(std::vector<Params::UnitRule>& unitRules, int armyIndex,
                        ArmyUnitListState& state, const IconAtlasManifest* iconManifest,
                        bool& bRecipeMoved) {
    if (ImGui::Button("Add Units...")) state.bAddUnitsPickerOpen = true;
    if (!state.bAddUnitsPickerOpen) return;
    if (iconManifest != nullptr)
        DrawIconGrid("Unit Icons", *iconManifest, state.iconGridState, state.iconGridHeight);
    else
        ImGui::TextUnformatted("No resident icon atlas: the new rule's tpId is typed below.");
    DrawSliderScalarInteger("Count", state.pendingUnitCount, state.pendingUnitCountRange,
                            state.pendingUnitCountToggle);
    if (ImGui::Button("Confirm")) {
        Params::UnitRule rule;
        rule.armyIndex = armyIndex;
        rule.count     = state.pendingUnitCount;
        unitRules.push_back(rule);
        state.selectedRuleIndex  = static_cast<int>(unitRules.size()) - 1;
        state.bAddUnitsPickerOpen = false;
        bRecipeMoved = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) state.bAddUnitsPickerOpen = false;
}

// Delete the selected rule. Applied after the list is drawn so the vector never moves under a row.
bool DrawRemoveUnitRule(std::vector<Params::UnitRule>& unitRules, int armyIndex,
                        ArmyUnitListState& state) {
    if (!ImGui::Button("Remove Unit Rule")) return false;
    if (SelectedUnitRule(unitRules, armyIndex, state) == nullptr) return false;
    unitRules.erase(unitRules.begin() + state.selectedRuleIndex);
    state.selectedRuleIndex = -1;
    return true;
}

// How many units land, how far apart, and on what terrain.
void DrawUnitRuleSettings(Params::UnitRule& rule, ArmyUnitListState& state,
                          Pipeline::PreviewDriver* previewDriver) {
    if (!DrawSectionBegin("Unit Rule", state.gateSection)) return;
    NotifyPlacementChange(DrawSliderScalarInteger("Count", rule.count, state.countRange,
                                                  state.countToggle).bCommitted, previewDriver);
    NotifyPlacementChange(DrawSliderScalar("Spacing Minimum (cells)", rule.spacingMinimum,
                                           state.spacingRange, state.spacingToggle, WidgetStyle(),
                                           "%.2f").bCommitted, previewDriver);
    WidgetChange change = DrawRangeSlider("Slope Range (degrees)", state.slopeValues,
                                          state.slopeBounds, state.slopeToggle, WidgetStyle(), "%.1f");
    if (change.bValueChanged) StoreUnitRuleValues(state, rule);
    NotifyPlacementChange(change.bCommitted, previewDriver);
    change = DrawRangeSlider("Height Range (normalized)", state.heightValues, state.heightBounds,
                             state.heightToggle);
    if (change.bValueChanged) StoreUnitRuleValues(state, rule);
    NotifyPlacementChange(change.bCommitted, previewDriver);
    DrawSectionEnd();
}

} // namespace

Params::UnitRule* SelectedUnitRule(std::vector<Params::UnitRule>& unitRules, int armyIndex,
                                   const ArmyUnitListState& state) {
    if (state.selectedRuleIndex < 0 || state.selectedRuleIndex >= static_cast<int>(unitRules.size()))
        return nullptr;
    Params::UnitRule& rule = unitRules[static_cast<std::size_t>(state.selectedRuleIndex)];
    return rule.armyIndex == armyIndex ? &rule : nullptr;
}

void DrawArmyUnitList(std::vector<Params::UnitRule>& unitRules, int armyIndex,
                      ArmyUnitListState& state, Pipeline::PreviewDriver* previewDriver,
                      const IconAtlasManifest* iconManifest) {
    if (!DrawSectionBegin("Units", state.section)) return;
    CollectUnitRuleIndicesForArmy(unitRules, armyIndex, state.armyRuleIndices);
    ImGui::Text("%d unit rule(s) in this army.", static_cast<int>(state.armyRuleIndices.size()));
    DrawUnitRuleRows(unitRules, state);
    bool bRecipeMoved = DrawRemoveUnitRule(unitRules, armyIndex, state);
    DrawAddUnitsPicker(unitRules, armyIndex, state, iconManifest, bRecipeMoved);
    NotifyPlacementChange(bRecipeMoved, previewDriver);
    Params::UnitRule* const rule = SelectedUnitRule(unitRules, armyIndex, state);
    if (rule == nullptr) {
        ImGui::TextUnformatted("Select a unit rule to edit it.");
        DrawSectionEnd();
        return;
    }
    if (!state.slopeToggle.IsCommitDeferred() && !state.heightToggle.IsCommitDeferred())
        LoadUnitRuleValues(*rule, state);
    DrawUnitRuleSettings(*rule, state, previewDriver);
    DrawPlacementGateSection(rule->maskStratumIndex, rule->maskWeightMinimum, rule->mapEdgePadding,
                             state.gate, previewDriver);
    DrawPlacementSymmetryAxes("unitSymmetry", rule->bSymmetryUseGlobal, rule->symmetryMask,
                              previewDriver);
    DrawPlacementTransformSection(rule->transform, state.transform, previewDriver);
    DrawPlacementTemplatePicker(rule->transform, state.iconGridState, state.iconGridHeight,
                                iconManifest, previewDriver);
    DrawSectionEnd();
}

} // namespace Ui
} // namespace SanmapGen
