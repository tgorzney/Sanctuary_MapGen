#include "gen/Gen_Placement.h"
#include "gen/Gen_FlowAndAccumulation.h"
#include "gen/Gen_Erosion.h"
#include "gen/Gen_NoiseAndBlend.h"
#include "TerrainGenerator.h"
#include "gen/Gen_Noise.h"
#include "gen/Gen_Marker_Procedural.h"
#include "FastNoiseLite.h"
#include <random>
#include <future>
#include <thread>
#include <algorithm>
#include <cmath>
#include "ErosionSimulator.h"
#include "TerrainCompute.h"
#include "gen/Gen_Mask_Slope.h"
#include "gen/Gen_Marker_Placement.h"
#include "gen/Gen_Mask_Height.h"
#include "ErosionCompute.h"
#include "PlacementRules.h"
#include "math/Sanmath_SIMD.h"

namespace SanmapGen {

    // Helper: Insert a 0 bit after each of the 16 low bits of x
    inline uint32_t Part1By1(uint32_t x) {
        x &= 0x0000ffff;
        x = (x ^ (x <<  8)) & 0x00ff00ff;
        x = (x ^ (x <<  4)) & 0x0f0f0f0f;
        x = (x ^ (x <<  2)) & 0x33333333;
        x = (x ^ (x <<  1)) & 0x55555555;
        return x;
    }

    // Helper: Inverse of Part1By1
    inline uint32_t Compact1By1(uint32_t x) {
        x &= 0x55555555;
        x = (x ^ (x >>  1)) & 0x33333333;
        x = (x ^ (x >>  2)) & 0x0f0f0f0f;
        x = (x ^ (x >>  4)) & 0x00ff00ff;
        x = (x ^ (x >>  8)) & 0x0000ffff;
        return x;
    }

    


    

    void TerrainGenerator::GenerateMap(FloatMask& outMap, const GenerationParams& params, GenerationResult& inOutResult) {
        int vertSize = params.MapSize + 1;
        outMap.Resize(vertSize, vertSize, 0.0f);
        
        std::vector<FloatMask> Stratums;
        auto flatLayers = params.GetFlatLayers();
        for (size_t i = 0; i < flatLayers.size(); ++i) {
            Stratums.push_back(FloatMask(vertSize, vertSize, 0.0f));
        }
        
        size_t currentBlendHash = 0;
        Gen_NoiseAndBlend::Process(outMap, Stratums, params, inOutResult, currentBlendHash);
        
        size_t currentErosionHash = 0;
        Gen_Erosion::Process(outMap, Stratums, params, inOutResult, currentBlendHash, currentErosionHash);
        
        size_t currentFlowHash = 0;
        Gen_FlowAndAccumulation::Process(outMap, params, inOutResult, currentErosionHash, currentFlowHash);
        
        Gen_Placement::Process(outMap, params, inOutResult, currentErosionHash, currentFlowHash);
    }






    

    

    // Returns {MaxFlatRadius, Variance}

} // namespace SanmapGen
