#pragma once
#include "../Mask2D.h"
#include "../Parameters.h"
#include "../ErosionSimulator.h"
#include <vector>

namespace SanmapGen {

    class Gen_Hydraulic {
    public:
        // Processes a batch of hydraulic droplets on a thread-local copy of the map
        static void ProcessDroplets(std::vector<FloatMask>& threadStratums, FloatMask& threadTotalHeight,
                                    const std::vector<DropletSpawn>& spawns, int dropStart, int dropCount,
                                    const ErosionSettings& settings, const GenerationParams& params,
                                    int mapSize, int currentLayerIdx, const std::vector<size_t>& activeLayers,
                                    const std::vector<const NoiseLayer*>& flatLayers);
    };

}
