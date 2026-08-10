#include "../TerrainGenerator.h"
#include "../Parameters.h"
#include "Gen_Marker_Procedural.h"
#include "../math/Sanmath_Spatial.h"
#include "../math/Sanmath_SIMD.h"
#include "../math/Sanmath_FastMath.h"
#include "../TerrainCompute.h"
#include <random>
#include <omp.h>
#include <mutex>
#include <atomic>
#include <cmath>
#include <iostream>

namespace SanmapGen {

    struct MarkerCandidate {
        int x, y;
        int maxFlatRadius;
        float variance;
    };

    void Gen_Marker_Procedural::GenerateProceduralMarkers(const GenerationParams& params, const FloatMask& heightmap, const FloatMask& slopeMap, GenerationResult& inOutResult) {
        int mapSize = heightmap.GetWidth();
        int halfSize = mapSize / 2;
        inOutResult.GeneratedMarkers.clear();
        
        std::mt19937 globalRng(params.Seed + 5000);
        
        for (const auto& layer : params.ProceduralMarkerLayers) {
            for (const auto& rule : layer.Rules) {
                if (!rule.Enabled) continue;
                
                int targetCount = rule.Count;
                if (rule.UseAllPositions) {
                    targetCount = INT_MAX;
                } else if (rule.UseDensity) {
                    float area100 = (float)mapSize * (float)mapSize * Math::FastInv(10000.0f);
                    targetCount = (int)(rule.Density * area100);
                }
                if (targetCount <= 0) continue;
                
                int maxSearchRadius = (int)std::ceil(rule.AreaRadiusMin);
                if (rule.CheckMaxRadius) {
                    maxSearchRadius = std::max(maxSearchRadius, (int)std::ceil(rule.AreaRadiusMax));
                }
                if (rule.Priority == MarkerPriority::Priority_LargestArea) {
                    maxSearchRadius = std::max(maxSearchRadius, 100);
                }
                
                int symMask = rule.SymmetryUseGlobal ? params.GlobalSymmetryMask : rule.SymmetryMask;
                
                std::vector<MarkerCandidate> candidates;
                std::mutex mtx;
                std::atomic<int> globalMaxRadius((int)std::ceil(rule.AreaRadiusMin));
                
                // --- GOD TIER MATH: Convert Degrees to Squared Gradient ---
                float minSClamped = std::min(rule.MinSlope, 89.9f);
                float maxSClamped = std::min(rule.MaxSlope, 89.9f);
                float minRad = minSClamped * (3.14159265f / 180.0f);
                float maxRad = maxSClamped * (3.14159265f / 180.0f);
                float minGradSq = std::tan(minRad) * std::tan(minRad);
                float maxGradSq = std::tan(maxRad) * std::tan(maxRad);
                
                // Pre-calculate boundary padding to avoid inner loop branching
                int pad = (rule.MapEdgePadding > 0.0f) ? (int)rule.MapEdgePadding : 0;
                int startBound = pad;
                int endBound = mapSize - pad;
                if (startBound >= endBound) continue;
                
                // Pre-compute JFA Distance Field if accurate mode is selected
                FloatMask jfaDistanceMap(0, 0);
                if (!params.FastPreviewMode && rule.AreaRadiusMin > 0.0f) {
                    jfaDistanceMap = Math::ComputeJFADistanceField(heightmap, rule.MinHeight, rule.MaxHeight, rule.AreaHeightRange, (float)maxSearchRadius + 2.0f);
                }
                
                // FAST PATH: Random Selection (Stochastic)
                if (rule.RandomSelection) {
                    int placed = 0;
                    int attempts = 0;
                    int maxAttempts = targetCount * 1000;
                    
                    float clearanceSq = rule.ClearanceSpacing * rule.ClearanceSpacing;
                    std::vector<std::pair<float, float>> placedPositions;
                    
                    for (const auto& kvp : inOutResult.GeneratedMarkers) {
                        placedPositions.push_back({kvp.second.Position[0], kvp.second.Position[2]});
                    }
                    
                    std::uniform_int_distribution<int> dist(startBound, endBound - 1);
                    
                    while (placed < targetCount && attempts < maxAttempts) {
                        attempts++;
                        int x = dist(globalRng);
                        int y = dist(globalRng);
                        
                        // Symmetry Culling for random points
                        if (symMask & Symmetry_Point && y > halfSize) continue;
                        if (symMask & Symmetry_Z && y > halfSize) continue;
                        if (symMask & Symmetry_Point && y == halfSize && x > halfSize) continue;
                        if (symMask & Symmetry_X && x > halfSize) continue;
                        if (symMask & Symmetry_XY && x > y) continue;
                        
                        float centerSlope = slopeMap.Get(x, y);
                        if (centerSlope < minGradSq || centerSlope > maxGradSq) continue;
                        
                        float h = heightmap.Get(x, y);
                        if (h < rule.MinHeight || h > rule.MaxHeight) continue;
                        
                        // Focus Gradient Rejection
                        if (rule.FocusGradient != Gradient_None && rule.FocusGradientRadius > 0.0f) {
                            float dx = (float)x - halfSize;
                            float dy = (float)y - halfSize;
                            float distVal = std::sqrt(dx*dx + dy*dy);
                            float prob = 1.0f;
                            if (rule.FocusGradient == Gradient_CenterFocus) {
                                prob = 1.0f - std::clamp(distVal / rule.FocusGradientRadius, 0.0f, 1.0f);
                            } else if (rule.FocusGradient == Gradient_EdgeFocus) {
                                prob = std::clamp(distVal / rule.FocusGradientRadius, 0.0f, 1.0f);
                            } else if (rule.FocusGradient == Gradient_Torus) {
                                float normDist = distVal / rule.FocusGradientRadius;
                                prob = 1.0f - 2.0f * std::abs(normDist - 0.5f);
                                prob = std::clamp(prob, 0.0f, 1.0f);
                            }
                            prob = std::pow(prob, rule.FocusGradientContrast) * rule.FocusGradientStrength;
                            uint32_t rejectSeed = params.Seed ^ (x * 38243) ^ (y * 94833) ^ attempts;
                            if ((float)(rejectSeed % 1000) / 1000.0f > prob) continue;
                        }
                        
                        float flatRadius = 0.0f;
                        if (params.FastPreviewMode) {
                            auto res = Math::ScoreRadialClearance_Stochastic(heightmap, x, y, rule.MinHeight, rule.MaxHeight, rule.AreaHeightRange, maxSearchRadius, (int)rule.AreaRadiusMin, params.Seed ^ x ^ y);
                            flatRadius = (float)res.first;
                        } else {
                            if (rule.AreaRadiusMin > 0.0f) flatRadius = jfaDistanceMap.Get(x, y);
                            else flatRadius = (float)maxSearchRadius;
                        }
                        
                        if (flatRadius < rule.AreaRadiusMin || (rule.CheckMaxRadius && flatRadius > rule.AreaRadiusMax)) continue;
                        
                        // Build clones
                        std::vector<Point2D> clones;
                        clones.push_back({x, y});
                        if (symMask & Symmetry_XY) clones.push_back({y, x});
                        int initC = (int)clones.size();
                        for (int i = 0; i < initC; ++i) {
                            if (symMask & Symmetry_X) clones.push_back({mapSize - clones[i].x - 1, clones[i].y});
                            if (symMask & Symmetry_Z) clones.push_back({clones[i].x, mapSize - clones[i].y - 1});
                            if (symMask & Symmetry_Point) clones.push_back({mapSize - clones[i].x - 1, mapSize - clones[i].y - 1});
                        }
                        if (symMask & Symmetry_Radial && params.SpawnPointCount > 1) {
                            float dx = (float)(x - halfSize);
                            float dy = (float)(y - halfSize);
                            float rad = std::sqrt(dx*dx + dy*dy);
                            if (rad > 0.0f) {
                                float ang = std::atan2(dy, dx);
                                float stepA = (2.0f * 3.14159265f) / (float)params.SpawnPointCount;
                                for (int s = 1; s < params.SpawnPointCount; ++s) {
                                    float ca = ang + stepA * s;
                                    int nx = halfSize + (int)(std::cos(ca) * rad);
                                    int ny = halfSize + (int)(std::sin(ca) * rad);
                                    if (nx >= 0 && nx < mapSize && ny >= 0 && ny < mapSize) clones.push_back({nx, ny});
                                }
                            }
                        }
                        
                        bool clonesValid = true;
                        for (const auto& c : clones) {
                            if (c.x < startBound || c.x >= endBound || c.y < startBound || c.y >= endBound) { clonesValid = false; break; }
                            bool tooClose = false;
                            for (const auto& p : placedPositions) {
                                float dx = (float)c.x - p.first;
                                float dy = (float)c.y - p.second;
                                if (dx*dx + dy*dy < clearanceSq) {
                                    tooClose = true;
                                    break;
                                }
                            }
                            if (tooClose) { clonesValid = false; break; }
                        }
                        
                        if (!clonesValid) continue;
                        
                        // Place clones
                        for (const auto& c : clones) {
                            if (placed >= targetCount && !rule.UseAllPositions) break;
                            MarkerTransform mt;
                            mt.Type = rule.Type;
                            mt.Position[0] = (float)c.x;
                            mt.Position[1] = heightmap.Get(c.x, c.y);
                            mt.Position[2] = (float)c.y;
                            
                            placedPositions.push_back({(float)c.x, (float)c.y});
                            std::string key = rule.Type + "_Procedural_" + std::to_string(placed);
                            inOutResult.GeneratedMarkers[key] = mt;
                            placed++;
                        }
                    }
                    continue; 
                }
                
                // GPU PATH: Compute Shader
                if (params.UseGPUMarkers) {
                    std::vector<int> gpuMask;
                    TerrainCompute::DispatchMarkers(params, rule, heightmap, slopeMap, gpuMask);
                    
                    for (int y = startBound; y < endBound; ++y) {
                        for (int x = startBound; x < endBound; ++x) {
                            if (gpuMask[y * mapSize + x] == 1) {
                                float flatRadius = 0.0f;
                                if (params.FastPreviewMode) {
                                    auto res = Math::ScoreRadialClearance_Stochastic(heightmap, x, y, rule.MinHeight, rule.MaxHeight, rule.AreaHeightRange, maxSearchRadius, (int)rule.AreaRadiusMin, params.Seed ^ x ^ y);
                                    flatRadius = (float)res.first;
                                } else {
                                    if (rule.AreaRadiusMin > 0.0f) flatRadius = jfaDistanceMap.Get(x, y);
                                    else flatRadius = (float)maxSearchRadius;
                                }
                                
                                if (flatRadius >= rule.AreaRadiusMin && (!rule.CheckMaxRadius || flatRadius <= rule.AreaRadiusMax)) {
                                    MarkerCandidate cand;
                                    cand.x = x; cand.y = y;
                                    cand.maxFlatRadius = (int)flatRadius;
                                    cand.variance = 0.0f;
                                    candidates.push_back(cand);
                                }
                            }
                        }
                    }
                }
                else {
                    // DETERMINISTIC PATH: AVX2 L1-Tiled Loop
                    int tileSize = 64; 
                    
                    #pragma omp parallel for collapse(2)
                for (int tileY = 0; tileY < mapSize; tileY += tileSize) {
                    for (int tileX = 0; tileX < mapSize; tileX += tileSize) {
                        
                        std::vector<MarkerCandidate> localCandidates;
                        int minStart = globalMaxRadius.load(std::memory_order_relaxed);
                        if (rule.Priority != MarkerPriority::Priority_LargestArea) {
                            minStart = (int)rule.AreaRadiusMin;
                        }
                        
                        for (int y = tileY; y < tileY + tileSize && y < endBound; ++y) {
                            if (y < startBound) continue;
                            
                            // Symmetry Culling for Y
                            if (symMask & Symmetry_Point && y > halfSize) continue;
                            if (symMask & Symmetry_Z && y > halfSize) continue;
                            
                            for (int x = tileX; x < tileX + tileSize && x < endBound; x += 8) {
                                if (x < startBound - 7) continue;
                                
                                // AVX2 Slope Pre-Cull
                                __m256 slopes = _mm256_loadu_ps((const float*)slopeMap.GetDataPtr() + y * mapSize + x);
                                __m256 maxS = _mm256_set1_ps(maxGradSq);
                                __m256 minS = _mm256_set1_ps(minGradSq);
                                
                                __m256 maskMax = _mm256_cmp_ps(slopes, maxS, _CMP_LE_OQ);
                                __m256 maskMin = _mm256_cmp_ps(slopes, minS, _CMP_GE_OQ);
                                __m256 valid = _mm256_and_ps(maskMax, maskMin);
                                
                                int bitmask = _mm256_movemask_ps(valid);
                                if (bitmask == 0) continue; 
                                
                                for (int i = 0; i < 8; ++i) {
                                    if ((bitmask & (1 << i)) != 0) {
                                        int px = x + i;
                                        if (px >= endBound || px < startBound) continue;
                                        
                                        // Symmetry Culling for X
                                        if (symMask & Symmetry_Point && y == halfSize && px > halfSize) continue;
                                        if (symMask & Symmetry_X && px > halfSize) continue;
                                        if (symMask & Symmetry_XY && px > y) continue;
                                        
                                        float h = heightmap.Get(px, y);
                                        if (h < rule.MinHeight || h > rule.MaxHeight) continue;
                                        
                                        // Focus Gradient Rejection
                                        if (rule.FocusGradient != Gradient_None && rule.FocusGradientRadius > 0.0f) {
                                            float dx = (float)px - halfSize;
                                            float dy = (float)y - halfSize;
                                            float distVal = std::sqrt(dx*dx + dy*dy);
                                            float prob = 1.0f;
                                            if (rule.FocusGradient == Gradient_CenterFocus) {
                                                prob = 1.0f - std::clamp(distVal / rule.FocusGradientRadius, 0.0f, 1.0f);
                                            } else if (rule.FocusGradient == Gradient_EdgeFocus) {
                                                prob = std::clamp(distVal / rule.FocusGradientRadius, 0.0f, 1.0f);
                                            } else if (rule.FocusGradient == Gradient_Torus) {
                                                float normDist = distVal / rule.FocusGradientRadius;
                                                prob = 1.0f - 2.0f * std::abs(normDist - 0.5f);
                                                prob = std::clamp(prob, 0.0f, 1.0f);
                                            }
                                            prob = std::pow(prob, rule.FocusGradientContrast) * rule.FocusGradientStrength;
                                            uint32_t rejectSeed = params.Seed ^ (px * 38243) ^ (y * 94833);
                                            if ((float)(rejectSeed % 1000) / 1000.0f > prob) continue;
                                        }
                                        
                                        float flatRadius = 0.0f;
                                        if (params.FastPreviewMode) {
                                            auto res = Math::ScoreRadialClearance_Stochastic(heightmap, px, y, rule.MinHeight, rule.MaxHeight, rule.AreaHeightRange, maxSearchRadius, minStart, params.Seed ^ px ^ y);
                                            flatRadius = (float)res.first;
                                        } else {
                                            if (rule.AreaRadiusMin > 0.0f) flatRadius = jfaDistanceMap.Get(px, y);
                                            else flatRadius = (float)maxSearchRadius;
                                        }
                                        
                                        if (flatRadius >= rule.AreaRadiusMin) {
                                            if (!rule.CheckMaxRadius || flatRadius <= rule.AreaRadiusMax) {
                                                localCandidates.push_back({px, y, (int)flatRadius, 0.0f});
                                                if (rule.Priority == MarkerPriority::Priority_LargestArea) {
                                                    int currentMax = globalMaxRadius.load(std::memory_order_relaxed);
                                                    while (flatRadius > currentMax && !globalMaxRadius.compare_exchange_weak(currentMax, (int)flatRadius)) {}
                                                    minStart = globalMaxRadius.load(std::memory_order_relaxed);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        
                        if (!localCandidates.empty()) {
                            std::lock_guard<std::mutex> lock(mtx);
                            candidates.insert(candidates.end(), localCandidates.begin(), localCandidates.end());
                            }
                        }
                    }
                } // End of CPU/GPU branch
                
                if (candidates.empty()) continue;
                
                // Sorting
                std::sort(candidates.begin(), candidates.end(), [&rule](const MarkerCandidate& a, const MarkerCandidate& b) {
                    if (rule.Priority == MarkerPriority::Priority_LargestArea) {
                        if (a.maxFlatRadius != b.maxFlatRadius) return a.maxFlatRadius > b.maxFlatRadius;
                    } else if (rule.Priority == MarkerPriority::Priority_SmallestArea) {
                        if (a.maxFlatRadius != b.maxFlatRadius) return a.maxFlatRadius < b.maxFlatRadius;
                    }
                    uint32_t hashA = (uint32_t)a.x * 73856093 ^ (uint32_t)a.y * 19349663;
                    uint32_t hashB = (uint32_t)b.x * 73856093 ^ (uint32_t)b.y * 19349663;
                    return hashA < hashB;
                });
                
                // Placement with Spatial Hash Grid
                int placed = 0;
                float cellSize = std::max(1.0f, rule.ClearanceSpacing);
                int gridW = (int)std::ceil(mapSize * Math::FastInv(cellSize));
                int gridH = (int)std::ceil(mapSize * Math::FastInv(cellSize));
                
                std::vector<int> head(gridW * gridH, -1);
                std::vector<int> next;
                std::vector<std::pair<float, float>> placedPositions;
                
                auto getCell = [&](float px, float py) {
                    int cx = std::clamp((int)(px * Math::FastInv(cellSize)), 0, gridW - 1);
                    int cy = std::clamp((int)(py * Math::FastInv(cellSize)), 0, gridH - 1);
                    return cy * gridW + cx;
                };
                
                auto addPoint = [&](float px, float py) {
                    int idx = (int)placedPositions.size();
                    placedPositions.push_back({px, py});
                    int c = getCell(px, py);
                    next.push_back(head[c]);
                    head[c] = idx;
                };
                
                for (const auto& kvp : inOutResult.GeneratedMarkers) {
                    addPoint(kvp.second.Position[0], kvp.second.Position[2]);
                }
                
                float clearanceSq = rule.ClearanceSpacing * rule.ClearanceSpacing;
                
                for (const auto& cand : candidates) {
                    if (placed >= targetCount && !rule.UseAllPositions) break;
                    
                    std::vector<Point2D> clones;
                    clones.push_back({cand.x, cand.y});
                    if (symMask & Symmetry_XY) clones.push_back({cand.y, cand.x});
                    int initC = (int)clones.size();
                    for (int i = 0; i < initC; ++i) {
                        if (symMask & Symmetry_X) clones.push_back({mapSize - clones[i].x - 1, clones[i].y});
                        if (symMask & Symmetry_Z) clones.push_back({clones[i].x, mapSize - clones[i].y - 1});
                        if (symMask & Symmetry_Point) clones.push_back({mapSize - clones[i].x - 1, mapSize - clones[i].y - 1});
                    }
                    if (symMask & Symmetry_Radial && params.SpawnPointCount > 1) {
                        float dx = (float)(cand.x - halfSize);
                        float dy = (float)(cand.y - halfSize);
                        float rad = std::sqrt(dx*dx + dy*dy);
                        if (rad > 0.0f) {
                            float ang = std::atan2(dy, dx);
                            float stepA = (2.0f * 3.14159265f) / (float)params.SpawnPointCount;
                            for (int s = 1; s < params.SpawnPointCount; ++s) {
                                float ca = ang + stepA * s;
                                int nx = halfSize + (int)(std::cos(ca) * rad);
                                int ny = halfSize + (int)(std::sin(ca) * rad);
                                if (nx >= 0 && nx < mapSize && ny >= 0 && ny < mapSize) clones.push_back({nx, ny});
                            }
                        }
                    }
                    
                    bool clonesValid = true;
                    for (const auto& c : clones) {
                        if (c.x < startBound || c.x >= endBound || c.y < startBound || c.y >= endBound) { clonesValid = false; break; }
                        
                        float cpx = (float)c.x;
                        float cpy = (float)c.y;
                        int ccx = std::clamp((int)(cpx * Math::FastInv(cellSize)), 0, gridW - 1);
                        int ccy = std::clamp((int)(cpy * Math::FastInv(cellSize)), 0, gridH - 1);
                        
                        bool cloneTooClose = false;
                        for (int ny = std::max(0, ccy - 1); ny <= std::min(gridH - 1, ccy + 1) && !cloneTooClose; ++ny) {
                            for (int nx = std::max(0, ccx - 1); nx <= std::min(gridW - 1, ccx + 1) && !cloneTooClose; ++nx) {
                                int pIdx = head[ny * gridW + nx];
                                while (pIdx != -1) {
                                    float dx = cpx - placedPositions[pIdx].first;
                                    float dy = cpy - placedPositions[pIdx].second;
                                    if (dx*dx + dy*dy < clearanceSq) {
                                        cloneTooClose = true;
                                        break;
                                    }
                                    pIdx = next[pIdx];
                                }
                            }
                        }
                        if (cloneTooClose) { clonesValid = false; break; }
                    }
                    
                    if (!clonesValid) continue;
                    
                    for (const auto& c : clones) {
                        if (placed >= targetCount && !rule.UseAllPositions) break;
                        MarkerTransform mt;
                        mt.Type = rule.Type;
                        mt.Position[0] = (float)c.x;
                        mt.Position[1] = heightmap.Get(c.x, c.y);
                        mt.Position[2] = (float)c.y;
                        
                        addPoint((float)c.x, (float)c.y);
                        std::string key = rule.Type + "_Procedural_" + std::to_string(placed);
                        inOutResult.GeneratedMarkers[key] = mt;
                        placed++;
                    }
                }
            }
        }
    }

} // namespace SanmapGen
