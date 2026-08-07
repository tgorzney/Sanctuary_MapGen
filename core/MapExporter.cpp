#include "MapExporter.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <filesystem>

#include "stb_image_write.h"
#include <algorithm>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace SanmapGen {

void MapExporter::ExportSanmap(const std::string& folderPath, const GenerationParams& params, const FloatMask& heightmap, const GenerationResult& genData, bool exportTextures) {
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
    mapdef["heightmapResolution"] = params.MapSize + 1;
    
    mapdef["hasWater"] = true;
    
    mapdef["waterLevel"] = params.Water.WaterLevelMin;
    mapdef["waterDepth"] = params.Water.DeepWaterDepthMin;
    
    mapdef["waterWindSpeed"] = params.Water.WaterWindSpeed;
    mapdef["waterWindDirection"] = params.Water.WaterWindDirection;
    mapdef["waterShoreDepthOffset"] = params.Water.WaterShoreDepthOffset;
    mapdef["waterShoreDepthStrength"] = params.Water.WaterShoreDepthStrength;
    mapdef["waterShoreDistanceOffset"] = params.Water.WaterShoreDistanceOffset;
    mapdef["waterShoreDistanceStrength"] = params.Water.WaterShoreDistanceStrength;
    mapdef["waveGeneratorBlueprint"] = params.Water.WaveGeneratorBlueprint;

    mapdef["shader"] = "RTS/TerrainLit";
    mapdef["heightTransition"] = 0.5;
    mapdef["fadeDistance"] = 128.0;
    mapdef["fadeStartDistance"] = 1.0;

    // Atmosphere
    mapdef["sunRA"] = params.Atmosphere.SunRA;
    mapdef["sunDA"] = params.Atmosphere.SunDA;
    mapdef["sunIntensity"] = params.Atmosphere.SunIntensity;
    mapdef["sunTint"] = {
        {"r", params.Atmosphere.SunTint[0]},
        {"g", params.Atmosphere.SunTint[1]},
        {"b", params.Atmosphere.SunTint[2]},
        {"a", params.Atmosphere.SunTint[3]}
    };
    mapdef["sunTemperature"] = params.Atmosphere.SunTemperature;
    mapdef["sunAngularDiameter"] = params.Atmosphere.SunAngularDiameter;
    mapdef["sunVolumetricsMultiplier"] = params.Atmosphere.SunVolumetricsMultiplier;
    mapdef["sunVolumetricsShadowDimer"] = params.Atmosphere.SunVolumetricsShadowDimer;
    
    mapdef["skylightIntensity"] = params.Atmosphere.SkylightIntensity;
    mapdef["skylightTint"] = {
        {"r", params.Atmosphere.SkylightTint[0]},
        {"g", params.Atmosphere.SkylightTint[1]},
        {"b", params.Atmosphere.SkylightTint[2]},
        {"a", params.Atmosphere.SkylightTint[3]}
    };
    mapdef["skylightTemperature"] = params.Atmosphere.SkylightTemperature;
    
    mapdef["exposure"] = params.Atmosphere.Exposure;
    mapdef["exposureCompensation"] = params.Atmosphere.ExposureCompensation;
    mapdef["skyboxExposure"] = params.Atmosphere.SkyboxExposure;
    
    mapdef["fogAttenuationDistance"] = params.Atmosphere.FogAttenuationDistance;
    mapdef["fogBaseHeight"] = params.Atmosphere.FogBaseHeight;
    mapdef["fogMaximumHeight"] = params.Atmosphere.FogMaximumHeight;
    mapdef["fogMaximumDistance"] = params.Atmosphere.FogMaximumDistance;
    mapdef["fogAnisotropy"] = params.Atmosphere.FogAnisotropy;
    
    mapdef["skyboxPath"] = params.Atmosphere.SkyboxPath;
    
    mapdef["globalWindSpeed"] = params.Atmosphere.GlobalWindSpeed;
    mapdef["globalWindDirection"] = params.Atmosphere.GlobalWindDirection;

    // Stratum Layers
    json strata = json::array();
    for (const auto& stratum : params.Stratums) {
        json s;
        s["albedo"] = { {"path", stratum.AlbedoPath} };
        s["normal"] = { {"path", stratum.NormalPath} };
        s["mask"] = { {"path", stratum.MaskPath} };
        
        s["tileSize"] = { {"x", stratum.TileSize[0]}, {"y", stratum.TileSize[1]} };
        s["tileSizeFar"] = { {"x", stratum.TileSizeFar[0]}, {"y", stratum.TileSizeFar[1]} };
        s["tileSizeTriplanar"] = stratum.TileSizeTriplanar;
        s["tileSizeFarTriplanar"] = stratum.TileSizeFarTriplanar;
        
        s["normalScale"] = stratum.NormalScale;
        s["normalScaleFar"] = stratum.NormalScaleFar;
        s["normalFarNearBlend"] = stratum.NormalFarNearBlend;
        s["heightFarNearBlend"] = stratum.HeightFarNearBlend;
        
        s["diffuseRemap"] = { {"r", stratum.DiffuseRemap[0]}, {"g", stratum.DiffuseRemap[1]}, {"b", stratum.DiffuseRemap[2]}, {"a", stratum.DiffuseRemap[3]} };
        s["farColorRemap"] = { {"r", stratum.FarColorRemap[0]}, {"g", stratum.FarColorRemap[1]}, {"b", stratum.FarColorRemap[2]}, {"a", stratum.FarColorRemap[3]} };
        
        s["maskRemapMin"] = { {"x", stratum.MaskRemapMin[0]}, {"y", stratum.MaskRemapMin[1]}, {"z", stratum.MaskRemapMin[2]}, {"w", stratum.MaskRemapMin[3]} };
        s["maskRemapMax"] = { {"x", stratum.MaskRemapMax[0]}, {"y", stratum.MaskRemapMax[1]}, {"z", stratum.MaskRemapMax[2]}, {"w", stratum.MaskRemapMax[3]} };
        
        strata.push_back(s);
    }
    mapdef["stratumLayers"] = strata;
    
    // Areas, armies, chains
    mapdef["areas"] = json::object();
    mapdef["armies"] = json::object();
    
    json markersObj = json::object();
    
    // We want to group markers by Type (e.g. "Spawn", "Alloy", "Plasma")
    std::map<std::string, json> groupedTransforms;
    std::map<std::string, int> typeCounters;
    
    for (const auto& [key, marker] : params.MarkersList) {
        if (marker.IsHidden) continue;
        
        json tfObj;
        tfObj["position"] = {{"x", marker.Position[0]}, {"y", marker.Position[1]}, {"z", marker.Position[2]}};
        tfObj["rotation"] = {{"x", marker.Rotation[0]}, {"y", marker.Rotation[1]}, {"z", marker.Rotation[2]}, {"w", marker.Rotation[3]}};
        tfObj["scale"] = {{"x", marker.Scale[0]}, {"y", marker.Scale[1]}, {"z", marker.Scale[2]}};
        
        std::string transformKey = marker.CustomName;
        if (transformKey.empty()) {
            if (marker.Type == "Spawn") {
                transformKey = "ARMY_" + std::to_string(++typeCounters["Spawn"]);
            } else if (marker.Type == "Alloy") {
                transformKey = "Mex " + std::to_string(typeCounters["Alloy"]++);
            } else {
                transformKey = marker.Type + "_" + std::to_string(++typeCounters[marker.Type]);
            }
        }
        
        groupedTransforms[marker.Type][transformKey] = tfObj;
    }
    
    // Create the structured markers object
    for (const auto& [type, transforms] : groupedTransforms) {
        json typeObj = json::object();
        typeObj["resource"] = (type == "Alloy" || type == "Plasma" || type == "Hydro");
        typeObj["transforms"] = transforms;
        
        // Native expects "Alloys", "Plasmas", "Spawn"
        std::string outType = type;
        if (outType == "Alloy") outType = "Alloys";
        else if (outType == "Plasma") outType = "Plasmas";
        
        markersObj[outType] = typeObj;
    }
    
    mapdef["markers"] = markersObj;

    json propsArr = json::array();
    for (const auto& rule : params.Props) {
        if (!rule.Enabled) continue;
        json propType;
        propType["blueprintPath"] = rule.BlueprintPath;
        propType["transforms"] = json::array(); // Placeholder for actual transforms generated
        propsArr.push_back(propType);
    }
    mapdef["props"] = propsArr;

    json decalsArr = json::array();
    for (const auto& rule : params.Decals) {
        if (!rule.Enabled) continue;
        json decalType;
        decalType["albedoPath"] = rule.AlbedoPath; // Note: native uses blueprintPath for decals, but keeping this for now
        decalType["normalPath"] = rule.NormalPath;
        decalType["transforms"] = json::array(); // Placeholder for actual transforms generated
        decalsArr.push_back(decalType);
    }
    mapdef["decals"] = decalsArr;
    mapdef["chains"] = json::object();

    // Determine the map name from the folder path
    std::string mapName = "mapdef";
    size_t lastSlash = folderPath.find_last_of("/\\");
    if (lastSlash != std::string::npos && lastSlash + 1 < folderPath.length()) {
        mapName = folderPath.substr(lastSlash + 1);
    }
    
    // Set map name in JSON
    mapdef["name"] = mapName;

    // Export JSON matching the folder name
    std::string filePath = folderPath + "/" + mapName + ".sanmap";
    std::ofstream out(filePath);
    out << mapdef.dump(4);
    out.close();

    if (exportTextures) {
        // Create Textures subfolder required by Native Editor
        std::string texFolder = folderPath + "/Textures";
        if (!fs::exists(texFolder)) {
            fs::create_directories(texFolder);
        }

        // Export Heightmap
        std::string hmPath = texFolder + "/heightmap.raw";
        ExportHeightmap(hmPath, params, heightmap);
        
        // Export Stratums
        ExportStratums(texFolder, params, genData);

        auto exportTGA = [&](const std::string& name, const std::vector<uint8_t>& data, int w, int h, int comps) {
            std::string p = texFolder + "/" + name;
            stbi_write_tga(p.c_str(), w, h, comps, data.data());
        };

        int texSize = params.MapSize; // Assuming textures are MapSize x MapSize
        int pixelCount = texSize * texSize;

        // 4. Export tint_colors.tga (RGB = 128 for no tint, A = Smoothness 148 default)
        std::vector<uint8_t> tintColors(pixelCount * 4, 0);
        for (int i = 0; i < pixelCount * 4; i += 4) {
            tintColors[i + 0] = 128; // R
            tintColors[i + 1] = 128; // G
            tintColors[i + 2] = 128; // B
            tintColors[i + 3] = 148; // A
        }
        // TODO: Actually fill Tint/Smoothness based on layers if applicable in future
        exportTGA("tint_colors.tga", tintColors, texSize, texSize, 4);

        // 5. Export tint_geometry.tga (RG = Normals (128), B = Holes (255 for no hole))
        std::vector<uint8_t> tintGeom(pixelCount * 3, 0);
        for (int i = 0; i < pixelCount * 3; i += 3) {
            tintGeom[i + 0] = 128; // R
            tintGeom[i + 1] = 128; // G
            tintGeom[i + 2] = 255; // B
        }
        // TODO: Calculate real normals or holes from data
        exportTGA("tint_geometry.tga", tintGeom, texSize, texSize, 3);
    }
}

