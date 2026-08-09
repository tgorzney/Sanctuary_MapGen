#pragma once
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <functional>
#include <cstdint>

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

        // --- Scientific Flow Variables (TG_UE Architecture) ---
        float FluidViscosity = 1.0f;
        float BaseAbsorptionRate = 0.05f;
        float CarryingCapacityScale = 1.0f;
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
        std::string Name = "New Setting";
        std::vector<GradientStop> Stops;
        bool SmoothInterpolation = true;
    };
    
    struct FlowSettings {
        float Precipitation = 1.0f;
        int Iterations = 50;
        bool UseGPU = false; // Toggle CPU vs GPU
        
        // --- God-Tier Stochastic Flow Variables ---
        float FlowVolumeMultiplier = 1.0f;
        float StochasticVariance = 0.5f;
        float SlopeAdherence = 0.8f;
        float FlowMomentum = 0.2f;
        
        // --- Accumulation Variables ---
        bool AccurateSimultaneousAccumulation = false;
        float SpilloverThreshold = 0.01f;
        
        GradientSettings Gradient = {
            "Default", 
            {
                {0.0f, {0.0f, 0.0f, 0.2f, 1.0f}}, 
                {50.0f, {0.0f, 0.4f, 0.8f, 1.0f}}, 
                {100.0f, {0.0f, 1.0f, 1.0f, 1.0f}}
            },
            true
        };
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
    
    enum class ImportedMaskMode {
        Disabled,
        ProceduralStart,
        StaticOverride
    };

    struct GlobalTexturingSettings {
        std::string Shader = "RTS/TerrainLit";
        float HeightTransition = 0.5f;
        float FadeDistance = 128.0f;
        float FadeStartDistance = 1.0f;
    };

    struct SanTextureLoader { std::string path = ""; };
    struct SanNormalTextureLoader { std::string path = ""; };
    struct SanMaskTextureLoader { std::string path = ""; };
    struct SanVector2 { float x = 0.0f; float y = 0.0f; float& operator[](int i) { return (i==0) ? x : y; } const float& operator[](int i) const { return (i==0) ? x : y; } };
    struct SanVector4 { float x = 0.0f; float y = 0.0f; float z = 0.0f; float w = 0.0f; float& operator[](int i) { return (i==0) ? x : (i==1) ? y : (i==2) ? z : w; } const float& operator[](int i) const { return (i==0) ? x : (i==1) ? y : (i==2) ? z : w; } };
    struct SanColor { float r = 1.0f; float g = 1.0f; float b = 1.0f; float a = 1.0f; float& operator[](int i) { return (i==0) ? r : (i==1) ? g : (i==2) ? b : a; } const float& operator[](int i) const { return (i==0) ? r : (i==1) ? g : (i==2) ? b : a; } };

    struct StratumSettings {
        std::string name = "Stratum";
        
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

    enum MarkerPriority {
        Priority_LargestArea = 0,
        Priority_SmallestArea = 1,
        Priority_LeastVariance = 2
    };
    
    enum MarkerGradientType {
        Gradient_None = 0,
        Gradient_CenterFocus = 1,
        Gradient_EdgeFocus = 2,
        Gradient_Torus = 3
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
        std::string AlbedoPath = "";
        std::string NormalPath = "";
        
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
    
        enum class LayerType {
        Manual,
        Fixed
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

    struct GenerationParams {
        int PresetVersion = 1; // Used for backwards compatibility
        std::string GlobalEnvironmentPath = ""; // Path to the .sanpack or folder
        std::string MapFolderPath = ""; // Path to the currently loaded map folder
        
        int ShowFocusGradientDebugRuleIndex = -1; // -1 means off, otherwise the index of the MarkerRule being adjusted
        
        std::string DebugInfo = "";
        std::string IconScanDebugInfo = "";
        
        // --- General ---
        int Seed = 12345;
        int MapSize = 512;
        float TerrainMinHeight = 0.0f;
        float TerrainMaxHeight = 128.0f;
        bool ScaleFeaturesToMapSize = true;
        float GlobalGravity = 9.81f; // Global gravity used by erosion unless overridden
        
        // --- Markers & Gamedata ---
        std::string GamedataPath = ""; // Path to the Sanctuary Gamedata folder containing UI.zip / UI.sanpack
        float MarkerScaleAlloy = 1.0f;
        float MarkerScalePlasma = 1.0f;
        float MarkerScaleSpawn = 1.0f;
        float MarkerColorAlloy[4] = {0.8f, 0.8f, 0.2f, 1.0f};
        float MarkerColorPlasma[4] = {0.2f, 0.8f, 0.8f, 1.0f};
        float MarkerColorSpawn[4] = {0.8f, 0.2f, 0.2f, 1.0f};
        std::string GlobalIconAlloy = "Alloy";
        std::string GlobalIconPlasma = "Plasma";
        std::string GlobalIconSpawn = "Spawn";
        
        // Placed Markers (Key is map name e.g. "Alloys_037")
        std::map<std::string, MarkerTransform> MarkersList;
        std::vector<std::string> KnownMarkerTypes = {"Alloy", "Plasma", "Spawn", "Alloys", "Plasmas", "Spawns"};
        
        // --- Armies ---
        std::map<std::string, Army> Armies;
        
        // --- DOP Optimizations ---
        // Props (Trees, Rocks) are segregated from interactive markers to prevent UI loops from processing 100,000+ items.
        // We use a flat Struct-of-Arrays (SoA) style flat buffer for maximum cache coherence.
        struct PropInstance {
            float X, Y, Z;
            uint32_t TintColor;
        };
        std::vector<PropInstance> StaticPropsList;
        
        // Spatial Partitioning Grid for O(1) click detection on interactive markers
        struct MarkerChunk {
            std::vector<std::string> MarkerKeys;
        };
        std::vector<MarkerChunk> MarkerSpatialGrid;
        int SpatialGridResolution = 32; // 32x32 chunks
        
        std::vector<std::string> AvailableIcons; // Populated from .sanpack
        std::map<std::string, unsigned int> IconCache; // name -> GLuint texture ID
        
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
        
        float SymmetryDetectionTolerance = 1.0f;
        bool SnapImperfectSymmetry = false;
        
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
        GlobalTexturingSettings TexturingGlobals;
        
        std::vector<StratumSettings> Stratums;
        std::vector<ProceduralMarkerLayer> ProceduralMarkerLayers = { {"Procedural Markers", true, {}} };
        std::vector<PlacedMarkerLayer> PlacedMarkerLayers;
        int SelectedPlacedLayerIndex = -1;
        
        std::vector<const MarkerRule*> GetFlatMarkerRules() const {
            std::vector<const MarkerRule*> flat;
            for (const auto& l : ProceduralMarkerLayers) {
                if (!l.Enabled) continue;
                for (const auto& r : l.Rules) {
                    flat.push_back(&r);
                }
            }
            return flat;
        }
        
        std::vector<MarkerRule*> GetFlatMarkerRulesMutable() {
            std::vector<MarkerRule*> flat;
            for (auto& l : ProceduralMarkerLayers) {
                if (!l.Enabled) continue;
                for (auto& r : l.Rules) {
                    flat.push_back(&r);
                }
            }
            return flat;
        }
        
        bool EnableProceduralMarkers = false;
        std::vector<PropRule> Props;
        std::vector<DecalRule> Decals;
        
        // --- Performance & Accuracy (TG_UE Execution Tiers) ---
        bool UseGPUTerrain = false;
        bool UseGPUHydraulic = true;
        bool UseGPUDeposition = true;
        bool UseGPUFlowMap = false;
        
        bool WYSIWYGBaking = false; // Bypasses High-Accuracy CPU baking to bake the 1:1 Lossy GPU Preview
        int GPUPreviewIterations = 20; // Allows real-time optimization dialing
        
        bool FastPreviewMode = false; // If true, defers Flow/Placement calculations to maintain high FPS while dragging sliders
        
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
        bool ShowSymmetry = false;
        
        bool AutoLevelPreview = true;
        
        bool ShowSlopeMap = true;
        bool ShowFlowMap = true;
        bool ShowAccumulationMap = true;
        bool ShowStratums = true;
        bool ShowWater = true;
        bool ShowMarkers = true;
        bool ShowArmies = false;
        
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
            baseLayer.ImageContrast = 1.0f;
            baseLayer.ImageBrightness = 0.0f;
            
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
                s.name = "Stratum " + std::to_string(i);
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
        
        // --- Dependency Graph Hashing for Dirty Flags ---
        size_t GetBlendHash() const {
            size_t hash = 0;
            auto combine = [&hash](auto val) {
                std::hash<decltype(val)> hasher;
                hash ^= hasher(val) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            };
            
            auto flat = GetFlatLayers();
            combine(flat.size());
            for (const auto* layer : flat) {
                combine(layer->Enabled);
                combine(layer->Opacity);
                combine(layer->Blend);
                combine(layer->ImageContrast);
                combine(layer->ImageBrightness);
                combine(layer->LandDensity);
                combine(layer->MountainDensity);
                combine(layer->PlateauDensity);
                combine(layer->RampDensity);
                combine(layer->HeightBlendContrast);
                combine(layer->HeightBlendMin);
                combine(layer->HeightBlendMax);
                combine(layer->LevelsShadows);
                combine(layer->LevelsHighlights);
                combine(layer->LevelsMidtones);
                combine(layer->LevelsOutputBlack);
                combine(layer->LevelsOutputWhite);
                combine(layer->StratumIndex);
                combine(layer->GetNoiseHash(Seed, GlobalSymmetryMask, (int)SymAlgorithm)); // Re-blend if noise changes
            }
            
            for (const auto& stratum : Stratums) {
                combine((int)stratum.maskMode);
            }
            return hash;
        }
        
        size_t GetErosionHash(size_t blendHash) const {
            size_t hash = blendHash; // Erosion strictly depends on the blended heightmap
            auto combine = [&hash](auto val) {
                std::hash<decltype(val)> hasher;
                hash ^= hasher(val) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            };
            
            auto flat = GetFlatLayers();
            for (const auto* layer : flat) {
                if (!layer->Enabled || !layer->Erosion.Enabled) continue;
                combine(layer->Erosion.DropletCount);
                combine(layer->Erosion.MaxLifetime);
                combine(layer->Erosion.GravityUseGlobal);
                combine(layer->Erosion.Gravity);
                combine(layer->Erosion.EvaporationRate);
                combine(layer->Erosion.UseRainNoise);
                combine(layer->Erosion.RainNoiseFreq);
                combine(layer->Erosion.RainNoiseOctaves);
                combine(layer->Erosion.RainNoiseThreshold);
                combine(layer->Erosion.UseOrographicRain);
                combine(layer->Erosion.WindAngle);
                combine(layer->Erosion.DepositionMode);
                combine(layer->Erosion.SpawnMinHeight);
                combine(layer->Erosion.SpawnMaxHeight);
                combine(layer->Erosion.InitialSedimentLoad);
            }
            return hash;
        }
        size_t GetFlowHash(size_t erosionHash) const {
            size_t hash = erosionHash; // Flow depends on eroded heightmap
            auto combine = [&hash](auto val) {
                std::hash<decltype(val)> hasher;
                hash ^= hasher(val) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            };
            combine(FlowSettingsParams.Precipitation);
            combine(FlowSettingsParams.Iterations);
            combine(FlowSettingsParams.FlowVolumeMultiplier);
            combine(FlowSettingsParams.StochasticVariance);
            combine(FlowSettingsParams.SlopeAdherence);
            combine(FlowSettingsParams.FlowMomentum);
            return hash;
        }

        size_t GetPlacementHash(size_t flowHash) const {
            size_t hash = flowHash; // Placements depend on flow (if they use flow masks later, etc)
            auto combine = [&hash](auto val) {
                std::hash<decltype(val)> hasher;
                hash ^= hasher(val) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            };
            
            for (const auto* rule_ptr : GetFlatMarkerRules()) {
                const auto& rule = *rule_ptr;
                combine(rule.Enabled);
                combine(rule.Count);
                combine(rule.Density);
                combine(rule.Priority);
                combine(rule.MinSlope);
                combine(rule.MaxSlope);
                combine(rule.MinHeight);
                combine(rule.MaxHeight);
                combine(rule.ClearanceSpacing);
                combine(rule.MapEdgePadding);
            }
            
            for (const auto& kvp : MarkersList) {
                combine(kvp.first); // hash explicit markers
                combine(kvp.second.Position[0]);
                combine(kvp.second.Position[1]);
                combine(kvp.second.Position[2]);
            }
            
            return hash;
        }
        
        size_t GetHash() const {
            return GetPlacementHash(GetFlowHash(GetErosionHash(GetBlendHash())));
        }
    };

} // namespace SanmapGen



