#pragma once
#include "params/Params_Enums.h"
#include "params/Params_Environment.h"
#include "params/Params_Geometry.h"
#include "params/Params_ErosionFlow.h"
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <functional>
#include <cstdint>

namespace SanmapGen {


    // Bitmask for Symmetry Mixing








    



    








    


    
    


    struct GenerationParams {
        int PresetVersion = 1; // Used for backwards compatibility
        std::string GlobalEnvironmentPath = ""; // Path to the .sanpack or folder
        std::string MapFolderPath = ""; // Path to the currently loaded map folder
        
        std::string PendingSplat14Path = ""; // Used to trigger fallback UI popups
        std::string PendingSplat58Path = "";
        std::string PendingHeightmapPath = "";
        
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
        float MarkerScaleAlloy = 0.17f;
        float MarkerScalePlasma = 0.17f;
        float MarkerScaleSpawn = 0.17f;
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
        float HydroMultiplier = 1.0f;
        float ReclaimDensity = 1.0f;
        float MexDensity = 1.0f;
        
        // --- Tab Data ---
        WaterSettings Water;
        AtmosphereSettings Atmosphere;
        
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
        bool UseGPUFlowMap = false;
        bool UseGPUMarkers = true;
        
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
            PreviewLayers.push_back({ PreviewLayerType::Smoothness, "Smoothness", false, LayerBlendMode::None });
            PreviewLayers.push_back({ PreviewLayerType::Slope, "Slope Map", true, LayerBlendMode::None });
            PreviewLayers.push_back({ PreviewLayerType::Flow, "Flow Map", true, LayerBlendMode::None });
            PreviewLayers.push_back({ PreviewLayerType::Accumulation, "Accumulation Map", true, LayerBlendMode::None });
            PreviewLayers.push_back({ PreviewLayerType::Markers, "Markers", true, LayerBlendMode::None });
            PreviewLayers.push_back({ PreviewLayerType::Props, "Props", false, LayerBlendMode::None });
            PreviewLayers.push_back({ PreviewLayerType::Water, "Water", true, LayerBlendMode::None });
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



