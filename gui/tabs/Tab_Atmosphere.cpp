#include "../UITabs.h"
#include "../UIHelpers.h"
#include "imgui.h"

namespace SanmapGen {
namespace UI {

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
            if (ImGui::DragFloat3("Sun Position", params.Atmosphere.SunPosition)) bNeedsMapUpdate = true;
            
            char sunBuf[256]; strncpy(sunBuf, params.Atmosphere.SunCookiePath.c_str(), sizeof(sunBuf));
            if (ImGui::InputText("Sun Cookie Path", sunBuf, IM_ARRAYSIZE(sunBuf))) { params.Atmosphere.SunCookiePath = sunBuf; bNeedsMapUpdate = true; }
            if (ImGui::DragFloat2("Sun Cookie Size", params.Atmosphere.SunCookieSize)) bNeedsMapUpdate = true;
        }
        
        if (ImGui::CollapsingHeader("Skylight", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::DragFloat("Sky Intensity", &params.Atmosphere.SkylightIntensity, 100.0f, 0.0f, 100000.0f)) bNeedsPreviewRender = true;
            if (ImGui::ColorEdit4("Sky Tint", params.Atmosphere.SkylightTint)) bNeedsPreviewRender = true;
            if (ImGui::SliderFloat("Sky Temp", &params.Atmosphere.SkylightTemperature, 1000.0f, 10000.0f)) bNeedsPreviewRender = true;
        }
        
        if (ImGui::CollapsingHeader("Exposure & Skybox", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::SliderFloat("Exposure", &params.Atmosphere.Exposure, 0.0f, 20.0f)) bNeedsPreviewRender = true;
            if (ImGui::SliderFloat("Exp Comp", &params.Atmosphere.ExposureCompensation, -5.0f, 5.0f)) bNeedsPreviewRender = true;
            
            char skyBuf[256]; strncpy(skyBuf, params.Atmosphere.SkyboxPath.c_str(), sizeof(skyBuf));
            if (ImGui::InputText("Skybox Path", skyBuf, IM_ARRAYSIZE(skyBuf))) { params.Atmosphere.SkyboxPath = skyBuf; bNeedsMapUpdate = true; }
            if (ImGui::SliderFloat("Skybox Rotation", &params.Atmosphere.SkyboxRotation, 0.0f, 360.0f)) bNeedsMapUpdate = true;
            
            int mode = static_cast<int>(params.Atmosphere.SkyboxIntensityMode);
            const char* modes[] = { "Exposure", "Lux", "Multiplier" };
            if (ImGui::Combo("Intensity Mode", &mode, modes, IM_ARRAYSIZE(modes))) {
                params.Atmosphere.SkyboxIntensityMode = static_cast<SkyIntensityMode>(mode);
                bNeedsMapUpdate = true;
            }
            if (ImGui::SliderFloat("Skybox Exp", &params.Atmosphere.SkyboxExposure, 0.0f, 20.0f)) bNeedsPreviewRender = true;
            if (ImGui::SliderFloat("Skybox Mult", &params.Atmosphere.SkyboxMultiplier, 0.0f, 100.0f)) bNeedsMapUpdate = true;
            if (ImGui::DragFloat("Skybox Lux", &params.Atmosphere.SkyboxLuxValue, 10.0f, 0.0f, 100000.0f)) bNeedsMapUpdate = true;
        }
        
        if (ImGui::CollapsingHeader("Legacy Fog", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::SliderFloat("Fog Atten Dist", &params.Atmosphere.FogAttenuationDistance, 0.0f, 10.0f)) bNeedsMapUpdate = true;
            if (ImGui::SliderFloat("Fog Base H", &params.Atmosphere.FogBaseHeight, -100.0f, 500.0f)) bNeedsMapUpdate = true;
            if (ImGui::SliderFloat("Fog Max H", &params.Atmosphere.FogMaximumHeight, 0.0f, 1000.0f)) bNeedsMapUpdate = true;
            if (ImGui::SliderFloat("Fog Max Dist", &params.Atmosphere.FogMaximumDistance, 0.0f, 10000.0f)) bNeedsMapUpdate = true;
            if (ImGui::SliderFloat("Fog Anisotropy", &params.Atmosphere.FogAnisotropy, 0.0f, 1.0f)) bNeedsMapUpdate = true;
        }

