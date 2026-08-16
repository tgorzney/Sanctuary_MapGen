// LayerEditor_Action_UI.h — the Layer Editor's row actions: Add, Duplicate, Import RAW, Bake.
// Layer: UI. Accuracy class: Visual. Pure and headless-testable — no imgui, no draw.
//
// Same contract as the shared DraggableList (DraggableListWidget_UI.h): the draw path only
// DETECTS what the user asked for and hands back one action; applying it to the caller's arrays
// is a separate, assertable function. That is what makes "duplicate the third layer" testable
// with no window open, and it is why an action carries indices rather than pointers — a vector
// that grows during Apply would invalidate them.
//
// SCOPE NOTE (ARCH §8.4 — a coder never invents a missing field; reported, not invented):
// `ImportRawRequested` and `BakeToggleRequested` are DETECTED and never applied here.
// `Params::Layer` carries no imported-image path and no baked-state flag — ARCH §5.2 evicted the
// god-object's image-bake state from the layer record and did not give it a new home, so there is
// nothing legal to write. The affordances, the picker's extension fence and the signal are
// complete; the PARAMS field and the bake itself need their own work-order.
#pragma once
#include <string>
#include "../params/LayerStack_PARAMS.h"

namespace SanmapGen {
namespace Ui {

enum class LayerEditorActionKind : int {
    None = 0, AddGeoLayer, AddLayer, DuplicateLayer, ImportRawRequested, BakeToggleRequested
};

// One frame produces at most ONE action (first wins), exactly like a DraggableListSignal: every
// kind changes what the next frame draws.
struct LayerEditorAction {
    LayerEditorActionKind kind          = LayerEditorActionKind::None;
    int                   geoLayerIndex = -1;
    int                   layerIndex    = -1;   // -1 for the group-level kinds
    std::string           importRawPath;        // ImportRawRequested only; the picker's answer
    bool bHasAction() const { return kind != LayerEditorActionKind::None; }
};

// Records an action if none was recorded yet.
inline void RecordLayerEditorAction(LayerEditorAction& action, LayerEditorActionKind kind,
                                    int geoLayerIndex, int layerIndex = -1) {
    if (action.bHasAction()) return;
    action.kind          = kind;
    action.geoLayerIndex = geoLayerIndex;
    action.layerIndex    = layerIndex;
}

// True when the indices name a layer that exists.
inline bool LayerEditorActionNamesLayer(const Params::LayerStack& layerStack, int geoLayerIndex,
                                        int layerIndex) {
    if (geoLayerIndex < 0 || geoLayerIndex >= static_cast<int>(layerStack.geoLayers.size())) return false;
    const Params::GeoLayer& group = layerStack.geoLayers[static_cast<std::size_t>(geoLayerIndex)];
    return layerIndex >= 0 && layerIndex < static_cast<int>(group.layers.size());
}

// Applies the three STRUCTURAL kinds to the caller's stack and moves the selection onto whatever
// was created. The two reported-only kinds (see the SCOPE NOTE) answer false and touch nothing.
// Returns true when the RECIPE moved.
inline bool ApplyLayerEditorAction(Params::LayerStack& layerStack, const LayerEditorAction& action,
                                   int& selectedGeoLayerIndex, int& selectedLayerIndex) {
    if (action.kind == LayerEditorActionKind::AddGeoLayer) {
        layerStack.geoLayers.push_back(Params::GeoLayer());
        selectedGeoLayerIndex = static_cast<int>(layerStack.geoLayers.size()) - 1;
        selectedLayerIndex    = 0;
        return true;
    }
    if (action.geoLayerIndex < 0
        || action.geoLayerIndex >= static_cast<int>(layerStack.geoLayers.size())) return false;
    Params::GeoLayer& group = layerStack.geoLayers[static_cast<std::size_t>(action.geoLayerIndex)];
    if (action.kind == LayerEditorActionKind::AddLayer) {
        group.layers.push_back(Params::Layer());
        selectedGeoLayerIndex = action.geoLayerIndex;
        selectedLayerIndex    = static_cast<int>(group.layers.size()) - 1;
        return true;
    }
    if (action.kind != LayerEditorActionKind::DuplicateLayer) return false;
    if (!LayerEditorActionNamesLayer(layerStack, action.geoLayerIndex, action.layerIndex)) return false;
    // The copy lands directly ABOVE its source, where the v1 duplicate put it, so a duplicate is
    // adjacent to what it was cloned from rather than at the bottom of a forty-row stack.
    const Params::Layer duplicatedLayer = group.layers[static_cast<std::size_t>(action.layerIndex)];
    group.layers.insert(group.layers.begin() + action.layerIndex, duplicatedLayer);
    selectedGeoLayerIndex = action.geoLayerIndex;
    selectedLayerIndex    = action.layerIndex;
    return true;
}

} // namespace Ui
} // namespace SanmapGen
