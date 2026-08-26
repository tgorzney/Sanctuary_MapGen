// MarkersTab_RuleLayers_UI_Test.cpp — STEP80 acceptance: the two-level MarkerRuleLayer/MarkerRule
// list appliers and the two-index SelectedMarkerRule walk, driven headless (no imgui frame, window
// or GL context). Sibling TU to MarkersTab_UI_Test.cpp, which owns main() and the shared
// `Check`/`failureCount` (ARCH §1.5 — one binary, split translation units).
#include "MarkersTab_UI.h"
#include "ListWidget_TestFrame_UI.h"
#include "SymmetryClusterInstanceList_UI.h"
#include "../data/PlacementInstances_DATA.h"

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

// STEP125, ARCH §19.15(c): the `||` composition — type-mismatch-only, bundle-membership-only, both,
// and neither — proves the compound case isn't accidentally an XOR.
void RunIsMarkerRuleLayerRowSuppressedChecks() {
    Params::MarkerRuleLayer typeMismatchOnly;
    typeMismatchOnly.parentBundleIdentifier = -1;
    typeMismatchOnly.markerTypeName         = "Alloy";
    Check(!IsMarkerRuleLayerRowSuppressed(typeMismatchOnly, "Alloy"),
          "an ungrouped layer whose own type matches the filter is NOT suppressed");
    Check(IsMarkerRuleLayerRowSuppressed(typeMismatchOnly, "Plasma"),
          "an ungrouped layer whose own type does NOT match the filter IS suppressed (type-mismatch only)");

    Params::MarkerRuleLayer bundleMembershipOnly;
    bundleMembershipOnly.parentBundleIdentifier = 5;
    bundleMembershipOnly.markerTypeName         = "Alloy";
    Check(IsMarkerRuleLayerRowSuppressed(bundleMembershipOnly, "Alloy"),
          "a bundled layer IS suppressed even when its own type matches the filter (bundle-membership only)");

    Params::MarkerRuleLayer both;
    both.parentBundleIdentifier = 5;
    both.markerTypeName         = "Alloy";
    Check(IsMarkerRuleLayerRowSuppressed(both, "Plasma"),
          "a bundled layer with a mismatched type IS suppressed (the compound case, not an XOR)");
}

// STEP125: DrawAddMarkerRuleLayerButton's new markerTypeNameForNewLayer parameter. A live headless
// imgui frame (mirrors MarkersTab_ManualLayerColorOverrideHeader_UI_Test.cpp's own click pattern),
// since the button must actually be clicked to push a new row.
struct AddRuleLayerButtonFrameResult {
    ImVec2 origin;
    ImVec2 size;
    bool   bReturned = false;
};

AddRuleLayerButtonFrameResult RunAddRuleLayerButtonFrame(HeadlessMouseState mouse,
        std::vector<Params::MarkerRuleLayer>& layers, MarkersTabState& state,
        const std::string& markerTypeNameForNewLayer) {
    AddRuleLayerButtonFrameResult result;
    RunHeadlessFrame(mouse, ImVec2(300.0f, 100.0f), [&] {
        result.origin    = ImGui::GetCursorScreenPos();
        result.bReturned = DrawAddMarkerRuleLayerButton(layers, state, -1, markerTypeNameForNewLayer);
        result.size      = ImGui::GetItemRectSize();
    });
    return result;
}

void ClickAddRuleLayerButton(std::vector<Params::MarkerRuleLayer>& layers, MarkersTabState& state,
                             const std::string& markerTypeNameForNewLayer,
                             AddRuleLayerButtonFrameResult& outClickedResult) {
    const AddRuleLayerButtonFrameResult settle =
        RunAddRuleLayerButtonFrame(HeadlessMouseState(), layers, state, markerTypeNameForNewLayer);
    const ImVec2 center(settle.origin.x + settle.size.x * 0.5f, settle.origin.y + settle.size.y * 0.5f);
    HeadlessMouseState hover;   hover.position = center;
    HeadlessMouseState press   = hover; press.bLeftButtonDown   = true;
    HeadlessMouseState release = hover; release.bLeftButtonDown = false;
    RunAddRuleLayerButtonFrame(hover, layers, state, markerTypeNameForNewLayer);
    RunAddRuleLayerButtonFrame(press, layers, state, markerTypeNameForNewLayer);
    outClickedResult = RunAddRuleLayerButtonFrame(release, layers, state, markerTypeNameForNewLayer);
}

