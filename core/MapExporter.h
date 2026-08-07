#pragma once
#include "Parameters.h"
#include <string>

#include "TerrainGenerator.h"

namespace SanmapGen {

class MapExporter {
public:
    // Export the final playable map format to the specified folder
    static void ExportSanmap(const std::string& folderPath, const GenerationParams& params, const FloatMask& heightmap, const GenerationResult& genData, bool exportTextures = true);

    // Individual File Exporters
    static void ExportHeightmap(const std::string& filePath, const GenerationParams& params, const FloatMask& heightmap);
    static void ExportStratums(const std::string& folderPath, const GenerationParams& params, const GenerationResult& genData);
    static void ExportFlowMap(const std::string& filePath, const GenerationParams& params, const GenerationResult& genData);
    static void ExportSlopeMap(const std::string& filePath, const GenerationParams& params, const FloatMask& heightmap);

    // Save Map Generator settings to a project file (.json)
    static void SaveSettings(const std::string& filePath, const GenerationParams& params);

    // Load Map Generator settings from a project file (.json)
    static bool LoadSettings(const std::string& filePath, GenerationParams& outParams);
};

}
