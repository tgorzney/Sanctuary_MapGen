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
// so the vector never moves under a live row. Add targets whichever marker layer is SELECTED in
// the Manual Marker Layers list (`selectedMarkerLayerIndex`), not hardcoded to layer 0 — falling
// back to 0 only for an unselected/stale pick. Both buttons gate on `markerLayers`' lock state:
// Add on the selected marker layer itself, Remove on the SELECTED INSTANCE's own layer (which may
// differ from the selected marker layer).
bool DrawMarkerInstanceListButtons(std::vector<Params::MarkerTransform>& transforms, ManualMarkersState& state,
                                   const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                   int selectedMarkerLayerIndex) {
    bool bInstancesMoved = false;
    ImGui::BeginDisabled(IsMarkerInstanceLayerLocked(markerLayers, selectedMarkerLayerIndex));
    if (ImGui::Button("Add Instance")) {
        Params::MarkerTransform transform;
        transform.name = NextMarkerInstanceName(static_cast<int>(transforms.size()));
        transform.layerIndex = (selectedMarkerLayerIndex >= 0
                                && selectedMarkerLayerIndex < static_cast<int>(markerLayers.size()))
                               ? selectedMarkerLayerIndex : 0;
        transforms.push_back(transform);
        state.selectedInstanceIndex = static_cast<int>(transforms.size()) - 1;
        bInstancesMoved = true;
    }
    ImGui::EndDisabled();
    const Params::MarkerTransform* const selectedForRemove =
        SelectedMarkerInstance(transforms, state.selectedInstanceIndex);
    ImGui::SameLine();
    ImGui::BeginDisabled(selectedForRemove != nullptr
                         && IsMarkerInstanceLayerLocked(markerLayers, selectedForRemove->layerIndex));
    if (ImGui::Button("Remove Selected") && selectedForRemove != nullptr) {
        transforms.erase(transforms.begin() + state.selectedInstanceIndex);
        state.selectedInstanceIndex = ResolvedMarkerInstanceSelection(state.selectedInstanceIndex,
                                                                       static_cast<int>(transforms.size()));
        bInstancesMoved = true;
    }
    ImGui::EndDisabled();
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

    const bool bLayerLocked = IsMarkerInstanceLayerLocked(markerLayers, transform.layerIndex);
    ImGui::BeginDisabled(bLayerLocked);
    ImGui::TextUnformatted("Position");
    ImGui::Columns(3, "markerPositionColumns", false);
    const WidgetChange positionXChange = DrawSliderScalar("X", transform.transform.positionX,
        state.positionHorizontalRange, state.positionXToggle, WidgetStyle(), "%.1f");
    ImGui::NextColumn();
    const WidgetChange positionYChange = DrawSliderScalar("Y", transform.transform.positionY,
        state.positionElevationRange, state.positionYToggle, WidgetStyle(), "%.1f");
    ImGui::NextColumn();
    const WidgetChange positionZChange = DrawSliderScalar("Z", transform.transform.positionZ,
        state.positionHorizontalRange, state.positionZToggle, WidgetStyle(), "%.1f");
    ImGui::Columns(1);
    if (positionXChange.bCommitted || positionZChange.bCommitted)
        QuantizeMarkerPositionToLayerGrid(markerLayers, transform.layerIndex,
                                          transform.transform.positionX, transform.transform.positionZ);
    ImGui::EndDisabled();
    bCommitted = positionXChange.bCommitted || positionYChange.bCommitted || positionZChange.bCommitted || bCommitted;
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

// STEP114: the selected instance's icon-override picker — drawn once, after the rows, gated on
// the current target (`state.selectedGroupIndex`/`selectedInstanceIndex`, the SAME target-
// selection the Remove button above already uses), mirroring DrawGlobalIconPicker's "one shared
// grid below the rows" layout. Never writes `selected->iconNameOverride` directly from the grid's
// volatile int — the shell resolves `state.iconOverrideGridState.selectedIconId` into the string
// field (Application_AssetPanel_UI.cpp's ResolveIconSelections); this tab never touches the atlas.
void DrawMarkerInstanceIconOverridePicker(std::vector<Params::MarkerTransform>& transforms,
                                          ManualMarkersState& state,
                                          const IconAtlasManifest* iconManifest) {
    Params::MarkerTransform* const selected =
        SelectedMarkerInstance(transforms, state.selectedInstanceIndex);
    if (selected == nullptr) return;
    ImGui::Text("Icon Override: %s", selected->iconNameOverride.empty()
                                     ? "(type default)" : selected->iconNameOverride.c_str());
    if (!selected->iconNameOverride.empty() && ImGui::Button("Clear Icon Override"))
        selected->iconNameOverride.clear();
    if (iconManifest == nullptr) {
        ImGui::TextUnformatted("No resident icon atlas: run the host's icon scan first.");
        return;
    }
    DrawIconGrid("Marker Instance Icon", *iconManifest, state.iconOverrideGridState, state.iconOverrideGridHeight);
}

} // namespace

void DrawMarkerInstanceSection(Params::MarkerInstanceGroup& group, const std::vector<Params::Army>& armies,
                               const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                               ManualMarkersState& state, int selectedMarkerLayerIndex,
                               const IconAtlasManifest* iconManifest) {
    bool bInstancesMoved = false;
    bool bAnyInstanceCommitted = false;
    const DraggableListSignal signal =
        DrawMarkerInstanceList(group.transforms, group, armies, markerLayers, state, bAnyInstanceCommitted);
    if (signal.bHasSignal())
        bInstancesMoved = ApplyMarkerInstanceListSignal(group.transforms, state, signal) || bInstancesMoved;
    bInstancesMoved = DrawMarkerInstanceListButtons(group.transforms, state, markerLayers,
                                                    selectedMarkerLayerIndex) || bInstancesMoved;
    DrawMarkerInstanceIconOverridePicker(group.transforms, state, iconManifest);
    bInstancesMoved = bAnyInstanceCommitted || bInstancesMoved;

    // Scoped to THIS group only: the wire format's inner dictionary key only needs uniqueness
    // within its own outer group (MarkersTab_Manual_UI.h header comment).
    if (bInstancesMoved) MakeNamesUnique(group.transforms);
}

} // namespace Ui
} // namespace SanmapGen
