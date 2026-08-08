#pragma once
#include <imgui.h>
#include "../core/Parameters.h"
#include <vector>

namespace SanmapGen {
    class Widget_LayerManager {
    public:
        static void RenderLayerStack(GenerationParams& params, std::vector<NoiseLayer>& flatLayers, 
                                     std::vector<GeoLayerDef>* geoLayers, bool useGeoLayers, bool& bNeedsMapUpdate);
    private:
        static void RenderSingleLayerSettings(GenerationParams& params, size_t i, NoiseLayer& layer, 
                                              std::vector<NoiseLayer>& layerArray, bool& bNeedsMapUpdate);
    };
}
