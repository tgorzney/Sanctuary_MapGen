#include "Gen_NoiseAndBlend.h"
#include "../TerrainGenerator.h"
#include "../TerrainCompute.h"
#include "Gen_Noise.h"
#include "Gen_Mask_Height.h"
#include <algorithm>

namespace SanmapGen {

    void Gen_NoiseAndBlend::Process(FloatMask& outMap, std::vector<FloatMask>& Stratums, const GenerationParams& params, GenerationResult& inOutResult, size_t& outBlendHash) {
        int vertSize = params.MapSize + 1;
        uint32_t pow2Size = 1;
        while (pow2Size < (uint32_t)vertSize) pow2Size <<= 1;
        uint32_t totalMortonCells = pow2Size * pow2Size;
        
        auto flatLayers = params.GetFlatLayers();
        
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
                float effectiveFreq = layer.Frequency;
                if (params.ScaleFeaturesToMapSize && params.MapSize > 0) {
                    effectiveFreq *= (512.0f / (float)params.MapSize);
                }
                noise.SetFrequency(effectiveFreq);
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
        
        
        outBlendHash = currentBlendHash;
    }


} // namespace SanmapGen
   
 