// PropsTab_UI.cpp — the imgui composition of the prop tab. Layer: UI.
// Shared widgets only: DraggableList for the ordered prop stack, VirtualList for the read-only
// transform list, IconGrid for the pickers, Section/Checkbox/RangeSlider/Dial for the rest.
// No ImGui::SliderFloat / DragFloat / VSliderFloat in this file.
#include "PropsTab_UI.h"
#include "DraggableListWidget_UI.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../pipeline/PreviewDriver_PIPELINE.h"
#include "imgui.h"
#include <cstdio>

namespace SanmapGen {
namespace Ui {
namespace {

// The procedural prop STACK. STEP110: each row's own body draws that row's OWN Gates/Affinities/
// Gate/Symmetry/Transform/TemplatePicker/Resolve-Footprint sections, directly under its own
// header, whenever ITS OWN CollapsingHeader is open (DraggableList's own per-row expand/collapse
// state — never gated on `state.selectedRuleIndex`), so an expanded row never shows another row's
// settings. `propRules` is mutated live by those sections (the widgets write straight through the
// row's own reference); only the STRUCTURAL signal (reorder/delete/select) is still applied once
// the list has closed, by the caller.
DraggableListSignal DrawRuleList(std::vector<Params::PropRule>& propRules, PropsTabState& state,
                                 Pipeline::PreviewDriver* previewDriver,
                                 const IconAtlasManifest* iconManifest,
                                 const Io::TemplateIngestReport* templateIngestReport) {
    char rowLabel[48] = { 0 };
    return DraggableList<Params::PropRule>::Render(
        "propRules", propRules,
        [&](int rowIndex) {
            const Params::PropRule& rule = propRules[static_cast<std::size_t>(rowIndex)];
            // %.7s: the tpId is a fixed 8-byte field whose last byte need not be a terminator.
            std::snprintf(rowLabel, sizeof(rowLabel), "%d: %.7s density %.3f", rowIndex,
                          rule.transform.templateIdentifier, rule.density);
            DraggableListRow row;
            row.label    = rowLabel;
            row.bVisible = rule.bEnabled;
            return row;
        },
        [&](int rowIndex) {
            Params::PropRule& rule = propRules[static_cast<std::size_t>(rowIndex)];
            // Own row, own rule, own settings — the same sections DrawRuleStack used to draw once
            // at the bottom for whatever was "selected" (pre-STEP110), now drawn inline per
            // expanded row so they can never bleed across rows.
            if (!state.slopeToggle.IsCommitDeferred() && !state.heightToggle.IsCommitDeferred())
                LoadPropRuleValues(rule, state);
            DrawPropRuleGates(rule, state, previewDriver);
            DrawPropRuleAffinities(rule, state, previewDriver);
            DrawPlacementGateSection(rule.maskStratumIndex, rule.maskWeightMinimum,
                                     rule.mapEdgePadding, state.gate, previewDriver);
            DrawPlacementSymmetryAxes("propSymmetry", rule.bSymmetryUseGlobal, rule.symmetryMask,
                                      previewDriver);
            DrawPlacementTransformSection(rule.transform, state.transform, previewDriver);
            DrawPlacementTemplatePicker(rule.transform, state.iconGridState, state.iconGridHeight,
                                       iconManifest, previewDriver);
            // STEP96_FootprintBakeAndStalenessCheck_IO.md §2 — NOT inside DrawPlacementTemplatePicker
            // (PlacementRuleSections_UI.cpp): baseFootprintWidth/Depth/footprintBakeFingerprint live
            // on PropRule itself, not on the shared ScatterTransform that function edits.
            DrawResolvePropFootprintButton(rule, state, templateIngestReport);
        },
        state.selectedRuleIndex);
}

bool ApplyRuleListSignal(std::vector<Params::PropRule>& propRules, PropsTabState& state,
                         const DraggableListSignal& signal) {
    const int rowIndex = signal.sourceRowIndex;
    const bool bRowValid = rowIndex >= 0 && rowIndex < static_cast<int>(propRules.size());
    if (signal.kind == DraggableListSignalKind::Select && bRowValid) {
        state.selectedRuleIndex = rowIndex;
        LoadPropRuleValues(propRules[static_cast<std::size_t>(rowIndex)], state);
        return false;
    }
    if (signal.kind == DraggableListSignalKind::ToggleVisibility && bRowValid) {
        propRules[static_cast<std::size_t>(rowIndex)].bEnabled =
            !propRules[static_cast<std::size_t>(rowIndex)].bEnabled;
        return true;
    }
    return ApplyDraggableListSignal(propRules, signal);
}

bool DrawRuleListButtons(std::vector<Params::PropRule>& propRules, PropsTabState& state) {
    bool bRecipeMoved = false;
    if (ImGui::Button("Add Prop Rule")) { propRules.push_back(Params::PropRule()); bRecipeMoved = true; }
    ImGui::SameLine();
    if (ImGui::Button("Remove Prop Rule") && SelectedPropRule(propRules, state) != nullptr) {
        propRules.erase(propRules.begin() + state.selectedRuleIndex);
        bRecipeMoved = true;
    }
    if (bRecipeMoved) state.selectedRuleIndex = static_cast<int>(propRules.size()) - 1;
    return bRecipeMoved;
}

// The whole procedural stack: the list plus its buttons. STEP110: no trailing "selected rule"
// panel draw here any more — each row's own body (DrawRuleList) draws that row's settings inline,
// right under its own header, whenever ITS OWN CollapsingHeader is open. `selectedRuleIndex` stays
// (ApplyRuleListSignal, the DraggableList "Selected" highlight, and the Remove button's own
// `SelectedPropRule` lookup still need it) — only the redundant full-panel draw that used to run
// once at the bottom for whatever it pointed at is gone.
void DrawRuleStack(Params::MapRecipe& recipe, PropsTabState& state,
                   Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest,
                   const Io::TemplateIngestReport* templateIngestReport) {
    if (!DrawSectionBegin("Procedural Props", state.ruleStackSection)) return;
    const DraggableListSignal signal =
        DrawRuleList(recipe.propRules, state, previewDriver, iconManifest, templateIngestReport);
    bool bRecipeMoved = signal.bHasSignal() && ApplyRuleListSignal(recipe.propRules, state, signal);
    bRecipeMoved = DrawRuleListButtons(recipe.propRules, state) || bRecipeMoved;
    NotifyPlacementChange(bRecipeMoved, previewDriver);
    DrawSectionEnd();
}

} // namespace

Params::PropRule* SelectedPropRule(std::vector<Params::PropRule>& propRules,
                                   const PropsTabState& state) {
    if (state.selectedRuleIndex < 0 || state.selectedRuleIndex >= static_cast<int>(propRules.size()))
        return nullptr;
    return &propRules[static_cast<std::size_t>(state.selectedRuleIndex)];
}

void DrawPropsTab(Params::MapRecipe& recipe, PropsTabState& state,
                  Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest,
                  const Data::PlacementInstances* placedProps,
                  const Data::PlacementInstances* placedDecals,
                  const Io::TemplateIngestReport* templateIngestReport) {
    ImGui::PushID("propsTab");
    DrawManualPropLayers(state.manualLayers, recipe.propLayers, recipe.props, placedProps);
    DrawRuleStack(recipe, state, previewDriver, iconManifest, templateIngestReport);
    DrawManualDecalLayers(state.manualDecalLayers, recipe.decalLayers, recipe.decals, placedDecals);
    DrawDecalRuleStack(recipe.decalRules, state.decalStack, previewDriver, iconManifest);
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
