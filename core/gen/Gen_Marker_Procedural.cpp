#include "../TerrainGenerator.h"
#include "../Parameters.h"
#include "Gen_Marker_Procedural.h"
#include <random>
#include <omp.h>
#include <mutex>
#include <atomic>
#include <cmath>

namespace SanmapGen {

    struct MarkerCandidate {
            int x, y;
            int maxFlatRadius;
            float variance;
        };

    static std::pair<int, float> ScoreRadialClearance(const FloatMask& heightmap, int cx, int cy, float minSlope, float maxSlope, float minHeight, float maxHeight, float heightTolerance, int maxSearchRadius, int minStartRadius = 1) {
            float centerH = heightmap.Get(cx, cy);
            
            // Global height constraint for the center point
            if (centerH < minHeight || centerH > maxHeight) return {0, 0.0f};
            
            int mapSize = heightmap.GetWidth();
            float minH = centerH;
            float maxH = centerH;
            
            auto checkPerimeter = [&](int r) -> bool {
                if (r == 0) return true;
                bool ringValid = true;
                int bx = r;
                int by = 0;
                int err = 0;
                
                while (bx >= by) {
                    // Helper to check a pixel
                    auto check = [&](int px, int py) {
                        if (!ringValid || px < 0 || px >= mapSize || py < 0 || py >= mapSize) return;
                        
                        float h = heightmap.Get(px, py);
                        if (std::abs(h - centerH) > heightTolerance) {
                            ringValid = false;
                            return;
                        }
                        
                        float slope = std::abs(h - centerH) / (float)r * 100.0f;
                        if (slope < minSlope || slope > maxSlope) {
                            ringValid = false;
                            return;
                        }
                        
                        if (h < minH) minH = h;
                        if (h > maxH) maxH = h;
                    };
                    
                    check(cx + bx, cy + by); check(cx + by, cy + bx); check(cx - by, cy + bx); check(cx - bx, cy + by);
                    check(cx - bx, cy - by); check(cx - by, cy - bx); check(cx + by, cy - bx); check(cx + bx, cy - by);
                    
                    if (!ringValid) break;
                    
                    by += 1;
                    err += 1 + 2 * by;
                    if (2 * (err - bx) + 1 > 0) {
                        bx -= 1;
                        err += 1 - 2 * bx;
                    }
                }
                return ringValid;
            };
            
            int startR = minStartRadius > 0 ? minStartRadius : 1;
            if (!checkPerimeter(startR)) return {0, 0.0f};
            
            int low = startR;
            int high = maxSearchRadius;
            int step = startR;
            
            // Phase 1: Exponential growth (Try max, double it, double it)
            while (true) {
                int nextR = low + step;
                if (nextR > high) {
                    high = maxSearchRadius;
                    break; // Hit the max cap, switch to binary search
                }
                if (checkPerimeter(nextR)) {
                    low = nextR;
                    step *= 2;
                } else {
                    high = nextR - 1;
                    break; // Failed, we now know the bounds [low, nextR-1]
                }
            }
            
            // Phase 2: Binary Search (halve the step)
            while (low < high) {
                int mid = low + (high - low + 1) / 2; // Ceiling division
                if (checkPerimeter(mid)) {
                    low = mid;
                } else {
                    high = mid - 1;
                }
            }
            
            return {low, (maxH - minH)};
        }

void Gen_Marker_Procedural::GenerateProceduralMarkers(const GenerationParams& params, const FloatMask& heightmap, const FloatMask& slopeMap, GenerationResult& inOutResult) {
        int mapSize = heightmap.GetWidth();
        int halfSize = mapSize / 2;
        inOutResult.GeneratedMarkers.clear();
        
        std::mt19937 rng(params.Seed + 5000);
        
        // Loop over each marker rule
        for (const auto* rule_ptr : params.GetFlatMarkerRules()) {
            const auto& rule = *rule_ptr;
            // Calculate how many to place
            int targetCount = rule.Count;
            if (rule.UseAllPositions) {
                targetCount = INT_MAX;
            } else if (rule.UseDensity) {
                // Density: markers per 100x100 area
                float area100 = (float)mapSize * (float)mapSize / 10000.0f;
                targetCount = (int)(rule.Density * area100);
            }
            if (targetCount <= 0) continue;
            
            int symMask = rule.SymmetryUseGlobal ? params.GlobalSymmetryMask : rule.SymmetryMask;
            
            int maxSearchRadius = (int)std::ceil(rule.AreaRadiusMin);
            if (rule.CheckMaxRadius) {
                maxSearchRadius = std::max(maxSearchRadius, (int)std::ceil(rule.AreaRadiusMax));
            }
            if (rule.Priority == MarkerPriority::Priority_LargestArea) {
                maxSearchRadius = std::max(maxSearchRadius, 100); // Cap at 100 to prevent long stalls
            }
            
            std::vector<MarkerCandidate> candidates;
            std::mutex mtx;
            std::atomic<int> globalMaxRadius((int)std::ceil(rule.AreaRadiusMin));
            
            // 1. Multithreaded scoring of every pixel
            #pragma omp parallel for
            for (int y = 0; y < mapSize; ++y) {
                // For symmetric maps, we generally only generate in the top-left or top-half, 
                // depending on symmetry, to avoid duplicating clusters on the other side.
                if (symMask & Symmetry_Point && y > halfSize) continue;
                if (symMask & Symmetry_Z && y > halfSize) continue;
                
                std::vector<MarkerCandidate> localCandidates;
                
                for (int x = 0; x < mapSize; ++x) {
                    if (symMask & Symmetry_Point && y == halfSize && x > halfSize) continue;
                    if (symMask & Symmetry_X && x > halfSize) continue;
                    if (symMask & Symmetry_XY && x > y) continue;
                    
                    // Edge Padding Check
                    if (rule.MapEdgePadding > 0.0f) {
                        int pad = (int)std::ceil(rule.MapEdgePadding);
                        if (x < pad || x >= mapSize - pad || y < pad || y >= mapSize - pad) continue;
                    }
                    
                    // Deterministic Probability Gradient
                    if (rule.FocusGradient != Gradient_None) {
                        float dx = (float)(x - halfSize);
                        float dy = (float)(y - halfSize);
                        float dist = std::sqrt(dx*dx + dy*dy);
                        
                        float prob = 1.0f;
                        if (rule.FocusGradient == Gradient_CenterFocus) {
                            // High prob at center, degrades to edge
                            float norm = dist / rule.FocusGradientRadius;
                            if (norm > 1.0f) norm = 1.0f;
                            norm = std::pow(norm, rule.FocusGradientContrast);
                            prob = 1.0f - (norm * rule.FocusGradientStrength);
                        } else if (rule.FocusGradient == Gradient_EdgeFocus) {
                            // Low prob at center, high at edge
                            float norm = dist / rule.FocusGradientRadius;
                            if (norm > 1.0f) norm = 1.0f;
                            norm = std::pow(norm, rule.FocusGradientContrast);
                            float baseProb = 1.0f - norm;
                            prob = 1.0f - (baseProb * rule.FocusGradientStrength);
                        } else if (rule.FocusGradient == Gradient_Torus) {
                            float norm = std::abs(dist - rule.FocusGradientRadius) / rule.FocusGradientRadius;
                            if (norm > 1.0f) norm = 1.0f;
                            norm = std::pow(norm, rule.FocusGradientContrast);
                            prob = 1.0f - (norm * rule.FocusGradientStrength);
                        }
                        
                        prob = std::clamp(prob, 0.0f, 1.0f);
                        
                        // God-Tier Deterministic pseudo-random check
                        uint32_t hash = (uint32_t)x * 73856093 ^ (uint32_t)y * 19349663;
                        float hashProb = (float)(hash % 10000) / 10000.0f;
                        
                        if (prob <= 0.0f || hashProb >= prob) continue;
                    }
                    
                    int minStart = (rule.Priority == MarkerPriority::Priority_LargestArea) ? globalMaxRadius.load(std::memory_order_relaxed) : 1;
                    
                    auto [flatRadius, variance] = ScoreRadialClearance(heightmap, x, y, rule.MinSlope, rule.MaxSlope, rule.MinHeight, rule.MaxHeight, rule.AreaHeightRange, maxSearchRadius, minStart);
                    
                    if (flatRadius >= rule.AreaRadiusMin) {
                        if (!rule.CheckMaxRadius || flatRadius <= rule.AreaRadiusMax) {
                            localCandidates.push_back({x, y, flatRadius, variance});
                            
                            if (rule.Priority == MarkerPriority::Priority_LargestArea && flatRadius > minStart) {
                                int expected = minStart;
                                while (flatRadius > expected && !globalMaxRadius.compare_exchange_weak(expected, flatRadius, std::memory_order_relaxed)) {}
                            }
                        }
                    }
                }
                
                if (!localCandidates.empty()) {
                    std::lock_guard<std::mutex> lock(mtx);
                    candidates.insert(candidates.end(), localCandidates.begin(), localCandidates.end());
                }
            }
            
            if (candidates.empty()) continue;
            
            // 2. Sorting / Random Selection
            if (rule.RandomSelection) {
                std::shuffle(candidates.begin(), candidates.end(), rng);
            } else {
                std::sort(candidates.begin(), candidates.end(), [&rule](const MarkerCandidate& a, const MarkerCandidate& b) {
                    if (rule.Priority == MarkerPriority::Priority_LargestArea) {
                        if (a.maxFlatRadius != b.maxFlatRadius) return a.maxFlatRadius > b.maxFlatRadius;
                        if (a.variance != b.variance) return a.variance < b.variance;
                    } else if (rule.Priority == MarkerPriority::Priority_SmallestArea) {
                        if (a.maxFlatRadius != b.maxFlatRadius) return a.maxFlatRadius < b.maxFlatRadius;
                        if (a.variance != b.variance) return a.variance < b.variance;
                    } else {
                        // Least Variance
                        if (a.variance != b.variance) return a.variance < b.variance;
                        if (a.maxFlatRadius != b.maxFlatRadius) return a.maxFlatRadius > b.maxFlatRadius;
                    }
                    // Strict Determinism God-Tier Spatial Hash Tie-Breaker
                    // Breaks ties in a perfectly deterministic but organically scattered manner, preventing grid-artifacts
                    uint32_t hashA = (uint32_t)a.x * 73856093 ^ (uint32_t)a.y * 19349663;
                    uint32_t hashB = (uint32_t)b.x * 73856093 ^ (uint32_t)b.y * 19349663;
                    return hashA < hashB;
                });
            }
            
            // 3. Placement & NMS
            int placed = 0;
            
            // God-Tier DOD Spatial Hash Grid for O(1) clearance checking
            float cellSize = std::max(1.0f, rule.ClearanceSpacing);
            int gridW = (int)std::ceil(mapSize / cellSize);
            int gridH = (int)std::ceil(mapSize / cellSize);
            
            std::vector<int> head(gridW * gridH, -1);
            std::vector<int> next;
            std::vector<std::pair<float, float>> placedPositions; // Used for spacing checks
            
            auto getCell = [&](float px, float py) {
                int cx = std::clamp((int)(px / cellSize), 0, gridW - 1);
                int cy = std::clamp((int)(py / cellSize), 0, gridH - 1);
                return cy * gridW + cx;
            };
            
            auto addPoint = [&](float px, float py) {
                int idx = (int)placedPositions.size();
                placedPositions.push_back({px, py});
                int c = getCell(px, py);
                next.push_back(head[c]);
                head[c] = idx;
            };
            
            // Populate initial grid with previously placed markers
            // For now, let's just check against ALL generated markers to prevent stacking.
            for (const auto& kvp : inOutResult.GeneratedMarkers) {
                addPoint(kvp.second.Position[0], kvp.second.Position[2]);
            }
            
            float clearanceSq = rule.ClearanceSpacing * rule.ClearanceSpacing;
            
            for (const auto& cand : candidates) {
                if (placed >= targetCount) break;
                
                float px = (float)cand.x;
                float py = (float)cand.y;
                
                // Clearance check for primary point (O(1) Spatial Grid lookup)
                bool tooClose = false;
                int cx = std::clamp((int)(px / cellSize), 0, gridW - 1);
                int cy = std::clamp((int)(py / cellSize), 0, gridH - 1);
                
                for (int ny = std::max(0, cy - 1); ny <= std::min(gridH - 1, cy + 1) && !tooClose; ++ny) {
                    for (int nx = std::max(0, cx - 1); nx <= std::min(gridW - 1, cx + 1) && !tooClose; ++nx) {
                        int pIdx = head[ny * gridW + nx];
                        while (pIdx != -1) {
                            float dx = px - placedPositions[pIdx].first;
                            float dy = py - placedPositions[pIdx].second;
                            if (dx*dx + dy*dy < clearanceSq) {
                                tooClose = true;
                                break;
                            }
                            pIdx = next[pIdx];
                        }
                    }
                }
                if (tooClose) continue;
                
                // Create a list of mirrored points
                struct SymPt { int x, y; };
                std::vector<SymPt> clones;
                clones.push_back({cand.x, cand.y}); // Primary
                
                if (symMask & Symmetry_XY) {
                    clones.push_back({cand.y, cand.x});
                }
                
                size_t c_size = clones.size();
                for (size_t i = 0; i < c_size; ++i) {
                    if (symMask & Symmetry_X) clones.push_back({mapSize - clones[i].x - 1, clones[i].y});
                    if (symMask & Symmetry_Z) clones.push_back({clones[i].x, mapSize - clones[i].y - 1});
                    if (symMask & Symmetry_Point) clones.push_back({mapSize - clones[i].x - 1, mapSize - clones[i].y - 1});
                }
                
                // Add radial clones if spawn point count > 1
                if (symMask & Symmetry_Radial && params.SpawnPointCount > 1) {
                    float dx = (float)(cand.x - halfSize);
                    float dy = (float)(cand.y - halfSize);
                    float rad = std::sqrt(dx*dx + dy*dy);
                    float angle = std::atan2(dy, dx);
                    
                    for (int s = 1; s < params.SpawnPointCount; ++s) {
                        float rotAngle = angle + (3.14159f * 2.0f * (float)s) / (float)params.SpawnPointCount;
                        int cx_r = halfSize + (int)(std::cos(rotAngle) * rad);
                        int cy_r = halfSize + (int)(std::sin(rotAngle) * rad);
                        clones.push_back({cx_r, cy_r});
                    }
                }
                
                // Verify all clones are also valid and maintain spacing
                bool clonesValid = true;
                for (size_t i = 1; i < clones.size(); ++i) {
                    auto [flatRad, var] = ScoreRadialClearance(heightmap, clones[i].x, clones[i].y, rule.MinSlope, rule.MaxSlope, rule.MinHeight, rule.MaxHeight, rule.AreaHeightRange, maxSearchRadius);
                    if (flatRad < rule.AreaRadiusMin || (rule.CheckMaxRadius && flatRad > rule.AreaRadiusMax)) {
                        clonesValid = false;
                        break;
                    }
                    
                    // Check spacing for clone using Spatial Grid
                    float cpx = (float)clones[i].x;
                    float cpy = (float)clones[i].y;
                    
                    int ccx = std::clamp((int)(cpx / cellSize), 0, gridW - 1);
                    int ccy = std::clamp((int)(cpy / cellSize), 0, gridH - 1);
                    
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
                    if (cloneTooClose) {
                        clonesValid = false;
                        break;
                    }
                }
                
                if (!clonesValid) continue;
                
                // Place them!
                for (int i = 0; i < (int)clones.size(); ++i) {
                    MarkerTransform mt;
                    mt.Type = rule.Type;
                    mt.IsManual = false;
                    mt.Position[0] = (float)clones[i].x;
                    mt.Position[1] = heightmap.Get(clones[i].x, clones[i].y);
                    mt.Position[2] = (float)clones[i].y;
                    mt.IsValid = true;
                    mt.IsHidden = !rule.Enabled;
                    
                    addPoint(mt.Position[0], mt.Position[2]);
                    
                    std::string key = rule.Type + "_Procedural_" + std::to_string(placed) + "_" + std::to_string(i);
                    inOutResult.GeneratedMarkers[key] = mt;
                }
                
                placed += (int)clones.size();
            }
        }
    }

} // namespace SanmapGen
