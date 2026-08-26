// MarkersTab_RuleLayers_UI.cpp — the two-level list mechanics (outer/inner DraggableLists and
// appliers). Delete confirm / buttons / both tiers' settings live in MarkersTab_RuleLayerSettings_UI.cpp.
#include "MarkersTab_RuleLayers_UI.h"
#include "MarkersTab_UI.h"
#include "TextInput_UI.h"
#include "imgui.h"
#include <cstdio>

namespace SanmapGen {
namespace Ui {
namespace {

// One layer's rule rows, each row's own DrawRuleSettings inline (STEP110). Only the SELECTED layer
// renders a list — no cross-layer drag (STEP80 §1). STEP132: a row's own flat ruleIndex is
// `layerInstanceListContext.flatRuleIndex` (this layer's own base) + rowIndex.
DraggableListSignal DrawRuleLayerBody(Params::MarkerRuleLayer& layer, int layerIndex,
                                      MarkersTabState& state, Pipeline::PreviewDriver* previewDriver,
                                      const IconAtlasManifest* iconManifest,
                                      const ProceduralInstanceListContext_UI& layerInstanceListContext) {
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
            ProceduralInstanceListContext_UI rowInstanceListContext = layerInstanceListContext;
            rowInstanceListContext.flatRuleIndex += rowIndex;
            DrawRuleSettings(layer.rules[static_cast<std::size_t>(rowIndex)], state, previewDriver,
                             iconManifest, rowInstanceListContext);
        }, state.selectedRuleIndex);
}

// Applies the frame's traffic from both lists: the inner signal FIRST (a same-frame layer Delete
// would move the indices it is expressed in), then the outer, delete-confirm-gated.
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

} // namespace

// The outer list plus the nested rule list. STEP110: each row draws its OWN DrawRuleLayerSettings
// inline whenever ITS OWN header is open — never gated on `state.selectedRuleLayerIndex` (the nested
// rule list still is, drag-safety: DrawRuleLayerBody). Returns whether the LIST signals alone moved
// the recipe — the Delete confirm/NotifyPlacementChange live in DrawMarkerTypeSections instead.
bool DrawRuleLayerListBody(std::vector<Params::MarkerRuleLayer>& markerRuleLayers, MarkersTabState& state,
                           Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest,
                           const std::string& markerTypeNameFilter,
                           const Data::PlacementInstances* placedMarkers,
                           const std::function<void(int)>& selectProceduralMarkerInstanceCallback) {
    // STEP132 — built ONCE per call, mirrors ManualInstanceLayerIndex_UI's own build-once posture.
    const ProceduralInstanceRuleIndex_UI ruleIndexLookup(placedMarkers != nullptr
        ? BuildProceduralInstanceRuleIndex(*placedMarkers) : ProceduralInstanceRuleIndex_UI());
    ProceduralInstanceListContext_UI baseInstanceListContext;
    baseInstanceListContext.placedMarkers   = placedMarkers;
    baseInstanceListContext.ruleIndexLookup = &ruleIndexLookup;
    baseInstanceListContext.selectProceduralMarkerInstanceCallback = selectProceduralMarkerInstanceCallback;
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
            row.bRowSuppressed = IsMarkerRuleLayerRowSuppressed(layer, markerTypeNameFilter);   // CHANGED — STEP125
            row.label    = rowLabel;
            row.bVisible = layer.bEnabled;
            row.bLocked  = layer.bHidden;
            return row;
        },
        [&](int rowIndex) {
            Params::MarkerRuleLayer& layer = markerRuleLayers[static_cast<std::size_t>(rowIndex)];
            if (layer.markerTypeName.empty()) {   // STEP128 §4 — mirrors MarkersTab_BundleNodeBody_UI.cpp:87
                TextInputRules typeRules; typeRules.maximumLength = 48; typeRules.bAllowEmpty = true;
                DrawTextInput("Marker Type", layer.markerTypeName, typeRules);
            }
            DrawRuleLayerSettings(layer, previewDriver);
            ImGui::Separator();
            ProceduralInstanceListContext_UI layerInstanceListContext = baseInstanceListContext;
            layerInstanceListContext.flatRuleIndex = FlatMarkerRuleIndexBase(markerRuleLayers, rowIndex);
            const DraggableListSignal signal =
                DrawRuleLayerBody(layer, rowIndex, state, previewDriver, iconManifest,
                                 layerInstanceListContext);
            if (signal.bHasSignal()) { ruleSignal = signal; ruleSignalLayerIndex = rowIndex; }
        },
        state.selectedRuleLayerIndex);

    return ApplyRuleLayerFrameSignals(markerRuleLayers, state, ruleSignal, ruleSignalLayerIndex, layerSignal);
    // no DrawPendingDeleteRuleLayerDialog / NotifyPlacementChange here anymore — §5
}

// SelectedMarkerRuleLayer / ApplyMarkerRuleListSignal: STEP132 relocated both to the sibling
// MarkersTab_RuleLayerInstances_UI.cpp — a pure size-ceiling remediation (ARCH §1.5), not a
// thematic move; see that file's own header comment.

} // namespace Ui
} // namespace SanmapGen
