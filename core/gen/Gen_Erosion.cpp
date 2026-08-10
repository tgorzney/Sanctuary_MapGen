#include "Gen_Erosion.h"
#include "../TerrainGenerator.h"
#include "Gen_Noise.h"
#include "Gen_Mask_Height.h"
#include "../TerrainCompute.h"
#include "../ErosionCompute.h"
#include "../ErosionSimulator.h"
#include "../math/Sanmath_SIMD.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace SanmapGen {

FloatMask Gen_Erosion::SymmetrizeErodedTerrain(const FloatMask& terrainMap, const NoiseLayer& layer, const GenerationParams& params) {
        int mapSize = terrainMap.GetWidth();
        int halfSize = mapSize / 2;
        int spawnCount = params.SpawnPointCount;
        int symMask = layer.SymmetryUseGlobal ? params.GlobalSymmetryMask : layer.SymmetryMask;
        
        FloatMask outMap(mapSize, mapSize, 0.0f);
        
        for (int py = 0; py < mapSize; ++py) {
            for (int px = 0; px < mapSize; ++px) {
                float dx = static_cast<float>(px - halfSize);
                float dy = static_cast<float>(py - halfSize);
                
                float coordsX[128];
                float coordsY[128];
                int count = 0;
                coordsX[count] = dx;
                coordsY[count] = dy;
                count++;
                
                if (symMask & Symmetry_Radial && spawnCount > 1) {
                    float wedgeAngle = (2.0f * 3.14159265f) / static_cast<float>(spawnCount);
                    float cosW = std::cos(wedgeAngle);
                    float sinW = std::sin(wedgeAngle);
                    
                    int oldCount = count;
                    for (int c = 0; c < oldCount; ++c) {
                        float rx = coordsX[c];
                        float ry = coordsY[c];
                        for (int i = 1; i < spawnCount; ++i) {
                            float nx = rx * cosW - ry * sinW;
                            float ny = rx * sinW + ry * cosW;
                            coordsX[count] = nx;
                            coordsY[count] = ny;
                            count++;
                            rx = nx;
                            ry = ny;
                        }
                    }
                }
                
                if (symMask & Symmetry_X) {
                    int oldCount = count;
                    for (int i = 0; i < oldCount; ++i) { coordsX[count] = -coordsX[i]; coordsY[count] = coordsY[i]; count++; }
                }
                if (symMask & Symmetry_Z) {
                    int oldCount = count;
                    for (int i = 0; i < oldCount; ++i) { coordsX[count] = coordsX[i]; coordsY[count] = -coordsY[i]; count++; }
                }
                if (symMask & Symmetry_Point) {
                    int oldCount = count;
                    for (int i = 0; i < oldCount; ++i) { coordsX[count] = -coordsX[i]; coordsY[count] = -coordsY[i]; count++; }
                }
                
                float finalVal = Gen_Noise::BilinearGet(terrainMap, coordsX[0] + halfSize, coordsY[0] + halfSize);
                for (int i = 1; i < count; ++i) {
                    finalVal += Gen_Noise::BilinearGet(terrainMap, coordsX[i] + halfSize, coordsY[i] + halfSize);
                }
                
                if (count > 1) {
                    finalVal /= static_cast<float>(count); // Pure average for erosion synchronization
                }
                
                outMap.Set(px, py, finalVal);
            }
        }
        return outMap;
    }


