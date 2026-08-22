// MarkersTab_ManualInstance_UI.cpp — the instance-level half of the manual markers editor: the
// selected group's own instance roster (list, Add/Remove) and the selected instance's Alias/Name/
// Position editor. The group-level half lives in MarkersTab_Manual_UI.cpp (ARCH §1.5 — one class
// of methods split across two files behind MarkersTab_Manual_UI.h).
// Shared widgets only: DraggableList, Combo, TextInput, SliderScalar. Nothing here notifies
// Pipeline::PreviewDriver (MarkersTab_Manual_UI.h SCOPE).
#include "MarkersTab_Manual_UI.h"
#include "Combo_UI.h"
#include "DraggableListWidget_UI.h"
#include "TextInput_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

DraggableListSignal DrawMarkerInstanceList(const std::vector<Params::MarkerTransform>& transforms,
                                           int selectedInstanceIndex) {
    return DraggableList<Params::MarkerTransform>::Render(
        "manualMarkerInstances", transforms,
        [&](int rowIndex) {
            DraggableListRow row;
            row.label = MarkerInstanceRowLabel(transforms[static_cast<std::size_t>(rowIndex)]);
            return row;
        },
        [](int) {},
        selectedInstanceIndex);
}

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

// Alias (free text), Name (army picker for a Spawn group, else free text), and the three position
// sliders. Reports whether the recipe moved, for the caller's dirty bookkeeping — never
// Pipeline::PreviewDriver (MarkersTab_Manual_UI.h SCOPE).
bool DrawSelectedMarkerInstance(Params::MarkerTransform& transform, const Params::MarkerInstanceGroup& group,
                                const std::vector<Params::Army>& armies, ManualMarkersState& state) {
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

    bCommitted = DrawSliderScalar("Position X", transform.transform.positionX, state.positionHorizontalRange,
                                  state.positionXToggle, WidgetStyle(), "%.1f").bCommitted || bCommitted;
    bCommitted = DrawSliderScalar("Position Y (Elevation)", transform.transform.positionY,
                                  state.positionElevationRange, state.positionYToggle, WidgetStyle(),
                                  "%.1f").bCommitted || bCommitted;
    bCommitted = DrawSliderScalar("Position Z", transform.transform.positionZ, state.positionHorizontalRange,
                                  state.positionZToggle, WidgetStyle(), "%.1f").bCommitted || bCommitted;
    return bCommitted;
}

} // namespace

void DrawMarkerInstanceSection(Params::MarkerInstanceGroup& group, const std::vector<Params::Army>& armies,
                               ManualMarkersState& state) {
    bool bInstancesMoved = false;
    const DraggableListSignal signal = DrawMarkerInstanceList(group.transforms, state.selectedInstanceIndex);
    if (signal.bHasSignal())
        bInstancesMoved = ApplyMarkerInstanceListSignal(group.transforms, state, signal) || bInstancesMoved;
    bInstancesMoved = DrawMarkerInstanceListButtons(group.transforms, state) || bInstancesMoved;

    Params::MarkerTransform* const instance = SelectedMarkerInstance(group.transforms, state.selectedInstanceIndex);
    if (instance == nullptr) ImGui::TextUnformatted("Select a marker instance to edit it.");
    else bInstancesMoved = DrawSelectedMarkerInstance(*instance, group, armies, state) || bInstancesMoved;

    // Scoped to THIS group only: the wire format's inner dictionary key only needs uniqueness
    // within its own outer group (MarkersTab_Manual_UI.h header comment).
    if (bInstancesMoved) MakeNamesUnique(group.transforms);
}

} // namespace Ui
} // namespace SanmapGen
