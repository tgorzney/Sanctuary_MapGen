#include "MapExporter.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <filesystem>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace SanmapGen {

void MapExporter::ExportSanmap(const std::string& folderPath, const GenerationParams& params) {
    if (!fs::exists(folderPath)) {
        fs::create_directories(folderPath);
    }
    
    // Construct the sanmap JSON matching SanMap.cs
    json mapdef;
    mapdef["fileVersion"] = 3;
    mapdef["mapVersion"] = 1;
    mapdef["name"] = "Generated Map";
    mapdef["credits"] = "Sanctuary Map Generator";
    mapdef["width"] = params.MapSize;
    mapdef["length"] = params.MapSize;
    mapdef["height"] = 128; // Standard height
    
    mapdef["hasWater"] = true; // Maybe dynamically check based on water level
    mapdef["waterLevel"] = params.Water.WaterLevelMax; // or something
    mapdef["waterDepth"] = params.Water.DeepWaterDepthMax;
    mapdef["waterShoreGeneratorBlueprint"] = params.Water.WaveGeneratorBlueprint;

    mapdef["shader"] = "RTS/TerrainLit";

    // Stratum Layers
    json strata = json::array();
    for (const auto& stratum : params.Stratums) {
        json s;
        // Output base color and textures as part of the JSON, matching SanMap.cs properties if possible
        s["baseColor"] = { 
            {"r", stratum.BaseColor[0]},
            {"g", stratum.BaseColor[1]},
            {"b", stratum.BaseColor[2]},
            {"a", stratum.BaseColor[3]}
        };
        s["albedo"] = stratum.AlbedoPath;
        s["normal"] = stratum.NormalPath;
        s["composite"] = stratum.CompositePath;
        strata.push_back(s);
    }
    mapdef["stratumLayers"] = strata;

    // We can populate default values for other properties (lighting, fog, background) to match SanMap.cs
    mapdef["backgroundFogIntensity"] = 1.0f;
    mapdef["skyboxIntensityMode"] = "Exposure";
    
    // Areas, armies, chains
    mapdef["areas"] = json::object();
    mapdef["armies"] = json::object();
    
    json markersObj = json::object();
    // Populate markers based on MarkerRules
    for (const auto& rule : params.Markers) {
        if (!rule.Enabled) continue;
        json markerType;
        markerType["name"] = rule.Name;
        markerType["minSlope"] = rule.MinSlope;
        markerType["maxSlope"] = rule.MaxSlope;
        markerType["minHeight"] = rule.MinHeight;
        markerType["maxHeight"] = rule.MaxHeight;
        markerType["density"] = rule.Density;
        markersObj[rule.Name] = markerType;
    }
    mapdef["markers"] = markersObj;

    mapdef["decals"] = json::array();
    mapdef["props"] = json::array();

    // Export JSON
    std::string filePath = folderPath + "/mapdef.sanmap";
    std::ofstream out(filePath);
    out << mapdef.dump(4);
    out.close();

    // TODO: Write textures to folderPath + "/Textures"
}

