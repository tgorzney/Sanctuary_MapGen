// PropsTab_Manual_UI.cpp — the imgui composition of the manual prop layers block. Layer: UI.
// Shared widgets only: DraggableList for the group stack, VirtualList for the read-only transform
// list, Checkbox / ColorSwatch / SliderScalar / TextInput / Section for the rest.
// Nothing here notifies Pipeline::PreviewDriver (PropsTab_Manual_UI.h SCOPE NOTE 1).
#include "PropsTab_Manual_UI.h"
#include "Checkbox_UI.h"
#include "DraggableListWidget_UI.h"
#include "TextInput_UI.h"
#include "VirtualListWidget_UI.h"
#include "../data/PlacementInstances_DATA.h"
#include "imgui.h"
#include <cstdio>

namespace SanmapGen {
namespace Ui {
namespace {

// The block-wide settings: one shared tint for every group, and the icon scale the whole layer
// draws at.
void DrawLayerSettings(ManualPropLayersState& state) {
    DrawCheckbox("Use Group Color", state.bUseGroupColor);
    if (state.bUseGroupColor)
        DrawColorSwatch("Group Color", state.groupColor, state.previewColorOptions,
                        state.groupColorToggle);
    DrawSliderScalar("Layer Icon Scale", state.layerIconScale, state.iconScaleRange,
                     state.layerIconScaleToggle, WidgetStyle(), "%.2f");
}

// The group stack. MUTATES NOTHING while drawing: the signal is applied after the list closes.
DraggableListSignal DrawGroupList(const ManualPropLayersState& state) {
    return DraggableList<ManualPropGroup>::Render(
        "manualPropGroups", state.groups,
        [&](int rowIndex) {
            DraggableListRow row;
            row.label = ManualPropGroupRowLabel(state.groups[static_cast<std::size_t>(rowIndex)]);
            return row;
        },
        [](int) {},
        state.selectedGroupIndex);
}

void ApplyGroupListSignal(ManualPropLayersState& state, const DraggableListSignal& signal) {
    if (signal.kind == DraggableListSignalKind::Select) {
        state.selectedGroupIndex = signal.sourceRowIndex;
        return;
    }
    if (!ApplyDraggableListSignal(state.groups, signal)) return;
    if (state.selectedGroupIndex >= static_cast<int>(state.groups.size()))
        state.selectedGroupIndex = static_cast<int>(state.groups.size()) - 1;
}

// The selected group's own name, tint and icon scale. The tint is hidden while the block is set
// to one shared color: two live controls over one drawn color would be a rival control (ARCH §4).
void DrawSelectedGroup(ManualPropLayersState& state) {
    ManualPropGroup* const group = SelectedManualPropGroup(state);
    if (group == nullptr) {
        ImGui::TextUnformatted("Select a prop group to edit it.");
        return;
    }
    TextInputRules nameRules;
    nameRules.maximumLength = 48;
    nameRules.bAllowEmpty   = false;
    nameRules.fallbackText  = "Prop Group";
    DrawTextInput("Name", group->name, nameRules);
    if (!state.bUseGroupColor)
        DrawColorSwatch("Color", group->previewColor, state.previewColorOptions,
                        group->previewColorToggle);
    DrawSliderScalar("Icon Scale", group->iconScale, state.iconScaleRange, group->iconScaleToggle,
                     WidgetStyle(), "%.2f");
}

// The resolved transforms, read-only and virtualized (SCOPE NOTE 2).
void DrawTransformList(ManualPropLayersState& state, const Data::PlacementInstances* placedProps) {
    if (!DrawSectionBegin("Transforms (read-only)", state.transformListSection)) return;
    if (placedProps == nullptr || placedProps->IsEmpty()) {
        ImGui::TextUnformatted("No props have been generated yet.");
        DrawSectionEnd();
        return;
    }
    const int instanceCount = static_cast<int>(placedProps->Count());
    char rowLabel[80] = { 0 };
    RenderVirtualRows("manualPropTransforms", instanceCount, state.transformRowHeight,
                      state.transformListHeight, [&](int rowIndex) {
        const std::size_t instanceIndex = static_cast<std::size_t>(rowIndex);
        std::snprintf(rowLabel, sizeof(rowLabel), "%d: %.7s (%.1f, %.1f, %.1f) scale %.2f", rowIndex,
                      placedProps->templateIdentifier[instanceIndex].characters,
                      placedProps->positionX[instanceIndex], placedProps->positionY[instanceIndex],
                      placedProps->positionZ[instanceIndex], placedProps->scaleX[instanceIndex]);
        ImGui::TextUnformatted(rowLabel);
    });
    DrawSectionEnd();
}

} // namespace

void DrawManualPropLayers(ManualPropLayersState& state, const Data::PlacementInstances* placedProps) {
    if (!DrawSectionBegin("Manual Prop Layers", state.section)) return;
    DrawLayerSettings(state);
    if (ImGui::Button("Add Prop Group")) state.groups.push_back(ManualPropGroup());
    const DraggableListSignal signal = DrawGroupList(state);
    if (signal.bHasSignal()) ApplyGroupListSignal(state, signal);
    DrawSelectedGroup(state);
    DrawTransformList(state, placedProps);
    DrawSectionEnd();
}

} // namespace Ui
} // namespace SanmapGen
