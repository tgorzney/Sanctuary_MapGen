#include "../UITabs.h"
#include "../UIHelpers.h"
#include "imgui.h"

namespace SanmapGen {
namespace UI {

    void RenderWaterTab(GenerationParams& params, bool& bNeedsMapUpdate, bool& bNeedsPreviewRender) {
        ImGui::Text("Water & Waves");
        ImGui::Separator();
        
        if (ImGui::CollapsingHeader("Water Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (UI::RangeSliderFloat("Water Level", &params.Water.WaterLevelMin, &params.Water.WaterLevelMax, params.TerrainMinHeight, params.TerrainMaxHeight)) {
                bNeedsPreviewRender = true;
            }
            
            if (UI::RangeSliderFloat("Deep Water", &params.Water.DeepWaterDepthMin, &params.Water.DeepWaterDepthMax, 0.0f, 128.0f)) {
                bNeedsPreviewRender = true;
            }
            
            if (UI::GradientEditor("Water Preview Gradient", params.Water.Gradient, 128.0f)) {
                bNeedsPreviewRender = true;
            }
        }
        
        if (ImGui::CollapsingHeader("Shore & Wind", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::SliderFloat("Wind Speed", &params.Water.WaterWindSpeed, 0.0f, 1.0f)) bNeedsPreviewRender = true;
            if (ImGui::SliderFloat("Wind Direction", &params.Water.WaterWindDirection, 0.0f, 360.0f)) bNeedsPreviewRender = true;
            if (ImGui::SliderFloat("Shore Waves Remap", &params.Water.WaterWindShoreWavesRemap, 0.0f, 1.0f)) bNeedsPreviewRender = true;
            if (ImGui::SliderFloat("Shore Depth Offset", &params.Water.WaterShoreDepthOffset, -10.0f, 10.0f)) bNeedsPreviewRender = true;
            if (ImGui::SliderFloat("Shore Depth Str", &params.Water.WaterShoreDepthStrength, 0.0f, 5.0f)) bNeedsPreviewRender = true;
            if (ImGui::SliderFloat("Shore Dist Offset", &params.Water.WaterShoreDistanceOffset, -5.0f, 5.0f)) bNeedsPreviewRender = true;
            if (ImGui::SliderFloat("Shore Dist Str", &params.Water.WaterShoreDistanceStrength, 0.0f, 5.0f)) bNeedsPreviewRender = true;
        }
        
        char buf[256]; strncpy(buf, params.Water.WaveGeneratorBlueprint.c_str(), sizeof(buf));
        if (ImGui::InputText("Wave Blueprint", buf, IM_ARRAYSIZE(buf))) {
            params.Water.WaveGeneratorBlueprint = buf;
            bNeedsPreviewRender = true;
        }
    }


} // namespace UI
} // namespace SanmapGen
