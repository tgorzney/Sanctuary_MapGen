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
    j["UseGPUTerrain"] = params.UseGPUTerrain;
    j["Seed"] = params.Seed;
    j["MapSize"] = params.MapSize;
    
    // Simplistic save for layers (you can expand this later)
    json layers = json::array();
    for (const auto& layer : params.Layers) {
        json l;
        l["Name"] = layer.Name;
        l["Enabled"] = layer.Enabled;
        l["Frequency"] = layer.Frequency;
        l["Octaves"] = layer.Octaves;
        l["Gain"] = layer.Gain;
        l["Opacity"] = layer.Opacity;
        l["Stratum"] = static_cast<int>(layer.Stratum);
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

    if (j.contains("UseGPUTerrain")) outParams.UseGPUTerrain = j["UseGPUTerrain"];
    if (j.contains("Seed")) outParams.Seed = j["Seed"];
    if (j.contains("MapSize")) outParams.MapSize = j["MapSize"];

    if (j.contains("Layers")) {
        outParams.Layers.clear();
        for (const auto& l : j["Layers"]) {
            NoiseLayer layer;
            if (l.contains("Name")) layer.Name = l["Name"];
            if (l.contains("Enabled")) layer.Enabled = l["Enabled"];
            if (l.contains("Frequency")) layer.Frequency = l["Frequency"];
            if (l.contains("Octaves")) layer.Octaves = l["Octaves"];
            if (l.contains("Gain")) layer.Gain = l["Gain"];
            if (l.contains("Opacity")) layer.Opacity = l["Opacity"];
            if (l.contains("Stratum")) layer.Stratum = static_cast<StratumType>(l["Stratum"].get<int>());
            outParams.Layers.push_back(layer);
        }
    }
    return true;
}

} // namespace SanmapGen