        if (ImGui::CollapsingHeader("Background Fog", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::SliderFloat("Bg Fog Intensity", &params.Atmosphere.BackgroundFogIntensity, 0.0f, 10.0f)) bNeedsMapUpdate = true;
            if (ImGui::SliderFloat("Bg Fog Range", &params.Atmosphere.BackgroundFogRange, 0.0f, 10000.0f)) bNeedsMapUpdate = true;
            if (ImGui::SliderFloat("Bg Fog Min", &params.Atmosphere.BackgroundFogMinimum, 0.0f, 1.0f)) bNeedsMapUpdate = true;
            if (ImGui::SliderFloat("Bg Sky Intensity", &params.Atmosphere.BackgroundSkyColorIntensity, 0.0f, 10.0f)) bNeedsMapUpdate = true;
            if (ImGui::ColorEdit4("Bg Color", params.Atmosphere.BackgroundColor)) bNeedsMapUpdate = true;
            if (ImGui::SliderFloat("Bg Color Intensity", &params.Atmosphere.BackgroundColorIntensity, 0.0f, 10.0f)) bNeedsMapUpdate = true;
            if (ImGui::DragFloat("Bg Fadeout Range", &params.Atmosphere.BackgroundColorFadeoutRange, 100.0f, 0.0f, 500000.0f)) bNeedsMapUpdate = true;
            if (ImGui::SliderFloat("Bg Fadeout Power", &params.Atmosphere.BackgroundColorFadeoutPower, 0.0f, 10.0f)) bNeedsMapUpdate = true;
        }

        if (ImGui::CollapsingHeader("Height Fog", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::SliderFloat("Height Fog Intensity", &params.Atmosphere.HeightFogIntensity, 0.0f, 10.0f)) bNeedsMapUpdate = true;
            if (ImGui::DragFloat2("Height Fog Range", params.Atmosphere.HeightFogRange)) bNeedsMapUpdate = true;
            if (ImGui::DragFloat("Height Fog Start", &params.Atmosphere.HeightFogStart)) bNeedsMapUpdate = true;
            if (ImGui::DragFloat("Height Fog End", &params.Atmosphere.HeightFogEnd)) bNeedsMapUpdate = true;
            if (ImGui::SliderFloat("Height Fog Power", &params.Atmosphere.HeightFogPower, 0.01f, 10.0f)) bNeedsMapUpdate = true;
        }

        if (ImGui::CollapsingHeader("Linear Fog", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::SliderFloat("Linear Fog Intensity", &params.Atmosphere.LinearFogIntensity, 0.0f, 10.0f)) bNeedsMapUpdate = true;
            if (ImGui::DragFloat("Linear Fog Start", &params.Atmosphere.LinearFogStart)) bNeedsMapUpdate = true;
            if (ImGui::DragFloat("Linear Fog End", &params.Atmosphere.LinearFogEnd)) bNeedsMapUpdate = true;
            if (ImGui::SliderFloat("Linear Fog Power", &params.Atmosphere.LinearFogPower, 0.01f, 10.0f)) bNeedsMapUpdate = true;
            if (ImGui::SliderFloat("Linear Cam Intensity", &params.Atmosphere.LinearFogCameraIntensity, 0.0f, 1.0f)) bNeedsMapUpdate = true;
            if (ImGui::DragFloat("Linear Cam Start", &params.Atmosphere.LinearFogCameraStart)) bNeedsMapUpdate = true;
            if (ImGui::DragFloat("Linear Cam End", &params.Atmosphere.LinearFogCameraEnd)) bNeedsMapUpdate = true;
        }
        
        if (ImGui::CollapsingHeader("Wind (Global)", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::SliderFloat("Wind Speed", &params.Atmosphere.GlobalWindSpeed, 0.0f, 10.0f)) bNeedsMapUpdate = true;
            if (ImGui::SliderFloat("Wind Direction", &params.Atmosphere.GlobalWindDirection, 0.0f, 360.0f)) bNeedsMapUpdate = true;
        }
    }


} // namespace UI
} // namespace SanmapGen
