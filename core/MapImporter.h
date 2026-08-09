#pragma once

#include "Parameters.h"
#include <string>

namespace SanmapGen {

class MapImporter {
public:
    // Load a .sanmap folder. Returns true if successful.
    static bool LoadSanmap(const std::string& folderPath, GenerationParams& outParams, std::string& outDebugLog, bool& bSizeMismatch, int& outDiscoveredDim);
};

} // namespace SanmapGen
