// MarkersTab_ManualLayers_UI.cpp — the imgui composition of the Manual Marker Layers block.
// Layer: UI. Shared widgets only: DraggableList for the layer stack, Checkbox / ColorSwatch /
// SliderScalar / TextInput / Section for the rest. Nothing here notifies
// Pipeline::PreviewDriver (MarkersTab_ManualLayers_UI.h SCOPE NOTE 3).
#include "MarkersTab_ManualLayers_UI.h"
#include "Checkbox_UI.h"
#include "DraggableListWidget_UI.h"
#include "MarkerLayerId_UI.h"
#include "MarkerLayerIndexRepair_UI.h"
#include "MarkerLayerSymmetrySection_UI.h"
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
// STEP110: each row's body, whenever the row's own CollapsingHeader is open (never gated on
// `state.selectedLayerIndex`), draws that row's OWN settings below its header. `bAnyNameCommitted`
// is set true if any expanded row's name committed this frame, feeding the caller's uniqueness repair.
DraggableListSignal DrawLayerList(std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                  std::vector<Params::MarkerInstanceGroup>& markers,
                                  const Params::Geometry& geometry, int globalSymmetryMask, int globalRadialRepeatCount,
                                  Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings,
                                  ManualMarkerLayersState& state, bool& bAnyNameCommitted) {
    return DraggableList<Params::MarkerInstanceLayer>::Render(
        "manualMarkerLayers", markerLayers,
        [&](int rowIndex) {
            DraggableListRow row;
            row.bRowSuppressed = (markerLayers[static_cast<std::size_t>(rowIndex)].parentBundleIdentifier != -1);
            row.label   = ManualMarkerLayerRowLabel(markerLayers[static_cast<std::size_t>(rowIndex)]);
            row.bLocked = markerLayers[static_cast<std::size_t>(rowIndex)].bLocked;   // NEW
            return row;
        },
        [&](int rowIndex) {
            if (DrawLayerRowBody(markerLayers[static_cast<std::size_t>(rowIndex)], rowIndex, markerLayers, markers,
                                 geometry, globalSymmetryMask, globalRadialRepeatCount, markerSymmetryFixSettings, state))
                bAnyNameCommitted = true;
        },
        [&](int rowIndex) {
            DrawManualMarkerLayerColorOverrideHeaderControl(
                markerLayers[static_cast<std::size_t>(rowIndex)], state, bAnyNameCommitted);
        },
        kMarkerLayerColorOverrideHeaderWidthPixels,
        state.selectedLayerIndex);
}

// A removed layer clamps every referencing `layerIndex` to 0 (ClampMarkerLayerIndicesForRemovedLayer);
// a reordered layer renumbers them (RenumberMarkerLayerIndicesForReorder, called BEFORE the layers
// vector moves). Reports whether `markers` moved, which feeds no pipeline stage (SCOPE NOTE 3).
bool ApplyLayerListSignal(std::vector<Params::MarkerInstanceLayer>& markerLayers,
                         std::vector<Params::MarkerInstanceGroup>& markers, ManualMarkerLayersState& state,
                         const DraggableListSignal& signal) {
    if (signal.kind == DraggableListSignalKind::Select) {
        state.selectedLayerIndex = signal.sourceRowIndex;
        return false;
    }
    if (signal.kind == DraggableListSignalKind::ToggleLock) {
        if (signal.sourceRowIndex >= 0 && signal.sourceRowIndex < static_cast<int>(markerLayers.size()))
            markerLayers[static_cast<std::size_t>(signal.sourceRowIndex)].bLocked =
                !markerLayers[static_cast<std::size_t>(signal.sourceRowIndex)].bLocked;
        return false;   // cosmetic-only: no structural move, same posture as Select
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

} // namespace

// The Add Marker Layer button. Markers ship with `layerId` from day one (STEP60), unlike Props' still-
// unimplemented retrofit, so the fresh row mints one via `NextMarkerLayerId` BEFORE the push_back —
// that function scans for max(layerId) + 1, and calling it after would scan the new row's own `-1`.
// STEP120: gains an optional Bundle-scoped parent; moved out of the anonymous namespace so
// MarkersTab_Bundles_UI.cpp can call it for a non-root parent (MarkersTab_ManualLayers_UI.h).
bool DrawLayerListButtons(std::vector<Params::MarkerInstanceLayer>& markerLayers, ManualMarkerLayersState& state,
                          int parentBundleIdentifierForNewLayer) {
    if (!ImGui::Button(parentBundleIdentifierForNewLayer < 0 ? "Add Marker Layer" : "Add Manual Layer Here"))
        return false;
    Params::MarkerInstanceLayer layer;
    layer.name                   = NextMarkerLayerName(static_cast<int>(markerLayers.size()));
    layer.layerId                = NextMarkerLayerId(markerLayers);
    layer.parentBundleIdentifier = parentBundleIdentifierForNewLayer;   // STEP119 field
    markerLayers.push_back(layer);
    state.selectedLayerIndex = static_cast<int>(markerLayers.size()) - 1;
    return true;
}

void DrawManualMarkerLayers(ManualMarkerLayersState& state, std::vector<Params::MarkerInstanceLayer>& markerLayers,
                            std::vector<Params::MarkerInstanceGroup>& markers, const Params::Geometry& geometry,
                            int globalSymmetryMask, int globalRadialRepeatCount,
                            Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings) {
    if (!DrawSectionBegin("Ungrouped Manual Marker Layers", state.section)) return;
    DrawLayerSettings(state);
    bool bLayersMoved = DrawLayerListButtons(markerLayers, state, -1);   // root-scope, unchanged behavior
    bool bAnyNameCommitted = false;
    const DraggableListSignal signal = DrawLayerList(markerLayers, markers, geometry, globalSymmetryMask,
        globalRadialRepeatCount, markerSymmetryFixSettings, state, bAnyNameCommitted);
    if (signal.bHasSignal()) ApplyLayerListSignal(markerLayers, markers, state, signal);
    bLayersMoved = bAnyNameCommitted || bLayersMoved;
    // The export keys layers by NAME parity with Armies/Areas/Props (cosmetic here — `MarkerGroups`
    // exports as a plain array, STEP60 §3) — the repair runs on the frames a name settled.
    if (bLayersMoved) MakeNamesUnique(markerLayers);
    DrawSectionEnd();
}

} // namespace Ui
} // namespace SanmapGen
