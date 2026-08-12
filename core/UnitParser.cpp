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

    std::regex fpXRegex("x\\s*=\\s*([0-9.]+)");
    std::regex fpYRegex("y\\s*=\\s*([0-9.]+)");
    std::regex speedRegex("speed\\s*=\\s*([0-9.]+)");
    std::regex accelRegex("acceleration\\s*=\\s*([0-9.]+)");
    std::regex nameRegex("displayName\\s*=\\s*\"([^\"]+)\"");

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
                    if (inFootprint) inFootprint = false;
                }

                std::smatch match;
                if (inFootprint) {
                    if (std::regex_search(line, match, fpXRegex)) def.FootprintX = std::stof(match[1]);
                    if (std::regex_search(line, match, fpYRegex)) def.FootprintY = std::stof(match[1]);
                }
                
                if (std::regex_search(line, match, speedRegex)) def.Speed = std::stof(match[1]);
                if (std::regex_search(line, match, accelRegex)) def.Acceleration = std::stof(match[1]);
                
                if (inGeneral && std::regex_search(line, match, nameRegex)) def.DisplayName = match[1];
            }

            params.UnitDefinitions[typeId] = def;
        }
    }
    
    params.DebugInfo += "Loaded " + std::to_string(params.UnitDefinitions.size()) + " unit templates.\n";
}

} // namespace SanmapGen
