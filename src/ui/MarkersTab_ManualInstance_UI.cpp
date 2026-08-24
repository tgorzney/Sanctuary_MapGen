// MarkersTab_ManualInstance_UI.cpp — the instance-level half of the manual markers editor: the
// selected group's own instance roster (list, Add/Remove) and the selected instance's Alias/Name/
// Position editor. The group-level half lives in MarkersTab_Manual_UI.cpp (ARCH §1.5 — one class
// of methods split across two files behind MarkersTab_Manual_UI.h).
// Shared widgets only: DraggableList, Combo, TextInput, SliderScalar. Nothing here notifies
// Pipeline::PreviewDriver (MarkersTab_Manual_UI.h SCOPE).
#include "MarkersTab_Manual_UI.h"
#include "Combo_UI.h"
#include "DraggableListWidget_UI.h"
#include "MarkersTab_ManualLayers_UI.h"
#include "TextInput_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

bool ApplyMarkerInstanceListSignal(std::vector<Params::MarkerTransform>& transforms, ManualMarkersState& state,
                                   const DraggableListSignal& signal) {
    const int rowIndex = signal.sourceRowIndex;
    const bool bRowValid = rowIndex >= 0 && rowIndex < static_cast<int>(transforms.size());
    if (signal.kind == DraggableListSignalKind::Select) {
        if (bRowValid) state.selectedInstanceIndex = rowIndex;
        return false;
    }
    if (signal.kind == DraggableListSignalKind::ToggleVisibility
        || signal.kind == DraggableListSignalKind::ToggleLock) return false;
    if (!ApplyDraggableListSignal(transforms, signal)) return false;
    state.selectedInstanceIndex = ResolvedMarkerInstanceSelection(state.selectedInstanceIndex,
                                                                  static_cast<int>(transforms.size()));
    return true;
}

// Add/Remove — `MarkersTab_UI.cpp::DrawRuleListButtons`'s pattern, applied after the list closes
// so the vector never moves under a live row.
bool DrawMarkerInstanceListButtons(std::vector<Params::MarkerTransform>& transforms, ManualMarkersState& state) {
    bool bInstancesMoved = false;
    if (ImGui::Button("Add Instance")) {
        Params::MarkerTransform transform;
        transform.name = NextMarkerInstanceName(static_cast<int>(transforms.size()));
        transforms.push_back(transform);
        state.selectedInstanceIndex = static_cast<int>(transforms.size()) - 1;
        bInstancesMoved = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Remove Selected")
        && SelectedMarkerInstance(transforms, state.selectedInstanceIndex) != nullptr) {
        transforms.erase(transforms.begin() + state.selectedInstanceIndex);
        state.selectedInstanceIndex = ResolvedMarkerInstanceSelection(state.selectedInstanceIndex,
                                                                       static_cast<int>(transforms.size()));
        bInstancesMoved = true;
    }
    return bInstancesMoved;
}

// STEP81 part (b): the Layer picker, drawn after Name and before Position. `layerIndex`'s legal
// domain is [0, markerLayers.size()) with a 0 default; `-1` is not a valid `layerIndex` value
// anywhere in the marker domain (that sentinel belongs to `layerId`, a different field). A direct
// bind of `transform.layerIndex` to `DrawCombo` would let an empty `markerLayers` write -1 into a
// live `layerIndex` (Combo_UI.h's out-of-range-resolves-to--1 contract), so the value is mirrored,
// gated, and only a valid pick is stored — the same load/store mirror pattern MarkersTab_UI.h
// already uses for MarkerRule's int/range fields.
void DrawMarkerInstanceLayerPicker(Params::MarkerTransform& transform,
                                   const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                   ManualMarkersState& state) {
    if (markerLayers.empty()) {
        ImGui::TextUnformatted("No marker layers yet — add one in Manual Marker Layers.");
        return;
    }
    state.layerPickerLabels.clear();
    for (const Params::MarkerInstanceLayer& layer : markerLayers)
        state.layerPickerLabels.push_back(ManualMarkerLayerRowLabel(layer));
    ComboOptions options;
    options.labels = state.layerPickerLabels.data();
    options.count  = static_cast<int>(state.layerPickerLabels.size());
    int pickedLayerIndex = transform.layerIndex;                 // mirror, not a direct bind
    DrawCombo("Layer", pickedLayerIndex, options);
    if (pickedLayerIndex >= 0) transform.layerIndex = pickedLayerIndex;   // store only a valid pick
}

