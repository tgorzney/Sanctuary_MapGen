#pragma once
#include "../Mask2D.h"
#include "../Parameters.h"
#include <vector>

namespace SanmapGen {
    class Gen_Tex_Albedo {
    public:
        // Applies the material texturing mask based on height thickness and slopes
        static void ApplyAlbedoMask(std::vector<FloatMask>& materialMasks, float mask, 
                                    const NoiseLayer& layer, const GenerationParams& params, int x, int y);
    };
}
