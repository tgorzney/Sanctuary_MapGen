#pragma once
#include <string>
#include <vector>

namespace SanmapGen {

    // Bitmask for Symmetry Mixing
    enum SymmetryFlags {
        Symmetry_None   = 0,
        Symmetry_Point  = 1 << 0,
        Symmetry_X      = 1 << 1,
        Symmetry_Z      = 1 << 2,
        Symmetry_XY     = 1 << 3,
        Symmetry_Radial = 1 << 4
    };

    enum class BlendMode {
        Add,
        Subtract,
        Multiply,
        Overlay,
        Max,
        Min
    };

    enum class NoiseType {
        OpenSimplex2,
        OpenSimplex2S,
        Cellular,
        Perlin,
        ValueCubic,
        Value
    };

    enum class FractalType {
        None,
        FBm,
        Ridged,
        PingPong
    };

    enum class StratumType {
        Bedrock, // Bottom-most layer, high hardness, zero drainage
        Sand,    // Low cohesion, high drainage
        Silt,    // Medium cohesion, medium drainage
        Clay,    // High cohesion, low drainage
        Loam,    // Balanced soil
        Snow     // Melts into water
    };

    struct GlobalErosionSettings {
        bool Enabled = true;
        bool UseGPU = true;
        int DropletCount = 1000000;
        int MaxLifetime = 15;
        float Gravity = 4.0f;
        float EvaporationRate = 0.02f;

        // Precipitation
        bool UseRainNoise = true;
        float RainNoiseFreq = 0.01f;
        int RainNoiseOctaves = 4;
        float RainNoiseThreshold = 0.5f;

        // Orographic
        bool UseOrographicRain = true;
        float WindAngle = 45.0f; // degrees
    };

    struct NoiseLayer {
        std::string Name = "New Layer";
        bool Enabled = true;
        
        // Image / Freeze support
        bool UseImage = false;
        std::string ImagePath = "";
        std::vector<float> ImageData; // Cached heightmap (normalized 0.0 to 1.0)
        int ImageWidth = 0;
        int ImageHeight = 0;
        
        bool Erodable = true;
        
        StratumType Stratum = StratumType::Sand; // Stratum type dictates physics
        BlendMode Blend = BlendMode::Add; // Used for pre-masking thickness

        NoiseType Type = NoiseType::OpenSimplex2;
        FractalType Fractal = FractalType::FBm;
        int SymmetryMask = Symmetry_Point;
        
        // Soil Physics Settings
        float Hardness = 0.2f; // Sand defaults
        float Friction = 0.8f;
        float Cohesion = 0.5f;
        float CapacityMult = 2.0f;
        
        float Frequency = 0.005f;
        int Octaves = 5;
        float Gain = 0.5f;
        float PingPongStrength = 2.0f;
        float Opacity = 1.0f; // Multiplier/Weight for this layer
        float CellularJitter = 1.0f;
        
        // Terrain Density Shaping
        float LandDensity = 0.514f;
        float PlateauDensity = 0.0f;
        float MountainDensity = 0.243f;
        float RampDensity = 0.500f;
    };

    enum class SymmetryAlgorithm {
        Fold,
        Blur,
        CrossFade,
        Cylinder3D,
        Torus3D,
        NativeHash,
        Superposition
    };

    struct WaterSettings {
        float WaterLevelMin = 20.0f;
        float WaterLevelMax = 40.0f;
        float DeepWaterDepthMin = 10.0f;
        float DeepWaterDepthMax = 30.0f;
        std::string WaveGeneratorBlueprint = "";
    };

    struct StratumSettings {
        float BaseColor[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
        std::string AlbedoPath = "";
        std::string NormalPath = "";
        std::string CompositePath = "";
    };

    struct MarkerRule {
        std::string Name = "New Marker";
        bool Enabled = true;
        
        // Filtering thresholds
        float MinSlope = 0.0f;
        float MaxSlope = 90.0f;
        float MinHeight = 0.0f;
        float MaxHeight = 128.0f;
        
        // Density/Spawning
        float Density = 1.0f;
    };

    struct GenerationParams {
        // --- General ---
        bool UseGPUTerrain = false;
        int Seed = 12345;
        int MapSize = 512;
        
        // --- Symmetry Globals ---
        SymmetryAlgorithm SymAlgorithm = SymmetryAlgorithm::Superposition;
        BlendMode SymSuperpositionBlend = BlendMode::Max;
        float SymmetryBlurRadius = 10.0f;
        float CrossFadeWidth = 0.2f; // Radians to crossfade
        float CylinderZScale = 1.0f; // Stretch the cylinder length
        float TorusMajorRadius = 128.0f; // Donut ring size
        float TorusMinorRadius = 64.0f; // Donut tube size
        
        // --- The Dynamic Layer Stack ---
        // Geology
        std::vector<NoiseLayer> Layers;
        GlobalErosionSettings Erosion;
        
        // --- Gameplay ---
        int SpawnPointCount = 2;
        float AlloyMultiplier = 1.0f;
        float HydroMultiplier = 1.0f;
        
        // --- New Tabs Data ---
        WaterSettings Water;
        std::vector<StratumSettings> Stratums;
        std::vector<MarkerRule> Markers;
        
        // Tab Visibility Flags (for minimap composite)
        bool ShowHeightmap = true;
        bool ShowStratums = false;
        bool ShowDetailNormal = false;
        bool ShowTint = false;
        bool ShowHoles = false;
        bool ShowSmoothness = false;
        bool ShowWater = false;
        bool ShowMarkers = false;
        bool ShowReclaim = false;
        bool ShowProps = false;
        bool ShowDecals = false;
        
        // Default constructor to push one base layer
        GenerationParams() {
            NoiseLayer baseLayer;
            baseLayer.Name = "Base Mountain (Low Freq)";
            baseLayer.Frequency = 0.005f;
            baseLayer.Octaves = 5;
            baseLayer.Gain = 0.5f;
            baseLayer.PingPongStrength = 2.0f;
            baseLayer.Opacity = 1.0f;
            
            baseLayer.LandDensity = 0.514f;
            baseLayer.PlateauDensity = 0.0f;
            baseLayer.MountainDensity = 0.243f;
            baseLayer.RampDensity = 0.500f;
            
            Layers.push_back(baseLayer);

            // Initialize 9 blank stratums to match the Sanctuary format
            for (int i = 0; i < 9; ++i) {
                Stratums.push_back(StratumSettings());
            }
        }
    };

} // namespace SanmapGen
