// MarkersTab_RuleLayerInstances_UI.cpp — a THIRD aspect-split sibling of MarkersTab_RuleLayers_UI.h
// (ARCH §1.5): neither MarkersTab_RuleLayers_UI.cpp nor MarkersTab_RuleLayerSettings_UI.cpp had
// headroom left under the 150-line hard ceiling once STEP132's own plumbing (ARCH §19.27) landed.
// Carries DrawRuleInstanceList — the Rule row's own new per-Rule instance list, the procedural
// analog of DrawLayerRowBody's per-Layer one (ARCH §19.26), called once at the bottom of
// DrawRuleSettings — PLUS ApplyMarkerRuleListSignal/SelectedMarkerRuleLayer, relocated here from
// MarkersTab_RuleLayers_UI.cpp for the SAME size-ceiling reason, not a thematic one (see
// MarkersTab_RuleLayers_UI.h's own header comment).
#include "MarkersTab_RuleLayers_UI.h"
#include "MarkersTab_UI.h"
#include "SymmetryClusterInstanceList_UI.h"
#include "imgui.h"
#include <cstdio>

namespace SanmapGen {
namespace Ui {
namespace {

// One procedural instance row — a plain Selectable, no persistent row highlight (unlike the manual
// list's `selectedManualInstanceIdentifier`: no tab-local field exists for this session-only
// selection, and the ticket's own Verify section asks only that the click route through the shared
// setter, not that the list mirror the canvas's own highlight — ARCH §19.27's own out-of-scope note).
void DrawProceduralInstanceRow(const Data::PlacementInstances& placedMarkers, int position,
                               const std::function<void(int)>& selectProceduralMarkerInstanceCallback) {
    const std::size_t instanceIndex = static_cast<std::size_t>(position);
    char rowLabel[64];
    // %.7s: the tpId is a fixed 8-byte field whose last byte need not be a terminator (mirrors
    // MarkersTab_Placed_UI.cpp's own DrawPlacedMarkerRow).
    std::snprintf(rowLabel, sizeof(rowLabel), "%d: %.7s", position,
                 placedMarkers.templateIdentifier[instanceIndex].characters);
    if (ImGui::Selectable(rowLabel) && selectProceduralMarkerInstanceCallback)
        selectProceduralMarkerInstanceCallback(position);
}

} // namespace

// STEP132 (ARCH §19.27) — mirrors DrawLayerRowBody's per-Layer instance list (§19.26) one tier down:
// grouped by `symmetryIdentifier`, but with the OPPOSITE membership predicate (bucket size > 1, never
// `== 0` — a procedural `symmetryIdentifier` is never 0) via the SAME shared helper Part A calls.
void DrawRuleInstanceList(const ProceduralInstanceListContext_UI& instanceListContext) {
    ImGui::Separator();
    ImGui::TextUnformatted("Instances");
    if (instanceListContext.placedMarkers == nullptr || instanceListContext.ruleIndexLookup == nullptr) {
        ImGui::TextDisabled("(none - generate first)");
        return;
    }
    const ProceduralInstanceRuleIndex_UI& ruleIndexLookup = *instanceListContext.ruleIndexLookup;
    const auto instanceIt = ruleIndexLookup.instancesByRuleIndex.find(instanceListContext.flatRuleIndex);
    if (instanceIt == ruleIndexLookup.instancesByRuleIndex.end() || instanceIt->second.empty()) {
        ImGui::TextDisabled("(none)");
        return;
    }
    const Data::PlacementInstances& placedMarkers = *instanceListContext.placedMarkers;
    const std::function<void(int)>& selectCallback =
        instanceListContext.selectProceduralMarkerInstanceCallback;
    DrawSymmetryClusterInstanceList<int>(instanceIt->second,
        [&](const int& position) {
            return placedMarkers.symmetryIdentifier[static_cast<std::size_t>(position)];
        },
        [](int /*groupIdentifier*/, int bucketSize) { return bucketSize > 1; },
        [&](const int& position) { DrawProceduralInstanceRow(placedMarkers, position, selectCallback); });
}

// The layer `state.selectedRuleLayerIndex` points at, or null when it points at nothing (STEP80 §2,
// mirroring `SelectedLayer`, LayersTab_UI.cpp:120).
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

} // namespace Ui
} // namespace SanmapGen
