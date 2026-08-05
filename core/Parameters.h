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

    struct ErosionSettings {
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

        // Deposition (Soil Dropping) Pass
        bool DepositionMode = false;
        float SpawnMinHeight = 0.0f;
        float SpawnMaxHeight = 1.0f;
        float InitialSedimentLoad = 1.0f;
    };

    struct NoiseLayer {
        std::string Name = "New Layer";
        bool Enabled = true;
        
        // Image / Freeze support
        bool UseImage = false;
        std::string ImagePath = "";
        std::string OriginPresetPath = ""; // To track original procedural settings
        std::vector<float> ImageData; // Cached heightmap (normalized 0.0 to 1.0)
        int ImageWidth = 0;
        int ImageHeight = 0;
        
        bool Erodable = true;
        
        int StratumIndex = 1; // 0 to 8 mapping to the 9 Stratums
        BlendMode Blend = BlendMode::Add; // Used for pre-masking thickness

        NoiseType Type = NoiseType::OpenSimplex2;
        FractalType Fractal = FractalType::FBm;
        int SymmetryMask = Symmetry_Point;
        
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

        // Soil Physics
        float Hardness = 0.2f; 
        float Friction = 0.8f;
        float Cohesion = 0.5f;
        float CapacityMult = 2.0f;

        // Layer-specific Erosion
        ErosionSettings Erosion;
        bool ErodeBeneath = false; // If true, droplets can dig into layers underneath
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
        std::string Name = "Stratum";
        std::string EnvironmentTheme = ""; // e.g. "01_Highlands"
        std::string MaterialName = ""; // e.g. "highlands_100m_grass01"
        
        float BaseColor[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
        std::string AlbedoPath = "";
        std::string NormalPath = "";
        std::string CompositePath = "";
        
        float MaskRemapMax[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        float MaskRemapMin[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        float Tint[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        
        float NearTiling[2] = { 8.0f, 8.0f };
        float NearNormalScale = 1.0f;
        
        float FarTiling[2] = { 32.0f, 32.0f };
        float FarNormalScale = 0.7f;
        float TintBlend = 1.0f;
        float NormalNearBlend = 0.28f;
        float HeightNearBlend = 0.50f;
        float ColorOverride[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

        float HeightBlendContrast = 1.50f;
        float HeightBlendDepth = 0.50f;
        bool UseDarkerAreaFill = false;
        
        float FadeBegin = 1.0f;
        float FadeDistance = 250.0f;
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
        int PresetVersion = 1; // Used for backwards compatibility
        std::string GlobalEnvironmentPath = ""; // Path to the .sanpack or folder
        
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
                StratumSettings s;
                s.Name = "Stratum " + std::to_string(i);
                Stratums.push_back(s);
            }
        }
    };

} // namespace SanmapGen
