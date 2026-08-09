#include "Gen_Thermal.h"
#include <algorithm>

namespace SanmapGen {

    void Gen_Thermal::ProcessCohesion(std::vector<FloatMask>& threadStratums, FloatMask& threadTotalHeight,
                                      int mapSize, const std::vector<size_t>& cohesionLayers,
                                      const std::vector<const NoiseLayer*>& flatLayers,
                                      const GenerationParams& params) {
        
        for (int p = 0; p < 2; ++p) { // 2 passes
            for (int y = 1; y < mapSize - 1; ++y) {
                for (int x = 1; x < mapSize - 1; ++x) {
                    for (int l = (int)cohesionLayers.size() - 1; l >= 0; --l) {
                        int idx = cohesionLayers[l];
                        if (!(*flatLayers[idx]).Erodable) continue;
                        
                        float thickness = threadStratums[idx].Get(x, y);
                        // Skip trivial operations entirely (the only branch we want to keep for performance on sparse maps)
                        if (thickness <= 0.001f) continue;

                        float h = threadTotalHeight.Get(x, y);
                        
                        // Sample neighbors
                        float h_l = threadTotalHeight.Get(x - 1, y);
                        float h_r = threadTotalHeight.Get(x + 1, y);
                        float h_u = threadTotalHeight.Get(x, y - 1);
                        float h_d = threadTotalHeight.Get(x, y + 1);
                        
                        // Branchless differences (only positive differences count)
                        float dh_l = std::max(0.0f, h - h_l);
                        float dh_r = std::max(0.0f, h - h_r);
                        float dh_u = std::max(0.0f, h - h_u);
                        float dh_d = std::max(0.0f, h - h_d);
                        
                        float total_dh = dh_l + dh_r + dh_u + dh_d;
                        
                        // Cohesion determines the max angle/slope
                        float maxSlope = params.Stratums[(*flatLayers[idx]).StratumIndex].cohesion;
                        
                        // Branchless gate: If total_dh <= maxSlope, slide = 0
                        float slideActive = (total_dh > maxSlope) ? 1.0f : 0.0f;
                        
                        // Calculate total slide amount (bounded by thickness)
                        float slideAmount = std::min(thickness, (total_dh - maxSlope) / 2.0f) * slideActive;
                        
                        // Branchless divide-by-zero protection
                        float inv_total_dh = (total_dh > 0.00001f) ? (1.0f / total_dh) : 0.0f;
                        
                        // Distribute proportionally to seek true volumetric minimums
                        float slip_l = slideAmount * (dh_l * inv_total_dh);
                        float slip_r = slideAmount * (dh_r * inv_total_dh);
                        float slip_u = slideAmount * (dh_u * inv_total_dh);
                        float slip_d = slideAmount * (dh_d * inv_total_dh);
                        
                        float total_slip = slip_l + slip_r + slip_u + slip_d;
                        
                        // Apply modifications mathematically
                        threadStratums[idx].Set(x, y, threadStratums[idx].Get(x, y) - total_slip);
                        threadStratums[idx].Set(x - 1, y, threadStratums[idx].Get(x - 1, y) + slip_l);
                        threadStratums[idx].Set(x + 1, y, threadStratums[idx].Get(x + 1, y) + slip_r);
                        threadStratums[idx].Set(x, y - 1, threadStratums[idx].Get(x, y - 1) + slip_u);
                        threadStratums[idx].Set(x, y + 1, threadStratums[idx].Get(x, y + 1) + slip_d);
                        
                        threadTotalHeight.Set(x, y, h - total_slip);
                        threadTotalHeight.Set(x - 1, y, h_l + slip_l);
                        threadTotalHeight.Set(x + 1, y, h_r + slip_r);
                        threadTotalHeight.Set(x, y - 1, h_u + slip_u);
                        threadTotalHeight.Set(x, y + 1, h_d + slip_d);
                    }
                }
            }
        }
    }
}
