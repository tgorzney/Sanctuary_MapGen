#include "Gen_Tex_Albedo.h"
#include <algorithm>

namespace SanmapGen {
    void Gen_Tex_Albedo::ApplyAlbedoMask(std::vector<FloatMask>& materialMasks, float mask, 
        const NoiseLayer& layer, const GenerationParams& params, int x, int y) {
        // Obsolete: Material masks are now calculated post-erosion in TerrainGenerator.cpp
        // via the AVX2 top-down occlusion algorithm.
    }
} // namespace SanmapGen
