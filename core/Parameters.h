#pragma once
#include <string>
#include <vector>
#include <algorithm>

namespace SanmapGen {

    struct Point2D {
        int x, y;
    };

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
        Value,
        None
    };

    enum class FractalType {
        None,
        FBm,
        Ridged,
        PingPong
    };

    struct ErosionSettings {
        bool Enabled = false;
        
        int DropletCount = 1000000;
        int MaxLifetime = 15;
        bool GravityUseGlobal = true; // If true, use GenerationParams::GlobalGravity
        float Gravity = 4.0f;         // Per-erosion gravity override
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

        // Soil Physics (single source — also editable on Stratums tab)
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

    struct GradientStop {
        float Location = 0.0f; // 0.0 to 100.0 (or mapped to degrees 0-90)
        float Color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        
        bool operator<(const GradientStop& other) const {
            return Location < other.Location;
        }
    };

    struct GradientSettings {
        bool SmoothInterpolation = true;
        std::vector<GradientStop> Stops;
    };
    
    struct FlowSettings {
        float Precipitation = 1.0f;
        int Iterations = 50;
        bool UseGPU = false; // Toggle CPU vs GPU
        GradientSettings Gradient = {
            true, 
            {
                {0.0f, {0.0f, 0.0f, 0.2f, 1.0f}}, 
                {50.0f, {0.0f, 0.4f, 0.8f, 1.0f}}, 
                {100.0f, {0.0f, 1.0f, 1.0f, 1.0f}}
            }
        };
    };

    struct SlopeSettings {
        GradientSettings Gradient = {
            true, 
            {
                {0.0f, {0.2f, 0.6f, 0.2f, 1.0f}},   // Green (Flat)
                {30.0f, {0.6f, 0.6f, 0.2f, 1.0f}},  // Yellow (Moderate)
                {45.0f, {0.6f, 0.4f, 0.2f, 1.0f}},  // Brown (Steep)
                {90.0f, {0.4f, 0.4f, 0.4f, 1.0f}}   // Gray (Cliff)
            }
        };
    };

    struct WaterSettings {
        float WaterLevelMin = 20.0f;
        float WaterLevelMax = 40.0f;
        float DeepWaterDepthMin = 10.0f;
        float DeepWaterDepthMax = 30.0f;
        float WaterWindSpeed = 0.06f;
        float WaterWindDirection = 160.0f;
        float WaterShoreDepthOffset = 8.0f;
        float WaterShoreDepthStrength = 0.7f;
        float WaterShoreDistanceOffset = 0.0f;
        float WaterShoreDistanceStrength = 2.0f;
        std::string WaveGeneratorBlueprint = "";
    };

    struct AtmosphereSettings {
        float SunRA = 87.0f;
        float SunDA = 42.0f;
        float SunIntensity = 45000.0f;
        float SunTint[4] = { 1.0f, 0.938f, 0.9f, 1.0f };
        float SunTemperature = 5600.0f;
        float SunAngularDiameter = 0.5f;
        float SunVolumetricsMultiplier = 6.7f;
        float SunVolumetricsShadowDimer = 0.5f;
        
        float SkylightIntensity = 4000.0f;
        float SkylightTint[4] = { 0.921f, 0.925f, 0.98f, 1.0f };
        float SkylightTemperature = 5600.0f;
        
        float Exposure = 12.08f;
        float ExposureCompensation = 0.2f;
        float SkyboxExposure = 11.73f;
        
        float FogAttenuationDistance = 2.0f;
        float FogBaseHeight = -20.0f;
        float FogMaximumHeight = 110.0f;
        float FogMaximumDistance = 1500.0f;
        float FogAnisotropy = 1.0f;
        
        std::string SkyboxPath = "empty";
        
        float GlobalWindSpeed = 0.0f;
        float GlobalWindDirection = 0.0f;
    };

    struct StratumSettings {
        std::string Name = "Stratum";
        
        std::string AlbedoPath = "";
        std::string NormalPath = "";
        std::string MaskPath = "";
        
        float TileSize[2] = { 10.0f, 10.0f };
        float TileSizeFar[2] = { 64.0f, 64.0f };
        float TileSizeTriplanar = 12.0f;
        float TileSizeFarTriplanar = 36.0f;
        
        float NormalScale = 1.0f;
        float NormalScaleFar = 1.0f;
        float NormalFarNearBlend = 0.5f;
        float HeightFarNearBlend = 0.5f;
        
        float DiffuseRemap[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        float FarColorRemap[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        
        float PreviewColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f }; // Debug tint for Map Preview only
        
        float MaskRemapMin[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        float MaskRemapMax[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        
        unsigned int PreviewAlbedoTex = 0;
        unsigned int PreviewNormalTex = 0;
        unsigned int PreviewMaskTex = 0;
        unsigned int PreviewActualMaskTex = 0; // UI Thumbnail for the extracted uncompressed mask
        
        bool UseImportedMask = false;
        std::vector<float> ImportedMaskData;
        
        // Default Soil Physics for this Stratum
        float Hardness = 0.2f;
        float Friction = 0.8f;
        float Cohesion = 0.5f;
        float CapacityMult = 2.0f;
    };

    struct MarkerRule {
        std::string Name = "New Marker";
        bool Enabled = true;
        std::string IconPath = "";
        
        // Filtering thresholds
        float MinSlope = 0.0f;
        float MaxSlope = 90.0f;
        float MinHeight = 0.0f;
        float MaxHeight = 128.0f;
        
        // Density/Spawning
        float Density = 1.0f;
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
        std::string AlbedoPath = "";
        std::string NormalPath = "";
        
        float Density = 0.1f;
        float MinSlope = 0.0f;
        float MaxSlope = 30.0f;
        float MinHeight = 0.0f;
        float MaxHeight = 128.0f;
    };

    struct GeoLayerDef {
        bool Enabled = true;
        std::string Name = "New GeoLayer";
        std::vector<NoiseLayer> Layers;
    };

    struct GenerationParams {
        int PresetVersion = 1; // Used for backwards compatibility
        std::string GlobalEnvironmentPath = ""; // Path to the .sanpack or folder
        
        // --- General ---
        int Seed = 12345;
        int MapSize = 512;
        bool ScaleFeaturesToMapSize = true;
        float GlobalGravity = 9.81f; // Global gravity used by erosion unless overridden
        
        float FlowMapColor[4] = { 0.0f, 0.5f, 1.0f, 1.0f };

        
        // --- Symmetry Globals ---
        int GlobalSymmetryMask = Symmetry_Point; // Global default symmetry flags
        SymmetryAlgorithm SymAlgorithm = SymmetryAlgorithm::Superposition;
        BlendMode SymSuperpositionBlend = BlendMode::Max;
        float SymmetryBlurRadius = 10.0f;
        float CrossFadeWidth = 0.2f;
        float CylinderZScale = 1.0f;
        float TorusMajorRadius = 128.0f;
        float TorusMinorRadius = 64.0f;
        
        // --- The Dynamic Layer Stacks ---
        std::vector<GeoLayerDef> GeoLayers; // Main heightmap generation
        // Helper to get a flat list of all layers across all GeoLayers in calculation order
        std::vector<const NoiseLayer*> GetFlatLayers() const {
            std::vector<const NoiseLayer*> flat;
            for (const auto& gl : GeoLayers) {
                if (!gl.Enabled) continue;
                for (const auto& l : gl.Layers) {
                    flat.push_back(&l);
                }
            }
            return flat;
        }
        std::vector<NoiseLayer*> GetFlatLayersMutable() {
            std::vector<NoiseLayer*> flat;
            for (auto& gl : GeoLayers) {
                if (!gl.Enabled) continue;
                for (auto& l : gl.Layers) {
                    flat.push_back(&l);
                }
            }
            return flat;
        }

        
        // --- Visual / Post-Process Mask Layers ---
        std::vector<NoiseLayer> DetailNormalLayers;
        std::vector<NoiseLayer> SmoothnessLayers;
        std::vector<NoiseLayer> TintLayers;
        std::vector<NoiseLayer> HoleLayers;
        
        int DetailNormalMapSize = 1024;
        
        // --- Gameplay ---
        int SpawnPointCount = 2;
        float AlloyMultiplier = 1.0f;
        float HydroMultiplier = 1.0f;
        float ReclaimDensity = 1.0f;
        float MexDensity = 1.0f;
        
        // --- Tab Data ---
        WaterSettings Water;
        AtmosphereSettings Atmosphere;
        std::vector<StratumSettings> Stratums;
        std::vector<MarkerRule> Markers;
        std::vector<PropRule> Props;
        std::vector<DecalRule> Decals;
        
        // --- Performance ---
        bool UseGPUTerrain = false;
        bool UseGPUHydraulic = true;
        bool UseGPUDeposition = true;
        
        // --- Preview Layers (Z-Order Compositing) ---
        enum class LayerBlendMode {
            None, Normal, Add, Subtract, Multiply, Divide, Overlay, Screen, SoftLight, HardLight
        };

        enum class PreviewLayerType {
            Heightmap,
            DetailNormal,
            Holes,
            Stratums,
            Tint,
            Water,
            Smoothness,
            Slope,
            Flow,
            Accumulation,
            Markers,
            Props
        };

        struct PreviewLayer {
            PreviewLayerType Type;
            std::string Name;
            bool Enabled;
            LayerBlendMode Blend = LayerBlendMode::None;
        };
        
        std::vector<PreviewLayer> PreviewLayers;
        
        // Preview settings (not saved to file, UI only)
        
        bool ShowHeightmap = true;
        bool ShowDetailNormal = false;
        bool ShowTint = false;
        bool ShowHoles = false;
        bool ShowSmoothness = false;
        bool ShowReclaim = false;
        bool ShowProps = false;
        bool ShowAtmosphere = false;
        
        bool AutoLevelPreview = true;
        
        bool ShowSlopeMap = true;
        bool ShowFlowMap = true;
        bool ShowAccumulationMap = true;
        bool ShowStratums = true;
        bool ShowWater = true;
        bool ShowMarkers = true;
        
        SlopeSettings SlopeSettingsParams;
        FlowSettings FlowSettingsParams;

        // --- Generated Output Data ---
        std::vector<Point2D> GeneratedSpawns;
        std::vector<Point2D> GeneratedMexes;
        std::vector<Point2D> GeneratedHydros;
        std::vector<Point2D> GeneratedTrees;
        
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
            
            GeoLayerDef baseGeoLayer;
            baseGeoLayer.Name = "GeoLayer 0";
            baseGeoLayer.Layers.push_back(baseLayer);
            GeoLayers.push_back(baseGeoLayer);

            // Initialize 9 blank stratums to match the Sanctuary format
            for (int i = 0; i < 9; ++i) {
                StratumSettings s;
                s.Name = "Stratum " + std::to_string(i);
                Stratums.push_back(s);
            }

            // Initialize default preview layers in bottom-to-top Z-order
            PreviewLayers.push_back({ PreviewLayerType::Heightmap, "Heightmap", true, LayerBlendMode::Normal });
            PreviewLayers.push_back({ PreviewLayerType::DetailNormal, "Detail Normal", false, LayerBlendMode::None });
            PreviewLayers.push_back({ PreviewLayerType::Holes, "Holes", false, LayerBlendMode::None });
            PreviewLayers.push_back({ PreviewLayerType::Stratums, "Stratum Colors", true, LayerBlendMode::None });
            PreviewLayers.push_back({ PreviewLayerType::Tint, "Tint", false, LayerBlendMode::None });
            PreviewLayers.push_back({ PreviewLayerType::Water, "Water", true, LayerBlendMode::None });
            PreviewLayers.push_back({ PreviewLayerType::Smoothness, "Smoothness", false, LayerBlendMode::None });
            PreviewLayers.push_back({ PreviewLayerType::Slope, "Slope Map", true, LayerBlendMode::None });
            PreviewLayers.push_back({ PreviewLayerType::Flow, "Flow Map", true, LayerBlendMode::None });
            PreviewLayers.push_back({ PreviewLayerType::Accumulation, "Accumulation Map", true, LayerBlendMode::None });
            PreviewLayers.push_back({ PreviewLayerType::Markers, "Markers", true, LayerBlendMode::None });
            PreviewLayers.push_back({ PreviewLayerType::Props, "Props", false, LayerBlendMode::None });
        }
    };

} // namespace SanmapGen
