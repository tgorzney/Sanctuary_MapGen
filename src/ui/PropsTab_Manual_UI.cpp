// PropsTab_Manual_UI.cpp — the imgui composition of the manual prop layers block. Layer: UI.
// Shared widgets only: DraggableList for the layer stack, VirtualList for the read-only transform
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

// The block-wide settings: one shared tint for every layer, and the icon scale the whole layer
// stack draws at.
void DrawLayerSettings(ManualPropLayersState& state) {
    DrawCheckbox("Use Group Color", state.bUseGroupColor);
    if (state.bUseGroupColor)
        DrawColorSwatch("Group Color", state.groupColor, state.previewColorOptions,
                        state.groupColorToggle);
    DrawSliderScalar("Layer Icon Scale", state.layerIconScale, state.iconScaleRange,
                     state.layerIconScaleToggle, WidgetStyle(), "%.2f");
}

// One row's own name, tint and icon scale. The tint is hidden while the block is set to one shared
// color: two live controls over one drawn color would be a rival control (ARCH §4). Reports whether
// the name committed, the only field the uniqueness repair cares about. STEP110: was `DrawSelectedLayer`,
// drawn once at the bottom for the global `selectedLayerIndex`; now inline per row (STEP104's pattern).
bool DrawLayerRowSettings(Params::PropInstanceLayer& layer, ManualPropLayersState& state) {
    TextInputRules nameRules;
    nameRules.maximumLength = 48;
    nameRules.bAllowEmpty   = false;
    nameRules.fallbackText  = "Prop Layer";
    const bool bNameCommitted = DrawTextInput("Name", layer.name, nameRules).bCommitted;
    if (!state.bUseGroupColor)
        DrawColorSwatch("Color", layer.color, state.previewColorOptions, state.selectedLayerColorToggle);
    DrawSliderScalar("Icon Scale", layer.iconScale, state.iconScaleRange,
                     state.selectedLayerIconScaleToggle, WidgetStyle(), "%.2f");
    return bNameCommitted;
}

// The layer stack. STEP110: each row's body, whenever the row's own CollapsingHeader is open
// (DraggableList's own per-row expand/collapse state — never gated on `state.selectedLayerIndex`),
// draws that row's OWN settings directly, so an expanded row never shows another row's settings.
// `bAnyNameCommitted` is set (never cleared) when any row's name commits this frame.
DraggableListSignal DrawLayerList(std::vector<Params::PropInstanceLayer>& propLayers,
                                  ManualPropLayersState& state, bool& bAnyNameCommitted) {
    return DraggableList<Params::PropInstanceLayer>::Render(
        "manualPropLayers", propLayers,
        [&](int rowIndex) {
            DraggableListRow row;
            row.label   = ManualPropLayerRowLabel(propLayers[static_cast<std::size_t>(rowIndex)]);
            row.bLocked = propLayers[static_cast<std::size_t>(rowIndex)].bLocked;   // NEW
            return row;
        },
        [&](int rowIndex) {
            Params::PropInstanceLayer& layer = propLayers[static_cast<std::size_t>(rowIndex)];
            if (DrawLayerRowSettings(layer, state)) bAnyNameCommitted = true;
        },
        state.selectedLayerIndex);
}

// A removed layer clamps every referencing `layerIndex` to 0 rather than dropping the prop instance
// (ClampPropLayerIndicesForRemovedLayer, STEP22 ruling #5); a reordered layer renumbers them
// (RenumberPropLayerIndicesForReorder, called BEFORE the layers vector itself moves — the renumber
// needs the pre-move layer count), mirroring ArmiesTab_UI.cpp's ApplyArmyListSignal. Reports whether
// `props` moved, which — unlike `recipe.unitRules`/`armyIndex` — feeds no pipeline stage either
// (PropsTab_Manual_UI.h SCOPE NOTE 1), so the caller never notifies the preview driver with it.
bool ApplyLayerListSignal(std::vector<Params::PropInstanceLayer>& propLayers,
                         std::vector<Params::PropInstanceGroup>& props, ManualPropLayersState& state,
                         const DraggableListSignal& signal) {
    if (signal.kind == DraggableListSignalKind::Select) {
        state.selectedLayerIndex = signal.sourceRowIndex;
        return false;
    }
    if (signal.kind == DraggableListSignalKind::ToggleLock) {
        if (signal.sourceRowIndex >= 0 && signal.sourceRowIndex < static_cast<int>(propLayers.size()))
            propLayers[static_cast<std::size_t>(signal.sourceRowIndex)].bLocked =
                !propLayers[static_cast<std::size_t>(signal.sourceRowIndex)].bLocked;
        return false;   // cosmetic-only: no structural move, same posture as Select
    }
    const bool bDeleting            = signal.kind == DraggableListSignalKind::Delete;
    const bool bReordering          = signal.kind == DraggableListSignalKind::Reorder;
    const int  sourceLayerIndex     = signal.sourceRowIndex;
    const int  layerCountBeforeMove = static_cast<int>(propLayers.size());
    bool bPropsMoved = bReordering && RenumberPropLayerIndicesForReorder(
        props, sourceLayerIndex, signal.targetRowIndex, layerCountBeforeMove);
    if (!ApplyDraggableListSignal(propLayers, signal)) return bPropsMoved;
    if (bDeleting)
        bPropsMoved = ClampPropLayerIndicesForRemovedLayer(props, sourceLayerIndex) || bPropsMoved;
    if (state.selectedLayerIndex >= static_cast<int>(propLayers.size()))
        state.selectedLayerIndex = static_cast<int>(propLayers.size()) - 1;
    return bPropsMoved;
}

// The Add Prop Layer button. Reports whether a layer was added, so the caller knows to run the
// name-uniqueness repair (cosmetic here, STEP22 ruling #6 — see UniqueNameList_UI.h).
bool DrawLayerListButtons(std::vector<Params::PropInstanceLayer>& propLayers, ManualPropLayersState& state) {
    if (!ImGui::Button("Add Prop Layer")) return false;
    Params::PropInstanceLayer layer;
    layer.name = NextPropLayerName(static_cast<int>(propLayers.size()));
    layer.layerId = NextPropLayerId(propLayers);
    propLayers.push_back(layer);
    state.selectedLayerIndex = static_cast<int>(propLayers.size()) - 1;
    return true;
}

// The resolved transforms, read-only and virtualized (SCOPE NOTE 2). Unfiltered, unrelated to
// manual-layer membership: `Data::PlacementInstances` has no `layerIndex`-equivalent field.
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

void DrawManualPropLayers(ManualPropLayersState& state, std::vector<Params::PropInstanceLayer>& propLayers,
                          std::vector<Params::PropInstanceGroup>& props,
                          const Data::PlacementInstances* placedProps) {
    if (!DrawSectionBegin("Manual Prop Layers", state.section)) return;
    DrawLayerSettings(state);
    bool bLayersMoved = DrawLayerListButtons(propLayers, state);
    bool bAnyNameCommitted = false;
    const DraggableListSignal signal = DrawLayerList(propLayers, state, bAnyNameCommitted);
    if (signal.bHasSignal()) ApplyLayerListSignal(propLayers, props, state, signal);
    bLayersMoved = bAnyNameCommitted || bLayersMoved;
    DrawTransformList(state, placedProps);
    // The export keys layers by NAME parity with Armies/Areas (cosmetic here, STEP22 ruling #6) —
    // the repair runs on the frames a name settled, not every frame.
    if (bLayersMoved) MakeNamesUnique(propLayers);
    DrawSectionEnd();
}

} // namespace Ui
} // namespace SanmapGen
