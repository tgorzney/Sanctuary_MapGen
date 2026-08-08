#include "Gen_Mask_Height.h"
#include <algorithm>

namespace SanmapGen {
    void Gen_Mask_Height::ApplyHeightMask(FloatMask& stratum, float rawHeight, float currentTerrainHeight,
                                          const NoiseLayer& layer, int x, int y, float& outThickness, float& outMask) {
        float thickness = rawHeight - currentTerrainHeight;
        outThickness = thickness;
        outMask = 0.0f;
        
        if (thickness > 0.0f) {
            float mask = thickness * layer.HeightBlendContrast;
            float safeMin = std::min(layer.HeightBlendMin, layer.HeightBlendMax);
            float safeMax = std::max(layer.HeightBlendMin, layer.HeightBlendMax);
            if (safeMin == safeMax) safeMax = safeMin + 0.001f;
            mask = std::clamp(mask, safeMin, safeMax);
            outMask = mask;
            
            float heightDelta = thickness * layer.Opacity;
            stratum.Set(x, y, heightDelta);
        } else {
            stratum.Set(x, y, 0.0f);
        }
    }
}