void RunDrawAddMarkerRuleLayerButtonTypeSeedChecks() {
    HeadlessImguiSession session;

    std::vector<Params::MarkerRuleLayer> seededLayers;
    MarkersTabState seededState;
    AddRuleLayerButtonFrameResult seededClick;
    ClickAddRuleLayerButton(seededLayers, seededState, "Alloy", seededClick);
    Check(seededClick.bReturned, "clicking Add Layer reports the recipe moved");
    Check(!seededLayers.empty() && seededLayers.back().markerTypeName == "Alloy",
          "markerTypeNameForNewLayer = \"Alloy\" lands on the newly pushed layer's own markerTypeName");

    std::vector<Params::MarkerRuleLayer> defaultLayers;
    MarkersTabState defaultState;
    AddRuleLayerButtonFrameResult defaultClick;
    ClickAddRuleLayerButton(defaultLayers, defaultState, "", defaultClick);
    Check(!defaultLayers.empty() && defaultLayers.back().markerTypeName.empty(),
          "the parameter omitted (default, empty) leaves markerTypeName empty — unchanged existing behavior");
}

// STEP132 (ARCH §19.27) — the builder: markers tagged ruleIndex 0/0/1 map to exactly the right array
// positions per rule.
void RunProceduralInstanceRuleIndexBuilderChecks() {
    Data::PlacementInstances markers;
    Data::PlacementInstance first;  first.ruleIndex = 0;
    Data::PlacementInstance second; second.ruleIndex = 0;
    Data::PlacementInstance third;  third.ruleIndex = 1;
    markers.Append(first); markers.Append(second); markers.Append(third);

    const ProceduralInstanceRuleIndex_UI index = BuildProceduralInstanceRuleIndex(markers);
    Check(index.instancesByRuleIndex.at(0).size() == 2u
              && index.instancesByRuleIndex.at(0)[0] == 0 && index.instancesByRuleIndex.at(0)[1] == 1,
          "ruleIndex 0 maps to array positions {0, 1}, in order");
    Check(index.instancesByRuleIndex.at(1).size() == 1u && index.instancesByRuleIndex.at(1)[0] == 2,
          "ruleIndex 1 maps to array position {2}");
    Check(index.instancesByRuleIndex.find(2) == index.instancesByRuleIndex.end(),
          "a ruleIndex with no instances has no entry at all in the index");
}

// STEP132 (ARCH §19.27) — the OPPOSITE membership predicate from §19.26's manual `== 0`: bucket
// SIZE, not id value. `symmetryIdentifier` values {5,5,9,12} — bucket 5 has 2 members, 9 and 12 have
// 1 each — so exactly 1 cluster (id 5, count 2) renders first, then 2 flat rows, id 0 never appears
// (ARCH §19.27's own confirmed minting semantics).
void RunProceduralSymmetryBucketSizePredicateChecks() {
    HeadlessImguiSession session;
    const std::vector<int> symmetryIdentifiers = { 5, 5, 9, 12 };   // item == its own index
    std::vector<int> visitOrder;
    RunHeadlessFrame(HeadlessMouseState(), ImVec2(300.0f, 300.0f), [&] {
        const std::vector<int> items = { 0, 1, 2, 3 };
        DrawSymmetryClusterInstanceList<int>(items,
            [&](const int& item) { return symmetryIdentifiers[static_cast<std::size_t>(item)]; },
            [](int /*groupIdentifier*/, int bucketSize) { return bucketSize > 1; },
            [&](const int& item) { visitOrder.push_back(item); });
    });
    Check(visitOrder.size() == 4u, "every one of the 4 instances is visited exactly once");
    Check(visitOrder[0] == 0 && visitOrder[1] == 1,
          "symmetry id 5's 2 members render FIRST, as the ONE real cluster (bucket size, not id value)");
    Check(visitOrder[2] == 2 && visitOrder[3] == 3,
          "the two SIZE-1 buckets (id 9, id 12) render flat, in order, regardless of their non-zero id");
}

// STEP132 (ARCH §19.27) — before the first generation (placedMarkers == nullptr), the Rule row's
// instance list renders the "generate first" placeholder: inert, not a crash, not an interactive row.
void RunRuleInstanceListNullPlacedMarkersChecks() {
    HeadlessImguiSession session;
    ImGuiID lastItemId = 0;
    RunHeadlessFrame(HeadlessMouseState(), ImVec2(300.0f, 200.0f), [&] {
        DrawRuleInstanceList(ProceduralInstanceListContext_UI());
        lastItemId = ImGui::GetItemID();
    });
    Check(lastItemId == 0, "the null-placedMarkers placeholder is inert text, never an interactive row");
}

} // namespace

void RunMarkerRuleLayerAcceptanceChecks() {
    RunRuleLayerReorderAndToggleChecks();
    RunRuleLayerDeleteChecks();
    RunOutOfRangeRuleLayerChecks();
    RunSelectedMarkerRuleFenceChecks();
    RunIsMarkerRuleLayerRowSuppressedChecks();
    RunDrawAddMarkerRuleLayerButtonTypeSeedChecks();
    RunProceduralInstanceRuleIndexBuilderChecks();
    RunProceduralSymmetryBucketSizePredicateChecks();
    RunRuleInstanceListNullPlacedMarkersChecks();
}