void MapExporter::SaveSettings(const std::string& filePath, const GenerationParams& params) {
    json j;
    j["PresetVersion"] = params.PresetVersion;
    j["GlobalEnvironmentPath"] = params.GlobalEnvironmentPath;
    
    j["UseGPUTerrain"] = params.UseGPUTerrain;
    j["Seed"] = params.Seed;
    j["MapSize"] = params.MapSize;
    
    j["SymAlgorithm"] = static_cast<int>(params.SymAlgorithm);
    j["SpawnPointCount"] = params.SpawnPointCount;
    
    // Save Stratums
    json stratums = json::array();
    for (const auto& s : params.Stratums) {
        json sj;
        sj["Name"] = s.Name;
        sj["EnvironmentTheme"] = s.EnvironmentTheme;
        sj["MaterialName"] = s.MaterialName;
        sj["BaseColor"] = {s.BaseColor[0], s.BaseColor[1], s.BaseColor[2], s.BaseColor[3]};
        sj["AlbedoPath"] = s.AlbedoPath;
        sj["NormalPath"] = s.NormalPath;
        sj["CompositePath"] = s.CompositePath;
        
        sj["MaskRemapMax"] = {s.MaskRemapMax[0], s.MaskRemapMax[1], s.MaskRemapMax[2], s.MaskRemapMax[3]};
        sj["MaskRemapMin"] = {s.MaskRemapMin[0], s.MaskRemapMin[1], s.MaskRemapMin[2], s.MaskRemapMin[3]};
        sj["Tint"] = {s.Tint[0], s.Tint[1], s.Tint[2], s.Tint[3]};
        sj["NearTiling"] = {s.NearTiling[0], s.NearTiling[1]};
        sj["NearNormalScale"] = s.NearNormalScale;
        sj["FarTiling"] = {s.FarTiling[0], s.FarTiling[1]};
        sj["FarNormalScale"] = s.FarNormalScale;
        sj["TintBlend"] = s.TintBlend;
        sj["NormalNearBlend"] = s.NormalNearBlend;
        sj["HeightNearBlend"] = s.HeightNearBlend;
        sj["ColorOverride"] = {s.ColorOverride[0], s.ColorOverride[1], s.ColorOverride[2], s.ColorOverride[3]};
        sj["HeightBlendContrast"] = s.HeightBlendContrast;
        sj["HeightBlendDepth"] = s.HeightBlendDepth;
        sj["UseDarkerAreaFill"] = s.UseDarkerAreaFill;
        sj["FadeBegin"] = s.FadeBegin;
        sj["FadeDistance"] = s.FadeDistance;
        stratums.push_back(sj);
    }
    j["Stratums"] = stratums;

    // Save Layers
    json layers = json::array();
    for (const auto& layer : params.Layers) {
        json l;
        l["Name"] = layer.Name;
        l["Enabled"] = layer.Enabled;
        l["UseImage"] = layer.UseImage;
        l["ImagePath"] = layer.ImagePath;
        l["OriginPresetPath"] = layer.OriginPresetPath;
        l["Erodable"] = layer.Erodable;
        l["StratumIndex"] = layer.StratumIndex;
        l["Blend"] = static_cast<int>(layer.Blend);
        l["Type"] = static_cast<int>(layer.Type);
        l["Fractal"] = static_cast<int>(layer.Fractal);
        l["SymmetryMask"] = layer.SymmetryMask;
        
        l["Frequency"] = layer.Frequency;
        l["Octaves"] = layer.Octaves;
        l["Gain"] = layer.Gain;
        l["PingPongStrength"] = layer.PingPongStrength;
        l["Opacity"] = layer.Opacity;
        l["CellularJitter"] = layer.CellularJitter;
        
        l["LandDensity"] = layer.LandDensity;
        l["PlateauDensity"] = layer.PlateauDensity;
        l["MountainDensity"] = layer.MountainDensity;
        l["RampDensity"] = layer.RampDensity;

        l["Hardness"] = layer.Hardness;
        l["Friction"] = layer.Friction;
        l["Cohesion"] = layer.Cohesion;
        l["CapacityMult"] = layer.CapacityMult;

        json e;
        e["Enabled"] = layer.Erosion.Enabled;
        e["UseGPU"] = layer.Erosion.UseGPU;
        e["DropletCount"] = layer.Erosion.DropletCount;
        e["MaxLifetime"] = layer.Erosion.MaxLifetime;
        e["Gravity"] = layer.Erosion.Gravity;
        e["EvaporationRate"] = layer.Erosion.EvaporationRate;
        e["UseRainNoise"] = layer.Erosion.UseRainNoise;
        e["RainNoiseFreq"] = layer.Erosion.RainNoiseFreq;
        e["RainNoiseOctaves"] = layer.Erosion.RainNoiseOctaves;
        e["RainNoiseThreshold"] = layer.Erosion.RainNoiseThreshold;
        e["UseOrographicRain"] = layer.Erosion.UseOrographicRain;
        e["WindAngle"] = layer.Erosion.WindAngle;
        e["DepositionMode"] = layer.Erosion.DepositionMode;
        e["SpawnMinHeight"] = layer.Erosion.SpawnMinHeight;
        e["SpawnMaxHeight"] = layer.Erosion.SpawnMaxHeight;
        e["InitialSedimentLoad"] = layer.Erosion.InitialSedimentLoad;
        l["Erosion"] = e;
        l["ErodeBeneath"] = layer.ErodeBeneath;

        layers.push_back(l);
    }
    j["Layers"] = layers;

    std::ofstream out(filePath);
    out << j.dump(4);
    out.close();
}

