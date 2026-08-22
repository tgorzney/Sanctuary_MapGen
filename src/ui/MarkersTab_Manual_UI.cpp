// MarkersTab_Manual_UI.cpp — the group-level half of the manual markers editor: Add Marker Type,
// the group stack, and the selected group's own Name/Resource fields, plus the top-level
// `DrawManualMarkers` orchestration. The instance-level half lives in
// MarkersTab_ManualInstance_UI.cpp (ARCH §1.5 — one class of methods split across two files).
// Shared widgets only: DraggableList, Combo, TextInput, Checkbox, Section. Nothing here notifies
// Pipeline::PreviewDriver (MarkersTab_Manual_UI.h SCOPE).
#include "MarkersTab_Manual_UI.h"
#include "Checkbox_UI.h"
#include "Combo_UI.h"
#include "DraggableListWidget_UI.h"
#include "TextInput_UI.h"
#include "imgui.h"
#include <cstring>

namespace SanmapGen {
namespace Ui {
namespace {

DraggableListSignal DrawMarkerGroupList(const std::vector<Params::MarkerInstanceGroup>& markers,
                                        int selectedGroupIndex) {
    return DraggableList<Params::MarkerInstanceGroup>::Render(
        "manualMarkerGroups", markers,
        [&](int rowIndex) {
            DraggableListRow row;
            row.label = MarkerGroupRowLabel(markers[static_cast<std::size_t>(rowIndex)]);
            return row;
        },
        [](int) {},                                    // header-only rows: the editor is below
        selectedGroupIndex);
}

// A missing `Spawn` group degrades to "that army gets no commander" (soft engine behavior), so no
// row is protected from delete the way `PlayableArea` is (AreasTab_UI.cpp).
bool ApplyMarkerGroupListSignal(std::vector<Params::MarkerInstanceGroup>& markers,
                                ManualMarkersState& state, const DraggableListSignal& signal) {
    const int rowIndex = signal.sourceRowIndex;
    const bool bRowValid = rowIndex >= 0 && rowIndex < static_cast<int>(markers.size());
    if (signal.kind == DraggableListSignalKind::Select) {
        if (bRowValid && rowIndex != state.selectedGroupIndex) {
            state.selectedGroupIndex = rowIndex;
            state.selectedInstanceIndex = -1;   // a different group's roster: no carried-over pick
        }
        return false;
    }
    if (signal.kind == DraggableListSignalKind::ToggleVisibility
        || signal.kind == DraggableListSignalKind::ToggleLock) return false;
    if (!ApplyDraggableListSignal(markers, signal)) return false;
    state.selectedGroupIndex = ResolvedMarkerGroupSelection(state.selectedGroupIndex,
                                                            static_cast<int>(markers.size()));
    state.selectedInstanceIndex = -1;
    return true;
}

// "Add Marker Type": a Combo_UI seeded from the existing category vocabulary, seeding `bResource`
// true only for "Alloys" (editable after, on the selected-group settings below).
bool DrawAddMarkerGroupControls(std::vector<Params::MarkerInstanceGroup>& markers, ManualMarkersState& state) {
    ComboOptions options;
    options.labels = markerCategoryLabels;
    options.count  = kMarkerCategoryCount;
    DrawCombo("Marker Type", state.addGroupCategoryIndex, options);
    if (!ImGui::Button("Add Marker Type")) return false;
    const int categoryIndex = ResolvedComboSelection(state.addGroupCategoryIndex, options);
    const char* const categoryLabel = categoryIndex >= 0 ? markerCategoryLabels[categoryIndex] : "Generic";
    Params::MarkerInstanceGroup group;
    group.name      = categoryLabel;
    group.bResource = std::strcmp(categoryLabel, "Alloys") == 0;
    markers.push_back(group);
    state.selectedGroupIndex    = static_cast<int>(markers.size()) - 1;
    state.selectedInstanceIndex = -1;
    return true;
}

// The selected group's own name and resource flag. `.sanmap markers` is a dictionary keyed by
// `name`, so the uniqueness repair below is not cosmetic (unlike Props/Decals' layer names).
bool DrawSelectedMarkerGroupSettings(Params::MarkerInstanceGroup& group, ManualMarkersState& state) {
    const bool bNameCommitted     = DrawTextInput("Name", group.name, state.nameRules).bCommitted;
    const bool bResourceCommitted = DrawCheckbox("Resource", group.bResource).bCommitted;
    return bNameCommitted || bResourceCommitted;
}

} // namespace

Params::MarkerInstanceGroup* DrawMarkerGroupSection(std::vector<Params::MarkerInstanceGroup>& markers,
                                                     ManualMarkersState& state) {
    bool bGroupsMoved = DrawAddMarkerGroupControls(markers, state);
    const DraggableListSignal signal = DrawMarkerGroupList(markers, state.selectedGroupIndex);
    if (signal.bHasSignal()) bGroupsMoved = ApplyMarkerGroupListSignal(markers, state, signal) || bGroupsMoved;
    Params::MarkerInstanceGroup* const group = SelectedMarkerGroup(markers, state.selectedGroupIndex);
    if (group != nullptr) bGroupsMoved = DrawSelectedMarkerGroupSettings(*group, state) || bGroupsMoved;
    // `.sanmap markers` is a dictionary keyed by NAME, same posture AreasTab_UI.cpp uses for
    // `recipe.areas` — the repair runs whenever a group could have moved.
    if (bGroupsMoved) MakeNamesUnique(markers);
    return group;
}

void DrawManualMarkers(std::vector<Params::MarkerInstanceGroup>& markers,
                       const std::vector<Params::Army>& armies, ManualMarkersState& state) {
    if (!DrawSectionBegin("Manual Markers", state.section)) return;
    ImGui::TextWrapped("The hand-authored marker roster: commander spawns, resources and any other "
                       "fixed-position marker exported beside the procedural rules above.");
    Params::MarkerInstanceGroup* const group = DrawMarkerGroupSection(markers, state);
    if (group == nullptr) {
        ImGui::TextUnformatted("Select a marker type to edit its roster.");
    } else {
        ImGui::Separator();
        DrawMarkerInstanceSection(*group, armies, state);
    }
    DrawSectionEnd();
}

} // namespace Ui
} // namespace SanmapGen
