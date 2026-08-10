#include "gen/Gen_Erosion.h"
#include "gen/Gen_NoiseAndBlend.h"
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

    


    

    void TerrainGenerator::GenerateMap(FloatMask& outMap, const GenerationParams& params, GenerationResult& inOutResult) {
        int vertSize = params.MapSize + 1;
        outMap.Resize(vertSize, vertSize, 0.0f);
        
        std::vector<FloatMask> Stratums;
        auto flatLayers = params.GetFlatLayers();
        for (size_t i = 0; i < flatLayers.size(); ++i) {
            Stratums.push_back(FloatMask(vertSize, vertSize, 0.0f));
        }
        
        size_t currentBlendHash = 0;
        Gen_NoiseAndBlend::Process(outMap, Stratums, params, inOutResult, currentBlendHash);
        
        size_t currentErosionHash = 0;
        Gen_Erosion::Process(outMap, Stratums, params, inOutResult, currentBlendHash, currentErosionHash);
        
        size_t currentFlowHash = 0;
        ProcessFlow(outMap, params, inOutResult, currentErosionHash, currentFlowHash);
        
        ProcessPlacement(outMap, params, inOutResult, currentErosionHash, currentFlowHash);
    }



void TerrainGenerator::ProcessFlow(const FloatMask& outMap, const GenerationParams& params, GenerationResult& inOutResult, size_t currentErosionHash, size_t& outFlowHash) {
        int vertSize = params.MapSize + 1;
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

        
        outFlowHash = currentFlowHash;
    }

void TerrainGenerator::ProcessPlacement(const FloatMask& outMap, const GenerationParams& params, GenerationResult& inOutResult, size_t currentErosionHash, size_t currentFlowHash) {
        int vertSize = params.MapSize + 1;
size_t currentPlacementHash = params.GetPlacementHash(currentFlowHash);
        bool skipPlacement = false;
        
        if (currentPlacementHash == inOutResult.CachedPlacementHash) {
            skipPlacement = true;
            // GeneratedMarkers is already cached in inOutResult!
        }
        
        if (!skipPlacement) {
            if (params.FastPreviewMode) return;
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
