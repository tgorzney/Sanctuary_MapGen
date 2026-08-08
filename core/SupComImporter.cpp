#include "SupComImporter.h"
#include "TerrainGenerator.h"
#include "gen/Gen_Marker_Placement.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <iostream>

namespace SanmapGen {

bool SupComImporter::LoadLua(const std::string& filepath, GenerationParams& outParams, std::string& outDebugLog) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        outDebugLog += "Failed to open " + filepath + "\n";
        return false;
    }

    outDebugLog += "Parsing SupCom Lua: " + filepath + "\n";
    
    // 1. Clear existing markers
    outParams.MarkersList.clear();
    outParams.StaticPropsList.clear();
    PlacedMarkerLayer importedLayer;
    importedLayer.Name = "Imported SupCom Markers";

    std::string line;
    float originalMapSize = 0.0f;
    
    // Simple state tracking
    bool inAreas = false;
    bool inMasterChain = false;
    bool inArmies = false;
    
    std::string currentMarkerName = "";
    std::string currentMarkerType = "";
    float currentPosX = 0.0f, currentPosY = 0.0f, currentPosZ = 0.0f;
    bool currentMarkerValid = false;
    
    std::regex rectRegex(R"(RECTANGLE\(\s*[\d\.\-]+\s*,\s*[\d\.\-]+\s*,\s*([\d\.\-]+)\s*,\s*([\d\.\-]+)\s*\))");
    std::regex typeRegex(R"(\['type'\]\s*=\s*STRING\(\s*'([^']+)'\s*\))");
    std::regex posRegex(R"(\['position'\]\s*=\s*VECTOR3\(\s*([\d\.\-]+)\s*,\s*([\d\.\-]+)\s*,\s*([\d\.\-]+)\s*\))");
    std::regex armyStartRegex(R"(\['ARMY_(\d+)'\]\s*=)");
    std::regex markerStartRegex(R"(\['([^']+)'\]\s*=)");

    while (std::getline(file, line)) {
        if (line.find("Areas = {") != std::string::npos) inAreas = true;
        else if (line.find("MasterChain = {") != std::string::npos) inMasterChain = true;
        else if (line.find("Armies = ") != std::string::npos) inArmies = true;
        
        if (inAreas) {
            std::smatch match;
            if (std::regex_search(line, match, rectRegex)) {
                float w = std::stof(match[1].str());
                float h = std::stof(match[2].str());
                if (w > originalMapSize) originalMapSize = w; // Usually AREA_1 or AREA_4 is the biggest
            }
            if (line.find("},") != std::string::npos && line.find("['rectangle']") == std::string::npos) {
                // Heuristic: end of Areas block
                if (line.find("    },") != std::string::npos && line.length() < 10) inAreas = false;
            }
        }
        
        if (inMasterChain || inArmies) {
            std::smatch match;
            
            // Check for position (applies to both MasterChain markers and Armies)
            if (std::regex_search(line, match, posRegex)) {
                currentPosX = std::stof(match[1].str());
                currentPosY = std::stof(match[2].str());
                currentPosZ = std::stof(match[3].str());
                currentMarkerValid = true;
                
                // If we are in an Army block and found position, save it immediately
                if (inArmies && !currentMarkerName.empty() && currentMarkerValid) {
                    MarkerTransform mt;
                    mt.Type = "Spawn";
                    mt.CustomName = "Spawn_" + currentMarkerName;
                    mt.IsManual = true;
                    mt.Position[0] = currentPosX;
                    mt.Position[1] = currentPosY;
                    mt.Position[2] = currentPosZ;
                    outParams.MarkersList[mt.CustomName] = mt;
                    importedLayer.MarkerKeys.push_back(mt.CustomName);
                    
                    currentMarkerName = "";
                    currentMarkerValid = false;
                }
                continue;
            }
            
            if (inMasterChain) {
                if (std::regex_search(line, match, typeRegex)) {
                    currentMarkerType = match[1].str();
                }
                
                // End of a marker block in MasterChain
                if (line.find("},") != std::string::npos) {
                    if (currentMarkerValid && !currentMarkerType.empty() && !currentMarkerName.empty()) {
                        std::string targetType = "";
                        if (currentMarkerType == "Mass") targetType = "Alloy";
                        else if (currentMarkerType == "Hydrocarbon") targetType = "Plasma";
                        else if (currentMarkerName.find("ARMY_") == 0) targetType = "Spawn";
                        
                        if (!targetType.empty()) {
                            MarkerTransform mt;
                            mt.Type = targetType;
                            mt.CustomName = targetType + "_" + currentMarkerName;
                            mt.IsManual = true;
                            mt.Position[0] = currentPosX;
                            mt.Position[1] = currentPosY;
                            mt.Position[2] = currentPosZ;
                            outParams.MarkersList[mt.CustomName] = mt;
                    importedLayer.MarkerKeys.push_back(mt.CustomName);
                        }
                    }
                    currentMarkerName = "";
                    currentMarkerType = "";
                    currentMarkerValid = false;
                }
                
                // Start of a marker block
                if (currentMarkerName.empty() && std::regex_search(line, match, markerStartRegex)) {
                    currentMarkerName = match[1].str();
                }
            }
            
            if (inArmies) {
                if (std::regex_search(line, match, armyStartRegex)) {
                    currentMarkerName = "ARMY_" + match[1].str();
                    currentMarkerValid = false;
                }
            }
        }
    }
    
    // Post-process: Auto-scaling
    if (originalMapSize <= 0.0f) originalMapSize = 1024.0f; // Fallback
    
    float scaleFactor = static_cast<float>(outParams.MapSize) / originalMapSize;
    outDebugLog += "Original SupCom Size: " + std::to_string(originalMapSize) + ", Target Sanctuary Size: " + std::to_string(outParams.MapSize) + "\n";
    outDebugLog += "Applying Scale Factor: " + std::to_string(scaleFactor) + "\n";
    
    for (auto& [key, marker] : outParams.MarkersList) {
        marker.Position[0] *= scaleFactor;
        marker.Position[2] *= scaleFactor;
    }
    
    // Rebuild Spatial Grid
    int chunks = outParams.SpatialGridResolution;
    outParams.MarkerSpatialGrid.assign(chunks * chunks, GenerationParams::MarkerChunk());
    for (const auto& [key, marker] : outParams.MarkersList) {
        float normX = marker.Position[0] / outParams.MapSize;
        float normY = marker.Position[2] / outParams.MapSize;
        int cx = std::clamp(static_cast<int>(normX * chunks), 0, chunks - 1);
        int cy = std::clamp(static_cast<int>(normY * chunks), 0, chunks - 1);
        outParams.MarkerSpatialGrid[cy * chunks + cx].MarkerKeys.push_back(key);
    }
    
    outParams.PlacedMarkerLayers.push_back(importedLayer);
    
    // Auto-detect symmetry
    Gen_Marker_Placement::CalculateMarkerSymmetryGroups(outParams);
    
    outDebugLog += "Successfully imported " + std::to_string(outParams.MarkersList.size()) + " markers from Lua.\n";
    return true;
}

} // namespace SanmapGen
