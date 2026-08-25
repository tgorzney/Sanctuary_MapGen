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

    // Generation-included layers of generation-included GeoLayers, in stack order (top group
    // first). Gated on bDisabled ONLY (STEP152) — bEnabled is UI-visibility metadata and has no
    // say here; a hidden-but-not-disabled layer still generates.
    // The returned pointers are a transient view — valid only until the stack is modified.
    std::vector<const Layer*> GetFlatLayers() const {
        std::vector<const Layer*> flat;
        for (const GeoLayer& group : geoLayers) {
            if (group.bDisabled) continue;
            for (const Layer& layer : group.layers)
                if (!layer.bDisabled) flat.push_back(&layer);
        }
        return flat;
    }

    // True when the flattened stack has at least one layer that will actually be live-computed
    // this run: generation-included, not frozen, and not the always-flat NoiseType::None sentinel
    // (an unbaked recipe-less layer contributes nothing and must not count as "active"). Gates
    // Erosion/Thermal/FlowAccumulation (GenerationAssembler_Stages_PIPELINE.cpp, STEP152).
    bool HasActiveProceduralLayer() const {
        for (const Layer* layer : GetFlatLayers())
            if (!layer->bBaked && layer->noiseType != NoiseType::None) return true;
        return false;
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
