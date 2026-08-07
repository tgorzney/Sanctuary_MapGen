#include "UITabs.h"
#include "imgui.h"
#include "FileDialog.h"
#include <GLFW/glfw3.h>

extern GLuint GetMarkerIcon(const std::string& typeName, SanmapGen::GenerationParams& params, void* openZipArchive = nullptr);
extern void ForceScanIcons(SanmapGen::GenerationParams& params);

namespace SanmapGen {
namespace UI {

    void RenderWaterTab(GenerationParams& params, bool& bNeedsMapUpdate, bool& bNeedsPreviewRender) {
        ImGui::Text("Water & Waves");
        ImGui::Separator();
        
        if (ImGui::CollapsingHeader("Water Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            float vLevels[2] = { params.Water.WaterLevelMin, params.Water.WaterLevelMax };
            if (ImGui::DragFloat2("Water Level", vLevels, 0.1f, 0.0f, 128.0f)) {
                vLevels[0] = std::min(vLevels[0], vLevels[1] - 0.1f);
                vLevels[1] = std::max(vLevels[1], vLevels[0] + 0.1f);
                vLevels[0] = std::max(0.0f, vLevels[0]);
                vLevels[1] = std::min(128.0f, vLevels[1]);
                params.Water.WaterLevelMin = vLevels[0];
                params.Water.WaterLevelMax = vLevels[1];
                bNeedsPreviewRender = true;
            }
            
            float vDeep[2] = { params.Water.DeepWaterDepthMin, params.Water.DeepWaterDepthMax };
            if (ImGui::DragFloat2("Deep Water", vDeep, 0.1f, 0.0f, 128.0f)) {
                vDeep[0] = std::min(vDeep[0], vDeep[1] - 0.1f);
                vDeep[1] = std::max(vDeep[1], vDeep[0] + 0.1f);
                vDeep[0] = std::max(0.0f, vDeep[0]);
                vDeep[1] = std::min(128.0f, vDeep[1]);
                params.Water.DeepWaterDepthMin = vDeep[0];
                params.Water.DeepWaterDepthMax = vDeep[1];
                bNeedsPreviewRender = true;
            }
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

    void RenderIconPicker(const char* labelId, std::string& targetString, SanmapGen::GenerationParams& params, bool& bNeedsUpdate) {
        GLuint currentTex = ::GetMarkerIcon(targetString, params);
        if (currentTex == 0 && !params.IconCache.empty()) currentTex = params.IconCache.begin()->second; // fallback for display
        
        ImGui::PushID(labelId);
        if (ImGui::ImageButton(labelId, (void*)(intptr_t)currentTex, ImVec2(20, 20))) {
            ImGui::OpenPopup("IconPickerPopup");
        }
        if (ImGui::BeginPopup("IconPickerPopup")) {
            int itemsPerRow = 5;
            int col = 0;
            
            ImGui::BeginChild("IconScroll", ImVec2(250, 300), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
            
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 0.5f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.6f, 0.6f, 0.5f));
            
            for (const auto& iconName : params.AvailableIcons) {
                ImGui::PushID(iconName.c_str());
                GLuint tex = ::GetMarkerIcon(iconName, params);
                if (tex != 0) {
                    if (ImGui::ImageButton(iconName.c_str(), (void*)(intptr_t)tex, ImVec2(32, 32))) {
                        targetString = iconName;
                        bNeedsUpdate = true;
                        ImGui::CloseCurrentPopup();
                    }
                } else {
                    if (ImGui::Button(iconName.c_str(), ImVec2(32, 32))) {
                        targetString = iconName;
                        bNeedsUpdate = true;
                        ImGui::CloseCurrentPopup();
                    }
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", iconName.c_str());
                
                if (++col < itemsPerRow) ImGui::SameLine();
                else col = 0;
                ImGui::PopID();
            }
            
            ImGui::PopStyleColor(3);
            ImGui::EndChild();
            ImGui::EndPopup();
        }
        ImGui::PopID();
    }

    void RenderMarkersTab(GenerationParams& params, std::string& selectedMarkerKey, bool& bNeedsMapUpdate, bool& bNeedsPreviewRender) {
        params.ShowFocusGradientDebugRuleIndex = -1; // Reset each frame
        
        ImGui::Text("Marker Gamedata");
        ImGui::Separator();

        if (ImGui::Button("Browse Gamedata...")) {
            std::string outPath;
            if (FileDialog::SelectFolder(outPath)) {
                params.GamedataPath = outPath;
                bNeedsPreviewRender = true;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Scan for Icons")) {
            ::ForceScanIcons(params);
            bNeedsPreviewRender = true;
        }
        
        if (!params.IconScanDebugInfo.empty()) {
            if (ImGui::CollapsingHeader("Scanner Debug Log")) {
                ImGui::BeginChild("ScannerDebugChild", ImVec2(0, 200), true);
                ImGui::TextUnformatted(params.IconScanDebugInfo.c_str());
                ImGui::EndChild();
            }
        }
        
        if (!params.DebugInfo.empty()) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Icon Debug Info:");
            ImGui::TextWrapped("%s", params.DebugInfo.c_str());
            ImGui::Spacing();
        }
        
        ImGui::Spacing();
        ImGui::Text("Marker Scale");
        ImGui::Separator();
        
        ImGui::PushItemWidth(100);
        RenderIconPicker("GlobalAlloy", params.GlobalIconAlloy, params, bNeedsPreviewRender);
        ImGui::SameLine();
        if (ImGui::SliderFloat("Alloy", &params.MarkerScaleAlloy, 0.1f, 10.0f)) bNeedsPreviewRender = true;
        ImGui::SameLine();
        if (ImGui::ColorEdit4("##ColorAlloy", params.MarkerColorAlloy, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) bNeedsPreviewRender = true;
        
        ImGui::SameLine();
        RenderIconPicker("GlobalPlasma", params.GlobalIconPlasma, params, bNeedsPreviewRender);
        ImGui::SameLine();
        if (ImGui::SliderFloat("Plasma", &params.MarkerScalePlasma, 0.1f, 10.0f)) bNeedsPreviewRender = true;
        ImGui::SameLine();
        if (ImGui::ColorEdit4("##ColorPlasma", params.MarkerColorPlasma, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) bNeedsPreviewRender = true;
        
        ImGui::SameLine();
        RenderIconPicker("GlobalSpawn", params.GlobalIconSpawn, params, bNeedsPreviewRender);
        ImGui::SameLine();
        if (ImGui::SliderFloat("Spawn", &params.MarkerScaleSpawn, 0.1f, 10.0f)) bNeedsPreviewRender = true;
        ImGui::SameLine();
        if (ImGui::ColorEdit4("##ColorSpawn", params.MarkerColorSpawn, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) bNeedsPreviewRender = true;
        ImGui::PopItemWidth();

        ImGui::Spacing();
        ImGui::Text("Placed Markers");
        ImGui::Separator();
        
        if (ImGui::Button("Add Marker", ImVec2(-1, 30))) {
            SanmapGen::MarkerTransform newMarker;
            newMarker.Type = "Spawn";
            newMarker.CustomName = "New_Marker_" + std::to_string(params.MarkersList.size());
            params.MarkersList[newMarker.CustomName] = newMarker;
            bNeedsMapUpdate = true;
            bNeedsPreviewRender = true;
        }
        ImGui::Spacing();
        
        std::string keyToDelete = "";
        
        std::vector<std::map<std::string, SanmapGen::MarkerTransform>::iterator> markerIters;
        for (auto it = params.MarkersList.begin(); it != params.MarkersList.end(); ++it) {
            markerIters.push_back(it);
        }
        
        ImGuiListClipper clipper;
        clipper.Begin((int)markerIters.size());
        while (clipper.Step()) {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                auto& kvp = *markerIters[i];
                auto& key = kvp.first;
                auto& marker = kvp.second;
                
                ImGui::PushID(key.c_str());
                char label[128];
                snprintf(label, sizeof(label), "%s (%s)", marker.CustomName.empty() ? key.c_str() : marker.CustomName.c_str(), marker.Type.c_str());
                
                if (ImGui::CollapsingHeader(label)) {
                    char nameBuf[128]; 
                    strncpy(nameBuf, marker.CustomName.c_str(), sizeof(nameBuf));
                    if (ImGui::InputText("Name/ID", nameBuf, sizeof(nameBuf))) {
                        marker.CustomName = nameBuf;
                        bNeedsMapUpdate = true;
                    }
                    
                    if (ImGui::DragFloat3("Position (X,Y,Z)", marker.Position, 1.0f, 0.0f, 4096.0f)) bNeedsMapUpdate = true;
                    if (ImGui::ColorEdit4("Color Override", marker.Color)) bNeedsMapUpdate = true;
                    
                    std::string iconToDisplay = marker.IconOverride.empty() ? marker.Type : marker.IconOverride;
                    RenderIconPicker("MarkerIcon", iconToDisplay, params, bNeedsPreviewRender);
                    if (iconToDisplay != marker.Type) marker.IconOverride = iconToDisplay;
                    else marker.IconOverride = "";
                    
                    // Select Marker Type using params.KnownMarkerTypes
                    if (!params.KnownMarkerTypes.empty()) {
                        int currentType = -1;
                        for (int j = 0; j < (int)params.KnownMarkerTypes.size(); ++j) {
                            if (params.KnownMarkerTypes[j] == marker.Type) {
                                currentType = j;
                                break;
                            }
                        }
                        std::vector<const char*> types;
                        for (const auto& t : params.KnownMarkerTypes) types.push_back(t.c_str());
                        
                        if (ImGui::Combo("Type", &currentType, types.data(), (int)types.size())) {
                            if (currentType >= 0 && currentType < (int)params.KnownMarkerTypes.size()) {
                                marker.Type = params.KnownMarkerTypes[currentType];
                                bNeedsMapUpdate = true;
                            }
                        }
                    }
                    
                    if (ImGui::Button("Delete Marker", ImVec2(-1, 20))) {
                        keyToDelete = key;
                    }
                }
                ImGui::PopID();
            }
        }
        clipper.End();
        
        if (!keyToDelete.empty()) {
            if (selectedMarkerKey == keyToDelete) selectedMarkerKey = "";
            params.MarkersList.erase(keyToDelete);
            bNeedsMapUpdate = true;
            bNeedsPreviewRender = true;
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
                
                if (ImGui::DragFloat2("Slope Range", &params.Props[i].MinSlope, 0.01f, 0.0f, 90.0f, "%.2f")) {
                    params.Props[i].MinSlope = std::min(params.Props[i].MinSlope, params.Props[i].MaxSlope - 0.01f);
                    params.Props[i].MaxSlope = std::max(params.Props[i].MaxSlope, params.Props[i].MinSlope + 0.01f);
                    params.Props[i].MinSlope = std::max(0.0f, params.Props[i].MinSlope);
                    params.Props[i].MaxSlope = std::min(90.0f, params.Props[i].MaxSlope);
                    bNeedsPreviewRender = true;
                }
                
                if (ImGui::DragFloat2("Height Range", &params.Props[i].MinHeight, 0.1f, 0.0f, 128.0f, "%.1f")) {
                    params.Props[i].MinHeight = std::min(params.Props[i].MinHeight, params.Props[i].MaxHeight - 0.1f);
                    params.Props[i].MaxHeight = std::max(params.Props[i].MaxHeight, params.Props[i].MinHeight + 0.1f);
                    params.Props[i].MinHeight = std::max(0.0f, params.Props[i].MinHeight);
                    params.Props[i].MaxHeight = std::min(128.0f, params.Props[i].MaxHeight);
                    bNeedsPreviewRender = true;
                }
                
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
            SanmapGen::DecalRule rule;
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
                
                if (ImGui::DragFloat2("Slope Range", &params.Decals[i].MinSlope, 0.01f, 0.0f, 90.0f, "%.2f")) {
                    params.Decals[i].MinSlope = std::min(params.Decals[i].MinSlope, params.Decals[i].MaxSlope - 0.01f);
                    params.Decals[i].MaxSlope = std::max(params.Decals[i].MaxSlope, params.Decals[i].MinSlope + 0.01f);
                    params.Decals[i].MinSlope = std::max(0.0f, params.Decals[i].MinSlope);
                    params.Decals[i].MaxSlope = std::min(90.0f, params.Decals[i].MaxSlope);
                    bNeedsPreviewRender = true;
                }
                
                if (ImGui::DragFloat2("Height Range", &params.Decals[i].MinHeight, 0.1f, 0.0f, 128.0f, "%.1f")) {
                    params.Decals[i].MinHeight = std::min(params.Decals[i].MinHeight, params.Decals[i].MaxHeight - 0.1f);
                    params.Decals[i].MaxHeight = std::max(params.Decals[i].MaxHeight, params.Decals[i].MinHeight + 0.1f);
                    params.Decals[i].MinHeight = std::max(0.0f, params.Decals[i].MinHeight);
                    params.Decals[i].MaxHeight = std::min(128.0f, params.Decals[i].MaxHeight);
                    bNeedsPreviewRender = true;
                }
                
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
