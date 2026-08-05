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
    
    mapdef["hasWater"] = true;
    
    json waterObj;
    waterObj["waterLevelMin"] = params.Water.WaterLevelMin;
    waterObj["waterLevelMax"] = params.Water.WaterLevelMax;
    waterObj["deepWaterDepthMin"] = params.Water.DeepWaterDepthMin;
    waterObj["deepWaterDepthMax"] = params.Water.DeepWaterDepthMax;
    waterObj["waterWindSpeed"] = params.Water.WaterWindSpeed;
    waterObj["waterWindDirection"] = params.Water.WaterWindDirection;
    waterObj["waterShoreDepthOffset"] = params.Water.WaterShoreDepthOffset;
    waterObj["waterShoreDepthStrength"] = params.Water.WaterShoreDepthStrength;
    waterObj["waterShoreDistanceOffset"] = params.Water.WaterShoreDistanceOffset;
    waterObj["waterShoreDistanceStrength"] = params.Water.WaterShoreDistanceStrength;
    waterObj["waveGeneratorBlueprint"] = params.Water.WaveGeneratorBlueprint;
    mapdef["water"] = waterObj;

    mapdef["shader"] = "RTS/TerrainLit";

    // Atmosphere
    json atmosObj;
    atmosObj["sunRA"] = params.Atmosphere.SunRA;
    atmosObj["sunDA"] = params.Atmosphere.SunDA;
    atmosObj["sunIntensity"] = params.Atmosphere.SunIntensity;
    atmosObj["sunTint"] = { {"r", params.Atmosphere.SunTint[0]}, {"g", params.Atmosphere.SunTint[1]}, {"b", params.Atmosphere.SunTint[2]}, {"a", params.Atmosphere.SunTint[3]} };
    atmosObj["sunTemperature"] = params.Atmosphere.SunTemperature;
    atmosObj["sunAngularDiameter"] = params.Atmosphere.SunAngularDiameter;
    atmosObj["sunVolumetricsMultiplier"] = params.Atmosphere.SunVolumetricsMultiplier;
    atmosObj["sunVolumetricsShadowDimer"] = params.Atmosphere.SunVolumetricsShadowDimer;
    
    atmosObj["skylightIntensity"] = params.Atmosphere.SkylightIntensity;
    atmosObj["skylightTint"] = { {"r", params.Atmosphere.SkylightTint[0]}, {"g", params.Atmosphere.SkylightTint[1]}, {"b", params.Atmosphere.SkylightTint[2]}, {"a", params.Atmosphere.SkylightTint[3]} };
    atmosObj["skylightTemperature"] = params.Atmosphere.SkylightTemperature;
    
    atmosObj["exposure"] = params.Atmosphere.Exposure;
    atmosObj["exposureCompensation"] = params.Atmosphere.ExposureCompensation;
    atmosObj["skyboxExposure"] = params.Atmosphere.SkyboxExposure;
    
    atmosObj["fogAttenuationDistance"] = params.Atmosphere.FogAttenuationDistance;
    atmosObj["fogBaseHeight"] = params.Atmosphere.FogBaseHeight;
    atmosObj["fogMaximumHeight"] = params.Atmosphere.FogMaximumHeight;
    atmosObj["fogMaximumDistance"] = params.Atmosphere.FogMaximumDistance;
    atmosObj["fogAnisotropy"] = params.Atmosphere.FogAnisotropy;
    
    atmosObj["skyboxPath"] = params.Atmosphere.SkyboxPath;
    
    atmosObj["globalWindSpeed"] = params.Atmosphere.GlobalWindSpeed;
    atmosObj["globalWindDirection"] = params.Atmosphere.GlobalWindDirection;
    mapdef["atmosphere"] = atmosObj;

    // Stratum Layers
    json strata = json::array();
    for (const auto& stratum : params.Stratums) {
        json s;
        s["albedo"] = stratum.AlbedoPath;
        s["normal"] = stratum.NormalPath;
        s["mask"] = stratum.MaskPath;
        
        s["tileSize"] = { {"x", stratum.TileSize[0]}, {"y", stratum.TileSize[1]} };
        s["tileSizeFar"] = { {"x", stratum.TileSizeFar[0]}, {"y", stratum.TileSizeFar[1]} };
        s["tileSizeTriplanar"] = stratum.TileSizeTriplanar;
        s["tileSizeFarTriplanar"] = stratum.TileSizeFarTriplanar;
        
        s["normalScale"] = stratum.NormalScale;
        s["normalScaleFar"] = stratum.NormalScaleFar;
        s["normalFarNearBlend"] = stratum.NormalFarNearBlend;
        s["heightFarNearBlend"] = stratum.HeightFarNearBlend;
        
        s["diffuseRemap"] = { {"x", stratum.DiffuseRemap[0]}, {"y", stratum.DiffuseRemap[1]}, {"z", stratum.DiffuseRemap[2]}, {"w", stratum.DiffuseRemap[3]} };
        s["farColorRemap"] = { {"x", stratum.FarColorRemap[0]}, {"y", stratum.FarColorRemap[1]}, {"z", stratum.FarColorRemap[2]}, {"w", stratum.FarColorRemap[3]} };
        
        s["maskRemapMin"] = { {"x", stratum.MaskRemapMin[0]}, {"y", stratum.MaskRemapMin[1]}, {"z", stratum.MaskRemapMin[2]}, {"w", stratum.MaskRemapMin[3]} };
        s["maskRemapMax"] = { {"x", stratum.MaskRemapMax[0]}, {"y", stratum.MaskRemapMax[1]}, {"z", stratum.MaskRemapMax[2]}, {"w", stratum.MaskRemapMax[3]} };
        
        strata.push_back(s);
    }
    mapdef["stratumLayers"] = strata;
    
    // Areas, armies, chains
    mapdef["areas"] = json::object();
    mapdef["armies"] = json::object();
    
    json markersObj = json::object();
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

    json propsObj = json::object();
    for (const auto& rule : params.Props) {
        if (!rule.Enabled) continue;
        json propType;
        propType["blueprintPath"] = rule.BlueprintPath;
        propType["transforms"] = json::array(); // Placeholder for actual transforms generated
        propsObj[rule.Name] = propType;
    }
    mapdef["props"] = propsObj;

    json decalsObj = json::object();
    for (const auto& rule : params.Decals) {
        if (!rule.Enabled) continue;
        json decalType;
        decalType["albedoPath"] = rule.AlbedoPath;
        decalType["normalPath"] = rule.NormalPath;
        decalType["transforms"] = json::array(); // Placeholder for actual transforms generated
        decalsObj[rule.Name] = decalType;
    }
    mapdef["decals"] = decalsObj;

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
        sj["AlbedoPath"] = s.AlbedoPath;
        sj["NormalPath"] = s.NormalPath;
        sj["MaskPath"] = s.MaskPath;
        
        sj["TileSize"] = {s.TileSize[0], s.TileSize[1]};
        sj["TileSizeFar"] = {s.TileSizeFar[0], s.TileSizeFar[1]};
        sj["TileSizeTriplanar"] = s.TileSizeTriplanar;
        sj["TileSizeFarTriplanar"] = s.TileSizeFarTriplanar;
        
        sj["NormalScale"] = s.NormalScale;
        sj["NormalScaleFar"] = s.NormalScaleFar;
        sj["NormalFarNearBlend"] = s.NormalFarNearBlend;
        sj["HeightFarNearBlend"] = s.HeightFarNearBlend;
        
        sj["DiffuseRemap"] = {s.DiffuseRemap[0], s.DiffuseRemap[1], s.DiffuseRemap[2], s.DiffuseRemap[3]};
        sj["FarColorRemap"] = {s.FarColorRemap[0], s.FarColorRemap[1], s.FarColorRemap[2], s.FarColorRemap[3]};
        
        sj["MaskRemapMin"] = {s.MaskRemapMin[0], s.MaskRemapMin[1], s.MaskRemapMin[2], s.MaskRemapMin[3]};
        sj["MaskRemapMax"] = {s.MaskRemapMax[0], s.MaskRemapMax[1], s.MaskRemapMax[2], s.MaskRemapMax[3]};
        
        sj["Hardness"] = s.Hardness;
        sj["Friction"] = s.Friction;
        sj["Cohesion"] = s.Cohesion;
        sj["CapacityMult"] = s.CapacityMult;
        
        stratums.push_back(sj);
    }
    j["Stratums"] = stratums;
    
    // Save Props & Decals
    json props = json::array();
    for (const auto& p : params.Props) {
        json pj; pj["Name"] = p.Name; pj["Enabled"] = p.Enabled; pj["BlueprintPath"] = p.BlueprintPath;
        pj["Density"] = p.Density; pj["MinSlope"] = p.MinSlope; pj["MaxSlope"] = p.MaxSlope;
        pj["MinHeight"] = p.MinHeight; pj["MaxHeight"] = p.MaxHeight;
        pj["AvoidWater"] = p.AvoidWater; pj["NearCliffs"] = p.NearCliffs;
        props.push_back(pj);
    }
    j["Props"] = props;
    
    json decals = json::array();
    for (const auto& d : params.Decals) {
        json dj; dj["Name"] = d.Name; dj["Enabled"] = d.Enabled; dj["AlbedoPath"] = d.AlbedoPath; dj["NormalPath"] = d.NormalPath;
        dj["Density"] = d.Density; dj["MinSlope"] = d.MinSlope; dj["MaxSlope"] = d.MaxSlope;
        dj["MinHeight"] = d.MinHeight; dj["MaxHeight"] = d.MaxHeight;
        decals.push_back(dj);
    }
    j["Decals"] = decals;

    // Save Layers
    json layers = json::array();
    for (const auto* layerPtr : params.GetFlatLayers()) {
        const auto& layer = *layerPtr;
        if (!layer.Enabled) continue;
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
        e["UseGPU"] = params.UseGPUHydraulic;
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
            if (sj.contains("AlbedoPath")) s.AlbedoPath = sj["AlbedoPath"];
            if (sj.contains("NormalPath")) s.NormalPath = sj["NormalPath"];
            if (sj.contains("MaskPath")) s.MaskPath = sj["MaskPath"];
            
            if (sj.contains("TileSize")) { s.TileSize[0] = sj["TileSize"][0]; s.TileSize[1] = sj["TileSize"][1]; }
            if (sj.contains("TileSizeFar")) { s.TileSizeFar[0] = sj["TileSizeFar"][0]; s.TileSizeFar[1] = sj["TileSizeFar"][1]; }
            if (sj.contains("TileSizeTriplanar")) s.TileSizeTriplanar = sj["TileSizeTriplanar"];
            if (sj.contains("TileSizeFarTriplanar")) s.TileSizeFarTriplanar = sj["TileSizeFarTriplanar"];
            
            if (sj.contains("NormalScale")) s.NormalScale = sj["NormalScale"];
            if (sj.contains("NormalScaleFar")) s.NormalScaleFar = sj["NormalScaleFar"];
            if (sj.contains("NormalFarNearBlend")) s.NormalFarNearBlend = sj["NormalFarNearBlend"];
            if (sj.contains("HeightFarNearBlend")) s.HeightFarNearBlend = sj["HeightFarNearBlend"];
            
            if (sj.contains("DiffuseRemap")) { s.DiffuseRemap[0] = sj["DiffuseRemap"][0]; s.DiffuseRemap[1] = sj["DiffuseRemap"][1]; s.DiffuseRemap[2] = sj["DiffuseRemap"][2]; s.DiffuseRemap[3] = sj["DiffuseRemap"][3]; }
            if (sj.contains("FarColorRemap")) { s.FarColorRemap[0] = sj["FarColorRemap"][0]; s.FarColorRemap[1] = sj["FarColorRemap"][1]; s.FarColorRemap[2] = sj["FarColorRemap"][2]; s.FarColorRemap[3] = sj["FarColorRemap"][3]; }
            
            if (sj.contains("MaskRemapMin")) { s.MaskRemapMin[0] = sj["MaskRemapMin"][0]; s.MaskRemapMin[1] = sj["MaskRemapMin"][1]; s.MaskRemapMin[2] = sj["MaskRemapMin"][2]; s.MaskRemapMin[3] = sj["MaskRemapMin"][3]; }
            if (sj.contains("MaskRemapMax")) { s.MaskRemapMax[0] = sj["MaskRemapMax"][0]; s.MaskRemapMax[1] = sj["MaskRemapMax"][1]; s.MaskRemapMax[2] = sj["MaskRemapMax"][2]; s.MaskRemapMax[3] = sj["MaskRemapMax"][3]; }
            
            if (sj.contains("Hardness")) s.Hardness = sj["Hardness"];
            if (sj.contains("Friction")) s.Friction = sj["Friction"];
            if (sj.contains("Cohesion")) s.Cohesion = sj["Cohesion"];
            if (sj.contains("CapacityMult")) s.CapacityMult = sj["CapacityMult"];
            
            outParams.Stratums.push_back(s);
        }
    } else if (version == 0) {
        // Migration: If no stratums were saved, outParams.Stratums already has the 9 default stratums.
    }
    
    if (j.contains("Props")) {
        outParams.Props.clear();
        for (const auto& pj : j["Props"]) {
            PropRule p;
            if (pj.contains("Name")) p.Name = pj["Name"];
            if (pj.contains("Enabled")) p.Enabled = pj["Enabled"];
            if (pj.contains("BlueprintPath")) p.BlueprintPath = pj["BlueprintPath"];
            if (pj.contains("Density")) p.Density = pj["Density"];
            if (pj.contains("MinSlope")) p.MinSlope = pj["MinSlope"];
            if (pj.contains("MaxSlope")) p.MaxSlope = pj["MaxSlope"];
            if (pj.contains("MinHeight")) p.MinHeight = pj["MinHeight"];
            if (pj.contains("MaxHeight")) p.MaxHeight = pj["MaxHeight"];
            if (pj.contains("AvoidWater")) p.AvoidWater = pj["AvoidWater"];
            if (pj.contains("NearCliffs")) p.NearCliffs = pj["NearCliffs"];
            outParams.Props.push_back(p);
        }
    }
    
    if (j.contains("Decals")) {
        outParams.Decals.clear();
        for (const auto& dj : j["Decals"]) {
            DecalRule d;
            if (dj.contains("Name")) d.Name = dj["Name"];
            if (dj.contains("Enabled")) d.Enabled = dj["Enabled"];
            if (dj.contains("AlbedoPath")) d.AlbedoPath = dj["AlbedoPath"];
            if (dj.contains("NormalPath")) d.NormalPath = dj["NormalPath"];
            if (dj.contains("Density")) d.Density = dj["Density"];
            if (dj.contains("MinSlope")) d.MinSlope = dj["MinSlope"];
            if (dj.contains("MaxSlope")) d.MaxSlope = dj["MaxSlope"];
            if (dj.contains("MinHeight")) d.MinHeight = dj["MinHeight"];
            if (dj.contains("MaxHeight")) d.MaxHeight = dj["MaxHeight"];
            outParams.Decals.push_back(d);
        }
    }

    if (j.contains("Layers")) {
        outParams.GeoLayers.clear();
        outParams.GeoLayers.push_back(GeoLayerDef());
        outParams.GeoLayers[0].Name = "Migrated GeoLayer";
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
                if (e.contains("UseGPU")) outParams.UseGPUHydraulic = e["UseGPU"];
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

            outParams.GeoLayers[0].Layers.push_back(layer);
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
