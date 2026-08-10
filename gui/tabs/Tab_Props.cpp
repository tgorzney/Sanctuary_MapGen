#include "../UITabs.h"
#include "../UIHelpers.h"
#include "../widgets/Widget_LayerManager.h"
#include "imgui.h"

namespace SanmapGen {
namespace UI {

    void RenderPropsTab(GenerationParams& params, bool& bNeedsMapUpdate, bool& bNeedsPreviewRender) {
        ImGui::Text("Props & Decals (Unified GeoLayers)");
        ImGui::Separator();
        
        ImGui::TextDisabled("Props use the same procedural layering system as terrain.");
        ImGui::TextDisabled("Mask density automatically defines prop density and placement chances.");
        ImGui::Spacing();
        
        std::vector<NoiseLayer> dummy;
        Widget_LayerManager::RenderLayerStack(params, dummy, &params.GeoLayers, true, bNeedsMapUpdate, LayerType::Prop);
        
        ImGui::Separator();
        ImGui::Text("Decal Rules");
        Widget_LayerManager::RenderLayerStack(params, dummy, &params.GeoLayers, true, bNeedsMapUpdate, LayerType::Decal);
        
        // Reclaim density moved to markers tab
}


} // namespace UI
} // namespace SanmapGen
