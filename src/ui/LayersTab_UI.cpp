// LayersTab_UI.cpp — the imgui composition of the layer-stack tab. Layer: UI.
// Stack editing is the shared DraggableList; every scalar is a shared dial or range slider. No
// ImGui::SliderFloat/DragFloat/VSliderFloat here — the only raw imgui is the checkbox/dropdown/
// label vocabulary the widget library does not (yet) cover.
#include "LayersTab_UI.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../pipeline/PreviewDriver_PIPELINE.h"
#include "imgui.h"
#include <cstdio>

namespace SanmapGen {
namespace Ui {
namespace {

// The ONE thing a tab does with a commit. WHICH tier it becomes is the driver's derivation from
// the stage parameter hashes, never this call site's decision.
void NotifyChange(bool bCommitted, Pipeline::PreviewDriver* previewDriver) {
    if (bCommitted && previewDriver != nullptr) previewDriver->NotifyParametersChanged();
}

// A dropdown has no shared-library equivalent to compose from (M5-1/2/3 built scalars, ranges,
// lists, ramps and icon grids — no enum control). It has no drag to defer, so it commits at once.
template <typename EnumType>
void DrawEnumSetting(const char* label, EnumType& value, const char* const* names, int nameCount,
                     Pipeline::PreviewDriver* previewDriver) {
    int selectedIndex = static_cast<int>(value);
    if (!ImGui::Combo(label, &selectedIndex, names, nameCount)) return;
    value = static_cast<EnumType>(selectedIndex);
    NotifyChange(true, previewDriver);
}

const char* const noiseTypeNames[] = { "OpenSimplex2", "OpenSimplex2Smooth", "Cellular",
                                       "Perlin", "ValueCubic", "Value", "None" };
const char* const fractalTypeNames[] = { "None", "FractionalBrownian", "Ridged", "PingPong" };
const char* const blendModeNames[] = { "Add", "Subtract", "Multiply", "Overlay", "Maximum", "Minimum" };

// The per-layer noise + blend controls, all shared widgets.
void DrawLayerControls(Params::Layer& layer, LayersTabState& state,
                       Pipeline::PreviewDriver* previewDriver) {
    if (!state.octaveCountToggle.IsCommitDeferred() && !state.heightBlendToggle.IsCommitDeferred())
        LoadLayerTabValues(layer, state);
    DrawEnumSetting("Noise Type", layer.noiseType, noiseTypeNames, IM_ARRAYSIZE(noiseTypeNames), previewDriver);
    DrawEnumSetting("Fractal Type", layer.fractalType, fractalTypeNames, IM_ARRAYSIZE(fractalTypeNames), previewDriver);
    NotifyChange(DrawLabelledDial("Frequency", layer.frequency, state.frequencyRange,
                                  state.frequencyToggle, WidgetStyle(), "%.4f").bCommitted, previewDriver);
    WidgetChange change = DrawLabelledDial("Octaves", state.octaveCountValue, state.octaveCountRange,
                                           state.octaveCountToggle, WidgetStyle(), "%.0f");
    if (change.bValueChanged) StoreLayerTabValues(state, layer);
    NotifyChange(change.bCommitted, previewDriver);
    NotifyChange(DrawLabelledDial("Gain", layer.gain, state.gainRange, state.gainToggle).bCommitted, previewDriver);
    NotifyChange(DrawLabelledDial("Lacunarity", layer.lacunarity, state.lacunarityRange, state.lacunarityToggle).bCommitted, previewDriver);
    NotifyChange(DrawLabelledDial("Opacity", layer.opacity, state.opacityRange, state.opacityToggle).bCommitted, previewDriver);
    DrawEnumSetting("Blend Mode", layer.blendMode, blendModeNames, IM_ARRAYSIZE(blendModeNames), previewDriver);
    change = DrawRangeSlider("Height Blend Window", state.heightBlendValues,
                             state.heightBlendBounds, state.heightBlendToggle);
    if (change.bValueChanged) StoreLayerTabValues(state, layer);
    NotifyChange(change.bCommitted, previewDriver);
}

// One group's layer rows. Only the SELECTED group renders its list, so a drag can never carry a
// row index from one group onto another group's list.
DraggableListSignal DrawGroupBody(Params::GeoLayer& group, int groupIndex,
                                  const LayersTabState& state) {
    if (groupIndex != state.selectedGeoLayerIndex) {
        ImGui::Text("%d layer(s) - select this group to edit them", static_cast<int>(group.layers.size()));
        return DraggableListSignal();
    }
    char rowLabel[40] = { 0 };   // borrowed by describeRow for the duration of this Render only
    return DraggableList<Params::Layer>::Render(
        "layers", group.layers,
        [&](int rowIndex) {
            const Params::Layer& layer = group.layers[static_cast<std::size_t>(rowIndex)];
            std::snprintf(rowLabel, sizeof(rowLabel), "Layer %d (stratum %d)", rowIndex, layer.stratumIndex);
            DraggableListRow row;
            row.label    = rowLabel;
            row.bVisible = layer.bEnabled;
            row.bLocked  = layer.bLocked;
            return row;
        },
        [](int) {}, state.selectedLayerIndex);
}

// The group list plus the nested layer list, and the signals both produce.
void DrawGeoLayerList(Params::LayerStack& layerStack, LayersTabState& state,
                      Pipeline::PreviewDriver* previewDriver) {
    DraggableListSignal layerSignal;
    int layerSignalGroupIndex = -1;
    const DraggableListSignal groupSignal = DraggableList<Params::GeoLayer>::Render(
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
            const DraggableListSignal signal = DrawGroupBody(
                layerStack.geoLayers[static_cast<std::size_t>(rowIndex)], rowIndex, state);
            if (signal.bHasSignal()) { layerSignal = signal; layerSignalGroupIndex = rowIndex; }
        },
        state.selectedGeoLayerIndex);