void MapExporter::ExportHeightmap(const std::string& filePath, const GenerationParams& params, const FloatMask& heightmap) {
    int dim = params.MapSize + 1;
    int hWidth = heightmap.GetWidth();
    std::vector<uint16_t> rawHeightmap(dim * dim);
    for (int y = 0; y < dim; ++y) {
        for (int x = 0; x < dim; ++x) {
            float val = 0.0f;
            if (x < hWidth && y < hWidth) val = heightmap.Get(x, y);
            val = std::clamp(val, 0.0f, 1.0f);
            rawHeightmap[y * dim + x] = static_cast<uint16_t>(val * 65535.0f);
        }
    }
    std::ofstream hmOut(filePath, std::ios::binary);
    if (hmOut) {
        hmOut.write(reinterpret_cast<const char*>(rawHeightmap.data()), rawHeightmap.size() * sizeof(uint16_t));
        hmOut.close();
    }
}

void MapExporter::ExportStratums(const std::string& folderPath, const GenerationParams& params, const GenerationResult& genData) {
    int texSize = params.MapSize;
    int pixelCount = texSize * texSize;
    std::vector<uint8_t> s1_4(pixelCount * 4, 0);
    std::vector<uint8_t> s5_8(pixelCount * 4, 0);

    for (int y = 0; y < texSize; ++y) {
        for (int x = 0; x < texSize; ++x) {
            int idx = (y * texSize + x) * 4;
            for (int i = 0; i < 4; ++i) {
                float val = (i < genData.MaterialMasks.size()) ? genData.MaterialMasks[i].Get(x, y) : 0.0f;
                s1_4[idx + i] = static_cast<uint8_t>(std::clamp(val, 0.0f, 1.0f) * 255.0f);
            }
            for (int i = 0; i < 4; ++i) {
                float val = ((i + 4) < genData.MaterialMasks.size()) ? genData.MaterialMasks[i + 4].Get(x, y) : 0.0f;
                s5_8[idx + i] = static_cast<uint8_t>(std::clamp(val, 0.0f, 1.0f) * 255.0f);
            }
        }
    }
    std::string p1 = folderPath + "/stratums_1_4.tga";
    std::string p2 = folderPath + "/stratums_5_8.tga";
    stbi_write_tga(p1.c_str(), texSize, texSize, 4, s1_4.data());
    stbi_write_tga(p2.c_str(), texSize, texSize, 4, s5_8.data());
}

