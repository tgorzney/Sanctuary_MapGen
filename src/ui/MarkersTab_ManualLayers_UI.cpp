// MarkersTab_ManualLayers_UI.cpp — the imgui composition of the Manual Marker Layers block.
// Layer: UI. Shared widgets only: DraggableList for the layer stack, Checkbox / ColorSwatch /
// SliderScalar / TextInput / Section for the rest. Nothing here notifies
// Pipeline::PreviewDriver (MarkersTab_ManualLayers_UI.h SCOPE NOTE 3).
#include "MarkersTab_ManualLayers_UI.h"
#include "Checkbox_UI.h"
#include "DraggableListWidget_UI.h"
#include "MarkerLayerId_UI.h"
#include "MarkerLayerIndexRepair_UI.h"
#include "PlacementRuleSections_UI.h"
#include "TextInput_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// The block-wide settings: one shared tint for every layer, and the icon scale the whole layer
// stack draws at.
void DrawLayerSettings(ManualMarkerLayersState& state) {
    DrawCheckbox("Use Group Color", state.bUseGroupColor);
    if (state.bUseGroupColor)
        DrawColorSwatch("Group Color", state.groupColor, state.previewColorOptions,
                        state.groupColorToggle);
    DrawSliderScalar("Layer Icon Scale", state.layerIconScale, state.iconScaleRange,
                     state.layerIconScaleToggle, WidgetStyle(), "%.2f");
}

// The layer stack. MUTATES NOTHING while drawing: the signal is applied after the list closes.
DraggableListSignal DrawLayerList(const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                  int selectedLayerIndex) {
    return DraggableList<Params::MarkerInstanceLayer>::Render(
        "manualMarkerLayers", markerLayers,
        [&](int rowIndex) {
            DraggableListRow row;
            row.label = ManualMarkerLayerRowLabel(markerLayers[static_cast<std::size_t>(rowIndex)]);
            return row;
        },
        [](int) {},
        selectedLayerIndex);
}

// A removed layer clamps every referencing `layerIndex` to 0 rather than dropping the marker
// instance (ClampMarkerLayerIndicesForRemovedLayer); a reordered layer renumbers them
// (RenumberMarkerLayerIndicesForReorder, called BEFORE the layers vector itself moves — the
// renumber needs the pre-move layer count). Reports whether `markers` moved, which feeds no
// pipeline stage either (SCOPE NOTE 3), so the caller never notifies the preview driver with it.
bool ApplyLayerListSignal(std::vector<Params::MarkerInstanceLayer>& markerLayers,
                         std::vector<Params::MarkerInstanceGroup>& markers, ManualMarkerLayersState& state,
                         const DraggableListSignal& signal) {
    if (signal.kind == DraggableListSignalKind::Select) {
        state.selectedLayerIndex = signal.sourceRowIndex;
        return false;
    }
    const bool bDeleting            = signal.kind == DraggableListSignalKind::Delete;
    const bool bReordering          = signal.kind == DraggableListSignalKind::Reorder;
    const int  sourceLayerIndex     = signal.sourceRowIndex;
    const int  layerCountBeforeMove = static_cast<int>(markerLayers.size());
    bool bMarkersMoved = bReordering && RenumberMarkerLayerIndicesForReorder(
        markers, sourceLayerIndex, signal.targetRowIndex, layerCountBeforeMove);
    if (!ApplyDraggableListSignal(markerLayers, signal)) return bMarkersMoved;
    if (bDeleting)
        bMarkersMoved = ClampMarkerLayerIndicesForRemovedLayer(markers, sourceLayerIndex) || bMarkersMoved;
    if (state.selectedLayerIndex >= static_cast<int>(markerLayers.size()))
        state.selectedLayerIndex = static_cast<int>(markerLayers.size()) - 1;
    return bMarkersMoved;
}

// The Add Marker Layer button. Markers ship with `layerId` from day one (STEP60), unlike Props'
// still-unimplemented retrofit, so the fresh row mints one via `NextMarkerLayerId` BEFORE the
// push_back — that function scans for max(layerId) + 1, and calling it after would have the new
// row (still carrying the `-1` sentinel) scan itself.
bool DrawLayerListButtons(std::vector<Params::MarkerInstanceLayer>& markerLayers, ManualMarkerLayersState& state) {
    if (!ImGui::Button("Add Marker Layer")) return false;
    Params::MarkerInstanceLayer layer;
    layer.name    = NextMarkerLayerName(static_cast<int>(markerLayers.size()));
    layer.layerId = NextMarkerLayerId(markerLayers);
    markerLayers.push_back(layer);
    state.selectedLayerIndex = static_cast<int>(markerLayers.size()) - 1;
    return true;
}

// The selected layer's own name, tint, icon scale, and its own symmetry setting. The tint is
// hidden while the block is set to one shared color: two live controls over one drawn color would
// be a rival control (ARCH §4). Layer-level symmetry (unlike Props/Decals, which have none — see
// ARCH_14_13_OpenItems.md §14.13 Ruling 3): manual markers are the deliberate, separately-ratified
// exception the human required for symmetry participation, unlike manual props/decals.
// `previewDriver` is passed as nullptr — this field has no consumer yet (SCOPE NOTE 3).
bool DrawSelectedLayer(std::vector<Params::MarkerInstanceLayer>& markerLayers, ManualMarkerLayersState& state) {
    Params::MarkerInstanceLayer* const layer = SelectedManualMarkerLayer(markerLayers, state.selectedLayerIndex);
    if (layer == nullptr) {
        ImGui::TextUnformatted("Select a marker layer to edit it.");
        return false;
    }
    TextInputRules nameRules;
    nameRules.maximumLength = 48;
    nameRules.bAllowEmpty   = false;
    nameRules.fallbackText  = "Marker Layer";
    const bool bNameCommitted = DrawTextInput("Name", layer->name, nameRules).bCommitted;
    if (!state.bUseGroupColor)
        DrawColorSwatch("Color", layer->color, state.previewColorOptions, state.selectedLayerColorToggle);
    DrawSliderScalar("Icon Scale", layer->iconScale, state.iconScaleRange,
                     state.selectedLayerIconScaleToggle, WidgetStyle(), "%.2f");
    if (DrawSectionBegin("Layer Symmetry", state.symmetrySection)) {
        DrawPlacementSymmetryAxes("markerLayerSymmetry", layer->symmetry.bSymmetryUseGlobal,
                                  layer->symmetry.symmetryMask, nullptr);
        DrawSectionEnd();
    }
    return bNameCommitted;
}

} // namespace

void DrawManualMarkerLayers(ManualMarkerLayersState& state, std::vector<Params::MarkerInstanceLayer>& markerLayers,
                            std::vector<Params::MarkerInstanceGroup>& markers) {
    if (!DrawSectionBegin("Manual Marker Layers", state.section)) return;
    DrawLayerSettings(state);
    bool bLayersMoved = DrawLayerListButtons(markerLayers, state);
    const DraggableListSignal signal = DrawLayerList(markerLayers, state.selectedLayerIndex);
    if (signal.bHasSignal()) ApplyLayerListSignal(markerLayers, markers, state, signal);
    bLayersMoved = DrawSelectedLayer(markerLayers, state) || bLayersMoved;
    // The export keys layers by NAME parity with Armies/Areas/Props (cosmetic here — `MarkerGroups`
    // exports as a plain array, STEP60 §3) — the repair runs on the frames a name settled.
    if (bLayersMoved) MakeNamesUnique(markerLayers);
    DrawSectionEnd();
}

} // namespace Ui
} // namespace SanmapGen
