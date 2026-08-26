// MarkersTab_RuleLayers_UI_Test.cpp — STEP80 acceptance: the two-level MarkerRuleLayer/MarkerRule
// list appliers and the two-index SelectedMarkerRule walk, driven headless (no imgui frame, window
// or GL context). Sibling TU to MarkersTab_UI_Test.cpp, which owns main() and the shared
// `Check`/`failureCount` (ARCH §1.5 — one binary, split translation units).
#include "MarkersTab_UI.h"
#include "ListWidget_TestFrame_UI.h"

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

// STEP128 §4: the row's own free-text "Marker Type" field draws ONLY when the row's own
// markerTypeName is empty — proved by height diff (RunGroupStratumIndexRemovedCheck's own technique,
// LayerEditor_InlineSettings_UI_Test.cpp), since presence/absence of a whole row is what needs
// asserting, not the field's own pixel content ("verified by eye against a live frame" everywhere
// else in this library). One layer, one frame, headless — `previewDriver`/`iconManifest` stay null;
// `state.selectedRuleLayerIndex` stays -1 (default) so the nested rule list never draws either,
// isolating the height delta to exactly the conditional field.
float RunRuleLayerListBodyHeight(const std::string& markerTypeName, const std::string& markerTypeNameFilter) {
    HeadlessImguiSession session;
    std::vector<Params::MarkerRuleLayer> layers(1);
    layers[0].markerTypeName = markerTypeName;
    MarkersTabState state;
    float height = 0.0f;
    RunHeadlessFrame(HeadlessMouseState(), ImVec2(400.0f, 400.0f), [&] {
        const float startY = ImGui::GetCursorPosY();
        DrawRuleLayerListBody(layers, state, nullptr, nullptr, markerTypeNameFilter);
        height = ImGui::GetCursorPosY() - startY;
    });
    return height;
}

void RunRuleLayerMarkerTypeFieldConditionalCheck() {
    const float emptyTypeHeight = RunRuleLayerListBodyHeight("", "");
    const float namedTypeHeight = RunRuleLayerListBodyHeight("Alloy", "Alloy");
    Check(emptyTypeHeight > namedTypeHeight,
          "an ungrouped row with markerTypeName.empty() draws the free-text 'Marker Type' field, "
          "costing extra height a non-empty-typed row does not");
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

} // namespace

void RunMarkerRuleLayerAcceptanceChecks() {
    RunRuleLayerReorderAndToggleChecks();
    RunRuleLayerDeleteChecks();
    RunOutOfRangeRuleLayerChecks();
    RunSelectedMarkerRuleFenceChecks();
    RunIsMarkerRuleLayerRowSuppressedChecks();
    RunDrawAddMarkerRuleLayerButtonTypeSeedChecks();
    RunRuleLayerMarkerTypeFieldConditionalCheck();
}
