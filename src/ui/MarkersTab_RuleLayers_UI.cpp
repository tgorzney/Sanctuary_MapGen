// MarkersTab_RuleLayers_UI.cpp — the two-level list mechanics (outer/inner DraggableLists and
// appliers). Delete confirm / buttons / both tiers' settings live in MarkersTab_RuleLayerSettings_UI.cpp.
#include "MarkersTab_RuleLayers_UI.h"
#include "MarkersTab_UI.h"
#include "imgui.h"
#include <cstdio>

namespace SanmapGen {
namespace Ui {
namespace {

// One layer's rule rows, each row's own DrawRuleSettings inline (STEP110, one tier down from
// STEP104). Only the SELECTED layer renders a list — no cross-layer drag (STEP80 §1).
DraggableListSignal DrawRuleLayerBody(Params::MarkerRuleLayer& layer, int layerIndex,
                                      MarkersTabState& state, Pipeline::PreviewDriver* previewDriver,
                                      const IconAtlasManifest* iconManifest) {
    if (layerIndex != state.selectedRuleLayerIndex) {
        ImGui::Text("%d rule(s) - select this layer to edit them", static_cast<int>(layer.rules.size()));
        return DraggableListSignal();
    }
    char rowLabel[56] = { 0 };
    return DraggableList<Params::MarkerRule>::Render(
        "rules", layer.rules,
        [&](int rowIndex) {
            const Params::MarkerRule& rule = layer.rules[static_cast<std::size_t>(rowIndex)];
            std::snprintf(rowLabel, sizeof(rowLabel), "%d: %s x%d", rowIndex,
                          MarkerCategoryLabel(rule.category), rule.count);
            DraggableListRow row;
            row.label    = rowLabel;
            row.bVisible = rule.bEnabled;
            row.bLocked  = rule.bHidden;
            return row;
        },
        // Own row, own settings — no bleed (STEP110; was a bottom-of-stack draw in DrawRuleStack).
        [&](int rowIndex) {
            DrawRuleSettings(layer.rules[static_cast<std::size_t>(rowIndex)], state, previewDriver,
                             iconManifest);
        }, state.selectedRuleIndex);
}

// Applies the frame's traffic from both lists: the inner signal FIRST — a same-frame layer Delete
// would move the indices it is expressed in (LayersTab_UI.cpp) — then the outer, delete-confirm-gated.
bool ApplyRuleLayerFrameSignals(std::vector<Params::MarkerRuleLayer>& markerRuleLayers, MarkersTabState& state,
                                const DraggableListSignal& ruleSignal, int ruleSignalLayerIndex,
                                const DraggableListSignal& layerSignal) {
    bool bRecipeMoved = false;
    if (ruleSignalLayerIndex >= 0)
        bRecipeMoved = ApplyMarkerRuleListSignal(
            markerRuleLayers[static_cast<std::size_t>(ruleSignalLayerIndex)], ruleSignal, state);

    const bool bDeleteRow = layerSignal.kind == DraggableListSignalKind::Delete
        && layerSignal.sourceRowIndex >= 0
        && layerSignal.sourceRowIndex < static_cast<int>(markerRuleLayers.size());
    if (bDeleteRow && !markerRuleLayers[static_cast<std::size_t>(layerSignal.sourceRowIndex)].rules.empty()) {
        // Deleting a layer deletes its rules with it (STEP80 §4) — gated behind a confirm dialog
        // rather than applied inline; an empty layer falls through below.
        state.pendingDeleteRuleLayerIndex = layerSignal.sourceRowIndex;
        state.deleteRuleLayerConfirmState.bOpenRequested = true;
    } else if (layerSignal.bHasSignal()) {
        bRecipeMoved = ApplyMarkerRuleLayerListSignal(markerRuleLayers, layerSignal,
                                                      state.selectedRuleLayerIndex,
                                                      state.selectedRuleIndex) || bRecipeMoved;
    }
    return bRecipeMoved;
}

// The outer list plus the nested rule list. STEP110 Fix part 1: each row draws its OWN
// DrawRuleLayerSettings inline whenever ITS OWN header is open — never gated on
// `state.selectedRuleLayerIndex` (the nested rule list still is, drag-safety: DrawRuleLayerBody).
void DrawRuleLayerListBody(std::vector<Params::MarkerRuleLayer>& markerRuleLayers, MarkersTabState& state,
                           Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest) {
    DraggableListSignal ruleSignal;
    int ruleSignalLayerIndex = -1;
    char rowLabel[128] = { 0 };
    const DraggableListSignal layerSignal = DraggableList<Params::MarkerRuleLayer>::Render(
        "markerRuleLayers", markerRuleLayers,
        [&](int rowIndex) {
            const Params::MarkerRuleLayer& layer = markerRuleLayers[static_cast<std::size_t>(rowIndex)];
            const char* const suffix = !layer.bEnabled ? " - DISABLED, not generated"
                                     : layer.bHidden    ? " - hidden, still generated" : "";
            std::snprintf(rowLabel, sizeof(rowLabel), "%s (%d rules)%s",
                          layer.name.empty() ? "Marker Layer" : layer.name.c_str(),
                          static_cast<int>(layer.rules.size()), suffix);
            DraggableListRow row;
            row.bRowSuppressed = (layer.parentBundleIdentifier != -1);   // NEW — STEP120: bundled
                                                                          // layers show in the tree
            row.label    = rowLabel;
            row.bVisible = layer.bEnabled;
            row.bLocked  = layer.bHidden;
            return row;
        },
        [&](int rowIndex) {
            Params::MarkerRuleLayer& layer = markerRuleLayers[static_cast<std::size_t>(rowIndex)];
            DrawRuleLayerSettings(layer, previewDriver);
            ImGui::Separator();
            const DraggableListSignal signal =
                DrawRuleLayerBody(layer, rowIndex, state, previewDriver, iconManifest);
            if (signal.bHasSignal()) { ruleSignal = signal; ruleSignalLayerIndex = rowIndex; }
        },
        state.selectedRuleLayerIndex);

    bool bRecipeMoved = ApplyRuleLayerFrameSignals(markerRuleLayers, state, ruleSignal,
                                                   ruleSignalLayerIndex, layerSignal);
    bRecipeMoved = DrawPendingDeleteRuleLayerDialog(markerRuleLayers, state) || bRecipeMoved;
    NotifyPlacementChange(bRecipeMoved, previewDriver);
}

} // namespace

Params::MarkerRuleLayer* SelectedMarkerRuleLayer(std::vector<Params::MarkerRuleLayer>& markerRuleLayers,
                                                 const MarkersTabState& state) {
    if (state.selectedRuleLayerIndex < 0
        || state.selectedRuleLayerIndex >= static_cast<int>(markerRuleLayers.size())) return nullptr;
    return &markerRuleLayers[static_cast<std::size_t>(state.selectedRuleLayerIndex)];
}

// The existing per-rule ApplyRuleListSignal body, rehomed against `layer.rules`.
bool ApplyMarkerRuleListSignal(Params::MarkerRuleLayer& layer, const DraggableListSignal& signal,
                               MarkersTabState& state) {
    std::vector<Params::MarkerRule>& rules = layer.rules;
    const int rowIndex = signal.sourceRowIndex;
    const bool bRowValid = rowIndex >= 0 && rowIndex < static_cast<int>(rules.size());
    if (signal.kind == DraggableListSignalKind::Select && bRowValid) {
        state.selectedRuleIndex = rowIndex;
        LoadMarkerRuleValues(rules[static_cast<std::size_t>(rowIndex)], state);
        LoadMarkerRuleEnumIndices(rules[static_cast<std::size_t>(rowIndex)], state.ruleDetail);
        return false;
    }
    if (signal.kind == DraggableListSignalKind::ToggleVisibility && bRowValid) {
        rules[static_cast<std::size_t>(rowIndex)].bEnabled =
            !rules[static_cast<std::size_t>(rowIndex)].bEnabled;
        return true;
    }
    if (signal.kind == DraggableListSignalKind::ToggleLock && bRowValid) {
        rules[static_cast<std::size_t>(rowIndex)].bHidden =
            !rules[static_cast<std::size_t>(rowIndex)].bHidden;
        return true;
    }
    return ApplyDraggableListSignal(rules, signal);
}

void DrawMarkerRuleLayerList(std::vector<Params::MarkerRuleLayer>& markerRuleLayers,
                             MarkersTabState& state, Pipeline::PreviewDriver* previewDriver,
                             const IconAtlasManifest* iconManifest) {
    DrawRuleLayerListBody(markerRuleLayers, state, previewDriver, iconManifest);
    DrawRuleLayerButtons(markerRuleLayers, state, previewDriver);
    // STEP110: no trailing settings draw here — DrawRuleLayerListBody's own row body, above, draws
    // each row's settings inline, under its own header.
}

} // namespace Ui
} // namespace SanmapGen
