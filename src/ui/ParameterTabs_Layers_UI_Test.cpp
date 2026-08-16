// ParameterTabs_Layers_UI_Test.cpp — the Layers and Water tab checks (M5-6 acceptance).
// Stack editing is asserted through the DraggableList SIGNAL path, which is what makes a reorder,
// an enable and a lock testable with no window open: the widget detects, the tab applies.
#include "LayersTab_UI.h"
#include "WaterTab_UI.h"
#include "ParameterTabs_TestSupport_UI.h"
#include "../params/MapRecipe_PARAMS.h"

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

DraggableListSignal MakeSignal(DraggableListSignalKind kind, int sourceRowIndex,
                               int targetRowIndex = -1) {
    DraggableListSignal signal;
    signal.kind           = kind;
    signal.sourceRowIndex = sourceRowIndex;
    signal.targetRowIndex = targetRowIndex;
    return signal;
}

// Reorder / enable / lock on the stack itself.
void RunStackSignalChecks(Params::LayerStack& layerStack, LayersTabState& state) {
    Params::GeoLayer& group = layerStack.geoLayers[0];
    const float firstFrequency  = group.layers[0].frequency;
    const float secondFrequency = group.layers[1].frequency;
    Check(ApplyLayerListSignal(group, MakeSignal(DraggableListSignalKind::Reorder, 0, 1), state.selectedLayerIndex),
          "a layer reorder moves the recipe");
    Check(group.layers[1].frequency == firstFrequency && group.layers[0].frequency == secondFrequency,
          "the dragged layer landed on the drop row");
    Check(state.selectedLayerIndex == 1, "the selection followed the dragged row");

    Check(ApplyLayerListSignal(group, MakeSignal(DraggableListSignalKind::ToggleVisibility, 0), state.selectedLayerIndex),
          "the visibility affordance moves the recipe");
    Check(!group.layers[0].bEnabled, "the layer's bEnabled flipped");
    Check(layerStack.GetFlatLayers().size() == 1, "and the disabled layer left the flattened stack");
    ApplyLayerListSignal(group, MakeSignal(DraggableListSignalKind::ToggleVisibility, 0), state.selectedLayerIndex);

    Check(!IsGeoLayerLocked(group), "the group starts unlocked");
    Check(ApplyGeoLayerListSignal(layerStack, MakeSignal(DraggableListSignalKind::ToggleLock, 0),
                                  state.selectedGeoLayerIndex),
          "the group lock affordance moves the recipe");
    Check(IsGeoLayerLocked(group) && group.layers[0].bLocked && group.layers[1].bLocked,
          "a GeoLayer lock is exactly its layers' Layer::bLocked — no invented group field");

    Check(!ApplyGeoLayerListSignal(layerStack, MakeSignal(DraggableListSignalKind::Select, 0),
                                   state.selectedGeoLayerIndex),
          "a selection change is not a recipe change");
    Check(ApplyGeoLayerListSignal(layerStack, MakeSignal(DraggableListSignalKind::ToggleVisibility, 0),
                                  state.selectedGeoLayerIndex)
              && !layerStack.geoLayers[0].bEnabled,
          "a group can be disabled as a whole");
    ApplyGeoLayerListSignal(layerStack, MakeSignal(DraggableListSignalKind::ToggleVisibility, 0),
                            state.selectedGeoLayerIndex);
}

// The per-layer noise and blend controls.
void RunLayerControlChecks(Params::LayerStack& layerStack, LayersTabState& state) {
    Params::Layer* const layer = SelectedLayer(layerStack, state);
    Check(layer != nullptr, "the selection resolves to a layer");
    if (layer == nullptr) return;
    LoadLayerTabValues(*layer, state);

    const float settledFrequency = layer->frequency;
    WidgetChange change = StepDialInteraction(state.frequencyToggle, layer->frequency,
                                              state.frequencyRange, DialDrag(-25.0f));
    Check(change.bValueChanged && layer->frequency > settledFrequency,
          "the frequency dial writes the layer in place");
    Check(StepDialInteraction(state.frequencyToggle, layer->frequency, state.frequencyRange,
                              DialRelease()).bCommitted, "and commits on release");

    const int settledOctaves = layer->octaves;
    StepDialInteraction(state.octaveCountToggle, state.octaveCountValue, state.octaveCountRange,
                        DialDrag(-100.0f));
    Check(StoreLayerTabValues(state, *layer), "the octave mirror reports a recipe move");
    Check(layer->octaves > settledOctaves, "the octave count reached the recipe");

    change = StepRangeSliderInteraction(state.heightBlendToggle, state.heightBlendValues,
                                        state.heightBlendBounds,
                                        GrabRangeHandle(RangeSliderHandle::Maximum, 0.5f));
    Check(change.bValueChanged, "the height-blend slider moved");
    StoreLayerTabValues(state, *layer);
    Check(layer->heightBlendMaximum == 0.5f, "the blend window maximum reached the recipe");
    Check(StepRangeSliderInteraction(state.heightBlendToggle, state.heightBlendValues,
                                     state.heightBlendBounds, ReleaseRangeHandle()).bCommitted,
          "and the range slider commits on release");
}

} // namespace

void RunLayersTabChecks(Params::MapRecipe& recipe) {
    LayersTabState state;
    RunStackSignalChecks(recipe.layerStack, state);
    RunLayerControlChecks(recipe.layerStack, state);
}

void RunWaterTabChecks(Params::MapRecipe& recipe) {
    WaterTabState state;
    LoadWaterTabValues(recipe.water, state);
    const float settledLevel = recipe.water.waterLevelMaximum;
    StepDialInteraction(state.waterLevelToggle, recipe.water.waterLevelMaximum,
                        state.waterLevelRange, DialDrag(-40.0f));
    Check(recipe.water.waterLevelMaximum > settledLevel, "the water level reached the recipe");

    StepRangeSliderInteraction(state.deepWaterDepthToggle, state.deepWaterDepthValues,
                               state.deepWaterDepthBounds,
                               GrabRangeHandle(RangeSliderHandle::Maximum, 40.0f));
    Check(StoreWaterTabValues(state, recipe.water), "the depth window store reports a move");
    Check(recipe.water.deepWaterDepthMaximum == 40.0f, "the deep-water depth reached the recipe");
    Check(recipe.water.deepWaterDepthMinimum <= recipe.water.deepWaterDepthMaximum,
          "the window stayed ordered");
}
