#pragma once
#include "Mask2D.h"
#include "Parameters.h"
#include <vector>

namespace SanmapGen {
    struct GenerationResult;

    class TerrainCompute {
    private:
        static unsigned int s_ComputeProgram;
        static bool s_Initialized;

    public:
        // Run GPU Compute for Base Terrain Generation
        static void DispatchTerrain(std::vector<FloatMask>& stratums, const GenerationParams& params, GenerationResult& inOutResult);
        
        // Cleanup shader
        static void Shutdown();
    };
}
