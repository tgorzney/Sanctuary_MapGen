#include "TerrainGenerator.h"
#include "gen/Gen_Noise.h"
#include "gen/Gen_Marker_Procedural.h"
#include "FastNoiseLite.h"
#include <random>
#include <future>
#include <thread>
#include <algorithm>
#include <cmath>
#include "ErosionSimulator.h"
#include "TerrainCompute.h"
#include "gen/Gen_Mask_Slope.h"
#include "gen/Gen_Marker_Placement.h"
#include "gen/Gen_Mask_Height.h"
#include "ErosionCompute.h"
#include "PlacementRules.h"
#include "math/Sanmath_SIMD.h"

namespace SanmapGen {

    // Helper: Insert a 0 bit after each of the 16 low bits of x
    inline uint32_t Part1By1(uint32_t x) {
        x &= 0x0000ffff;
        x = (x ^ (x <<  8)) & 0x00ff00ff;
        x = (x ^ (x <<  4)) & 0x0f0f0f0f;
        x = (x ^ (x <<  2)) & 0x33333333;
        x = (x ^ (x <<  1)) & 0x55555555;
        return x;
    }

    // Helper: Inverse of Part1By1
    inline uint32_t Compact1By1(uint32_t x) {
        x &= 0x55555555;
        x = (x ^ (x >>  1)) & 0x33333333;
        x = (x ^ (x >>  2)) & 0x0f0f0f0f;
        x = (x ^ (x >>  4)) & 0x00ff00ff;
        x = (x ^ (x >>  8)) & 0x0000ffff;
        return x;
    }

    

FloatMask TerrainGenerator::SymmetrizeErodedTerrain(const FloatMask& terrainMap, const NoiseLayer& layer, const GenerationParams& params) {
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

    

    void TerrainGenerator::GenerateMap(FloatMask& outMap, const GenerationParams& params, GenerationResult& inOutResult) {
        int vertSize = params.MapSize + 1;
        
        outMap.Resize(vertSize, vertSize, 0.0f);
        
        for (int y = 0; y < vertSize; ++y)
            for (int x = 0; x < vertSize; ++x)
                outMap.Set(x, y, 0.0f);
        
        uint32_t pow2Size = 1;
        while (pow2Size < (uint32_t)vertSize) pow2Size <<= 1;
        uint32_t totalMortonCells = pow2Size * pow2Size;
        
        std::vector<FloatMask> Stratums;
        auto flatLayers = params.GetFlatLayers();
        for (size_t i = 0; i < flatLayers.size(); ++i) {
            Stratums.push_back(FloatMask(vertSize, vertSize, 0.0f));
        }
        
        inOutResult.MaterialMasks.clear();
        for (size_t i = 0; i < 9; ++i) {
            inOutResult.MaterialMasks.push_back(FloatMask(vertSize, vertSize, 0.0f));
        }
        
        if (inOutResult.CachedRawNoise.size() != flatLayers.size() ||
            (!inOutResult.CachedRawNoise.empty() && inOutResult.CachedRawNoise[0].GetWidth() != vertSize)) {
            inOutResult.CachedRawNoise.clear();
            inOutResult.CachedNoiseHashes.clear();
            for (size_t i = 0; i < flatLayers.size(); ++i) {
                inOutResult.CachedRawNoise.push_back(FloatMask(vertSize, vertSize, 0.0f));
                inOutResult.CachedNoiseHashes.push_back(0);
            }
        }
        
        size_t currentBlendHash = params.GetBlendHash();
        bool skipBlending = false;
        
        if (currentBlendHash == inOutResult.CachedBlendHash && inOutResult.CachedBlendedMap.GetWidth() == vertSize) {
            skipBlending = true;
            outMap = inOutResult.CachedBlendedMap;
            Stratums = inOutResult.CachedBlendedStratums;
        }
        
        if (!skipBlending) {
            if (params.UseGPUTerrain) {
                TerrainCompute::DispatchTerrain(Stratums, params, inOutResult);
            } else {
            
        for (size_t i = 0; i < flatLayers.size(); ++i) {
            const auto& layer = *flatLayers[i];
            if (!layer.Enabled) continue;
            
            size_t layerHash = layer.GetNoiseHash(params.Seed + (int)i, params.GlobalSymmetryMask, (int)params.SymAlgorithm);
            FloatMask& layerMap = inOutResult.CachedRawNoise[i];
            
            if (inOutResult.CachedNoiseHashes[i] != layerHash) {
                FastNoiseLite noise;
                noise.SetSeed(params.Seed + i); // Distinct seed per layer gives better variation
                switch (layer.Type) {
                    case NoiseType::OpenSimplex2: noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2); break;
                    case NoiseType::OpenSimplex2S: noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S); break;
                    case NoiseType::Cellular: noise.SetNoiseType(FastNoiseLite::NoiseType_Cellular); break;
                    case NoiseType::Perlin: noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin); break;
                    case NoiseType::ValueCubic: noise.SetNoiseType(FastNoiseLite::NoiseType_ValueCubic); break;
                    case NoiseType::Value: noise.SetNoiseType(FastNoiseLite::NoiseType_Value); break;
                    case NoiseType::None: break;
                }
                switch (layer.Fractal) {
                    case FractalType::None: noise.SetFractalType(FastNoiseLite::FractalType_None); break;
                    case FractalType::FBm: noise.SetFractalType(FastNoiseLite::FractalType_FBm); break;
                    case FractalType::Ridged: noise.SetFractalType(FastNoiseLite::FractalType_Ridged); break;
                    case FractalType::PingPong: noise.SetFractalType(FastNoiseLite::FractalType_PingPong); break;
                }
                noise.SetFractalOctaves(layer.Octaves);
                noise.SetFractalGain(layer.Gain);
                noise.SetFractalPingPongStrength(layer.PingPongStrength);
                noise.SetFrequency(layer.Frequency);
                noise.SetCellularJitter(layer.CellularJitter);
                
                if (params.SymAlgorithm == SymmetryAlgorithm::NativeHash) {
                    bool symX = (layer.SymmetryMask & Symmetry_X) != 0;
                    bool symZ = (layer.SymmetryMask & Symmetry_Z) != 0;
                    bool symPoint = (layer.SymmetryMask & Symmetry_Point) != 0;
                    noise.SetNativeSymmetry(symX, symZ, symPoint);
                }
                
                Gen_Noise::ChunkTask task;
                task.StartZ = 0;
                task.EndZ = totalMortonCells;
                task.Params = &params;
                task.Layer = &layer;
                task.Noise = &noise;
                task.OutputMap = &layerMap;
                Gen_Noise::ProcessLayerChunk(task);
                
                inOutResult.CachedNoiseHashes[i] = layerHash;
            }
            
            // Calculate Height Blend (Thickness Mask) against underlying terrain
            #pragma omp parallel for
            for(int y=0; y<vertSize; ++y) {
                for(int x=0; x<vertSize; ++x) {
                    // Raw height generated by noise for this layer
                    float noiseVal = layerMap.Get(x, y);
                    
                    // Legacy Image Contrast and Brightness (Linear addition/subtraction and pivot)
                    noiseVal = (noiseVal - 0.5f) * layer.ImageContrast + 0.5f + layer.ImageBrightness;
                    noiseVal = std::clamp(noiseVal, 0.0f, 1.0f);
                    
                    // Post-process Shaping
                    noiseVal = noiseVal * (layer.LandDensity * 2.0f);
                    float origNoise = noiseVal;
                    if (layer.MountainDensity > 0.0f) {
                        float smooth = noiseVal * noiseVal * (3.0f - 2.0f * noiseVal);
                        noiseVal = (noiseVal * (1.0f - layer.MountainDensity)) + (smooth * layer.MountainDensity);
                        if (noiseVal > 0.5f) noiseVal += (noiseVal - 0.5f) * layer.MountainDensity;
                        noiseVal = std::clamp(noiseVal, 0.0f, 1.0f);
                    }
                    if (layer.PlateauDensity > 0.0f) {
                        float terraces = 3.0f + (layer.PlateauDensity * 27.0f); 
                        float terraceHeight = 1.0f / terraces;
                        noiseVal = std::floor(noiseVal / terraceHeight) * terraceHeight;
                    }
                    if (layer.RampDensity > 0.0f) {
                        noiseVal = (noiseVal * (1.0f - layer.RampDensity)) + (origNoise * layer.RampDensity);
                    }
                    
                    // Levels
                    float s = layer.LevelsShadows;
                    float h = layer.LevelsHighlights;
                    float m = layer.LevelsMidtones;
                    if (h > s) noiseVal = std::clamp((noiseVal - s) / (h - s), 0.0f, 1.0f);
                    else noiseVal = (noiseVal >= s) ? 1.0f : 0.0f;
                    if (m != 1.0f && m > 0.0f) noiseVal = std::pow(noiseVal, m);
                    noiseVal = layer.LevelsOutputBlack + noiseVal * (layer.LevelsOutputWhite - layer.LevelsOutputBlack);
                    noiseVal = std::clamp(noiseVal, 0.0f, 1.0f);
                    
                    float rawHeight = noiseVal;
                    
                    // Sum underlying terrain height
                    float currentTerrainHeight = 0.0f;
                    for (size_t prev = 0; prev < i; ++prev) {
                        if (flatLayers[prev]->Enabled) {
                            currentTerrainHeight += Stratums[prev].Get(x, y);
                        }
                    }
                    
                    // 1. Calculate protrusion thickness and add to stratum (Height Mask)
                    float thickness = 0.0f;
                    float mask = 0.0f;
                    Gen_Mask_Height::ApplyHeightMask(Stratums[i], rawHeight, currentTerrainHeight, layer, x, y, thickness, mask);
                    
                    // Note: MaterialMask computation moved to post-erosion AVX2 pass
                }
            }
        }
        } // End CPU/GPU split
        
