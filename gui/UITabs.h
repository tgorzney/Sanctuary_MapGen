#pragma once
#include "Parameters.h"

namespace SanmapGen {
namespace UI {

    // Reusable UI Framework for layer stacks
    // useGeoLayers=true groups layers by GeoLayer headers. useGeoLayers=false treats the array as a flat list.
    void RenderLayerStack(GenerationParams& params, std::vector<NoiseLayer>& flatLayers, std::vector<GeoLayerDef>* geoLayers, bool useGeoLayers, bool& bNeedsMapUpdate);

    // Tab rendering functions
    bool GradientEditor(const char* label, GradientSettings& gradient, float maxLocation = 100.0f);
    
    void RenderHeightmapTab(GenerationParams& params, bool& bNeedsMapUpdate);
    void RenderSlopeMapTab(GenerationParams& params, bool& bNeedsPreviewRender);
    void RenderFlowMapTab(GenerationParams& params, bool& bNeedsPreviewRender);
    void RenderAccumulationMapTab(GenerationParams& params, bool& bNeedsPreviewRender);
    void RenderStratumsTab(GenerationParams& params, bool& bNeedsMapUpdate);
    void RenderDetailNormalTab(GenerationParams& params, bool& bNeedsMapUpdate);
    void RenderSmoothnessTab(GenerationParams& params, bool& bNeedsMapUpdate);
    void RenderTintTab(GenerationParams& params, bool& bNeedsMapUpdate);
    void RenderWaterTab(GenerationParams& params, bool& bNeedsMapUpdate, bool& bNeedsPreviewRender);
    void RenderAtmosphereTab(GenerationParams& params, bool& bNeedsMapUpdate);
    void RenderHolesTab(GenerationParams& params, bool& bNeedsMapUpdate);
    void RenderMarkersTab(GenerationParams& params, bool& bNeedsMapUpdate);
    void RenderPropsTab(GenerationParams& params, bool& bNeedsMapUpdate);
    void RenderPerformanceTab(GenerationParams& params, bool& bNeedsMapUpdate);
    void RenderSaveExportTab(GenerationParams& params, bool& bNeedsMapUpdate);

} // namespace UI
} // namespace SanmapGen