void Gen_Erosion::Process(FloatMask& outMap, std::vector<FloatMask>& Stratums, const GenerationParams& params, GenerationResult& inOutResult, size_t currentBlendHash, size_t& outErosionHash) {
        int vertSize = params.MapSize + 1;
        auto flatLayers = params.GetFlatLayers();
// --- Process Erosion Sequentially Layer-by-Layer ---
        size_t currentErosionHash = params.GetErosionHash(currentBlendHash);
        bool skipErosion = false;
        
        if (currentErosionHash == inOutResult.CachedErosionHash && inOutResult.CachedErodedMap.GetWidth() == vertSize) {
            skipErosion = true;
            outMap = inOutResult.CachedErodedMap;
            Stratums = inOutResult.CachedErodedStratums;
            inOutResult.MaterialMasks = inOutResult.CachedErodedMaterialMasks;
        }
        
        if (!skipErosion) {
            if (!params.FastPreviewMode) {
            for (size_t currentLayerIdx = 0; currentLayerIdx < flatLayers.size(); ++currentLayerIdx) {
                const auto& layer = *flatLayers[currentLayerIdx];
                if (!layer.Enabled || !flatLayers[currentLayerIdx]->Erosion.Enabled) continue;
                
                // Generate Rain/Precipitation Map based on this layer's settings
                FloatMask rainMap(vertSize, vertSize, 1.0f); // Default to uniform
                
                if (flatLayers[currentLayerIdx]->Erosion.UseRainNoise) {
                    FastNoiseLite rainNoise;
                    rainNoise.SetSeed(params.Seed + 9999 + currentLayerIdx);
                    rainNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
                    rainNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
                    rainNoise.SetFractalOctaves(flatLayers[currentLayerIdx]->Erosion.RainNoiseOctaves);
                    rainNoise.SetFrequency(flatLayers[currentLayerIdx]->Erosion.RainNoiseFreq);
                    
                    // Use the layer itself to route the noise through Gen_Noise::EvaluateSymmetricNoise
                    for(int y=0; y<vertSize; ++y) {
                        for(int x=0; x<vertSize; ++x) {
                            float n = (Gen_Noise::EvaluateSymmetricNoise(x, y, vertSize, rainNoise, layer, &params) + 1.0f) * 0.5f;
                            // Threshold mask
                            if (n < flatLayers[currentLayerIdx]->Erosion.RainNoiseThreshold) {
                                rainMap.Set(x, y, 0.0f);
                            } else {
                                float val = (n - flatLayers[currentLayerIdx]->Erosion.RainNoiseThreshold) / (1.0f - flatLayers[currentLayerIdx]->Erosion.RainNoiseThreshold);
                                rainMap.Set(x, y, val);
                            }
                        }
                    }
                }
                
                // Calculate TotalHeight for slope detection and spawn height filtering
                FloatMask totalHeight(vertSize, vertSize, 0.0f);
                for(int y=0; y<vertSize; ++y) {
                    for(int x=0; x<vertSize; ++x) {
                        float h = 0.0f;
                        for (size_t i = 0; i < flatLayers.size(); ++i) {
                            if (flatLayers[i]->Enabled) h += Stratums[i].Get(x, y);
                        }
                        totalHeight.Set(x, y, h);
                    }
                }
                
                // Apply Orographic Rain (Rain Shadows)
                if (flatLayers[currentLayerIdx]->Erosion.UseOrographicRain) {
                    float baseWindAngleRad = flatLayers[currentLayerIdx]->Erosion.WindAngle * (3.14159265f / 180.0f);
                    float baseWindX = std::cos(baseWindAngleRad);
                    float baseWindY = std::sin(baseWindAngleRad);
                    
                    int halfSize = vertSize / 2;
                    
                    for(int y=1; y<vertSize-1; ++y) {
                        for(int x=1; x<vertSize-1; ++x) {
                            float hX1 = totalHeight.Get(x-1, y);
                            float hX2 = totalHeight.Get(x+1, y);
                            float hY1 = totalHeight.Get(x, y-1);
                            float hY2 = totalHeight.Get(x, y+1);
                            
                            float normalX = (hX1 - hX2) * 0.5f;
                            float normalY = (hY1 - hY2) * 0.5f;
                            
                            // Calculate symmetry-aligned wind vector for this pixel
                            float localWindX = baseWindX;
                            float localWindY = baseWindY;
                            
                            // Apply legacy folding math for wind vectors to match terrain folding
                            int mx = x, my = y;
                            int effectiveSymMask = layer.SymmetryUseGlobal ? params.GlobalSymmetryMask : layer.SymmetryMask;
                            if (effectiveSymMask & Symmetry_X) { if (mx > halfSize) { mx = vertSize - mx - 1; localWindX = -localWindX; } }
                            if (effectiveSymMask & Symmetry_Z) { if (my > halfSize) { my = vertSize - my - 1; localWindY = -localWindY; } }
                            if (effectiveSymMask & Symmetry_XY) { if (mx > my) { std::swap(localWindX, localWindY); } }
                            if (effectiveSymMask & Symmetry_Point) { if (my > halfSize) { localWindX = -localWindX; localWindY = -localWindY; } }
                            
                            if (effectiveSymMask & Symmetry_Radial && params.SpawnPointCount > 1) {
                                float dx = static_cast<float>(x - halfSize);
                                float dy = static_cast<float>(y - halfSize);
                                float angle = std::atan2(dy, dx);
                                if (angle < 0.0f) angle += 2.0f * 3.14159265f;
                                float wedgeAngle = (2.0f * 3.14159265f) / static_cast<float>(params.SpawnPointCount);
                                
                                // Determine which wedge we are in
                                float wedgeIndex = std::floor(angle / wedgeAngle);
                                
                                // Rotate the wind vector backwards by (wedgeIndex * wedgeAngle) to align it
                                float rotAngle = -wedgeIndex * wedgeAngle;
                                float cosRot = std::cos(rotAngle);
                                float sinRot = std::sin(rotAngle);
                                float wx = localWindX * cosRot - localWindY * sinRot;
                                float wy = localWindX * sinRot + localWindY * cosRot;
                                localWindX = wx;
                                localWindY = wy;
                            }
                            
                            // Dot product with local wind
                            float slopeTowardsWind = (normalX * localWindX + normalY * localWindY);
                            
                            // Height multiplier: clouds drop more rain up high
                            float h = totalHeight.Get(x, y);
                            float heightMult = std::clamp(h * 2.0f, 0.5f, 1.5f);
                            
                            // If slope opposes wind (windward), slopeTowardsWind > 0
                            float orographicMult = 1.0f + (slopeTowardsWind * 100.0f); // Arbitrary tuning
                            orographicMult = std::clamp(orographicMult, 0.1f, 2.0f);
                            
                            rainMap.Set(x, y, rainMap.Get(x, y) * orographicMult * heightMult);
                        }
                    }
                }
                
                // Filter rain map by Spawn Height for Deposition mode
                if (flatLayers[currentLayerIdx]->Erosion.DepositionMode) {
                    for(int y=0; y<vertSize; ++y) {
                        for(int x=0; x<vertSize; ++x) {
                            float h = totalHeight.Get(x, y);
                            if (h < flatLayers[currentLayerIdx]->Erosion.SpawnMinHeight || h > flatLayers[currentLayerIdx]->Erosion.SpawnMaxHeight) {
                                rainMap.Set(x, y, 0.0f); // Cannot spawn outside height range
                            }
                        }
                    }
                }
                
                // Rejection Sampling to fill EXACTLY DropletCount drops
                std::vector<DropletSpawn> spawns;
                spawns.reserve(flatLayers[currentLayerIdx]->Erosion.DropletCount);
                
                std::mt19937 spawnGen(params.Seed + currentLayerIdx);
                std::uniform_real_distribution<float> distCoord(1.0f, static_cast<float>(vertSize - 2));
                std::uniform_real_distribution<float> distProb(0.0f, 1.0f);
                
                // Find max rain value to normalize rejection sampling
                float maxRain = 0.001f;
                for(int y=0; y<vertSize; ++y) {
                    for(int x=0; x<vertSize; ++x) {
                        maxRain = std::max(maxRain, rainMap.Get(x, y));
                    }
                }
                
                int safetyCounter = 0;
                while(spawns.size() < (size_t)flatLayers[currentLayerIdx]->Erosion.DropletCount) {
                    float px = distCoord(spawnGen);
                    float py = distCoord(spawnGen);
                    float prob = rainMap.Get((int)px, (int)py) / maxRain;
                    
                    if (distProb(spawnGen) <= prob) {
                        spawns.push_back({px, py});
                        safetyCounter = 0;
                    } else {
                        safetyCounter++;
                        if (safetyCounter > 1000000) {
                            // Rain map is completely empty, fallback to uniform
                            spawns.push_back({px, py});
                        }
                    }
                }
                
                if (params.WYSIWYGBaking) {
                    ErosionCompute::DispatchStratified(Stratums, spawns, flatLayers[currentLayerIdx]->Erosion, params, vertSize, currentLayerIdx);
                } else {
                    ErosionSimulator::SimulateStratifiedErosionDelta(Stratums, spawns, flatLayers[currentLayerIdx]->Erosion, params, vertSize, currentLayerIdx);
                }
                
                // Symmetrize the eroded stratums to fix divergent erosion paths for layers we touched
                for (size_t i = 0; i <= currentLayerIdx; ++i) {
                    int effectiveSymMask = (*flatLayers[i]).SymmetryUseGlobal ? params.GlobalSymmetryMask : (*flatLayers[i]).SymmetryMask;
                    if (flatLayers[i]->Enabled && effectiveSymMask != 0) {
                        Stratums[i] = SymmetrizeErodedTerrain(Stratums[i], (*flatLayers[i]), params);
                    }
                }
            }
        }
        
        // Sum the final eroded stratums to output the final heightmap
        #pragma omp parallel for
        for (int y = 0; y < vertSize; ++y) {
            for (int x = 0; x < vertSize; ++x) {
                float totalHeight = 0.0f;
                
        for (size_t i = 0; i < flatLayers.size(); ++i) {
                    if (flatLayers[i]->Enabled) {
                        totalHeight += Stratums[i].Get(x, y);
                    }
                }
                outMap.Set(x, y, std::clamp(totalHeight, 0.0f, 1.0f));
            }
        }
        
        if (params.SymAlgorithm == SymmetryAlgorithm::Blur && params.SymmetryBlurRadius > 0.0f) {
            int combinedMask = 0;
            for (const auto& l : params.GetFlatLayers()) combinedMask |= l->SymmetryMask;
            if (combinedMask != 0) Gen_Noise::ApplySymmetryBlur(outMap, vertSize, params.SymmetryBlurRadius, combinedMask, params.SpawnPointCount);
        }
        
            // Save cache
            if (!params.FastPreviewMode) {
                inOutResult.CachedErosionHash = currentErosionHash;
                inOutResult.CachedErodedMap = outMap;
                inOutResult.CachedErodedStratums = Stratums;
            } else {
                inOutResult.CachedErosionHash = 0; // Force recalculation on slider release
            }
            
            // --- Post-Erosion Top-Down Occlusion (AVX2 SIMD) ---
            #pragma omp parallel for
            for (int y = 0; y < vertSize; ++y) {
                // Process 8 pixels simultaneously using AVX2 SIMD registers
                for (int x = 0; x < vertSize; x += 8) {
                    // Check bounds just in case (though vertSize is usually padded for SIMD, or we just handle remainder)
                    if (x + 7 >= vertSize) {
                        // Fallback scalar loop for edge cases
                        for (int px = x; px < vertSize; ++px) {
                            float remainingVis = 1.0f;
                            for (int i = (int)flatLayers.size() - 1; i >= 0; --i) {
                                if (remainingVis <= 0.0f) break;
                                const auto& layer = *flatLayers[i];
                                if (!layer.Enabled) continue;
                                float t = Stratums[i].Get(px, y);
                                if (t > 0.0f) {
                                    float a = t * layer.HeightBlendContrast;
                                    float sMin = std::min(layer.HeightBlendMin, layer.HeightBlendMax);
                                    float sMax = std::max(layer.HeightBlendMin, layer.HeightBlendMax);
                                    if (sMin == sMax) sMax = sMin + 0.001f;
                                    a = std::clamp(a, sMin, sMax) * layer.Opacity;
                                    float contrib = std::min(a, remainingVis);
                                    int sIdx = std::clamp(layer.StratumIndex, 0, 8);
                                    inOutResult.MaterialMasks[sIdx].Set(px, y, inOutResult.MaterialMasks[sIdx].Get(px, y) + contrib);
                                    remainingVis -= contrib;
                                }
                            }
                            if (remainingVis > 0.0f && !flatLayers.empty()) {
                                int baseIdx = std::clamp(flatLayers[0]->StratumIndex, 0, 8);
                                inOutResult.MaterialMasks[baseIdx].Set(px, y, inOutResult.MaterialMasks[baseIdx].Get(px, y) + remainingVis);
                            }
                        }
                        break;
                    }
                    
                    __m256 remainingVisibility = _mm256_set1_ps(1.0f);
                    __m256 zeroVec = _mm256_setzero_ps();
                    
                    for (int i = (int)flatLayers.size() - 1; i >= 0; --i) {
                        if (SanmapGen::Math::CheckThreshold8_AVX(remainingVisibility, 0.0f) == 0xFF) break;
                        
                        const auto& layer = *flatLayers[i];
                        if (!layer.Enabled) continue;
                        
                        __m256 thickness = _mm256_loadu_ps(Stratums[i].GetDataPtr() + (y * vertSize + x));
                        __m256 validMask = _mm256_cmp_ps(thickness, zeroVec, _CMP_GT_OQ);
                        if (_mm256_movemask_ps(validMask) == 0) continue;
                        
                        __m256 alpha = _mm256_mul_ps(thickness, _mm256_set1_ps(layer.HeightBlendContrast));
                        float safeMin = std::min(layer.HeightBlendMin, layer.HeightBlendMax);
                        float safeMax = std::max(layer.HeightBlendMin, layer.HeightBlendMax);
                        if (safeMin == safeMax) safeMax = safeMin + 0.001f;
                        
                        alpha = _mm256_max_ps(_mm256_set1_ps(safeMin), _mm256_min_ps(alpha, _mm256_set1_ps(safeMax)));
                        alpha = _mm256_mul_ps(alpha, _mm256_set1_ps(layer.Opacity));
                        
                        __m256 contribution = _mm256_min_ps(alpha, remainingVisibility);
                        contribution = _mm256_blendv_ps(zeroVec, contribution, validMask);
                        
                        int sIdx = std::clamp(layer.StratumIndex, 0, 8);
                        __m256 currentMask = _mm256_loadu_ps(inOutResult.MaterialMasks[sIdx].GetDataPtr() + (y * vertSize + x));
                        _mm256_storeu_ps(inOutResult.MaterialMasks[sIdx].GetMutableDataPtr() + (y * vertSize + x), _mm256_add_ps(currentMask, contribution));
                        
                        remainingVisibility = _mm256_sub_ps(remainingVisibility, contribution);
                    }
                    
                    __m256 validRem = _mm256_cmp_ps(remainingVisibility, zeroVec, _CMP_GT_OQ);
                    if (_mm256_movemask_ps(validRem) != 0 && !flatLayers.empty()) {
                        int baseIdx = std::clamp(flatLayers[0]->StratumIndex, 0, 8);
                        __m256 currentMask = _mm256_loadu_ps(inOutResult.MaterialMasks[baseIdx].GetDataPtr() + (y * vertSize + x));
                        _mm256_storeu_ps(inOutResult.MaterialMasks[baseIdx].GetMutableDataPtr() + (y * vertSize + x), _mm256_add_ps(currentMask, remainingVisibility));
                    }
                }
            }
            
            // Handle Imported Masks (StaticOverride and ProceduralStart)
            for (int sIdx = 0; sIdx < 9; ++sIdx) {
                if (sIdx < params.Stratums.size()) {
                    const auto& stratum = params.Stratums[sIdx];
                    if (stratum.maskMode != ImportedMaskMode::Disabled && !stratum.importedMaskData.empty()) {
                        int maskSize = static_cast<int>(std::sqrt(stratum.importedMaskData.size()));
                        #pragma omp parallel for
                        for (int y = 0; y < vertSize; ++y) {
                            int sy = std::clamp(static_cast<int>((static_cast<float>(y) / params.MapSize) * (maskSize - 1)), 0, maskSize - 1);
                            for (int x = 0; x < vertSize; ++x) {
                                int sx = std::clamp(static_cast<int>((static_cast<float>(x) / params.MapSize) * (maskSize - 1)), 0, maskSize - 1);
                                float importedVal = stratum.importedMaskData[sy * maskSize + sx];
                                
                                if (stratum.maskMode == ImportedMaskMode::StaticOverride) {
                                    inOutResult.MaterialMasks[sIdx].Set(x, y, importedVal);
                                } else if (stratum.maskMode == ImportedMaskMode::ProceduralStart) {
                                    float currentMask = inOutResult.MaterialMasks[sIdx].Get(x, y);
                                    inOutResult.MaterialMasks[sIdx].Set(x, y, std::clamp(currentMask + importedVal, 0.0f, 1.0f));
                                }
                            }
                        }
                    }
                }
            }
            
            inOutResult.CachedErodedMaterialMasks = inOutResult.MaterialMasks;
        } // End !skipErosion
        
        inOutResult.Stratums = Stratums;
        
        float minH = 99999.0f, maxH = -99999.0f;
        int numPixels = vertSize * vertSize;
        const float* mapData = outMap.GetDataPtr();
        
        __m256 minVec = _mm256_set1_ps(99999.0f);
        __m256 maxVec = _mm256_set1_ps(-99999.0f);
        
        int i = 0;
        for (; i <= numPixels - 8; i += 8) {
            __m256 val = _mm256_loadu_ps(mapData + i);
            minVec = _mm256_min_ps(minVec, val);
            maxVec = _mm256_max_ps(maxVec, val);
        }
        
        float minArr[8];
        float maxArr[8];
        _mm256_storeu_ps(minArr, minVec);
        _mm256_storeu_ps(maxArr, maxVec);
        
        for (int j = 0; j < 8; ++j) {
            if (minArr[j] < minH) minH = minArr[j];
            if (maxArr[j] > maxH) maxH = maxArr[j];
        }
        
        for (; i < numPixels; ++i) {
            float h = mapData[i];
            if (h < minH) minH = h;
            if (h > maxH) maxH = h;
        }

        inOutResult.TerrainMinHeight = minH;
        inOutResult.TerrainMaxHeight = maxH;
        
        
        outErosionHash = currentErosionHash;
    }


} // namespace SanmapGen
