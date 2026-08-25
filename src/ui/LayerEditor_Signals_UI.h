// LayerEditor_Signals_UI.h — one frame's worth of list traffic, and the order it is applied in.
// Layer: UI. Accuracy class: Visual. Pure and headless-testable.
//
// The editor draws a list of GeoLayers, each of which draws a list of layers, and a row can carry
// a Duplicate/Import/Bake action on top of that — so a single frame can produce up to three
// mutations expressed in indices that the FIRST of them would invalidate. They are therefore
// collected while drawing and applied here, once, in a fixed order.
//
// REUSE, not a rival: the two structural appliers and the derived group lock are LayersTab_UI.h's
// (`ApplyGeoLayerListSignal`, `ApplyLayerListSignal`, `IsGeoLayerLocked`). They already do exactly
// this over `Params::LayerStack`; a second copy here is how two behaviours drift apart.
#pragma once
#include "LayerEditor_Action_UI.h"
#include "LayersTab_UI.h"

namespace SanmapGen {
namespace Ui {

struct LayerEditorFrameSignals {
    DraggableListSignal groupSignal;              // the GeoLayer list
    DraggableListSignal layerSignal;              // the selected group's layer list
    int                 layerSignalGroupIndex = -1;
    LayerEditorAction   action;                   // Add / Duplicate / Import RAW / Bake
};

// Turns a layer row's header Bake/Unbake affordance click (DraggableListSignalKind::ExtraButton,
// STEP150) into the SAME `BakeToggleRequested` action the old in-body button recorded — only WHERE
// the click is detected moved, never the action it produces or how it is applied
// (LayerEditor_BakedImage_UI.h's ApplyBakedImageAction, untouched). A no-op for every other signal
// kind, so calling it on the group list's own signal (which never carries ExtraButton) is free.
inline void RecordBakeToggleFromRowSignal(const DraggableListSignal& signal, int groupIndex,
                                          LayerEditorFrameSignals& signals) {
    if (signal.kind != DraggableListSignalKind::ExtraButton) return;
    RecordLayerEditorAction(signals.action, LayerEditorActionKind::BakeToggleRequested,
                            groupIndex, signal.sourceRowIndex);
}

// Applies a frame in the ONE safe order:
//   1. the inner layer signal — a group delete in the same frame would move the indices it is
//      expressed in;
//   2. the group signal;
//   3. the row action, which is validated against the stack as it stands AFTER both.
// Returns true when the RECIPE moved (a selection change alone did not).
inline bool ApplyLayerEditorFrameSignals(Params::LayerStack& layerStack,
                                         const LayerEditorFrameSignals& signals,
                                         int& selectedGeoLayerIndex, int& selectedLayerIndex) {
    bool bRecipeMoved = false;
    if (signals.layerSignalGroupIndex >= 0
        && signals.layerSignalGroupIndex < static_cast<int>(layerStack.geoLayers.size()))
        bRecipeMoved = ApplyLayerListSignal(
            layerStack.geoLayers[static_cast<std::size_t>(signals.layerSignalGroupIndex)],
            signals.layerSignal, selectedLayerIndex);
    if (signals.groupSignal.bHasSignal())
        bRecipeMoved = ApplyGeoLayerListSignal(layerStack, signals.groupSignal,
                                               selectedGeoLayerIndex) || bRecipeMoved;
    if (signals.action.bHasAction())
        bRecipeMoved = ApplyLayerEditorAction(layerStack, signals.action, selectedGeoLayerIndex,
                                              selectedLayerIndex) || bRecipeMoved;
    return bRecipeMoved;
}

// Pins the selection inside the stack after a frame that may have deleted rows out from under it.
inline void ClampLayerEditorSelection(const Params::LayerStack& layerStack,
                                      int& selectedGeoLayerIndex, int& selectedLayerIndex) {
    const int groupCount = static_cast<int>(layerStack.geoLayers.size());
    if (selectedGeoLayerIndex < 0) selectedGeoLayerIndex = 0;
    if (selectedGeoLayerIndex > groupCount - 1) selectedGeoLayerIndex = groupCount - 1;
    if (selectedGeoLayerIndex < 0) { selectedLayerIndex = -1; return; }
    const int layerCount = static_cast<int>(
        layerStack.geoLayers[static_cast<std::size_t>(selectedGeoLayerIndex)].layers.size());
    if (selectedLayerIndex < 0) selectedLayerIndex = 0;
    if (selectedLayerIndex > layerCount - 1) selectedLayerIndex = layerCount - 1;
}

} // namespace Ui
} // namespace SanmapGen
