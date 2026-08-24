// LayerEditor_UI.cpp — the Layer Editor's top-level composition: the GeoLayer group list and the
// one frame of collected signals. Layer: UI. Every group body, every layer row's own inline
// settings and every scalar below is a shared widget composed through LayerEditor_Draw_UI.h
// (LayerEditor_Group_UI.cpp draws each row's settings directly under its own header — STEP104).
// The group list is the shared DraggableList. Nothing here mutates the stack while a list is open
// — the whole frame is applied once, in LayerEditor_Signals_UI.h's order.
#include "LayerEditor_BakedImage_UI.h"
#include "LayerEditor_Draw_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// The GeoLayer rows plus the Add button above them. `bDrawOwnAddGeoLayerButton` false means a
// caller (HeightmapTab_UI.cpp) drew its own affordance beside the "GeoLayers" section header
// instead (STEP104 Fix part 2); `bAddGeoLayerRequestedExternally` is whether THAT button was
// clicked this frame. Returns the frame's collected signals.
LayerEditorFrameSignals DrawGeoLayerList(Params::LayerStack& layerStack, LayerEditorState& state,
                                         Pipeline::GenerationAssembler* generationAssembler,
                                         Pipeline::PreviewDriver* previewDriver,
                                         bool bDrawOwnAddGeoLayerButton,
                                         bool bAddGeoLayerRequestedExternally) {
    LayerEditorFrameSignals signals;
    const bool bAddGeoLayerClicked = bDrawOwnAddGeoLayerButton ? ImGui::SmallButton("Add GeoLayer")
                                                               : bAddGeoLayerRequestedExternally;
    if (bAddGeoLayerClicked)
        RecordLayerEditorAction(signals.action, LayerEditorActionKind::AddGeoLayer, -1);
    signals.groupSignal = DraggableList<Params::GeoLayer>::Render(
        "geoLayers", layerStack.geoLayers,
        [&](int rowIndex) {
            const Params::GeoLayer& group = layerStack.geoLayers[static_cast<std::size_t>(rowIndex)];
            DraggableListRow row;
            row.label    = group.name.empty() ? "GeoLayer" : group.name.c_str();
            row.bVisible = group.bEnabled;
            row.bLocked  = IsGeoLayerLocked(group);
            return row;
        },
        [&](int rowIndex) {
            DrawLayerEditorGroupBody(layerStack, rowIndex, state, signals, generationAssembler,
                                     previewDriver);
        },
        state.selectedGeoLayerIndex);
    return signals;
}

} // namespace

Params::Layer* SelectedLayerEditorLayer(Params::LayerStack& layerStack,
                                        const LayerEditorState& state) {
    if (state.selectedGeoLayerIndex < 0
        || state.selectedGeoLayerIndex >= static_cast<int>(layerStack.geoLayers.size())) return nullptr;
    Params::GeoLayer& group =
        layerStack.geoLayers[static_cast<std::size_t>(state.selectedGeoLayerIndex)];
    if (state.selectedLayerIndex < 0
        || state.selectedLayerIndex >= static_cast<int>(group.layers.size())) return nullptr;
    return &group.layers[static_cast<std::size_t>(state.selectedLayerIndex)];
}

void DrawLayerEditor(Params::LayerStack& layerStack, LayerEditorState& state,
                     Pipeline::GenerationAssembler* generationAssembler,
                     Pipeline::PreviewDriver* previewDriver,
                     bool bDrawOwnAddGeoLayerButton, bool bAddGeoLayerRequestedExternally) {
    ImGui::PushID("layerEditor");
    const LayerEditorFrameSignals signals = DrawGeoLayerList(
        layerStack, state, generationAssembler, previewDriver, bDrawOwnAddGeoLayerButton,
        bAddGeoLayerRequestedExternally);
    bool bRecipeMoved = ApplyLayerEditorFrameSignals(
        layerStack, signals, state.selectedGeoLayerIndex, state.selectedLayerIndex);
    // Import RAW / Bake, AFTER the structural signals above so the index shifts from
    // Add/Duplicate/Delete are already resolved (LayerEditor_Action_UI.h's own action carries
    // indices, not pointers, for exactly this reason). Both need a real pipeline behind them
    // (STEP102) -- with none bound, the affordance is drawn but does nothing, same posture as the
    // per-row soil/erosion sections (LayerEditor_Group_UI.cpp's DrawGroupLayerList, STEP104).
    if (generationAssembler != nullptr
        && (signals.action.kind == LayerEditorActionKind::ImportRawRequested
            || signals.action.kind == LayerEditorActionKind::BakeToggleRequested)) {
        bRecipeMoved = ApplyBakedImageAction(signals.action, layerStack, *generationAssembler)
                     || bRecipeMoved;
    }
    ClampLayerEditorSelection(layerStack, state.selectedGeoLayerIndex, state.selectedLayerIndex);
    NotifyLayerEditorChange(bRecipeMoved, previewDriver);
    // STEP104: no trailing "selected layer" panel draw here any more — each GeoLayer row's own
    // body (DrawLayerEditorGroupBody -> DrawGroupLayerList) draws that row's settings inline,
    // right under its own header, whenever ITS OWN CollapsingHeader is open. `selectedGeoLayerIndex`
    // / `selectedLayerIndex` stay (ApplyLayerEditorAction, DrawLayerRowActions's single-picker gate,
    // the DraggableList "Selected" highlight, and `SelectedLayerEditorLayer` — still a real,
    // separately-tested query several test binaries drive edits through without a draw at all —
    // all still need them); only the redundant full-panel draw that used to run once at the bottom
    // for whatever they pointed at is gone.
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
