#pragma once
#include "../Parameters.h"
#include "../TerrainGenerator.h"
#include <string>
#include <vector>

namespace SanmapGen {
    namespace TextureExporter {
        void ExportHeightmap(const std::string& filePath, const GenerationParams& params, const FloatMask& heightmap);
        void ExportStratums(const std::string& folderPath, const GenerationParams& params, const GenerationResult& genData);
        void ExportFlowMap(const std::string& filePath, const GenerationParams& params, const GenerationResult& genData);
        void ExportSlopeMap(const std::string& filePath, const GenerationParams& params, const FloatMask& heightmap);
        void ExportTints(const std::string& folderPath, const GenerationParams& params);
    }
}
