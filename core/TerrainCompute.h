#pragma once
#include "Mask2D.h"
#include "Parameters.h"
#include <vector>

namespace SanmapGen {
    class TerrainCompute {
    public:
        // Run GPU Compute for Base Terrain Generation
        static void DispatchTerrain(std::vector<FloatMask>& stratums, const GenerationParams& params);
    };
}
