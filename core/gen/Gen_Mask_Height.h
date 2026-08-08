#pragma once
#include "../Mask2D.h"
#include "../Parameters.h"
#include <vector>

namespace SanmapGen {
    class Gen_Mask_Height {
    public:
        // Calculates the protrusion thickness and adds it to the layer stratum
        static void ApplyHeightMask(FloatMask& stratum, float rawHeight, float currentTerrainHeight,
                                    const NoiseLayer& layer, int x, int y, float& outThickness, float& outMask);
    };
}
