#include "UnitParser.h"
#include <filesystem>
#include <fstream>
#include <regex>
#include <iostream>
#include <nlohmann/json.hpp>
#include "TextureLoader.h"

using json = nlohmann::json;

namespace SanmapGen {

void UnitParser::LoadUnitDefinitions(GenerationParams& params) {
    if (params.GamedataPath.empty()) return;

    // GamedataPath is typically: .../engine/Sanctuary_Data/Gamedata
    // We want to reach: .../engine/LJ/lua/common/units/unitsTemplates
    std::filesystem::path gdPath(params.GamedataPath);
    std::filesystem::path enginePath = gdPath.parent_path().parent_path();
    std::filesystem::path templatesPath = enginePath / "LJ" / "lua" / "common" / "units" / "unitsTemplates";
    
    std::filesystem::path cacheFile = gdPath / "SanmapGen_UnitCache.json";
    
    if (std::filesystem::exists(cacheFile)) {
        try {
            std::ifstream i(cacheFile);
            json j;
            i >> j;
            for (auto& element : j) {
                UnitDefinition def;
                def.Type = element.value("Type", "");
                def.Name = element.value("Name", "");
                def.DisplayName = element.value("DisplayName", "");
                def.FootprintX = element.value("FootprintX", 1.0f);
                def.FootprintY = element.value("FootprintY", 1.0f);
                def.Speed = element.value("Speed", 10.0f);
                def.Acceleration = element.value("Acceleration", 10.0f);
                params.UnitDefinitions[def.Type] = def;
                
                if (element.contains("UV")) {
                    params.UnitAtlasUVs[def.Type] = { 
                        element["UV"][0].get<float>(), 
                        element["UV"][1].get<float>(), 
                        element["UV"][2].get<float>(), 
                        element["UV"][3].get<float>() 
                    };
                }
            }
            
            // Load Atlas texture
            TextureLoader::GenerateUnitAtlas(params);
            
            params.DebugInfo += "Loaded " + std::to_string(params.UnitDefinitions.size()) + " unit templates from CACHE.\n";
            return;
        } catch (...) {
            params.DebugInfo += "Cache load failed, parsing manually...\n";
        }
    }

    if (!std::filesystem::exists(templatesPath) || !std::filesystem::is_directory(templatesPath)) {
        params.DebugInfo += "UnitTemplates path not found: " + templatesPath.string() + "\n";
        return;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(templatesPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".santp") {
            std::string typeId = entry.path().stem().string(); // e.g. "uca1001"
            std::ifstream file(entry.path());
            if (!file.is_open()) continue;

            UnitDefinition def;
            def.Type = typeId;

            std::string line;
            bool inFootprint = false;
            bool inMovement = false;
            bool inGeneral = false;
            
            while (std::getline(file, line)) {
                if (line.find("footprint = {") != std::string::npos) inFootprint = true;
                if (line.find("movement = {") != std::string::npos) inMovement = true;
                if (line.find("general = {") != std::string::npos) inGeneral = true;
                if (line.find("},") != std::string::npos) {
                    inFootprint = false;
                    inMovement = false;
                    inGeneral = false;
                }

                if (inFootprint) {
                    size_t xPos = line.find("x");
                    if (xPos != std::string::npos) {
                        size_t eq = line.find("=", xPos);
                        if (eq != std::string::npos) {
                            try { def.FootprintX = std::stof(line.substr(eq + 1)); } catch(...) {}
                        }
                    }
                    size_t yPos = line.find("y");
                    if (yPos != std::string::npos && yPos > line.find("footprint") && line.find("type") == std::string::npos) {
                        // Ensure it's isolated y, basic check
                        size_t eq = line.find("=", yPos);
                        if (eq != std::string::npos) {
                            try { def.FootprintY = std::stof(line.substr(eq + 1)); } catch(...) {}
                        }
                    }
                }
                
                if (inMovement) {
                    size_t sPos = line.find("speed");
                    if (sPos != std::string::npos) {
                        size_t eq = line.find("=", sPos);
                        if (eq != std::string::npos) {
                            try { def.Speed = std::stof(line.substr(eq + 1)); } catch(...) {}
                        }
                    }
                    size_t aPos = line.find("acceleration");
                    if (aPos != std::string::npos) {
                        size_t eq = line.find("=", aPos);
                        if (eq != std::string::npos) {
                            try { def.Acceleration = std::stof(line.substr(eq + 1)); } catch(...) {}
                        }
                    }
                }
                
                if (inGeneral) {
                    size_t dPos = line.find("displayName");
                    if (dPos != std::string::npos) {
                        size_t start = line.find("\"", dPos);
                        if (start != std::string::npos) {
                            size_t end = line.find("\"", start + 1);
                            if (end != std::string::npos) {
                                def.DisplayName = line.substr(start + 1, end - start - 1);
                            }
                        }
                    }
                }
            }

            params.UnitDefinitions[typeId] = def;
        }
    }
    
    // Generate Atlas (this builds the raw file and the UVs)
    TextureLoader::GenerateUnitAtlas(params);
    
    // Save Cache
    try {
        json j = json::array();
        for (const auto& kv : params.UnitDefinitions) {
            const auto& def = kv.second;
            json d;
            d["Type"] = def.Type;
            d["Name"] = def.Name;
            d["DisplayName"] = def.DisplayName;
            d["FootprintX"] = def.FootprintX;
            d["FootprintY"] = def.FootprintY;
            d["Speed"] = def.Speed;
            d["Acceleration"] = def.Acceleration;
            
            auto uvIt = params.UnitAtlasUVs.find(def.Type);
            if (uvIt != params.UnitAtlasUVs.end()) {
                d["UV"] = { uvIt->second[0], uvIt->second[1], uvIt->second[2], uvIt->second[3] };
            }
            
            j.push_back(d);
        }
        std::ofstream o(cacheFile);
        o << j.dump(4);
    } catch (...) {}
    
    params.DebugInfo += "Loaded " + std::to_string(params.UnitDefinitions.size()) + " unit templates.\n";
}

} // namespace SanmapGen
