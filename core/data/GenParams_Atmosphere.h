#pragma once
#include <string>
#include <vector>

namespace SanmapGen {

    struct AtmosphereSettings {
            // --- Lighting ---
            float SunRA = 0.0f;
            float SunDA = 0.0f;
            float SunIntensity = 15000.0f;
            float SunTint[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
            float SunTemperature = 6300.0f;
            float SunAngularDiameter = 0.5f;
            float SunVolumetricsMultiplier = 6.7f;
            float SunVolumetricsShadowDimer = 0.5f;
            float SunPosition[3] = { 512.0f, 10.0f, 256.0f };
            
            float SkylightIntensity = 0.0f;
            float SkylightTint[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
            float SkylightTemperature = 9000.0f;
            
            float Exposure = 12.0f;
            float ExposureCompensation = 2.5f;
            
            std::string SkyboxPath = "";
            float SkyboxRotation = 0.0f;
            float SkyboxExposure = 12.0f;
            float SkyboxMultiplier = 1.0f;
            float SkyboxLuxValue = 10000.0f;
            
            // --- Background Fog ---
            float BackgroundFogIntensity = 1.0f;
            float BackgroundFogRange = 1024.0f;
            float BackgroundFogMinimum = 0.1f;
            float BackgroundSkyColorIntensity = 1.0f;
            float BackgroundColorIntensity = 0.0f;
            float BackgroundColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            float BackgroundColorFadeoutRange = 150000.0f;
            float BackgroundColorFadeoutPower = 0.3f;
            
            // --- Height Fog ---
            float HeightFogIntensity = 1.0f;
            float HeightFogRange[2] = { -10.0f, 100.0f };
            float HeightFogStart = -10.0f;
            float HeightFogEnd = 500.0f;
            float HeightFogPower = 6.0f;
            
            // --- Linear Fog ---
            float LinearFogIntensity = 0.24f;
            float LinearFogStart = 100.0f;
            float LinearFogEnd = 5000.0f;
            float LinearFogPower = 1.0f;
            float LinearFogCameraIntensity = 0.0f;
            float LinearFogCameraStart = 500.0f;
            float LinearFogCameraEnd = 5000.0f;
            
            // --- Legacy Fog (Kept for backwards compatibility if needed) ---
            float FogAttenuationDistance = 200.0f;
            float FogBaseHeight = 15.0f;
            float FogMaximumHeight = 100.0f;
            float FogMaximumDistance = 1500.0f;
            float FogAnisotropy = 0.5f;
            
            // --- Global Wind ---
            float GlobalWindSpeed = 0.25f;
            float GlobalWindDirection = 160.0f;
        };

}
