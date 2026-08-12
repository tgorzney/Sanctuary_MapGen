#pragma once
#include "../Mask2D.h"
#include "../TerrainGenerator.h"

namespace SanmapGen {

    class Gen_Mask_Slope {
    public:
        // Calculates a 0-90 approx slope map from a heightmap
        // Designed as a pure data-oriented pass to allow easy 1:1 mapping to a Compute Shader
        static void GenerateSlopeMap(const FloatMask& heightMap, FloatMask& outSlopeMap, bool bUseEngineParityMath = false, GenerationResult* result = nullptr, float terrainMaxHeight = 128.0f, float cellSize = 1.0f);
    };

}
