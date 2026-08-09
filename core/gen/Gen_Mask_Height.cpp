#include "Gen_Mask_Height.h"
#include <algorithm>

namespace SanmapGen {
    void Gen_Mask_Height::ApplyHeightMask(FloatMask& stratum, float rawHeight, float currentTerrainHeight,
                                          const NoiseLayer& layer, int x, int y, float& outThickness, float& outMask) {
        float clampedRawHeight = rawHeight;
        
        // Use HeightBlendMin/Max as a hard floor and ceiling limiter for the noise
        float safeMin = std::min(layer.HeightBlendMin, layer.HeightBlendMax);
        float safeMax = std::max(layer.HeightBlendMin, layer.HeightBlendMax);
        if (safeMin == safeMax) safeMax = safeMin + 0.001f;
        
        // If noise is below the floor, mask it out entirely
        if (clampedRawHeight < safeMin) {
            clampedRawHeight = 0.0f;
        } 
        // If noise is above the ceiling, hard-cap it
        else if (clampedRawHeight > safeMax) {
            clampedRawHeight = safeMax;
        }
        
        float thickness = clampedRawHeight - currentTerrainHeight;
        
        // Ensure that if we masked it out completely, it generates 0 thickness
        if (clampedRawHeight == 0.0f) {
            thickness = 0.0f;
        }
        
        outThickness = thickness;
        outMask = 1.0f; // Unused, keeping for compatibility
        
        if (thickness > 0.0f) {
            float heightDelta = thickness * layer.Opacity;
            stratum.Set(x, y, heightDelta);
        } else {
            stratum.Set(x, y, 0.0f);
        }
    }
}
