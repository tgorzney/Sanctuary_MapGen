#include "UnitParser.h"
#include <filesystem>
#include <fstream>
#include <regex>
#include <iostream>

namespace SanmapGen {

void UnitParser::LoadUnitDefinitions(GenerationParams& params) {
    if (params.GamedataPath.empty()) return;

    // GamedataPath is typically: .../engine/Sanctuary_Data/Gamedata
    // We want to reach: .../engine/LJ/lua/common/units/unitsTemplates
    std::filesystem::path gdPath(params.GamedataPath);
    std::filesystem::path enginePath = gdPath.parent_path().parent_path();
    std::filesystem::path templatesPath = enginePath / "LJ" / "lua" / "common" / "units" / "unitsTemplates";

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
    
    params.DebugInfo += "Loaded " + std::to_string(params.UnitDefinitions.size()) + " unit templates.\n";
}

} // namespace SanmapGen