            // Save cache
            inOutResult.CachedBlendHash = currentBlendHash;
            inOutResult.CachedBlendedMap = outMap;
            inOutResult.CachedBlendedStratums = Stratums;
        }
        
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
            inOutResult.CachedErosionHash = currentErosionHash;
            inOutResult.CachedErodedMap = outMap;
            inOutResult.CachedErodedStratums = Stratums;
            
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
                        int texSize = params.MapSize;
                        #pragma omp parallel for
                        for (int y = 0; y < vertSize; ++y) {
                            int sy = std::min(y, texSize - 1);
                            for (int x = 0; x < vertSize; ++x) {
                                int sx = std::min(x, texSize - 1);
                                float importedVal = stratum.importedMaskData[sy * texSize + sx];
                                
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
        
        size_t currentFlowHash = params.GetFlowHash(currentErosionHash);
        bool skipFlow = false;
        
        if (currentFlowHash == inOutResult.CachedFlowHash && inOutResult.CachedFlowMap.GetWidth() == vertSize) {
            skipFlow = true;
            inOutResult.FlowMap = inOutResult.CachedFlowMap;
            inOutResult.AccumulationMap = inOutResult.CachedAccumulationMap;
        }
        
        if (!skipFlow) {
            inOutResult.FlowMap = FloatMask(vertSize, vertSize, 0.0f);
            inOutResult.AccumulationMap = FloatMask(vertSize, vertSize, params.FlowSettingsParams.Precipitation * params.FlowSettingsParams.FlowVolumeMultiplier);
            
            if (!params.FastPreviewMode && !params.UseGPUFlowMap) {
                // --- GOD-TIER STOCHASTIC PROBABILITY MASK (AVX2 + OpenMP) ---
                const float* hMap = outMap.GetDataPtr();
                float* flowPtr = inOutResult.FlowMap.GetMutableDataPtr();
                float* accPtr = inOutResult.AccumulationMap.GetMutableDataPtr();
                
                float variance = params.FlowSettingsParams.StochasticVariance;
                float adherence = params.FlowSettingsParams.SlopeAdherence;
                int iters = params.FlowSettingsParams.Iterations;
                
                // We use Morton Z-Curve / Cache blocking
                int tileSize = 64;
                
                for (int iter = 0; iter < iters; ++iter) {
                    #pragma omp parallel for collapse(2)
                    for (int tileY = 0; tileY < vertSize; tileY += tileSize) {
                        for (int tileX = 0; tileX < vertSize; tileX += tileSize) {
                            
                            for (int y = tileY; y < tileY + tileSize && y < vertSize - 1; ++y) {
                                if (y < 1) continue;
                                for (int x = tileX; x < tileX + tileSize && x < vertSize - 1; x += 8) {
                                    if (x < 1) continue;
                                    
                                    // Make sure we don't overrun the edges
                                    if (x + 7 >= vertSize - 1) {
                                        // Scalar fallback for edges
                                        for (int px = x; px < vertSize - 1; ++px) {
                                            float h = hMap[y * vertSize + px];
                                            float maxDrop = 0.0f;
                                            int bestX = -1, bestY = -1;
                                            
                                            for(int dy = -1; dy <= 1; ++dy) {
                                                for(int dx = -1; dx <= 1; ++dx) {
                                                    if(dx == 0 && dy == 0) continue;
                                                    float nh = hMap[(y+dy)*vertSize + (px+dx)];
                                                    float drop = h - nh;
                                                    if (drop > 0.0f) {
                                                        // Stochastic Probability Roll
                                                        // Fake pseudo-random using coords and iter
                                                        uint32_t seed = (px * 73856093) ^ (y * 19349663) ^ (iter * 83492791);
                                                        float randVal = (float)(seed % 1000) / 1000.0f;
                                                        
                                                        float noiseImpact = variance * (1.0f - std::min(1.0f, drop * adherence));
                                                        float weight = drop + (randVal * noiseImpact * 10.0f);
                                                        
                                                        if (weight > maxDrop) {
                                                            maxDrop = weight;
                                                            bestX = px + dx;
                                                            bestY = y + dy;
                                                        }
                                                    }
                                                }
                                            }
                                            
                                            if (bestX != -1) {
                                                float transfer = accPtr[y*vertSize + px];
                                                // Atomic add for safety in parallel (simplification for scalar edge)
                                                #pragma omp atomic
                                                accPtr[bestY*vertSize + bestX] += transfer;
                                                #pragma omp atomic
                                                flowPtr[bestY*vertSize + bestX] += transfer + maxDrop;
                                            }
                                        }
                                        continue;
                                    }
                                    
                                    // SIMD execution for inner 8-pixel blocks
                                    __m256 h = _mm256_loadu_ps(&hMap[y * vertSize + x]);
                                    __m256 maxDropVec = _mm256_setzero_ps();
                                    __m256i bestXVec = _mm256_set1_epi32(-1);
                                    __m256i bestYVec = _mm256_set1_epi32(-1);
                                    
                                    // Pseudo-random vectorized generator
                                    __m256i xVec = _mm256_setr_epi32(x, x+1, x+2, x+3, x+4, x+5, x+6, x+7);
                                    __m256i yVec = _mm256_set1_epi32(y);
                                    __m256i iterVec = _mm256_set1_epi32(iter);
                                    
                                    __m256i seed1 = _mm256_mullo_epi32(xVec, _mm256_set1_epi32(73856093));
                                    __m256i seed2 = _mm256_mullo_epi32(yVec, _mm256_set1_epi32(19349663));
                                    __m256i seed3 = _mm256_mullo_epi32(iterVec, _mm256_set1_epi32(83492791));
                                    __m256i seed = _mm256_xor_si256(_mm256_xor_si256(seed1, seed2), seed3);
                                    
                                    // Poor man's stochastic noise in SIMD
                                    __m256i modVal = _mm256_set1_epi32(1000);
                                    // For lack of AVX2 integer modulo, we do a quick shift/mask for noise
                                    __m256i noiseInt = _mm256_and_si256(seed, _mm256_set1_epi32(1023));
                                    __m256 randValVec = _mm256_mul_ps(_mm256_cvtepi32_ps(noiseInt), _mm256_set1_ps(1.0f / 1024.0f));
                                    
                                    for(int dy = -1; dy <= 1; ++dy) {
                                        for(int dx = -1; dx <= 1; ++dx) {
                                            if(dx == 0 && dy == 0) continue;
                                            
                                            __m256 nh = _mm256_loadu_ps(&hMap[(y+dy) * vertSize + (x+dx)]);
                                            __m256 drop = _mm256_sub_ps(h, nh);
                                            
                                            __m256 dropValid = _mm256_cmp_ps(drop, _mm256_setzero_ps(), _CMP_GT_OQ);
                                            
                                            __m256 ad = _mm256_mul_ps(drop, _mm256_set1_ps(adherence));
                                            __m256 clampAd = _mm256_min_ps(_mm256_set1_ps(1.0f), _mm256_max_ps(_mm256_setzero_ps(), ad));
                                            __m256 noiseImpact = _mm256_mul_ps(_mm256_set1_ps(variance), _mm256_sub_ps(_mm256_set1_ps(1.0f), clampAd));
                                            
                                            // Modulate the noise based on X,Y,DX,DY
                                            __m256 modRand = _mm256_add_ps(randValVec, _mm256_set1_ps((float)(dx*17 + dy*31)*0.01f));
                                            modRand = _mm256_sub_ps(modRand, _mm256_floor_ps(modRand)); // Fract
                                            
                                            __m256 weight = _mm256_add_ps(drop, _mm256_mul_ps(_mm256_mul_ps(modRand, noiseImpact), _mm256_set1_ps(10.0f)));
                                            
                                            __m256 isGreater = _mm256_cmp_ps(weight, maxDropVec, _CMP_GT_OQ);
                                            __m256 updateMask = _mm256_and_ps(dropValid, isGreater);
                                            
                                            maxDropVec = _mm256_blendv_ps(maxDropVec, weight, updateMask);
                                            bestXVec = _mm256_blendv_epi8(bestXVec, _mm256_set1_epi32(dx), _mm256_castps_si256(updateMask));
                                            bestYVec = _mm256_blendv_epi8(bestYVec, _mm256_set1_epi32(dy), _mm256_castps_si256(updateMask));
                                        }
                                    }
                                    
                                    // Evaluate the results and dispatch accumulations
                                    int bestXArr[8];
                                    int bestYArr[8];
                                    float maxDropArr[8];
                                    _mm256_storeu_si256((__m256i*)bestXArr, bestXVec);
                                    _mm256_storeu_si256((__m256i*)bestYArr, bestYVec);
                                    _mm256_storeu_ps(maxDropArr, maxDropVec);
                                    
                                    for (int i = 0; i < 8; ++i) {
                                        if (bestXArr[i] != -1) {
                                            int bx = x + i + bestXArr[i];
                                            int by = y + bestYArr[i];
                                            float transfer = accPtr[y*vertSize + x + i];
                                            
                                            #pragma omp atomic
                                            accPtr[by*vertSize + bx] += transfer;
                                            #pragma omp atomic
                                            flowPtr[by*vertSize + bx] += transfer + maxDropArr[i];
                                        }
                                    }
                                }
                            }
                        }
                    }
                } // end iter
            } // end CPU flow
            
            // Normalize Velocity and Accumulation maps for rendering
            float maxVel = 0.001f;
            float maxAcc = 0.001f;
            size_t numPixels = vertSize * vertSize;
            float* flowPtr = inOutResult.FlowMap.GetMutableDataPtr();
            float* accPtr = inOutResult.AccumulationMap.GetMutableDataPtr();
            
            for (size_t i = 0; i < numPixels; ++i) {
                if (flowPtr[i] > maxVel) maxVel = flowPtr[i];
                if (accPtr[i] > maxAcc) maxAcc = accPtr[i];
            }
            for (size_t i = 0; i < numPixels; ++i) {
                flowPtr[i] /= maxVel;
                accPtr[i] /= maxAcc;
            }
            
            inOutResult.CachedFlowHash = currentFlowHash;
            inOutResult.CachedFlowMap = inOutResult.FlowMap;
            inOutResult.CachedAccumulationMap = inOutResult.AccumulationMap;
        } // End skipFlow

        size_t currentPlacementHash = params.GetPlacementHash(currentFlowHash);
        bool skipPlacement = false;
        
        if (currentPlacementHash == inOutResult.CachedPlacementHash) {
            skipPlacement = true;
            // GeneratedMarkers is already cached in inOutResult!
        }
        
        if (!skipPlacement) {
            // 1. Calculate slopemap for procedural rules (Cached)
            if (inOutResult.CachedSlopeHash != currentErosionHash || inOutResult.CachedSlopeMap.GetWidth() != vertSize) {
                inOutResult.CachedSlopeMap.Resize(vertSize, vertSize, 0.0f);
                Gen_Mask_Slope::GenerateSlopeMap(outMap, inOutResult.CachedSlopeMap, &inOutResult);
                inOutResult.CachedSlopeHash = currentErosionHash;
            }
            
            // 2. Generate Procedural Markers
            inOutResult.GeneratedMarkers.clear();
            if (params.EnableProceduralMarkers) {
                Gen_Marker_Procedural::GenerateProceduralMarkers(params, outMap, inOutResult.CachedSlopeMap, inOutResult);
            }
            
            inOutResult.CachedPlacementHash = currentPlacementHash;
        }
    }

    

    

    // Returns {MaxFlatRadius, Variance}

} // namespace SanmapGen
