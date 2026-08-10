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
        
        static unsigned int s_MarkerComputeProgram;
        static bool s_MarkerInitialized;

    public:
        // Run GPU Compute for Base Terrain Generation
        static void DispatchTerrain(std::vector<FloatMask>& stratums, const GenerationParams& params, GenerationResult& inOutResult);
        
        // Run GPU Compute for Markers
        static void DispatchMarkers(const GenerationParams& params, const MarkerRule& rule, const FloatMask& heightmap, const FloatMask& slopeMap, std::vector<int>& outMask);
        
        // Cleanup shader
        static void Shutdown();
    };
}
