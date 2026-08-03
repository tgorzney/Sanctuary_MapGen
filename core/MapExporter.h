#pragma once
#include "Parameters.h"
#include <string>

namespace SanmapGen {

class MapExporter {
public:
    // Export the final playable map format to the specified folder
    static void ExportSanmap(const std::string& folderPath, const GenerationParams& params);

    // Save Map Generator settings to a project file (.json)
    static void SaveSettings(const std::string& filePath, const GenerationParams& params);

    // Load Map Generator settings from a project file (.json)
    static bool LoadSettings(const std::string& filePath, GenerationParams& outParams);
};

}