    // The inner signal is applied FIRST: a group Delete in the same frame would move the indices
    // the layer signal is expressed in.
    bool bRecipeMoved = false;
    if (layerSignalGroupIndex >= 0)
        bRecipeMoved = ApplyLayerListSignal(
            layerStack.geoLayers[static_cast<std::size_t>(layerSignalGroupIndex)], layerSignal,
            state.selectedLayerIndex);
    if (groupSignal.bHasSignal())
        bRecipeMoved = ApplyGeoLayerListSignal(layerStack, groupSignal,
                                               state.selectedGeoLayerIndex) || bRecipeMoved;
    NotifyChange(bRecipeMoved, previewDriver);
}

} // namespace

Params::Layer* SelectedLayer(Params::LayerStack& layerStack, const LayersTabState& state) {
    if (state.selectedGeoLayerIndex < 0
        || state.selectedGeoLayerIndex >= static_cast<int>(layerStack.geoLayers.size())) return nullptr;
    Params::GeoLayer& group = layerStack.geoLayers[static_cast<std::size_t>(state.selectedGeoLayerIndex)];
    if (state.selectedLayerIndex < 0
        || state.selectedLayerIndex >= static_cast<int>(group.layers.size())) return nullptr;
    return &group.layers[static_cast<std::size_t>(state.selectedLayerIndex)];
}

void DrawLayersTab(Params::MapRecipe& recipe, LayersTabState& state,
                   Pipeline::PreviewDriver* previewDriver) {
    Params::LayerStack& layerStack = recipe.layerStack;
    ImGui::PushID("layersTab");
    bool bUnified = layerStack.simulationGrouping == Params::SimulationGrouping::Unified;
    if (ImGui::Checkbox("Unified simulation (off = Separate per GeoLayer)", &bUnified)) {
        layerStack.simulationGrouping = bUnified ? Params::SimulationGrouping::Unified
                                                 : Params::SimulationGrouping::Separate;
        NotifyChange(true, previewDriver);
    }
    ImGui::Separator();
    DrawGeoLayerList(layerStack, state, previewDriver);
    ImGui::Separator();
    Params::Layer* const layer = SelectedLayer(layerStack, state);
    if (layer != nullptr) DrawLayerControls(*layer, state, previewDriver);
    else ImGui::TextUnformatted("Select a layer to edit its noise and blend settings.");
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
