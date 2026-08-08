#pragma once
#include "../Mask2D.h"
#include "../Parameters.h"
#include <vector>
#include <memory>

namespace SanmapGen {

    class Gen_Thermal {
    public:
        // Runs a thermal cohesion/talus angle pass on the thread-local map
        static void ProcessCohesion(std::vector<FloatMask>& threadStratums, FloatMask& threadTotalHeight,
                                    int mapSize, const std::vector<size_t>& cohesionLayers,
                                    const std::vector<const NoiseLayer*>& flatLayers);
    };

}