void MapExporter::ExportFlowMap(const std::string& filePath, const GenerationParams& params, const GenerationResult& genData) {
    int texSize = params.MapSize;
    std::vector<uint8_t> pixels(texSize * texSize * 4, 0);
    for (int y = 0; y < texSize; ++y) {
        for (int x = 0; x < texSize; ++x) {
            float val = genData.FlowMap.Get(x, y) * 100.0f; // Scale it a bit for visibility
            uint8_t intensity = static_cast<uint8_t>(std::clamp(val, 0.0f, 1.0f) * 255.0f);
            int idx = (y * texSize + x) * 4;
            pixels[idx] = intensity; // R
            pixels[idx+1] = intensity; // G
            pixels[idx+2] = intensity; // B
            pixels[idx+3] = 255;
        }
    }
    stbi_write_png(filePath.c_str(), texSize, texSize, 4, pixels.data(), texSize * 4);
}

void MapExporter::ExportSlopeMap(const std::string& filePath, const GenerationParams& params, const FloatMask& heightmap) {
    int texSize = params.MapSize;
    std::vector<uint8_t> pixels(texSize * texSize * 4, 0);
    float quadWidth = 1024.0f;
    float cellSize = static_cast<float>(params.MapSize) / quadWidth;
    if (cellSize < 1.0f) cellSize = 1.0f;

    for (int y = 0; y < texSize; ++y) {
        for (int x = 0; x < texSize; ++x) {
            float v00 = heightmap.Get(x, y);
            float v10 = heightmap.Get(std::min(x + 1, texSize - 1), y);
            float v01 = heightmap.Get(x, std::min(y + 1, texSize - 1));
            float v11 = heightmap.Get(std::min(x + 1, texSize - 1), std::min(y + 1, texSize - 1));

            float dx = (((v10 + v11) - (v00 + v01)) * 0.5f * 128.0f) / cellSize;
            float dy = (((v01 + v11) - (v00 + v10)) * 0.5f * 128.0f) / cellSize;
            float slopeDegrees = atan(sqrt(dx*dx + dy*dy)) * (180.0f / 3.14159265f);

            // Normalize slope to 0-90 degrees for export visualization
            float val = slopeDegrees / 90.0f; 
            uint8_t intensity = static_cast<uint8_t>(std::clamp(val, 0.0f, 1.0f) * 255.0f);
            
            int idx = (y * texSize + x) * 4;
            pixels[idx] = intensity;
            pixels[idx+1] = intensity;
            pixels[idx+2] = intensity;
            pixels[idx+3] = 255;
        }
    }
    stbi_write_png(filePath.c_str(), texSize, texSize, 4, pixels.data(), texSize * 4);
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
