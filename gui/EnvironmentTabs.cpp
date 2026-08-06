#include "UITabs.h"
#include "imgui.h"
#include "FileDialog.h"

namespace SanmapGen {
namespace UI {

    void RenderWaterTab(GenerationParams& params, bool& bNeedsMapUpdate, bool& bNeedsPreviewRender) {
        ImGui::Text("Water & Waves");
        ImGui::Separator();
        
        if (ImGui::CollapsingHeader("Water Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::SliderFloat("Water Level Min", &params.Water.WaterLevelMin, 0.0f, 128.0f)) bNeedsPreviewRender = true;
            if (ImGui::SliderFloat("Water Level Max", &params.Water.WaterLevelMax, 0.0f, 128.0f)) bNeedsPreviewRender = true;
            if (ImGui::SliderFloat("Deep Min", &params.Water.DeepWaterDepthMin, 0.0f, 128.0f)) bNeedsPreviewRender = true;
            if (ImGui::SliderFloat("Deep Max", &params.Water.DeepWaterDepthMax, 0.0f, 128.0f)) bNeedsPreviewRender = true;
        }
        
        if (ImGui::CollapsingHeader("Shore & Wind", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::SliderFloat("Wind Speed", &params.Water.WaterWindSpeed, 0.0f, 1.0f)) bNeedsPreviewRender = true;
            if (ImGui::SliderFloat("Wind Direction", &params.Water.WaterWindDirection, 0.0f, 360.0f)) bNeedsPreviewRender = true;
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

    void RenderAtmosphereTab(GenerationParams& params, bool& bNeedsMapUpdate, bool& bNeedsPreviewRender) {
        ImGui::Text("Atmosphere & Lighting");
        ImGui::Separator();
        
        if (ImGui::CollapsingHeader("Sun", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::SliderFloat("Right Ascension", &params.Atmosphere.SunRA, 0.0f, 360.0f)) bNeedsPreviewRender = true;
            if (ImGui::SliderFloat("Declination", &params.Atmosphere.SunDA, -90.0f, 90.0f)) bNeedsPreviewRender = true;
            if (ImGui::DragFloat("Intensity", &params.Atmosphere.SunIntensity, 100.0f, 0.0f, 100000.0f)) bNeedsPreviewRender = true;
            if (ImGui::ColorEdit4("Tint", params.Atmosphere.SunTint)) bNeedsPreviewRender = true;
            if (ImGui::SliderFloat("Temperature", &params.Atmosphere.SunTemperature, 1000.0f, 10000.0f)) bNeedsPreviewRender = true;
            if (ImGui::SliderFloat("Angular Dia", &params.Atmosphere.SunAngularDiameter, 0.1f, 5.0f)) bNeedsPreviewRender = true;
            if (ImGui::SliderFloat("Volumetric Mult", &params.Atmosphere.SunVolumetricsMultiplier, 0.0f, 10.0f)) bNeedsPreviewRender = true;
            if (ImGui::SliderFloat("Volumetric Dimer", &params.Atmosphere.SunVolumetricsShadowDimer, 0.0f, 1.0f)) bNeedsPreviewRender = true;
        }
        
        if (ImGui::CollapsingHeader("Skylight", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::DragFloat("Sky Intensity", &params.Atmosphere.SkylightIntensity, 100.0f, 0.0f, 100000.0f)) bNeedsPreviewRender = true;
            if (ImGui::ColorEdit4("Sky Tint", params.Atmosphere.SkylightTint)) bNeedsPreviewRender = true;
            if (ImGui::SliderFloat("Sky Temp", &params.Atmosphere.SkylightTemperature, 1000.0f, 10000.0f)) bNeedsPreviewRender = true;
        }
        
        if (ImGui::CollapsingHeader("Exposure & Fog", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::SliderFloat("Exposure", &params.Atmosphere.Exposure, 0.0f, 20.0f)) bNeedsPreviewRender = true;
            if (ImGui::SliderFloat("Exp Comp", &params.Atmosphere.ExposureCompensation, -5.0f, 5.0f)) bNeedsPreviewRender = true;
            if (ImGui::SliderFloat("Skybox Exp", &params.Atmosphere.SkyboxExposure, 0.0f, 20.0f)) bNeedsPreviewRender = true;
            
            if (ImGui::SliderFloat("Fog Atten Dist", &params.Atmosphere.FogAttenuationDistance, 0.0f, 10.0f)) bNeedsMapUpdate = true;
            if (ImGui::SliderFloat("Fog Base H", &params.Atmosphere.FogBaseHeight, -100.0f, 500.0f)) bNeedsMapUpdate = true;
            if (ImGui::SliderFloat("Fog Max H", &params.Atmosphere.FogMaximumHeight, 0.0f, 1000.0f)) bNeedsMapUpdate = true;
            if (ImGui::SliderFloat("Fog Max Dist", &params.Atmosphere.FogMaximumDistance, 0.0f, 10000.0f)) bNeedsMapUpdate = true;
            if (ImGui::SliderFloat("Fog Anisotropy", &params.Atmosphere.FogAnisotropy, 0.0f, 1.0f)) bNeedsMapUpdate = true;
        }
        
        if (ImGui::CollapsingHeader("Wind (Global)", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::SliderFloat("Wind Speed", &params.Atmosphere.GlobalWindSpeed, 0.0f, 10.0f)) bNeedsMapUpdate = true;
            if (ImGui::SliderFloat("Wind Direction", &params.Atmosphere.GlobalWindDirection, 0.0f, 360.0f)) bNeedsMapUpdate = true;
        }
    }

    void RenderMarkersTab(GenerationParams& params, bool& bNeedsMapUpdate, bool& bNeedsPreviewRender) {
        ImGui::Text("Main Settings");
        ImGui::Separator();
        
        if (ImGui::SliderInt("Spawn Count", &params.SpawnPointCount, 2, 16)) bNeedsMapUpdate = true;
        if (ImGui::SliderFloat("Alloy Mult", &params.AlloyMultiplier, 0.0f, 3.0f)) bNeedsPreviewRender = true;
        if (ImGui::SliderFloat("Hydro Mult", &params.HydroMultiplier, 0.0f, 3.0f)) bNeedsPreviewRender = true;
        if (ImGui::SliderFloat("Mex Density", &params.MexDensity, 0.0f, 3.0f)) bNeedsPreviewRender = true;
        // Also move reclaim density here since it belongs with markers
        if (ImGui::SliderFloat("Reclaim Density", &params.ReclaimDensity, 0.0f, 5.0f)) bNeedsPreviewRender = true;
        
        ImGui::Spacing();
        ImGui::Text("Custom Marker Rules");
        ImGui::Separator();
        
        if (ImGui::Button("Add Marker Rule", ImVec2(-1, 30))) {
            MarkerRule rule;
            rule.Name = "Marker " + std::to_string(params.Markers.size());
            params.Markers.push_back(rule);
            bNeedsPreviewRender = true;
        }
        ImGui::Spacing();
        
        for (int i = 0; i < (int)params.Markers.size(); ++i) {
            ImGui::PushID(i);
            char label[64]; snprintf(label, sizeof(label), "Marker %d - %s", i, params.Markers[i].Name.c_str());
            if (ImGui::CollapsingHeader(label)) {
                if (ImGui::Checkbox("Enabled", &params.Markers[i].Enabled)) bNeedsPreviewRender = true;
                
                char nameBuf[128]; strncpy(nameBuf, params.Markers[i].Name.c_str(), sizeof(nameBuf));
                if (ImGui::InputText("Name", nameBuf, IM_ARRAYSIZE(nameBuf))) params.Markers[i].Name = nameBuf;
                
                ImGui::Spacing();
                ImGui::Text("Icon Selection");
                char iconBuf[256]; strncpy(iconBuf, params.Markers[i].IconPath.c_str(), sizeof(iconBuf));
                if (ImGui::InputText("Icon Path", iconBuf, IM_ARRAYSIZE(iconBuf))) params.Markers[i].IconPath = iconBuf;
                ImGui::SameLine();
                if (ImGui::Button("...##Icon")) {
                    std::string path;
                    if (FileDialog::OpenFile("Images\0*.png;*.dds;*.tga\0All Files\0*.*\0", path)) {
                        params.Markers[i].IconPath = path;
                        bNeedsPreviewRender = true;
                    }
                }
                ImGui::Spacing();
                
                if (ImGui::SliderFloat("Density", &params.Markers[i].Density, 0.0f, 10.0f)) bNeedsPreviewRender = true;
                if (ImGui::SliderFloat("Min Slope", &params.Markers[i].MinSlope, 0.0f, 90.0f)) bNeedsPreviewRender = true;
                if (ImGui::SliderFloat("Max Slope", &params.Markers[i].MaxSlope, 0.0f, 90.0f)) bNeedsPreviewRender = true;
                if (ImGui::SliderFloat("Min Height", &params.Markers[i].MinHeight, 0.0f, 128.0f)) bNeedsPreviewRender = true;
                if (ImGui::SliderFloat("Max Height", &params.Markers[i].MaxHeight, 0.0f, 128.0f)) bNeedsPreviewRender = true;
                
                if (ImGui::Button("Delete Rule", ImVec2(-1, 20))) {
                    params.Markers.erase(params.Markers.begin() + i);
                    bNeedsPreviewRender = true;
                    ImGui::PopID();
                    break; // break to avoid invalid iterators
                }
            }
            ImGui::PopID();
        }
    }

    void RenderPropsTab(GenerationParams& params, bool& bNeedsMapUpdate, bool& bNeedsPreviewRender) {
        ImGui::Text("Props & Decals");
        ImGui::Separator();
        
        if (ImGui::Button("Add Prop Rule", ImVec2(-1, 30))) {
            PropRule rule;
            rule.Name = "Prop " + std::to_string(params.Props.size());
            params.Props.push_back(rule);
            bNeedsPreviewRender = true;
        }
        ImGui::Spacing();
        
        for (int i = 0; i < (int)params.Props.size(); ++i) {
            ImGui::PushID(i + 1000);
            char label[64]; snprintf(label, sizeof(label), "Prop %d - %s", i, params.Props[i].Name.c_str());
            if (ImGui::CollapsingHeader(label)) {
                if (ImGui::Checkbox("Enabled", &params.Props[i].Enabled)) bNeedsPreviewRender = true;
                
                char nameBuf[128]; strncpy(nameBuf, params.Props[i].Name.c_str(), sizeof(nameBuf));
                if (ImGui::InputText("Name", nameBuf, IM_ARRAYSIZE(nameBuf))) params.Props[i].Name = nameBuf;
                
                char bpBuf[256]; strncpy(bpBuf, params.Props[i].BlueprintPath.c_str(), sizeof(bpBuf));
                if (ImGui::InputText("Blueprint", bpBuf, IM_ARRAYSIZE(bpBuf))) params.Props[i].BlueprintPath = bpBuf;
                
                if (ImGui::SliderFloat("Density", &params.Props[i].Density, 0.0f, 10.0f)) bNeedsPreviewRender = true;
                if (ImGui::SliderFloat("Min Slope", &params.Props[i].MinSlope, 0.0f, 90.0f)) bNeedsPreviewRender = true;
                if (ImGui::SliderFloat("Max Slope", &params.Props[i].MaxSlope, 0.0f, 90.0f)) bNeedsPreviewRender = true;
                if (ImGui::SliderFloat("Min Height", &params.Props[i].MinHeight, 0.0f, 128.0f)) bNeedsPreviewRender = true;
                if (ImGui::SliderFloat("Max Height", &params.Props[i].MaxHeight, 0.0f, 128.0f)) bNeedsPreviewRender = true;
                if (ImGui::Checkbox("Avoid Water", &params.Props[i].AvoidWater)) bNeedsPreviewRender = true;
                if (ImGui::Checkbox("Near Cliffs", &params.Props[i].NearCliffs)) bNeedsPreviewRender = true;
                
                if (ImGui::Button("Delete Rule", ImVec2(-1, 20))) {
                    params.Props.erase(params.Props.begin() + i);
                    bNeedsPreviewRender = true;
                    ImGui::PopID();
                    break;
                }
            }
            ImGui::PopID();
        }
        
        ImGui::Separator();
        
        if (ImGui::Button("Add Decal Rule", ImVec2(-1, 30))) {
            DecalRule rule;
            rule.Name = "Decal " + std::to_string(params.Decals.size());
            params.Decals.push_back(rule);
            bNeedsPreviewRender = true;
        }
        ImGui::Spacing();
        
        for (int i = 0; i < (int)params.Decals.size(); ++i) {
            ImGui::PushID(i + 2000);
            char label[64]; snprintf(label, sizeof(label), "Decal %d - %s", i, params.Decals[i].Name.c_str());
            if (ImGui::CollapsingHeader(label)) {
                if (ImGui::Checkbox("Enabled", &params.Decals[i].Enabled)) bNeedsPreviewRender = true;
                
                char nameBuf[128]; strncpy(nameBuf, params.Decals[i].Name.c_str(), sizeof(nameBuf));
                if (ImGui::InputText("Name", nameBuf, IM_ARRAYSIZE(nameBuf))) params.Decals[i].Name = nameBuf;
                
                char albBuf[256]; strncpy(albBuf, params.Decals[i].AlbedoPath.c_str(), sizeof(albBuf));
                if (ImGui::InputText("Albedo", albBuf, IM_ARRAYSIZE(albBuf))) params.Decals[i].AlbedoPath = albBuf;
                
                char nrmBuf[256]; strncpy(nrmBuf, params.Decals[i].NormalPath.c_str(), sizeof(nrmBuf));
                if (ImGui::InputText("Normal", nrmBuf, IM_ARRAYSIZE(nrmBuf))) params.Decals[i].NormalPath = nrmBuf;
                
                if (ImGui::SliderFloat("Density", &params.Decals[i].Density, 0.0f, 10.0f)) bNeedsPreviewRender = true;
                if (ImGui::SliderFloat("Min Slope", &params.Decals[i].MinSlope, 0.0f, 90.0f)) bNeedsPreviewRender = true;
                if (ImGui::SliderFloat("Max Slope", &params.Decals[i].MaxSlope, 0.0f, 90.0f)) bNeedsPreviewRender = true;
                if (ImGui::SliderFloat("Min Height", &params.Decals[i].MinHeight, 0.0f, 128.0f)) bNeedsPreviewRender = true;
                if (ImGui::SliderFloat("Max Height", &params.Decals[i].MaxHeight, 0.0f, 128.0f)) bNeedsPreviewRender = true;
                
                if (ImGui::Button("Delete Rule", ImVec2(-1, 20))) {
                    params.Decals.erase(params.Decals.begin() + i);
                    bNeedsPreviewRender = true;
                    ImGui::PopID();
                    break;
                }
            }
            ImGui::PopID();
        }
        
        // Reclaim density moved to markers tab
    }

} // namespace UI
} // namespace SanmapGen
