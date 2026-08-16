// PropsTab_Decals_UI.cpp — the imgui composition of the Decal Rules stack. Layer: UI.
// Shared widgets only: DraggableList for the ordered stack, RangeSlider/Dial for the scalars,
// Section for the blocks, IconGrid for the template picker.
#include "PropsTab_Decals_UI.h"
#include "DraggableListWidget_UI.h"
#include "imgui.h"
#include <cstdio>

namespace SanmapGen {
namespace Ui {
namespace {

// MUTATES NOTHING while drawing: the signal is applied after the list has closed.
DraggableListSignal DrawDecalList(const std::vector<Params::DecalRule>& decalRules,
                                  const DecalRuleStackState& state) {
    char rowLabel[48] = { 0 };
    return DraggableList<Params::DecalRule>::Render(
        "decalRules", decalRules,
        [&](int rowIndex) {
            const Params::DecalRule& rule = decalRules[static_cast<std::size_t>(rowIndex)];
            // %.7s: the tpId is a fixed 8-byte field whose last byte need not be a terminator.
            std::snprintf(rowLabel, sizeof(rowLabel), "%d: %.7s density %.3f", rowIndex,
                          rule.transform.templateIdentifier, rule.density);
            DraggableListRow row;
            row.label    = rowLabel;
            row.bVisible = rule.bEnabled;
            return row;
        },
        [](int) {},
        state.selectedRuleIndex);
}

bool ApplyDecalListSignal(std::vector<Params::DecalRule>& decalRules, DecalRuleStackState& state,
                          const DraggableListSignal& signal) {
    const int rowIndex = signal.sourceRowIndex;
    const bool bRowValid = rowIndex >= 0 && rowIndex < static_cast<int>(decalRules.size());
    if (signal.kind == DraggableListSignalKind::Select && bRowValid) {
        state.selectedRuleIndex = rowIndex;
        LoadDecalRuleValues(decalRules[static_cast<std::size_t>(rowIndex)], state);
        return false;
    }
    if (signal.kind == DraggableListSignalKind::ToggleVisibility && bRowValid) {
        decalRules[static_cast<std::size_t>(rowIndex)].bEnabled =
            !decalRules[static_cast<std::size_t>(rowIndex)].bEnabled;
        return true;
    }
    return ApplyDraggableListSignal(decalRules, signal);
}

bool DrawDecalListButtons(std::vector<Params::DecalRule>& decalRules, DecalRuleStackState& state) {
    bool bRecipeMoved = false;
    if (ImGui::Button("Add Decal Rule")) { decalRules.push_back(Params::DecalRule()); bRecipeMoved = true; }
    ImGui::SameLine();
    if (ImGui::Button("Remove Decal Rule") && SelectedDecalRule(decalRules, state) != nullptr) {
        decalRules.erase(decalRules.begin() + state.selectedRuleIndex);
        bRecipeMoved = true;
    }
    if (bRecipeMoved) state.selectedRuleIndex = static_cast<int>(decalRules.size()) - 1;
    return bRecipeMoved;
}

// What terrain a decal may land on, and how densely it lands.
void DrawDecalGates(Params::DecalRule& rule, DecalRuleStackState& state,
                    Pipeline::PreviewDriver* previewDriver) {
    if (!DrawSectionBegin("Decal Gates", state.gateSection)) return;
    WidgetChange change = DrawRangeSlider("Slope Range (degrees)", state.slopeValues,
                                          state.slopeBounds, state.slopeToggle, WidgetStyle(), "%.1f");
    if (change.bValueChanged) StoreDecalRuleValues(state, rule);
    NotifyPlacementChange(change.bCommitted, previewDriver);
    change = DrawRangeSlider("Height Range (normalized)", state.heightValues, state.heightBounds,
                             state.heightToggle);
    if (change.bValueChanged) StoreDecalRuleValues(state, rule);
    NotifyPlacementChange(change.bCommitted, previewDriver);
    NotifyPlacementChange(DrawLabelledDial("Density", rule.density, state.densityRange,
                                           state.densityToggle, WidgetStyle(), "%.4f").bCommitted,
                          previewDriver);
    NotifyPlacementChange(DrawLabelledDial("Spacing Minimum (cells)", rule.spacingMinimum,
                                           state.spacingRange, state.spacingToggle, WidgetStyle(),
                                           "%.2f").bCommitted, previewDriver);
    DrawSectionEnd();
}

} // namespace

Params::DecalRule* SelectedDecalRule(std::vector<Params::DecalRule>& decalRules,
                                     const DecalRuleStackState& state) {
    if (state.selectedRuleIndex < 0 || state.selectedRuleIndex >= static_cast<int>(decalRules.size()))
        return nullptr;
    return &decalRules[static_cast<std::size_t>(state.selectedRuleIndex)];
}

void DrawDecalRuleStack(std::vector<Params::DecalRule>& decalRules, DecalRuleStackState& state,
                        Pipeline::PreviewDriver* previewDriver,
                        const IconAtlasManifest* iconManifest) {
    if (!DrawSectionBegin("Decal Rules", state.stackSection)) return;
    const DraggableListSignal signal = DrawDecalList(decalRules, state);
    bool bRecipeMoved = signal.bHasSignal() && ApplyDecalListSignal(decalRules, state, signal);
    bRecipeMoved = DrawDecalListButtons(decalRules, state) || bRecipeMoved;
    NotifyPlacementChange(bRecipeMoved, previewDriver);
    Params::DecalRule* const rule = SelectedDecalRule(decalRules, state);
    if (rule == nullptr) {
        ImGui::TextUnformatted("Select a decal rule to edit it.");
        DrawSectionEnd();
        return;
    }
    if (!state.slopeToggle.IsCommitDeferred() && !state.heightToggle.IsCommitDeferred())
        LoadDecalRuleValues(*rule, state);
    DrawDecalGates(*rule, state, previewDriver);
    DrawPlacementGateSection(rule->maskStratumIndex, rule->maskWeightMinimum, rule->mapEdgePadding,
                             state.gate, previewDriver);
    DrawPlacementTransformSection(rule->transform, state.transform, previewDriver);
    DrawPlacementTemplatePicker(rule->transform, state.iconGridState, state.iconGridHeight,
                                iconManifest, previewDriver);
    DrawSectionEnd();
}

} // namespace Ui
} // namespace SanmapGen