bool MapExporter::LoadSettings(const std::string& filePath, GenerationParams& outParams) {
    std::ifstream in(filePath);
    if (!in.is_open()) return false;

    json j;
    in >> j;
    in.close();
    
    int version = j.value("PresetVersion", 0); // Default to V0 if not found

    if (j.contains("GlobalEnvironmentPath")) outParams.GlobalEnvironmentPath = j["GlobalEnvironmentPath"];
    if (j.contains("UseGPUTerrain")) outParams.UseGPUTerrain = j["UseGPUTerrain"];
    if (j.contains("Seed")) outParams.Seed = j["Seed"];
    if (j.contains("MapSize")) outParams.MapSize = j["MapSize"];
    
    if (j.contains("SymAlgorithm")) outParams.SymAlgorithm = static_cast<SymmetryAlgorithm>(j["SymAlgorithm"].get<int>());
    if (j.contains("SpawnPointCount")) outParams.SpawnPointCount = j["SpawnPointCount"];
    
    if (j.contains("Stratums")) {
        outParams.Stratums.clear();
        for (const auto& sj : j["Stratums"]) {
            StratumSettings s;
            if (sj.contains("Name")) s.Name = sj["Name"];
            if (sj.contains("EnvironmentTheme")) s.EnvironmentTheme = sj["EnvironmentTheme"];
            if (sj.contains("MaterialName")) s.MaterialName = sj["MaterialName"];
            
            if (sj.contains("BaseColor")) { s.BaseColor[0] = sj["BaseColor"][0]; s.BaseColor[1] = sj["BaseColor"][1]; s.BaseColor[2] = sj["BaseColor"][2]; s.BaseColor[3] = sj["BaseColor"][3]; }
            if (sj.contains("AlbedoPath")) s.AlbedoPath = sj["AlbedoPath"];
            if (sj.contains("NormalPath")) s.NormalPath = sj["NormalPath"];
            if (sj.contains("CompositePath")) s.CompositePath = sj["CompositePath"];
            
            if (sj.contains("MaskRemapMax")) { s.MaskRemapMax[0] = sj["MaskRemapMax"][0]; s.MaskRemapMax[1] = sj["MaskRemapMax"][1]; s.MaskRemapMax[2] = sj["MaskRemapMax"][2]; s.MaskRemapMax[3] = sj["MaskRemapMax"][3]; }
            if (sj.contains("MaskRemapMin")) { s.MaskRemapMin[0] = sj["MaskRemapMin"][0]; s.MaskRemapMin[1] = sj["MaskRemapMin"][1]; s.MaskRemapMin[2] = sj["MaskRemapMin"][2]; s.MaskRemapMin[3] = sj["MaskRemapMin"][3]; }
            if (sj.contains("Tint")) { s.Tint[0] = sj["Tint"][0]; s.Tint[1] = sj["Tint"][1]; s.Tint[2] = sj["Tint"][2]; s.Tint[3] = sj["Tint"][3]; }
            
            if (sj.contains("NearTiling")) { s.NearTiling[0] = sj["NearTiling"][0]; s.NearTiling[1] = sj["NearTiling"][1]; }
            if (sj.contains("NearNormalScale")) s.NearNormalScale = sj["NearNormalScale"];
            if (sj.contains("FarTiling")) { s.FarTiling[0] = sj["FarTiling"][0]; s.FarTiling[1] = sj["FarTiling"][1]; }
            if (sj.contains("FarNormalScale")) s.FarNormalScale = sj["FarNormalScale"];
            
            if (sj.contains("TintBlend")) s.TintBlend = sj["TintBlend"];
            if (sj.contains("NormalNearBlend")) s.NormalNearBlend = sj["NormalNearBlend"];
            if (sj.contains("HeightNearBlend")) s.HeightNearBlend = sj["HeightNearBlend"];
            
            if (sj.contains("ColorOverride")) { s.ColorOverride[0] = sj["ColorOverride"][0]; s.ColorOverride[1] = sj["ColorOverride"][1]; s.ColorOverride[2] = sj["ColorOverride"][2]; s.ColorOverride[3] = sj["ColorOverride"][3]; }
            
            if (sj.contains("HeightBlendContrast")) s.HeightBlendContrast = sj["HeightBlendContrast"];
            if (sj.contains("HeightBlendDepth")) s.HeightBlendDepth = sj["HeightBlendDepth"];
            if (sj.contains("UseDarkerAreaFill")) s.UseDarkerAreaFill = sj["UseDarkerAreaFill"];
            if (sj.contains("FadeBegin")) s.FadeBegin = sj["FadeBegin"];
            if (sj.contains("FadeDistance")) s.FadeDistance = sj["FadeDistance"];
            
            outParams.Stratums.push_back(s);
        }
    } else if (version == 0) {
        // Migration: If no stratums were saved, outParams.Stratums already has the 9 default stratums.
        // We will just keep those.
    }

    if (j.contains("Layers")) {
        outParams.Layers.clear();
        for (const auto& l : j["Layers"]) {
            NoiseLayer layer;
            if (l.contains("Name")) layer.Name = l["Name"];
            if (l.contains("Enabled")) layer.Enabled = l["Enabled"];
            if (l.contains("UseImage")) layer.UseImage = l["UseImage"];
            if (l.contains("ImagePath")) layer.ImagePath = l["ImagePath"];
            if (l.contains("OriginPresetPath")) layer.OriginPresetPath = l["OriginPresetPath"];
            if (l.contains("Erodable")) layer.Erodable = l["Erodable"];
            
            // VERSION MIGRATION for Stratum Type -> Stratum Index
            if (version == 0 && l.contains("Stratum")) {
                int oldEnum = l["Stratum"].get<int>();
                // old mapping: 0=Bedrock, 1=Sand, 2=Silt, 3=Clay, 4=Loam, 5=Snow
                switch(oldEnum) {
                    case 0: layer.StratumIndex = 0; break;
                    case 1: layer.StratumIndex = 2; break; // Map sand to stratum 2
                    case 2: layer.StratumIndex = 3; break;
                    case 3: layer.StratumIndex = 4; break;
                    case 4: layer.StratumIndex = 5; break;
                    case 5: layer.StratumIndex = 8; break; // Snow to top
                    default: layer.StratumIndex = 1; break;
                }
                
                // In V4, physics moved to Layer, so no need to map legacy V0 V1 physics to Stratums anymore.
                // We will attempt to load V1 physics from Stratum to Layer below if it's missing in Layer.
                
            } else if (l.contains("StratumIndex")) {
                layer.StratumIndex = l["StratumIndex"];
            }
            
            if (l.contains("Blend")) layer.Blend = static_cast<BlendMode>(l["Blend"].get<int>());
            if (l.contains("Type")) layer.Type = static_cast<NoiseType>(l["Type"].get<int>());
            if (l.contains("Fractal")) layer.Fractal = static_cast<FractalType>(l["Fractal"].get<int>());
            if (l.contains("SymmetryMask")) layer.SymmetryMask = l["SymmetryMask"];
            
            if (l.contains("Frequency")) layer.Frequency = l["Frequency"];
            if (l.contains("Octaves")) layer.Octaves = l["Octaves"];
            if (l.contains("Gain")) layer.Gain = l["Gain"];
            if (l.contains("PingPongStrength")) layer.PingPongStrength = l["PingPongStrength"];
            if (l.contains("Opacity")) layer.Opacity = l["Opacity"];
            if (l.contains("CellularJitter")) layer.CellularJitter = l["CellularJitter"];
            
            if (l.contains("LandDensity")) layer.LandDensity = l["LandDensity"];
            if (l.contains("PlateauDensity")) layer.PlateauDensity = l["PlateauDensity"];
            if (l.contains("MountainDensity")) layer.MountainDensity = l["MountainDensity"];
            if (l.contains("RampDensity")) layer.RampDensity = l["RampDensity"];

            if (l.contains("Hardness")) layer.Hardness = l["Hardness"];
            else if (version > 0 && j.contains("Stratums") && j["Stratums"].is_array() && layer.StratumIndex >= 0 && layer.StratumIndex < j["Stratums"].size()) {
                // Migration: pull physics from matching Stratum JSON if it exists there
                const auto& sj = j["Stratums"][layer.StratumIndex];
                if (sj.contains("Hardness")) layer.Hardness = sj["Hardness"];
                if (sj.contains("Friction")) layer.Friction = sj["Friction"];
                if (sj.contains("Cohesion")) layer.Cohesion = sj["Cohesion"];
                if (sj.contains("CapacityMult")) layer.CapacityMult = sj["CapacityMult"];
            }

            if (l.contains("Friction")) layer.Friction = l["Friction"];
            if (l.contains("Cohesion")) layer.Cohesion = l["Cohesion"];
            if (l.contains("CapacityMult")) layer.CapacityMult = l["CapacityMult"];

            if (l.contains("Erosion")) {
                const auto& e = l["Erosion"];
                if (e.contains("Enabled")) layer.Erosion.Enabled = e["Enabled"];
                if (e.contains("UseGPU")) layer.Erosion.UseGPU = e["UseGPU"];
                if (e.contains("DropletCount")) layer.Erosion.DropletCount = e["DropletCount"];
                if (e.contains("MaxLifetime")) layer.Erosion.MaxLifetime = e["MaxLifetime"];
                if (e.contains("Gravity")) layer.Erosion.Gravity = e["Gravity"];
                if (e.contains("EvaporationRate")) layer.Erosion.EvaporationRate = e["EvaporationRate"];
                if (e.contains("UseRainNoise")) layer.Erosion.UseRainNoise = e["UseRainNoise"];
                if (e.contains("RainNoiseFreq")) layer.Erosion.RainNoiseFreq = e["RainNoiseFreq"];
                if (e.contains("RainNoiseOctaves")) layer.Erosion.RainNoiseOctaves = e["RainNoiseOctaves"];
                if (e.contains("RainNoiseThreshold")) layer.Erosion.RainNoiseThreshold = e["RainNoiseThreshold"];
                if (e.contains("UseOrographicRain")) layer.Erosion.UseOrographicRain = e["UseOrographicRain"];
                if (e.contains("WindAngle")) layer.Erosion.WindAngle = e["WindAngle"];
                if (e.contains("DepositionMode")) layer.Erosion.DepositionMode = e["DepositionMode"];
                if (e.contains("SpawnMinHeight")) layer.Erosion.SpawnMinHeight = e["SpawnMinHeight"];
                if (e.contains("SpawnMaxHeight")) layer.Erosion.SpawnMaxHeight = e["SpawnMaxHeight"];
                if (e.contains("InitialSedimentLoad")) layer.Erosion.InitialSedimentLoad = e["InitialSedimentLoad"];
            }
            if (l.contains("ErodeBeneath")) layer.ErodeBeneath = l["ErodeBeneath"];

            outParams.Layers.push_back(layer);
        }
    }
    
    // Make sure we have exactly 9 stratums
    while (outParams.Stratums.size() < 9) {
        StratumSettings s;
        s.Name = "Stratum " + std::to_string(outParams.Stratums.size());
        outParams.Stratums.push_back(s);
    }
    
    return true;
}

} // namespace SanmapGen
