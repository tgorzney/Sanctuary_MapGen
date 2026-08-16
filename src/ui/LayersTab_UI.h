// LayersTab_UI.h — the layer-stack tab: the GeoLayer groups, their layers, and the
// Separate/Unified simulation toggle. Layer: UI. Accuracy class: Visual.
// Edits exactly one recipe slice — `Params::LayerStack` (its GeoLayers and their Layers).
//
// Reorder / enable / lock / delete run through the shared DraggableList (M5-2), which DETECTS
// what the user asked for and mutates nothing; the two functions below apply that signal to the
// caller's stack, which is what makes a reorder assertable with no window open. The tier of any
// resulting edit is derived by Pipeline::PreviewDriver from the stage hashes — never here.
//
// GEOLAYER LOCK: `Params::GeoLayer` has no lock field of its own; `Params::Layer::bLocked` does
// exist, so a GROUP counts as locked exactly when every one of its layers is, and the widget's
// lock affordance sets them together. No PARAMS type was extended to add a group flag
// (ARCH §8.4 — a coder never invents a missing field).
#pragma once
#include "DraggableListWidget_UI.h"
#include "RangeSliderWidget_UI.h"
#include "LabelledDialWidget_UI.h"
#include "../params/LayerStack_PARAMS.h"

namespace SanmapGen {
namespace Params { struct MapRecipe; }
namespace Pipeline { class PreviewDriver; }
namespace Ui {

struct LayersTabState {
    // Control limits — settings, not literals at a use site (Constitution §8).
    DialRange frequencyRange{ 0.0001f, 0.2f, 0.0f, 600.0f };
    DialRange octaveCountRange{ 1.0f, 12.0f, 1.0f, 400.0f };
    DialRange gainRange{ 0.0f, 1.0f, 0.0f, 400.0f };
    DialRange lacunarityRange{ 1.0f, 4.0f, 0.0f, 400.0f };
    DialRange opacityRange{ 0.0f, 1.0f, 0.0f, 400.0f };
    RangeSliderBounds heightBlendBounds{ 0.0f, 1.0f, 0.001f };

    RealtimeToggle frequencyToggle;
    RealtimeToggle octaveCountToggle;
    RealtimeToggle gainToggle;
    RealtimeToggle lacunarityToggle;
    RealtimeToggle opacityToggle;
    RealtimeToggle heightBlendToggle;

    int   selectedGeoLayerIndex = 0;
    int   selectedLayerIndex    = 0;
    float octaveCountValue      = 5.0f;             // int mirror: controls edit floats
    RangeSliderValues heightBlendValues;            // Layer::heightBlendMinimum/Maximum mirror
};

// A group is locked when it has layers and all of them are.
inline bool IsGeoLayerLocked(const Params::GeoLayer& group) {
    if (group.layers.empty()) return false;
    for (const Params::Layer& layer : group.layers)
        if (!layer.bLocked) return false;
    return true;
}

// Applies one group-list signal. Reorder/Delete are the shared structural helper; visibility is
// the group's own bEnabled; lock is the derived group lock above; Select moves the selection
// only. Returns true when the RECIPE moved (a selection change did not).
inline bool ApplyGeoLayerListSignal(Params::LayerStack& layerStack, const DraggableListSignal& signal,
                                    int& selectedGeoLayerIndex) {
    const int groupCount = static_cast<int>(layerStack.geoLayers.size());
    if (signal.sourceRowIndex < 0 || signal.sourceRowIndex >= groupCount) return false;
    Params::GeoLayer& group = layerStack.geoLayers[static_cast<std::size_t>(signal.sourceRowIndex)];
    if (signal.kind == DraggableListSignalKind::ToggleVisibility) {
        group.bEnabled = !group.bEnabled;
        return true;
    }
    if (signal.kind == DraggableListSignalKind::ToggleLock) {
        const bool bLock = !IsGeoLayerLocked(group);
        for (Params::Layer& layer : group.layers) layer.bLocked = bLock;
        return true;
    }
    if (signal.kind == DraggableListSignalKind::Select) {
        selectedGeoLayerIndex = signal.sourceRowIndex;
        return false;
    }
    if (signal.kind == DraggableListSignalKind::Reorder) selectedGeoLayerIndex = signal.targetRowIndex;
    return ApplyDraggableListSignal(layerStack.geoLayers, signal);
}

// The same for one group's layer list.
inline bool ApplyLayerListSignal(Params::GeoLayer& group, const DraggableListSignal& signal,
                                 int& selectedLayerIndex) {
    const int layerCount = static_cast<int>(group.layers.size());
    if (signal.sourceRowIndex < 0 || signal.sourceRowIndex >= layerCount) return false;
    Params::Layer& layer = group.layers[static_cast<std::size_t>(signal.sourceRowIndex)];
    if (signal.kind == DraggableListSignalKind::ToggleVisibility) {
        layer.bEnabled = !layer.bEnabled;
        return true;
    }
    if (signal.kind == DraggableListSignalKind::ToggleLock) {
        layer.bLocked = !layer.bLocked;
        return true;
    }
    if (signal.kind == DraggableListSignalKind::Select) {
        selectedLayerIndex = signal.sourceRowIndex;
        return false;
    }
    if (signal.kind == DraggableListSignalKind::Reorder) selectedLayerIndex = signal.targetRowIndex;
    return ApplyDraggableListSignal(group.layers, signal);
}

// The layer the per-layer controls edit, or null when the selection points at nothing.
Params::Layer* SelectedLayer(Params::LayerStack& layerStack, const LayersTabState& state);
inline void LoadLayerTabValues(const Params::Layer& layer, LayersTabState& state) {
    state.octaveCountValue = static_cast<float>(layer.octaves);
    state.heightBlendValues.minimumValue = layer.heightBlendMinimum;
    state.heightBlendValues.maximumValue = layer.heightBlendMaximum;
}
inline bool StoreLayerTabValues(const LayersTabState& state, Params::Layer& layer) {
    const int octaves = static_cast<int>(ClampDialValue(state.octaveCountValue, state.octaveCountRange) + 0.5f);
    const bool bMoved = octaves != layer.octaves
                     || state.heightBlendValues.minimumValue != layer.heightBlendMinimum
                     || state.heightBlendValues.maximumValue != layer.heightBlendMaximum;
    layer.octaves            = octaves;
    layer.heightBlendMinimum = state.heightBlendValues.minimumValue;
    layer.heightBlendMaximum = state.heightBlendValues.maximumValue;
    return bMoved;
}

void DrawLayersTab(Params::MapRecipe& recipe, LayersTabState& state,
                   Pipeline::PreviewDriver* previewDriver);

} // namespace Ui
} // namespace SanmapGen
