#pragma once
#include <string>
#include "Parameters.h"

namespace SanmapGen {

class UnitParser {
public:
    // Main entry point
    static void LoadUnitDefinitions(GenerationParams& params, void* openZipArchive = nullptr);
    static void ParseFootprint(const std::string& typeId, GenerationParams& params);
};

} // namespace SanmapGen
