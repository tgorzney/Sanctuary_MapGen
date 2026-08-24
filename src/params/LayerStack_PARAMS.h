// LayerStack_PARAMS.h — the whole editable layer stack (the recipe's terrain half).
// Layer: PARAMS. Ordered GeoLayers plus the Separate/Unified simulation toggle.
// GetFlatLayers() flattens the enabled layers of enabled groups, in stack order —
// the order downstream generation consumes.
#pragma once
#include <algorithm>
#include <vector>
#include "GenerationEnums_PARAMS.h"
#include "GeoLayer_PARAMS.h"

namespace SanmapGen {
namespace Params {

struct LayerStack {
    std::vector<GeoLayer> geoLayers;
    SimulationGrouping    simulationGrouping = SimulationGrouping::Unified;

    // Enabled layers of enabled GeoLayers, in stack order (top group first).
    // The returned pointers are a transient view — valid only until the stack is modified.
    std::vector<const Layer*> GetFlatLayers() const {
        std::vector<const Layer*> flat;
        for (const GeoLayer& group : geoLayers) {
            if (!group.bEnabled) continue;
            for (const Layer& layer : group.layers)
                if (layer.bEnabled) flat.push_back(&layer);
        }
        return flat;
    }

    int TotalLayerCount() const {
        int count = 0;
        for (const GeoLayer& group : geoLayers) count += static_cast<int>(group.layers.size());
        return count;
    }
};

// The stable id a newly created (or newly baked) layer takes: max-plus-one across every layer's
// `layerIdentifier` in the current in-memory stack, or 0 if none are baked yet, across ALL
// GeoLayers (not scoped to one group — the cache this keys, Data::BakedLayerImage, is stack-wide).
// Derive-on-create, NOT a persisted counter — same posture STEP56 ratified for
// PropInstanceLayer::layerId/DecalInstanceLayer::layerId (NextPropLayerId, PropsTab_Manual_UI.h):
// self-healing across manual JSON edits, ids already present in a loaded file are never renumbered.
// Declared here (not Layer_PARAMS.h) because it needs GeoLayer/LayerStack's complete types, both
// only available once this header's own includes are in place; the call sites themselves are
// STEP102's, since they require IO — UI already depends on IO, PARAMS does not and must not.
inline int NextLayerIdentifier(const Params::LayerStack& layerStack) {
    int maximumId = -1;
    for (const Params::GeoLayer& group : layerStack.geoLayers)
        for (const Params::Layer& layer : group.layers)
            maximumId = std::max(maximumId, layer.layerIdentifier);
    return maximumId + 1;
}

} // namespace Params
} // namespace SanmapGen
