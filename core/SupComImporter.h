#pragma once

#include <string>
#include "Parameters.h"

namespace SanmapGen {

class SupComImporter {
public:
    // Parses a Supreme Commander _save.lua file and populates the GenerationParams with markers and props.
    static bool LoadLua(const std::string& filepath, GenerationParams& outParams, std::string& outDebugLog);
};

} // namespace SanmapGen
