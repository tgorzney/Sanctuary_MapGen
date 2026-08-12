#pragma once
#include "../Parameters.h"
#include <string>
#include "../TerrainGenerator.h"
#include <nlohmann/json.hpp>

namespace SanmapGen {

class MetadataExporter {
public:
    // Export the final playable map format to the specified folder
    static void ExportSanmap(const std::string& folderPath, const GenerationParams& params, const FloatMask& heightmap, const GenerationResult& genData, bool exportTextures = true);

    // Save Map Generator settings to a project file (.json)
    static void SaveSettings(const std::string& filePath, const GenerationParams& params);

    // Serialize Map Generator settings to a JSON object
    static nlohmann::json SerializeSettings(const GenerationParams& params);

    // Load Map Generator settings from a project file (.json)
    static bool LoadSettings(const std::string& filePath, GenerationParams& outParams);
};

}
