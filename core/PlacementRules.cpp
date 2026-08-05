#include "PlacementRules.h"
#include <algorithm>
#include <cmath>
#include <queue>
#include <random>
#include <iostream>

namespace SanmapGen {

    void PlacementRules::ExecutePlacement(const FloatMask& heightmap, GenerationParams& params) {
        // 1. Calculate slopemap (equivalent to SupcomGradient)
        FloatMask slopeMap = CalculateSlopeMask(heightmap);
        
        // 2. Clear old data
        params.GeneratedSpawns.clear();
        params.GeneratedMexes.clear();
        params.GeneratedHydros.clear();
        params.GeneratedTrees.clear();
        
        // 3. Find Spawn Zones
        DetectSpawns(heightmap, slopeMap, params);
        
        // 4. Place Resources
        PlaceResources(heightmap, slopeMap, params);
        
        // 5. Generate Exclusion Mask (for Props)
        int mapSize = heightmap.GetWidth();
        BooleanMask exclusionMask(mapSize, mapSize, false);
        GenerateExclusionMask(exclusionMask, params, mapSize);
        
        // 6. Place Props
        PlaceProps(heightmap, slopeMap, exclusionMask, params);
    }

    FloatMask PlacementRules::CalculateSlopeMask(const FloatMask& heightmap) {
        int mapSize = heightmap.GetWidth();
        FloatMask slopeMap(mapSize, mapSize, 0.0f);
        
        for (int y = 1; y < mapSize - 1; ++y) {
            for (int x = 1; x < mapSize - 1; ++x) {
                float dx = (heightmap.Get(x+1, y) - heightmap.Get(x-1, y)) * 0.5f;
                float dy = (heightmap.Get(x, y+1) - heightmap.Get(x, y-1)) * 0.5f;
                float slope = std::sqrt(dx*dx + dy*dy);
                slopeMap.Set(x, y, slope);
            }
        }
        return slopeMap;
    }

    void PlacementRules::DetectSpawns(const FloatMask& heightmap, const FloatMask& slopeMap, GenerationParams& params) {
        int mapSize = heightmap.GetWidth();
        std::vector<bool> visited(mapSize * mapSize, false);
        std::vector<FlatZone> zones;
        
        float flatThreshold = 0.05f;
        float minHeight = params.Water.WaterLevelMax + 5.0f; // Must be above water
        
        for (int y = 10; y < mapSize - 10; ++y) {
            for (int x = 10; x < mapSize - 10; ++x) {
                int idx = y * mapSize + x;
                if (visited[idx]) continue;
                
                float slope = slopeMap.Get(x, y);
                float h = heightmap.Get(x, y);
                
                if (slope < flatThreshold && h >= minHeight) {
                    // Flood fill
                    FlatZone zone;
                    zone.Area = 0;
                    zone.AverageHeight = 0.0f;
                    
                    std::queue<Point2D> q;
                    q.push({x, y});
                    visited[idx] = true;
                    
                    long long sumX = 0;
                    long long sumY = 0;
                    
                    while (!q.empty()) {
                        Point2D p = q.front();
                        q.pop();
                        
                        zone.Points.push_back(p);
                        zone.Area++;
                        sumX += p.x;
                        sumY += p.y;
                        zone.AverageHeight += heightmap.Get(p.x, p.y);
                        
                        const int dx[] = {-1, 1, 0, 0};
                        const int dy[] = {0, 0, -1, 1};
                        for (int d = 0; d < 4; ++d) {
                            int nx = p.x + dx[d];
                            int ny = p.y + dy[d];
                            
                            if (nx >= 10 && nx < mapSize-10 && ny >= 10 && ny < mapSize-10) {
                                int nidx = ny * mapSize + nx;
                                if (!visited[nidx]) {
                                    if (slopeMap.Get(nx, ny) < flatThreshold && heightmap.Get(nx, ny) >= minHeight) {
                                        visited[nidx] = true;
                                        q.push({nx, ny});
                                    }
                                }
                            }
                        }
                    }
                    
                    if (zone.Area > 500) { // Arbitrary min size for a base
                        zone.CenterX = (int)(sumX / zone.Area);
                        zone.CenterY = (int)(sumY / zone.Area);
                        zone.AverageHeight /= (float)zone.Area;
                        zones.push_back(zone);
                    }
                }
            }
        }
        
        // Sort by area descending
        std::sort(zones.begin(), zones.end(), [](const FlatZone& a, const FlatZone& b) {
            return a.Area > b.Area;
        });
        
        // Assign spawns
        for (int i = 0; i < params.SpawnPointCount && i < zones.size(); ++i) {
            params.GeneratedSpawns.push_back({zones[i].CenterX, zones[i].CenterY});
        }
    }

