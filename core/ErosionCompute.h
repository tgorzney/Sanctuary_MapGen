#pragma once
#include "Mask2D.h"
#include "Parameters.h"
#include <vector>

#include "ErosionSimulator.h"

namespace SanmapGen {
    class ErosionCompute {
    public:
        // Run GPU Compute for Stratified Erosion directly on the stratums array
        static void DispatchStratified(std::vector<FloatMask>& stratums, const std::vector<DropletSpawn>& spawns, const GlobalErosionSettings& settings, const GenerationParams& params, int mapSize);
    };
}
