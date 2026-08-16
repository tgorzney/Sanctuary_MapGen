// MarkersTab_UI.cpp — the imgui composition of the marker tab. Layer: UI.
// Shared widgets only: DraggableList for the procedural rule stack, VirtualList for the placed
// markers, IconGrid for the pickers, Section/Checkbox/Combo/RangeSlider/Dial for the scalars.
// No ImGui::SliderFloat / DragFloat / VSliderFloat in this file.
#include "MarkersTab_UI.h"
#include "DraggableListWidget_UI.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../pipeline/PreviewDriver_PIPELINE.h"
#include "imgui.h"
#include <cstdio>

namespace SanmapGen {
namespace Ui {
namespace {

// The procedural rule STACK. It is a DraggableList, not a VirtualList: rule order decides which
// rule claims a contested position first, so every row is a drop target (and the set is tens of
// rows, not tens of thousands — that is what the placed-marker list is for).
// MUTATES NOTHING while drawing: the signal is applied after the list has closed.
DraggableListSignal DrawRuleList(const std::vector<Params::MarkerRule>& markerRules,
                                 const MarkersTabState& state) {
    char rowLabel[56] = { 0 };
    return DraggableList<Params::MarkerRule>::Render(
        "markerRules", markerRules,
        [&](int rowIndex) {
            const Params::MarkerRule& rule = markerRules[static_cast<std::size_t>(rowIndex)];
            std::snprintf(rowLabel, sizeof(rowLabel), "%d: %s x%d", rowIndex,
                          MarkerCategoryLabel(rule.category), rule.count);
            DraggableListRow row;
            row.label    = rowLabel;
            row.bVisible = rule.bEnabled;
            row.bLocked  = rule.bHidden;
            return row;
        },
        [](int) {},                       // header-only rows: the detail sections are below
        state.selectedRuleIndex);
}

// Applies one frame of list traffic. Reorder and Delete are the shared structural appliers; the
// two toggles belong to fields this tab owns, and Select is pure tab state.
bool ApplyRuleListSignal(std::vector<Params::MarkerRule>& markerRules, MarkersTabState& state,
                         const DraggableListSignal& signal) {
    const int rowIndex = signal.sourceRowIndex;
    const bool bRowValid = rowIndex >= 0 && rowIndex < static_cast<int>(markerRules.size());
    if (signal.kind == DraggableListSignalKind::Select && bRowValid) {
        state.selectedRuleIndex = rowIndex;
        LoadMarkerRuleValues(markerRules[static_cast<std::size_t>(rowIndex)], state);
        LoadMarkerRuleEnumIndices(markerRules[static_cast<std::size_t>(rowIndex)], state.ruleDetail);
        return false;
    }
    if (signal.kind == DraggableListSignalKind::ToggleVisibility && bRowValid) {
        markerRules[static_cast<std::size_t>(rowIndex)].bEnabled =
            !markerRules[static_cast<std::size_t>(rowIndex)].bEnabled;
        return true;
    }
    if (signal.kind == DraggableListSignalKind::ToggleLock && bRowValid) {
        markerRules[static_cast<std::size_t>(rowIndex)].bHidden =
            !markerRules[static_cast<std::size_t>(rowIndex)].bHidden;
        return true;
    }
    return ApplyDraggableListSignal(markerRules, signal);
}

// Add / remove, applied AFTER the list is drawn so the vector never moves under a live row.
bool DrawRuleListButtons(std::vector<Params::MarkerRule>& markerRules, MarkersTabState& state) {
    bool bRecipeMoved = false;
    if (ImGui::Button("Add Rule")) { markerRules.push_back(Params::MarkerRule()); bRecipeMoved = true; }
    ImGui::SameLine();
    if (ImGui::Button("Remove Selected") && SelectedMarkerRule(markerRules, state) != nullptr) {
        markerRules.erase(markerRules.begin() + state.selectedRuleIndex);
        bRecipeMoved = true;
    }
    if (bRecipeMoved) state.selectedRuleIndex = static_cast<int>(markerRules.size()) - 1;
    return bRecipeMoved;
}

// The whole procedural stack: the list, its buttons, and the selected rule's sections.
void DrawRuleStack(Params::MapRecipe& recipe, MarkersTabState& state,
                   Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest) {
    if (!DrawSectionBegin("Procedural Rules", state.ruleStackSection)) return;
    const DraggableListSignal signal = DrawRuleList(recipe.markerRules, state);
    bool bRecipeMoved = signal.bHasSignal() && ApplyRuleListSignal(recipe.markerRules, state, signal);
    bRecipeMoved = DrawRuleListButtons(recipe.markerRules, state) || bRecipeMoved;
    NotifyPlacementChange(bRecipeMoved, previewDriver);
    ImGui::Separator();
    Params::MarkerRule* const rule = SelectedMarkerRule(recipe.markerRules, state);
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
    DrawPlacementSymmetryAxes("markerSymmetry", rule->bSymmetryUseGlobal, rule->symmetryMask,
                              previewDriver);
    DrawPlacementTransformSection(rule->transform, state.transform, previewDriver);
    DrawPlacementTemplatePicker(rule->transform, state.iconGridState, state.iconGridHeight,
                                iconManifest, previewDriver);
    DrawSectionEnd();
}

} // namespace

Params::MarkerRule* SelectedMarkerRule(std::vector<Params::MarkerRule>& markerRules,
                                       const MarkersTabState& state) {
    if (state.selectedRuleIndex < 0 || state.selectedRuleIndex >= static_cast<int>(markerRules.size()))
        return nullptr;
    return &markerRules[static_cast<std::size_t>(state.selectedRuleIndex)];
}

void DrawMarkersTab(Params::MapRecipe& recipe, MarkersTabState& state,
                    Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest,
                    const Data::PlacementInstances* placedMarkers) {
    ImGui::PushID("markersTab");
    DrawMarkersTabGlobals(state.globals, iconManifest);
    DrawRuleStack(recipe, state, previewDriver, iconManifest);
    DrawPlacedMarkerList(placedMarkers, state.placedList);
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