// Alias (free text), Name (army picker for a Spawn group, else free text), Layer (STEP81 part (b)),
// and the three position sliders. Reports whether the recipe moved, for the caller's dirty
// bookkeeping — never Pipeline::PreviewDriver (MarkersTab_Manual_UI.h SCOPE).
bool DrawSelectedMarkerInstance(Params::MarkerTransform& transform, const Params::MarkerInstanceGroup& group,
                                const std::vector<Params::Army>& armies,
                                const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                ManualMarkersState& state) {
    bool bCommitted = DrawTextInput("Alias", transform.alias).bCommitted;

    if (IsSpawnMarkerGroup(group)) {
        std::vector<const char*> armyLabels;
        armyLabels.reserve(armies.size());
        for (const Params::Army& army : armies) armyLabels.push_back(ArmyPickerRowLabel(army));
        ComboOptions armyOptions;
        armyOptions.labels = armyLabels.data();
        armyOptions.count  = static_cast<int>(armyLabels.size());
        int armyPickIndex = ResolvedSpawnMarkerArmyPickIndex(armies, transform.name);
        const WidgetChange armyChange = DrawCombo("Name (Army)", armyPickIndex, armyOptions);
        if (armyChange.bCommitted && armyPickIndex >= 0 && armyPickIndex < static_cast<int>(armies.size())) {
            transform.name = armies[static_cast<std::size_t>(armyPickIndex)].name;
            bCommitted = true;
        }
    } else {
        bCommitted = DrawTextInput("Name", transform.name, state.nameRules).bCommitted || bCommitted;
    }

    DrawMarkerInstanceLayerPicker(transform, markerLayers, state);

    bCommitted = DrawSliderScalar("Position X", transform.transform.positionX, state.positionHorizontalRange,
                                  state.positionXToggle, WidgetStyle(), "%.1f").bCommitted || bCommitted;
    bCommitted = DrawSliderScalar("Position Y (Elevation)", transform.transform.positionY,
                                  state.positionElevationRange, state.positionYToggle, WidgetStyle(),
                                  "%.1f").bCommitted || bCommitted;
    bCommitted = DrawSliderScalar("Position Z", transform.transform.positionZ, state.positionHorizontalRange,
                                  state.positionZToggle, WidgetStyle(), "%.1f").bCommitted || bCommitted;
    return bCommitted;
}

// STEP110: each row's body, whenever the row's own CollapsingHeader is open (DraggableList's own
// per-row expand/collapse state — never gated on `state.selectedInstanceIndex`), draws that row's
// OWN Alias/Name-or-Army/Layer/Position settings directly below its header, so an expanded row
// never shows another row's settings. `bAnyInstanceCommitted` is set true if any expanded row's
// fields committed this frame, feeding the caller's dirty bookkeeping and `MakeNamesUnique` repair.
DraggableListSignal DrawMarkerInstanceList(std::vector<Params::MarkerTransform>& transforms,
                                           const Params::MarkerInstanceGroup& group,
                                           const std::vector<Params::Army>& armies,
                                           const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                           ManualMarkersState& state, bool& bAnyInstanceCommitted) {
    return DraggableList<Params::MarkerTransform>::Render(
        "manualMarkerInstances", transforms,
        [&](int rowIndex) {
            DraggableListRow row;
            row.label = MarkerInstanceRowLabel(transforms[static_cast<std::size_t>(rowIndex)]);
            return row;
        },
        [&](int rowIndex) {
            if (DrawSelectedMarkerInstance(transforms[static_cast<std::size_t>(rowIndex)], group, armies,
                                           markerLayers, state))
                bAnyInstanceCommitted = true;
        },
        state.selectedInstanceIndex);
}

} // namespace

void DrawMarkerInstanceSection(Params::MarkerInstanceGroup& group, const std::vector<Params::Army>& armies,
                               const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                               ManualMarkersState& state) {
    bool bInstancesMoved = false;
    bool bAnyInstanceCommitted = false;
    const DraggableListSignal signal =
        DrawMarkerInstanceList(group.transforms, group, armies, markerLayers, state, bAnyInstanceCommitted);
    if (signal.bHasSignal())
        bInstancesMoved = ApplyMarkerInstanceListSignal(group.transforms, state, signal) || bInstancesMoved;
    bInstancesMoved = DrawMarkerInstanceListButtons(group.transforms, state) || bInstancesMoved;
    bInstancesMoved = bAnyInstanceCommitted || bInstancesMoved;

    // Scoped to THIS group only: the wire format's inner dictionary key only needs uniqueness
    // within its own outer group (MarkersTab_Manual_UI.h header comment).
    if (bInstancesMoved) MakeNamesUnique(group.transforms);
}

} // namespace Ui
} // namespace SanmapGen
