// MarkersTab_RuleLayers_UI_Test.cpp — STEP80 acceptance: the two-level MarkerRuleLayer/MarkerRule
// list appliers and the two-index SelectedMarkerRule walk, driven headless (no imgui frame, window
// or GL context). Sibling TU to MarkersTab_UI_Test.cpp, which owns main() and the shared
// `Check`/`failureCount` (ARCH §1.5 — one binary, split translation units).
#include "MarkersTab_UI.h"

using namespace SanmapGen;
using namespace SanmapGen::Ui;

extern int failureCount;
void Check(bool bCondition, const char* label);

namespace {

// A signal literal, shaped like the one DraggableList::Render would have returned.
DraggableListSignal MakeSignal(DraggableListSignalKind kind, int sourceRowIndex, int targetRowIndex = -1) {
    DraggableListSignal signal;
    signal.kind           = kind;
    signal.sourceRowIndex = sourceRowIndex;
    signal.targetRowIndex = targetRowIndex;
    return signal;
}

std::vector<Params::MarkerRuleLayer> MakeTwoLayersOfTwoRules() {
    std::vector<Params::MarkerRuleLayer> layers(2);
    for (std::size_t layerIndex = 0; layerIndex < 2; ++layerIndex) {
        layers[layerIndex].name = layerIndex == 0 ? "First" : "Second";
        layers[layerIndex].rules.resize(2);
        for (std::size_t ruleIndex = 0; ruleIndex < 2; ++ruleIndex)
            layers[layerIndex].rules[ruleIndex].count =
                static_cast<int>(layerIndex * 10 + ruleIndex);
    }
    return layers;
}

// Acceptance items 1-2: the outer (layer) applier — reorder carries the layer's rules with it,
// Select moves only tab state, and the two toggles are the layer's own bEnabled/bHidden.
void RunRuleLayerReorderAndToggleChecks() {
    std::vector<Params::MarkerRuleLayer> layers = MakeTwoLayersOfTwoRules();
    int selectedLayerIndex = 0, selectedRuleIndex = 0;

    Check(ApplyMarkerRuleLayerListSignal(layers, MakeSignal(DraggableListSignalKind::Reorder, 0, 1),
                                         selectedLayerIndex, selectedRuleIndex),
          "a layer reorder moves the recipe");
    Check(layers[1].name == "First" && layers[1].rules[0].count == 0 && layers[1].rules[1].count == 1,
          "the moved layer and its rules landed at index 1 intact");
    Check(layers[0].name == "Second", "the drop row's original occupant shifted down");

    Check(!ApplyMarkerRuleLayerListSignal(layers, MakeSignal(DraggableListSignalKind::Select, 1),
                                         selectedLayerIndex, selectedRuleIndex),
          "a Select is not a recipe move");
    Check(selectedLayerIndex == 1 && selectedRuleIndex == 0,
          "Select moved the layer selection and reset the rule selection");

    Check(ApplyMarkerRuleLayerListSignal(layers, MakeSignal(DraggableListSignalKind::ToggleVisibility, 0),
                                         selectedLayerIndex, selectedRuleIndex)
              && !layers[0].bEnabled,
          "ToggleVisibility flips the layer's own bEnabled and moves the recipe");
    Check(ApplyMarkerRuleLayerListSignal(layers, MakeSignal(DraggableListSignalKind::ToggleLock, 0),
                                         selectedLayerIndex, selectedRuleIndex)
              && layers[0].bHidden,
          "ToggleLock flips the layer's own bHidden and moves the recipe");
}

// Acceptance item 3: deleting a layer takes every one of its rules with it — nothing orphaned,
// nothing reparented.
void RunRuleLayerDeleteChecks() {
    std::vector<Params::MarkerRuleLayer> layers = MakeTwoLayersOfTwoRules();
    const int totalRulesBefore =
        static_cast<int>(layers[0].rules.size() + layers[1].rules.size());
    int selectedLayerIndex = 0, selectedRuleIndex = 0;

    Check(ApplyMarkerRuleLayerListSignal(layers, MakeSignal(DraggableListSignalKind::Delete, 0),
                                         selectedLayerIndex, selectedRuleIndex),
          "deleting a layer moves the recipe");
    Check(layers.size() == 1, "markerRuleLayers shrank by one");
    const int totalRulesAfter = static_cast<int>(layers.empty() ? 0 : layers[0].rules.size());
    Check(totalRulesAfter == totalRulesBefore - 2, "the deleted layer's rule count left with it");
    Check(layers[0].name == "Second", "the survivor is the layer that was NOT deleted");
}

// Acceptance item 4: an out-of-range sourceRowIndex on either applier is rejected outright.
void RunOutOfRangeRuleLayerChecks() {
    std::vector<Params::MarkerRuleLayer> layers = MakeTwoLayersOfTwoRules();
    int selectedLayerIndex = 0, selectedRuleIndex = 0;
    Check(!ApplyMarkerRuleLayerListSignal(layers, MakeSignal(DraggableListSignalKind::Delete, 5),
                                          selectedLayerIndex, selectedRuleIndex),
          "an out-of-range layer row is rejected");
    Check(layers.size() == 2, "and nothing was mutated");

    MarkersTabState state;
    Check(!ApplyMarkerRuleListSignal(layers[0], MakeSignal(DraggableListSignalKind::Delete, 9), state),
          "an out-of-range rule row is rejected");
    Check(layers[0].rules.size() == 2, "and nothing was mutated");
}

// Acceptance item 5: SelectedMarkerRule's two-index walk, every miss shape.
void RunSelectedMarkerRuleFenceChecks() {
    std::vector<Params::MarkerRuleLayer> layers = MakeTwoLayersOfTwoRules();
    MarkersTabState state;
    state.selectedRuleLayerIndex = 0;
    state.selectedRuleIndex      = 1;
    Check(SelectedMarkerRule(layers, state) == &layers[0].rules[1], "a valid pair resolves");

    state.selectedRuleLayerIndex = 5;
    Check(SelectedMarkerRule(layers, state) == nullptr, "an out-of-range layer index resolves to null");

    state.selectedRuleLayerIndex = 0;
    state.selectedRuleIndex      = 9;
    Check(SelectedMarkerRule(layers, state) == nullptr,
          "an out-of-range rule index within a valid layer resolves to null");

    std::vector<Params::MarkerRuleLayer> emptyLayers;
    state.selectedRuleLayerIndex = 0;
    state.selectedRuleIndex      = 0;
    Check(SelectedMarkerRule(emptyLayers, state) == nullptr, "an empty markerRuleLayers resolves to null");
}

} // namespace

void RunMarkerRuleLayerAcceptanceChecks() {
    RunRuleLayerReorderAndToggleChecks();
    RunRuleLayerDeleteChecks();
    RunOutOfRangeRuleLayerChecks();
    RunSelectedMarkerRuleFenceChecks();
}
