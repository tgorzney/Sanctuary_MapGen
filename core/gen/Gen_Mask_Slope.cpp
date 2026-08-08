#include "Gen_Mask_Slope.h"
#include <cmath>

namespace SanmapGen {

    void Gen_Mask_Slope::GenerateSlopeMap(const FloatMask& heightMap, FloatMask& outSlopeMap) {
        int vertSize = heightMap.GetWidth();
        outSlopeMap.Resize(vertSize, vertSize, 0.0f);
        
        for (int y = 1; y < vertSize - 1; ++y) {
            for (int x = 1; x < vertSize - 1; ++x) {
                float dx = (heightMap.Get(x+1, y) - heightMap.Get(x-1, y)) * 0.5f;
                float dy = (heightMap.Get(x, y+1) - heightMap.Get(x, y-1)) * 0.5f;
                // Multiplied by 100 for better human readable slope 0-90 approx
                float slope = std::sqrt(dx*dx + dy*dy) * 100.0f;
                outSlopeMap.Set(x, y, slope);
            }
        }
    }

}
