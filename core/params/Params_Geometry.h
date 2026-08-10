#pragma once
#include "Params_Enums.h"
#include "Params_Gradients.h"
#include "Params_ErosionFlow.h"
#include <string>
#include <vector>
#include <map>

#include <string>
#include <vector>

namespace SanmapGen {
    struct Point2D {
        int x, y;
    };


    struct SlopeSettings {
        bool bUseEngineParityMath = false;
        GradientSettings Gradient = {
            "Slope", 
            {
                {0.0f, {0.2f, 0.6f, 0.2f, 1.0f}},   // Green (Flat)
                {5.0f, {1.0f, 1.0f, 0.0f, 1.0f}},   // Yellow (Moderate)
                {30.0f, {1.0f, 0.0f, 0.0f, 1.0f}}   // Red (Steep)
            },
            true
        };
    };

    struct NoiseLayer {
        std::string Name = "New Layer";
        bool Enabled = true;
        
        // Image / Freeze support (Baking)
        bool UseImage = false;
        std::string ImagePath = "";
        std::string OriginPresetPath = ""; // To track original procedural settings
        std::vector<float> ImageData; // Cached heightmap (normalized 0.0 to 1.0)
        int ImageWidth = 0;
        int ImageHeight = 0;
        
        bool Erodable = true;
        
        int StratumIndex = 1; // 0 to 8 mapping to the 9 Stratums (also selects GeoLayer group)
        BlendMode Blend = BlendMode::Add;

        // Height Blend (controls how deep this layer sits on top of the one below)
        float HeightBlendContrast = 1.0f;
        float HeightBlendMin = 0.0f;
        float HeightBlendMax = 1.0f;

        NoiseType Type = NoiseType::OpenSimplex2;
        FractalType Fractal = FractalType::FBm;
        
        // Per-layer symmetry (defaults to global)
        bool SymmetryUseGlobal = true;
        int SymmetryMask = Symmetry_Point;
        
        float Frequency = 0.005f;
        int Octaves = 5;
        float Gain = 0.5f;
        float PingPongStrength = 2.0f;
        float Opacity = 1.0f;
        float ImageContrast = 1.0f;
        float ImageBrightness = 0.0f;
        float CellularJitter = 1.0f;
        
        // Terrain Density Shaping
        float LandDensity = 0.514f;
        float PlateauDensity = 0.0f;
        float MountainDensity = 0.243f;
        float RampDensity = 0.500f;

        // Levels Adjustment (Photoshop-style)
        float LevelsShadows = 0.0f;       // 0 to 255 (or 0.0 to 1.0)
        float LevelsMidtones = 1.0f;      // 0.01 to 9.99
        float LevelsHighlights = 1.0f;    // 0 to 255 (or 0.0 to 1.0)
        float LevelsOutputBlack = 0.0f;   // 0.0 to 1.0
        float LevelsOutputWhite = 1.0f;   // 0.0 to 1.0

        // Layer-specific Erosion
        ErosionSettings Erosion;
        bool ErodeBeneath = false; // If true, droplets can dig into layers underneath
        
        // Hash for caching raw structural noise (excludes Photoshop Levels and Density Shaping)
        size_t GetNoiseHash(int globalSeed, int globalSymmetryMask, int symAlg) const {
            size_t hash = 0;
            auto combine = [&hash](auto val) {
                std::hash<decltype(val)> hasher;
                hash ^= hasher(val) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            };
            
            combine(globalSeed);
            combine(Type);
            combine(Fractal);
            combine(SymmetryUseGlobal ? globalSymmetryMask : SymmetryMask);
            combine(symAlg);
            combine(Frequency);
            combine(Octaves);
            combine(Gain);
            combine(PingPongStrength);
            combine(CellularJitter);
            combine(UseImage);
            combine(ImagePath);
            
            return hash;
        }
    };

    struct StratumSettings {
        std::string name = "Stratum";
        std::string EnvironmentTheme = "";
        std::string MaterialName = "";
        
        SanTextureLoader albedo;
        SanNormalTextureLoader normal;
        SanMaskTextureLoader mask;
        
