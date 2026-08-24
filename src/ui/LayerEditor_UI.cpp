// LayerEditor_UI.cpp — the Layer Editor's top-level composition: the GeoLayer group list, the
// one frame of collected signals, and the selected layer's sections. Layer: UI.
// The group list is the shared DraggableList; every group body, every layer row and every scalar
// below is a shared widget composed through LayerEditor_Draw_UI.h. Nothing here mutates the stack
// while a list is open — the whole frame is applied once, in LayerEditor_Signals_UI.h's order.
#include "LayerEditor_BakedImage_UI.h"
#include "LayerEditor_Draw_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// The GeoLayer rows plus the Add button above them. Returns the frame's collected signals.
LayerEditorFrameSignals DrawGeoLayerList(Params::LayerStack& layerStack, LayerEditorState& state,
                                         Pipeline::PreviewDriver* previewDriver) {
    LayerEditorFrameSignals signals;
    if (ImGui::SmallButton("Add GeoLayer"))
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
            DrawLayerEditorGroupBody(layerStack, rowIndex, state, signals, previewDriver);
        },
        state.selectedGeoLayerIndex);
    return signals;
}

// The selected layer's four panels. Soil, erosion and the advanced constants are keyed by the
// layer's STRATUM, which is the palette model: two layers on stratum 3 share one soil.
void DrawSelectedLayerPanels(Params::Layer& layer, LayerEditorState& state,
                             Pipeline::GenerationAssembler* generationAssembler,
                             Pipeline::PreviewDriver* previewDriver) {
    DrawLayerEditorLayerSections(layer, state, previewDriver);
    DrawLayerEditorSoilSection(layer.stratumIndex, state, generationAssembler, previewDriver);
    DrawLayerEditorErosionSections(layer.stratumIndex, state, generationAssembler, previewDriver);
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
                     Pipeline::PreviewDriver* previewDriver) {
    ImGui::PushID("layerEditor");
    const LayerEditorFrameSignals signals = DrawGeoLayerList(layerStack, state, previewDriver);
    bool bRecipeMoved = ApplyLayerEditorFrameSignals(
        layerStack, signals, state.selectedGeoLayerIndex, state.selectedLayerIndex);
    // Import RAW / Bake, AFTER the structural signals above so the index shifts from
    // Add/Duplicate/Delete are already resolved (LayerEditor_Action_UI.h's own action carries
    // indices, not pointers, for exactly this reason). Both need a real pipeline behind them
    // (STEP102) -- with none bound, the affordance is drawn but does nothing, same posture as the
    // soil/erosion sections just below.
    if (generationAssembler != nullptr
        && (signals.action.kind == LayerEditorActionKind::ImportRawRequested
            || signals.action.kind == LayerEditorActionKind::BakeToggleRequested)) {
        bRecipeMoved = ApplyBakedImageAction(signals.action, layerStack, *generationAssembler)
                     || bRecipeMoved;
    }
    ClampLayerEditorSelection(layerStack, state.selectedGeoLayerIndex, state.selectedLayerIndex);
    NotifyLayerEditorChange(bRecipeMoved, previewDriver);

    ImGui::Separator();
    Params::Layer* const layer = SelectedLayerEditorLayer(layerStack, state);
    if (layer != nullptr) DrawSelectedLayerPanels(*layer, state, generationAssembler, previewDriver);
    else ImGui::TextUnformatted("Select a layer to edit its noise, blend, soil and erosion settings.");
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
