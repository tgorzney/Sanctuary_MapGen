// MarkersTab_ManualLayers_UI.cpp — the imgui composition of the Manual Marker Layers block.
// Layer: UI. Shared widgets only: DraggableList for the layer stack, Checkbox / ColorSwatch /
// SliderScalar / TextInput / Section for the rest. Nothing here notifies
// Pipeline::PreviewDriver (MarkersTab_ManualLayers_UI.h SCOPE NOTE 3).
#include "MarkersTab_ManualLayers_UI.h"
#include "Checkbox_UI.h"
#include "MarkerLayerId_UI.h"
#include "MarkerLayerIndexRepair_UI.h"
#include "MarkersTab_ManualLayerHelpers_UI.h"
#include "MarkersTab_ManualLayerRowBody_UI.h"
#include "TextInput_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

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

// STEP125: promoted out of the anonymous namespace (was DrawLayerSettings), called exactly once,
// tab-wide, by DrawMarkerTypeSections — see MarkersTab_ManualLayers_UI.h.
void DrawManualMarkerLayerBlockSettings(ManualMarkerLayersState& state) {
    DrawCheckbox("Use Group Color", state.bUseGroupColor);
    if (state.bUseGroupColor)
        DrawColorSwatch("Group Color", state.groupColor, state.previewColorOptions,
                        state.groupColorToggle);
}

DraggableListSignal DrawLayerList(std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                  std::vector<Params::MarkerInstanceGroup>& markers,
                                  const Params::Geometry& geometry, int globalSymmetryMask, int globalRadialRepeatCount,
                                  Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings,
                                  ManualMarkerLayersState& state, bool& bAnyNameCommitted,
                                  const ManualInstanceLayerIndex_UI& instanceIndex,
                                  int& selectedManualInstanceIdentifier,
                                  const std::string& markerTypeNameFilter) {
    return DraggableList<Params::MarkerInstanceLayer>::Render(
        "manualMarkerLayers", markerLayers,
        [&](int rowIndex) {
            DraggableListRow row;
            row.bRowSuppressed = IsMarkerInstanceLayerRowSuppressed(
                markerLayers[static_cast<std::size_t>(rowIndex)], markerTypeNameFilter);   // CHANGED — STEP125
            row.label   = ManualMarkerLayerRowLabel(markerLayers[static_cast<std::size_t>(rowIndex)]);
            row.bLocked = markerLayers[static_cast<std::size_t>(rowIndex)].bLocked;
            return row;
        },
        [&](int rowIndex) {
            if (DrawLayerRowBody(markerLayers[static_cast<std::size_t>(rowIndex)], rowIndex, markerLayers, markers,
                                 geometry, globalSymmetryMask, globalRadialRepeatCount, markerSymmetryFixSettings,
                                 state, instanceIndex, selectedManualInstanceIdentifier))
                bAnyNameCommitted = true;
        },
        [&](int rowIndex) {
            DrawManualMarkerLayerColorOverrideHeaderControl(
                markerLayers[static_cast<std::size_t>(rowIndex)], state, bAnyNameCommitted);
        },
        kMarkerLayerColorOverrideHeaderWidthPixels,
        state.selectedLayerIndex);
}

// STEP120: gains an optional Bundle-scoped parent; moved out of the anonymous namespace so
// MarkersTab_Bundles_UI.cpp can call it for a non-root parent. Markers ship with `layerId` from day
// one (STEP60), unlike Props' still-unimplemented retrofit, so the fresh row mints one via
// `NextMarkerLayerId` BEFORE the push_back — that function scans for max(layerId) + 1, and calling
// it after would scan the new row's own `-1`. STEP125: gains `markerTypeNameForNewLayer`.
bool DrawLayerListButtons(std::vector<Params::MarkerInstanceLayer>& markerLayers, ManualMarkerLayersState& state,
                          int parentBundleIdentifierForNewLayer, const std::string& markerTypeNameForNewLayer) {
    if (!ImGui::Button(parentBundleIdentifierForNewLayer < 0 ? "Add Marker Layer" : "Add Manual Layer Here"))
        return false;
    Params::MarkerInstanceLayer layer;
    layer.name                   = NextMarkerLayerName(static_cast<int>(markerLayers.size()));
    layer.layerId                = NextMarkerLayerId(markerLayers);
    layer.parentBundleIdentifier = parentBundleIdentifierForNewLayer;   // STEP119 field
    layer.markerTypeName         = markerTypeNameForNewLayer;          // NEW — STEP125
    markerLayers.push_back(layer);
    state.selectedLayerIndex = static_cast<int>(markerLayers.size()) - 1;
    return true;
}

void DrawManualMarkerLayerListBody(ManualMarkerLayersState& state,
                                   std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                   std::vector<Params::MarkerInstanceGroup>& markers,
                                   const Params::Geometry& geometry, int globalSymmetryMask,
                                   int globalRadialRepeatCount,
                                   Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings,
                                   const std::string& markerTypeNameFilter,
                                   int& selectedManualInstanceIdentifier) {
    bool bLayersMoved = DrawLayerListButtons(markerLayers, state, -1, markerTypeNameFilter);
    bool bAnyNameCommitted = false;
    const ManualInstanceLayerIndex_UI instanceIndex = BuildManualInstanceLayerIndex(markers);
    const DraggableListSignal signal = DrawLayerList(markerLayers, markers, geometry, globalSymmetryMask,
        globalRadialRepeatCount, markerSymmetryFixSettings, state, bAnyNameCommitted,
        instanceIndex, selectedManualInstanceIdentifier, markerTypeNameFilter);
    if (signal.bHasSignal()) ApplyLayerListSignal(markerLayers, markers, state, signal);
    bLayersMoved = bAnyNameCommitted || bLayersMoved;
    // The export keys layers by NAME parity with Armies/Areas/Props (cosmetic here — `MarkerGroups`
    // exports as a plain array, STEP60 §3) — the repair runs on the frames a name settled.
    if (bLayersMoved) MakeNamesUnique(markerLayers);
}

} // namespace Ui
} // namespace SanmapGen
