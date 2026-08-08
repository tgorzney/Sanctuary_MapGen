#include "Gen_Thermal.h"
#include <algorithm>

namespace SanmapGen {

    void Gen_Thermal::ProcessCohesion(std::vector<FloatMask>& threadStratums, FloatMask& threadTotalHeight,
                                      int mapSize, const std::vector<size_t>& cohesionLayers,
                                      const std::vector<const NoiseLayer*>& flatLayers) {
        
        for (int p = 0; p < 2; ++p) { // 2 passes
            for (int y = 1; y < mapSize - 1; ++y) {
                for (int x = 1; x < mapSize - 1; ++x) {
                    for (int l = (int)cohesionLayers.size() - 1; l >= 0; --l) {
                        int idx = cohesionLayers[l];
                        if (!(*flatLayers[idx]).Erodable) continue;
                        
                        float thickness = threadStratums[idx].Get(x, y);
                        if (thickness > 0.001f) {
                            const auto& layer = (*flatLayers[idx]);
                            float maxSlope = layer.Cohesion;
                            
                            float h = threadTotalHeight.Get(x, y);
                            int bestNX = x, bestNY = y;
                            float lowestH = h;
                            
                            const int dx[] = { -1, 1, 0, 0 };
                            const int dy[] = { 0, 0, -1, 1 };
                            for (int d = 0; d < 4; ++d) {
                                float nh = threadTotalHeight.Get(x + dx[d], y + dy[d]);
                                if (nh < lowestH) {
                                    lowestH = nh;
                                    bestNX = x + dx[d];
                                    bestNY = y + dy[d];
                                }
                            }
                            
                            float diff = h - lowestH;
                            if (diff > maxSlope) {
                                float slideAmount = (diff - maxSlope) / 2.0f;
                                slideAmount = std::min(slideAmount, thickness); 
                                
                                threadStratums[idx].Set(x, y, thickness - slideAmount);
                                threadStratums[idx].Set(bestNX, bestNY, threadStratums[idx].Get(bestNX, bestNY) + slideAmount);
                                
                                threadTotalHeight.Set(x, y, threadTotalHeight.Get(x, y) - slideAmount);
                                threadTotalHeight.Set(bestNX, bestNY, threadTotalHeight.Get(bestNX, bestNY) + slideAmount);
                            }
                        }
                    }
                }
            }
        }
    }
}