        SanVector2 tileSize = { 10.0f, 10.0f };
        SanVector2 tileSizeFar = { 64.0f, 64.0f };
        float tileSizeTriplanar = 12.0f;
        float tileSizeFarTriplanar = 36.0f;
        
        float normalScale = 1.0f;
        float normalScaleFar = 1.0f;
        float normalFarNearBlend = 0.5f;
        float heightFarNearBlend = 0.5f;
        
        SanColor diffuseRemap = { 1.0f, 1.0f, 1.0f, 1.0f };
        SanColor farColorRemap = { 0.0f, 0.0f, 0.0f, 0.0f };
        
        SanColor previewColor = { 1.0f, 1.0f, 1.0f, 1.0f }; // Debug tint for Map Preview only
        
        SanVector4 maskRemapMin = { 0.0f, 0.0f, 0.0f, 0.0f };
        SanVector4 maskRemapMax = { 1.0f, 1.0f, 1.0f, 1.0f };
        
        unsigned int previewAlbedoTex = 0;
        unsigned int previewNormalTex = 0;
        unsigned int previewMaskTex = 0;
        unsigned int previewActualMaskTex = 0; // UI Thumbnail for the extracted uncompressed mask
        
        ImportedMaskMode maskMode = ImportedMaskMode::Disabled;
        std::vector<float> importedMaskData;
        
        // Default Soil Physics for this Stratum
        float hardness = 0.2f;
        float friction = 0.8f;
        float cohesion = 0.5f;
        float capacityMult = 2.0f;
        float absorptionRate = 0.05f; // Determines how fast water sinks into this layer
    };

    struct MarkerRule {
        std::string Name = "New Marker";
        bool Enabled = true;
        std::string Type = "Alloy";
        std::string IconOverride = ""; // Leave empty to use Type default
        float Color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        
        // Filtering thresholds
        float MinSlope = 0.0f;
        float MaxSlope = 90.0f;
        float MinHeight = 0.0f;
        float MaxHeight = 128.0f;
        
        // Advanced Deterministic Placement
        bool RandomSelection = false;
        int Priority = Priority_LeastVariance;
        
        float AreaHeightRange = 0.5f; // Tolerance for height variance
        float AreaRadiusMin = 5.0f;
        bool CheckMaxRadius = false;
        float AreaRadiusMax = 50.0f;
        
        float ClearanceSpacing = 10.0f; // Minimum distance to other markers
        float MapEdgePadding = 0.0f;    // Distance from the edge of the map
        
        int FocusGradient = Gradient_None;
        float FocusGradientRadius = 250.0f;
        float FocusGradientStrength = 1.0f;
        float FocusGradientContrast = 1.0f;
        
        bool UseDensity = true;
        bool UseAllPositions = false;
        float Density = 0.5f;
        int Count = 10;
        
        bool SymmetryUseGlobal = true;
        int SymmetryMask = Symmetry_Point;
    };

    struct PropRule {
        std::string Name = "New Prop Layer";
        bool Enabled = true;
        std::string BlueprintPath = "";
        
        float Density = 0.5f;
        float MinSlope = 0.0f;
        float MaxSlope = 45.0f;
        float MinHeight = 0.0f;
        float MaxHeight = 128.0f;
        
        bool AvoidWater = true;
        bool NearCliffs = false;
    };

    struct DecalRule {
        std::string Name = "New Decal Layer";
        bool Enabled = true;
        std::string BlueprintPath = "";
        
        float Density = 0.1f;
        float MinSlope = 0.0f;
        float MaxSlope = 30.0f;
        float MinHeight = 0.0f;
        float MaxHeight = 128.0f;
    };

    struct ProceduralMarkerLayer {
        std::string Name = "New Layer";
        bool Enabled = true;
        bool Locked = false;
        std::vector<MarkerRule> Rules;
    };

    struct PlacedMarkerLayer {
        std::string Name = "Imported";
        LayerType Type = LayerType::Manual;
        bool Enabled = true;
        bool Locked = false;
        std::vector<std::string> MarkerKeys; // Keys for MarkersList
    };

    struct GeoLayerDef {
        bool Enabled = true;
        std::string Name = "New GeoLayer";
        std::vector<NoiseLayer> Layers;
    };


}