    void PlacementRules::PlaceResources(const FloatMask& heightmap, const FloatMask& slopeMap, GenerationParams& params) {
        int mapSize = heightmap.GetWidth();
        
        int targetMexCount = 10 * params.SpawnPointCount * params.MexDensity;
        int targetHydroCount = 1 * params.SpawnPointCount * params.HydroMultiplier;
        
        std::mt19937 rng(params.Seed + 2000);
        std::uniform_int_distribution<int> dist(20, mapSize - 20);
        
        // Simple random placement for now, masked by slope
        int placedMex = 0;
        int attempts = 0;
        while (placedMex < targetMexCount && attempts < targetMexCount * 100) {
            attempts++;
            int px = dist(rng);
            int py = dist(rng);
            
            if (slopeMap.Get(px, py) < 0.1f && heightmap.Get(px, py) > params.Water.WaterLevelMax) {
                // Check distance to spawns
                bool tooClose = false;
                for (const auto& sp : params.GeneratedSpawns) {
                    float dx = (float)(px - sp.x);
                    float dy = (float)(py - sp.y);
                    if (dx*dx + dy*dy < 100.0f) { // Spacing 10
                        tooClose = true;
                        break;
                    }
                }
                
                if (!tooClose) {
                    params.GeneratedMexes.push_back({px, py});
                    placedMex++;
                }
            }
        }
        
        int placedHydro = 0;
        attempts = 0;
        while (placedHydro < targetHydroCount && attempts < targetHydroCount * 100) {
            attempts++;
            int px = dist(rng);
            int py = dist(rng);
            
            if (slopeMap.Get(px, py) < 0.1f && heightmap.Get(px, py) > params.Water.WaterLevelMax) {
                // Hydros need more space
                params.GeneratedHydros.push_back({px, py});
                placedHydro++;
            }
        }
    }
    
    void PlacementRules::GenerateExclusionMask(BooleanMask& outMask, const GenerationParams& params, int mapSize) {
        // Init to clear (false)
        for (int y = 0; y < mapSize; ++y) {
            for (int x = 0; x < mapSize; ++x) {
                outMask.Set(x, y, false);
            }
        }
        
        auto fillCircle = [&](int cx, int cy, float radius) {
            int r = (int)std::ceil(radius);
            float r2 = radius * radius;
            for (int y = cy - r; y <= cy + r; ++y) {
                for (int x = cx - r; x <= cx + r; ++x) {
                    if (x >= 0 && x < mapSize && y >= 0 && y < mapSize) {
                        float dx = (float)(x - cx);
                        float dy = (float)(y - cy);
                        if (dx*dx + dy*dy <= r2) {
                            outMask.Set(x, y, true);
                        }
                    }
                }
            }
        };
        
        // Spawn Spacing = 30
        for (const auto& sp : params.GeneratedSpawns) {
            fillCircle(sp.x, sp.y, 30.0f);
        }
        
        // Mex Spacing = 2
        for (const auto& mex : params.GeneratedMexes) {
            fillCircle(mex.x, mex.y, 2.0f);
        }
        
        // Hydro Spacing = 8
        for (const auto& h : params.GeneratedHydros) {
            fillCircle(h.x, h.y, 8.0f);
        }
    }
    
    void PlacementRules::PlaceProps(const FloatMask& heightmap, const FloatMask& slopeMap, const BooleanMask& exclusionMask, GenerationParams& params) {
        int mapSize = heightmap.GetWidth();
        std::mt19937 rng(params.Seed + 3000);
        std::uniform_int_distribution<int> dist(10, mapSize - 10);
        std::uniform_real_distribution<float> prob(0.0f, 1.0f);
        
        // Only checking trees for now based on ReclaimDensity
        int targetTrees = 5000 * params.ReclaimDensity;
        
        int placed = 0;
        int attempts = 0;
        while (placed < targetTrees && attempts < targetTrees * 50) {
            attempts++;
            int px = dist(rng);
            int py = dist(rng);
            
            if (!exclusionMask.Get(px, py)) {
                float h = heightmap.Get(px, py);
                float slope = slopeMap.Get(px, py);
                
                // Trees prefer flat land, above water
                if (h > params.Water.WaterLevelMax && slope < 0.2f) {
                    params.GeneratedTrees.push_back({px, py});
                    placed++;
                }
            }
        }
    }

} // namespace SanmapGen
