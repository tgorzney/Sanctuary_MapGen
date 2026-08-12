#pragma once
#include "Params_Enums.h"
#include "Params_Gradients.h"
#include <string>
#include <vector>
#include <map>

namespace SanmapGen {
    struct WaterSettings {
        float WaterLevelMin = 0.0f;
        float WaterLevelMax = 0.0f;
        float DeepWaterDepthMin = 8.0f;
        float DeepWaterDepthMax = 8.0f;
        float WaterWindSpeed = 0.25f;
        float WaterWindDirection = 160.0f;
        float WaterWindShoreWavesRemap = 0.5f;
        float WaterShoreDepthOffset = 8.0f;
        float WaterShoreDepthStrength = 0.7f;
        float WaterShoreDistanceOffset = 0.0f;
        float WaterShoreDistanceStrength = 2.0f;
        std::string WaveGeneratorBlueprint = "";
        
        GradientSettings Gradient = {
            "Water", 
            {
                {0.0f, {0.05f, 0.1f, 0.3f, 0.85f}},    // Deep
                {1.0f, {0.2f, 0.6f, 0.8f, 0.5f}}       // Shallow
            },
            true
        };
    };

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
        std::string SunCookiePath = "";
        float SunCookieSize[2] = { 1024.0f, 1024.0f };
        
        float SkylightIntensity = 0.0f;
        float SkylightTint[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        float SkylightTemperature = 9000.0f;
        
        float Exposure = 12.0f;
        float ExposureCompensation = 2.5f;
        
        std::string SkyboxPath = "";
        float SkyboxRotation = 0.0f;
        SkyIntensityMode SkyboxIntensityMode = SkyIntensityMode::Exposure;
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


    struct SanTextureLoader { std::string path = ""; };
    struct SanNormalTextureLoader { std::string path = ""; };

    struct SanMaskTextureLoader { std::string path = ""; };
    struct SanVector2 { float x = 0.0f; float y = 0.0f; float& operator[](int i) { return (i==0) ? x : y; } const float& operator[](int i) const { return (i==0) ? x : y; } };

    struct SanVector4 { float x = 0.0f; float y = 0.0f; float z = 0.0f; float w = 0.0f; float& operator[](int i) { return (i==0) ? x : (i==1) ? y : (i==2) ? z : w; } const float& operator[](int i) const { return (i==0) ? x : (i==1) ? y : (i==2) ? z : w; } };
    struct SanColor { float r = 1.0f; float g = 1.0f; float b = 1.0f; float a = 1.0f; float& operator[](int i) { return (i==0) ? r : (i==1) ? g : (i==2) ? b : a; } const float& operator[](int i) const { return (i==0) ? r : (i==1) ? g : (i==2) ? b : a; } };

    struct UnitDefinition {
        std::string Type; // e.g., "uca1001"
        std::string Name = "";
        std::string DisplayName = "";
        float FootprintX = 1.0f;
        float FootprintY = 1.0f;
        float Speed = 10.0f;
        float Acceleration = 10.0f;
    };

    struct UnitTransform {
        std::string Type;
        std::string Tpid = "";
        float Position[3] = {0.0f, 0.0f, 0.0f};
        float Rotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        float Scale[3] = {1.0f, 1.0f, 1.0f};
    };

    struct UnitGroup {
        std::map<std::string, UnitTransform> Units;
        std::map<std::string, UnitGroup> Groups;
    };

    struct Army {
        int Faction = 0;
        float Alloys = 100.0f;
        float Energy = 1000.0f;
        float Color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        std::map<std::string, UnitGroup> Groups;
    };

    struct MarkerTransform {
        float Position[3] = {0.0f, 0.0f, 0.0f};
        float Rotation[4] = {0.0f, 0.0f, 0.0f, 1.0f}; // x,y,z,w quaternion
        float Scale[3] = {1.0f, 1.0f, 1.0f};
        float Color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        int SymmetryId = 0; // The ID of the symmetry group if it's a mirrored clone
        
        bool SymmetryUseGlobal = true;
        int SymmetryMask = Symmetry_Point;
        
        std::string GeneratorAlias = ""; // Internal Map Generator custom name alias
        
        // Keep track of what type this is (Spawn, Alloy, Plasma, etc.)
        std::string Type;
        // Allows customizing the JSON key for Spawn (e.g., "ARMY_1") or Alloys (e.g. "Mex 0")
        std::string CustomName = ""; 
        std::string IconOverride = ""; // Used to override the global visual icon for this specific marker
        
        // Differentiates procedurally generated markers from manual ones
        bool IsManual = false;
        // Used to highlight symmetrically forced invalid placements
        bool IsValid = true;
        // If the generating rule is disabled, keep it hidden but generate for clearance
        bool IsHidden = false;
    };


}
