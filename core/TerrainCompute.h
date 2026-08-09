#pragma once
#include "Mask2D.h"
#include "Parameters.h"
#include <vector>

namespace SanmapGen {
    class TerrainCompute {
    private:
        static unsigned int s_ComputeProgram;
        static bool s_Initialized;

    public:
        // Run GPU Compute for Base Terrain Generation
        static void DispatchTerrain(std::vector<FloatMask>& stratums, const GenerationParams& params);
        
        // Cleanup shader
        static void Shutdown();
    };
}
