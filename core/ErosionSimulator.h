#pragma once
#include "Mask2D.h"
#include "Parameters.h"

namespace SanmapGen {

    struct DropletSpawn {
        float x, y;
    };

    class ErosionSimulator {
    public:
        // Run the stratified erosion simulation on the map array in-place
        static void SimulateStratifiedErosionDelta(std::vector<FloatMask>& stratums, const std::vector<DropletSpawn>& spawns, const GlobalErosionSettings& settings, const GenerationParams& params, int mapSize);
        
    private:
        static void CalculateGradient(const FloatMask& map, float x, float y, float& height, float& gradX, float& gradY);
    };

} // namespace SanmapGen
