// LayerStack_PARAMS.h — the whole editable layer stack (the recipe's terrain half).
// Layer: PARAMS. Ordered GeoLayers plus the Separate/Unified simulation toggle.
// GetFlatLayers() flattens the enabled layers of enabled groups, in stack order —
// the order downstream generation consumes.
#pragma once
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

} // namespace Params
} // namespace SanmapGen
