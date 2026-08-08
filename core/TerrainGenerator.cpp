#include "TerrainGenerator.h"
#include "FastNoiseLite.h"
#include <random>
#include <future>
#include <thread>
#include <algorithm>
#include <cmath>
#include "ErosionSimulator.h"
#include "TerrainCompute.h"
#include "ErosionCompute.h"
#include "PlacementRules.h"

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

    uint32_t TerrainGenerator::EncodeMorton2D(uint32_t x, uint32_t y) {
        return (Part1By1(y) << 1) + Part1By1(x);
    }

    void TerrainGenerator::DecodeMorton2D(uint32_t code, uint32_t& x, uint32_t& y) {
        x = Compact1By1(code);
        y = Compact1By1(code >> 1);
    }

    float TerrainGenerator::EvaluateSymmetricNoise(int px, int py, int mapSize, FastNoiseLite& noise, const NoiseLayer& layer, const GenerationParams* params) {
        int halfSize = mapSize / 2;
        int spawnCount = params->SpawnPointCount;
        int symMask = layer.SymmetryUseGlobal ? params->GlobalSymmetryMask : layer.SymmetryMask;
        SymmetryAlgorithm alg = params->SymAlgorithm;
        
        // Helper lambda for legacy 2D folding
        auto fold2D = [&](int& mx, int& my) {
            if (symMask & Symmetry_X) { if (mx > halfSize) mx = mapSize - mx - 1; }
            if (symMask & Symmetry_Z) { if (my > halfSize) my = mapSize - my - 1; }
            if (symMask & Symmetry_XY) { if (mx > my) { int temp = mx; mx = my; my = temp; } }
            if (symMask & Symmetry_Point) { if (my > halfSize) { mx = mapSize - mx - 1; my = mapSize - my - 1; } }
            
            if (symMask & Symmetry_Radial && spawnCount > 1) {
                float dx = static_cast<float>(mx - halfSize);
                float dy = static_cast<float>(my - halfSize);
                float radius = std::sqrt(dx*dx + dy*dy);
                float angle = std::atan2(dy, dx);
                if (angle < 0.0f) angle += 2.0f * 3.14159265f;
                float wedgeAngle = (2.0f * 3.14159265f) / static_cast<float>(spawnCount);
                float foldedAngle = std::fmod(angle, wedgeAngle);
                dx = radius * std::cos(foldedAngle);
                dy = radius * std::sin(foldedAngle);
                mx = halfSize + static_cast<int>(std::round(dx));
                my = halfSize + static_cast<int>(std::round(dy));
            }
        };

        if (alg == SymmetryAlgorithm::NativeHash) {
            float dx = static_cast<float>(px - halfSize);
            float dy = static_cast<float>(py - halfSize);
            return noise.GetNoise(dx, dy);
        }
        else if (alg == SymmetryAlgorithm::Superposition) {
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
                for (int i = 0; i < oldCount; ++i) {
                    coordsX[count] = -coordsX[i];
                    coordsY[count] = coordsY[i];
                    count++;
                }
            }
            if (symMask & Symmetry_Z) {
                int oldCount = count;
                for (int i = 0; i < oldCount; ++i) {
                    coordsX[count] = coordsX[i];
                    coordsY[count] = -coordsY[i];
                    count++;
                }
            }
            if (symMask & Symmetry_Point) {
                int oldCount = count;
                for (int i = 0; i < oldCount; ++i) {
                    coordsX[count] = -coordsX[i];
                    coordsY[count] = -coordsY[i];
                    count++;
                }
            }
            
            float finalVal = noise.GetNoise(coordsX[0], coordsY[0]);
            for (int i = 1; i < count; ++i) {
                float val = noise.GetNoise(coordsX[i], coordsY[i]);
                switch (params->SymSuperpositionBlend) {
                    case BlendMode::Add: finalVal += val; break;
                    case BlendMode::Subtract: finalVal -= std::abs(val); break;
                    case BlendMode::Multiply: finalVal *= val; break;
                    case BlendMode::Max: finalVal = std::max(finalVal, val); break;
                    case BlendMode::Min: finalVal = std::min(finalVal, val); break;
                    case BlendMode::Overlay:
                        float norm1 = (finalVal + 1.0f) * 0.5f;
                        float norm2 = (val + 1.0f) * 0.5f;
                        float res = (norm1 < 0.5f) ? (2.0f * norm1 * norm2) : (1.0f - 2.0f * (1.0f - norm1) * (1.0f - norm2));
                        finalVal = (res * 2.0f) - 1.0f;
                        break;
                }
            }
            
            if (params->SymSuperpositionBlend == BlendMode::Add && count > 1) {
                finalVal /= std::sqrt(static_cast<float>(count));
            }
            return finalVal;
        }
        else if (alg == SymmetryAlgorithm::Cylinder3D || alg == SymmetryAlgorithm::Torus3D) {
            float dx = static_cast<float>(px - halfSize);
            float dy = static_cast<float>(py - halfSize);
            float radius = std::sqrt(dx*dx + dy*dy);
            float angle = std::atan2(dy, dx);
            if (angle < 0.0f) angle += 2.0f * 3.14159265f;
            
            if (symMask & Symmetry_Radial && spawnCount > 1) {
                float wrapAngle = angle * static_cast<float>(spawnCount);
                
                if (alg == SymmetryAlgorithm::Cylinder3D) {
                    float cylRadius = (mapSize / 2.0f);
                    float outX = cylRadius * std::cos(wrapAngle);
                    float outY = cylRadius * std::sin(wrapAngle);
                    float outZ = radius * params->CylinderZScale;
                    return noise.GetNoise(outX, outY, outZ);
                } 
                else if (alg == SymmetryAlgorithm::Torus3D) {
                    float minorAngle = (radius / static_cast<float>(halfSize)) * 3.14159265f;
                    float R = params->TorusMajorRadius;
                    float r = params->TorusMinorRadius;
                    float outX = (R + r * std::cos(minorAngle)) * std::cos(wrapAngle);
                    float outY = (R + r * std::cos(minorAngle)) * std::sin(wrapAngle);
                    float outZ = r * std::sin(minorAngle);
                    return noise.GetNoise(outX, outY, outZ);
                }
            }
            return noise.GetNoise(static_cast<float>(px), static_cast<float>(py), 0.0f);
        }
        else if (alg == SymmetryAlgorithm::CrossFade) {
            if ((symMask & Symmetry_Radial) && spawnCount > 1) {
                float dx = static_cast<float>(px - halfSize);
                float dy = static_cast<float>(py - halfSize);
                float radius = std::sqrt(dx*dx + dy*dy);
                float angle = std::atan2(dy, dx);
                if (angle < 0.0f) angle += 2.0f * 3.14159265f;
                
                float wedgeAngle = (2.0f * 3.14159265f) / static_cast<float>(spawnCount);
                float foldedAngle = std::fmod(angle, wedgeAngle);
                
                float dx1 = radius * std::cos(foldedAngle);
                float dy1 = radius * std::sin(foldedAngle);
                int mx1 = halfSize + static_cast<int>(std::round(dx1));
                int my1 = halfSize + static_cast<int>(std::round(dy1));
                if (symMask & Symmetry_X) { if (mx1 > halfSize) mx1 = mapSize - mx1 - 1; }
                if (symMask & Symmetry_Z) { if (my1 > halfSize) my1 = mapSize - my1 - 1; }
                float noise1 = noise.GetNoise(static_cast<float>(mx1), static_cast<float>(my1), 0.0f);
                
                float blendWidth = params->CrossFadeWidth;
                if (foldedAngle < blendWidth || foldedAngle > wedgeAngle - blendWidth) {
                    float altAngle = (foldedAngle < blendWidth) ? (foldedAngle + wedgeAngle) : (foldedAngle - wedgeAngle);
                    float dx2 = radius * std::cos(altAngle);
                    float dy2 = radius * std::sin(altAngle);
                    int mx2 = halfSize + static_cast<int>(std::round(dx2));
                    int my2 = halfSize + static_cast<int>(std::round(dy2));
                    if (symMask & Symmetry_X) { if (mx2 > halfSize) mx2 = mapSize - mx2 - 1; }
                    if (symMask & Symmetry_Z) { if (my2 > halfSize) my2 = mapSize - my2 - 1; }
                    float noise2 = noise.GetNoise(static_cast<float>(mx2), static_cast<float>(my2), 0.0f);
                    
                    float dist = (foldedAngle < blendWidth) ? foldedAngle : (wedgeAngle - foldedAngle);
                    float t = dist / blendWidth;
                    t = t * t * (3.0f - 2.0f * t);
                    return (noise1 * t) + (noise2 * (1.0f - t));
                }
                return noise1;
            } else {
                // Cartesian Cross-Fade (X, Z, Point)
                float blendWidth = params->CrossFadeWidth * mapSize; // Scale width to map size
                float dx = static_cast<float>(px - halfSize);
                float dy = static_cast<float>(py - halfSize);
                
                float wX = 1.0f, wZ = 1.0f;
                int mx1 = px, my1 = py;
                int mx2 = px, my2 = py;
                
                bool needsBlend = false;
                
                if (symMask & Symmetry_X) {
                    mx1 = halfSize + std::abs(px - halfSize);
                    mx2 = halfSize - std::abs(px - halfSize);
                    float dist = std::abs(dx);
                    if (dist < blendWidth) {
                        wX = dist / blendWidth;
                        wX = wX * wX * (3.0f - 2.0f * wX); // smoothstep
                        needsBlend = true;
                    }
                }
                
                if (symMask & Symmetry_Z) {
                    my1 = halfSize + std::abs(py - halfSize);
                    my2 = halfSize - std::abs(py - halfSize);
                    float dist = std::abs(dy);
                    if (dist < blendWidth) {
                        wZ = dist / blendWidth;
                        wZ = wZ * wZ * (3.0f - 2.0f * wZ); // smoothstep
                        needsBlend = true;
                    }
                }
                
                if (symMask & Symmetry_Point) {
                    // Fold everything into one quadrant
                    mx1 = halfSize + std::abs(px - halfSize);
                    my1 = halfSize + std::abs(py - halfSize);
                    mx2 = halfSize - std::abs(px - halfSize);
                    my2 = halfSize - std::abs(py - halfSize);
                    
                    float dist = std::sqrt(dx*dx + dy*dy);
                    if (dist < blendWidth) {
                        wX = dist / blendWidth;
                        wX = wX * wX * (3.0f - 2.0f * wX);
                        wZ = wX; // uniform blend across the origin
                        needsBlend = true;
                    }
                }
                
                if (needsBlend) {
                    float n1 = noise.GetNoise(static_cast<float>(mx1), static_cast<float>(my1), 0.0f);
                    float n2 = noise.GetNoise(static_cast<float>(mx2), static_cast<float>(my2), 0.0f);
                    // Use minimum weight to blend near origin
                    float t = std::min(wX, wZ);
                    return (n1 * t) + (n2 * (1.0f - t));
                } else {
                    return noise.GetNoise(static_cast<float>(mx1), static_cast<float>(my1), 0.0f);
                }
            }
        } 
        else {
            // Fold or Blur
            int mx = px;
            int my = py;
            fold2D(mx, my);
            return noise.GetNoise(static_cast<float>(mx), static_cast<float>(my), 0.0f);
        }
    }

    float TerrainGenerator::BilinearGet(const FloatMask& map, float x, float y) {
        int mapSize = map.GetWidth();
        int x0 = std::clamp(static_cast<int>(x), 0, mapSize - 1);
        int y0 = std::clamp(static_cast<int>(y), 0, mapSize - 1);
        int x1 = std::clamp(x0 + 1, 0, mapSize - 1);
        int y1 = std::clamp(y0 + 1, 0, mapSize - 1);
        
        float u = x - static_cast<float>(x0);
        float v = y - static_cast<float>(y0);
        
        float h00 = map.Get(x0, y0);
        float h10 = map.Get(x1, y0);
        float h01 = map.Get(x0, y1);
        float h11 = map.Get(x1, y1);
        
        return h00 * (1.0f - u) * (1.0f - v) + h10 * u * (1.0f - v) + h01 * (1.0f - u) * v + h11 * u * v;
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
                
                float finalVal = BilinearGet(terrainMap, coordsX[0] + halfSize, coordsY[0] + halfSize);
                for (int i = 1; i < count; ++i) {
                    finalVal += BilinearGet(terrainMap, coordsX[i] + halfSize, coordsY[i] + halfSize);
                }
                
                if (count > 1) {
                    finalVal /= static_cast<float>(count); // Pure average for erosion synchronization
                }
                
                outMap.Set(px, py, finalVal);
            }
        }
        return outMap;
    }

    void TerrainGenerator::ProcessLayerChunk(ChunkTask task) {
        int vertSize = task.Params->MapSize + 1;
        const NoiseLayer& layer = *task.Layer;
        
        #pragma omp parallel for
        for (long long z = task.StartZ; z < task.EndZ; ++z) {
            uint32_t px, py;
            DecodeMorton2D(static_cast<uint32_t>(z), px, py);
            
            if (px >= (uint32_t)vertSize || py >= (uint32_t)vertSize) continue;
            
            float noiseVal = 0.0f;
            
            if (layer.UseImage && !layer.ImageData.empty() && layer.ImageWidth > 0 && layer.ImageHeight > 0) {
                // Map the current pixel (px, py) to the image space (0 to ImageWidth-1)
                float u = static_cast<float>(px) / static_cast<float>(vertSize);
                float v = static_cast<float>(py) / static_cast<float>(vertSize);
                float imgX = u * static_cast<float>(layer.ImageWidth - 1);
                float imgY = v * static_cast<float>(layer.ImageHeight - 1);
                
                int x0 = std::clamp(static_cast<int>(imgX), 0, layer.ImageWidth - 1);
                int y0 = std::clamp(static_cast<int>(imgY), 0, layer.ImageHeight - 1);
                int x1 = std::clamp(x0 + 1, 0, layer.ImageWidth - 1);
                int y1 = std::clamp(y0 + 1, 0, layer.ImageHeight - 1);
                
                float fracX = imgX - static_cast<float>(x0);
                float fracY = imgY - static_cast<float>(y0);
                
                float h00 = layer.ImageData[y0 * layer.ImageWidth + x0];
                float h10 = layer.ImageData[y0 * layer.ImageWidth + x1];
                float h01 = layer.ImageData[y1 * layer.ImageWidth + x0];
                float h11 = layer.ImageData[y1 * layer.ImageWidth + x1];
                
                noiseVal = h00 * (1.0f - fracX) * (1.0f - fracY) + 
                           h10 * fracX * (1.0f - fracY) + 
                           h01 * (1.0f - fracX) * fracY + 
                           h11 * fracX * fracY;
            } else {
                if (layer.Type == NoiseType::None) {
                    noiseVal = 0.0f;
                } else {
                    noiseVal = (EvaluateSymmetricNoise(px, py, vertSize, *task.Noise, layer, task.Params) + 1.0f) * 0.5f;
                }
            }
            
            task.OutputMap->Set(px, py, noiseVal);
        }
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
            inOutResult.MaterialMasks = inOutResult.CachedBlendedMaterialMasks;
        }
        
        if (!skipBlending) {
            if (params.UseGPUTerrain) {
                TerrainCompute::DispatchTerrain(Stratums, params);
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
                
                ChunkTask task;
                task.StartZ = 0;
                task.EndZ = totalMortonCells;
                task.Params = &params;
                task.Layer = &layer;
                task.Noise = &noise;
                task.OutputMap = &layerMap;
                ProcessLayerChunk(task);
                
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
                    
                    // The difference in height determines if this layer protrudes above the previous
                    float thickness = rawHeight - currentTerrainHeight;
                    
                    if (thickness > 0.0f) {
                        float mask = thickness * layer.HeightBlendContrast;
                        float safeMin = std::min(layer.HeightBlendMin, layer.HeightBlendMax);
                        float safeMax = std::max(layer.HeightBlendMin, layer.HeightBlendMax);
                        if (safeMin == safeMax) safeMax = safeMin + 0.001f; // Prevent completely identical bounds if other code relies on it
                        mask = std::clamp(mask, safeMin, safeMax);
                        // Pure linear geometry addition
                        float heightDelta = thickness * layer.Opacity;
                        Stratums[i].Set(x, y, heightDelta);
                        
                        // Material texturing blend mask
                        float finalMask = mask * layer.Opacity;
                        if (layer.StratumIndex < params.Stratums.size() && params.Stratums[layer.StratumIndex].UseImportedMask && !params.Stratums[layer.StratumIndex].ImportedMaskData.empty()) {
                            int texSize = params.MapSize;
                            int sx = std::min(x, texSize - 1);
                            int sy = std::min(y, texSize - 1);
                            finalMask = params.Stratums[layer.StratumIndex].ImportedMaskData[sy * texSize + sx];
                        }
                        
                        int sIdx = std::clamp(layer.StratumIndex, 0, 8);
                        float currentMask = inOutResult.MaterialMasks[sIdx].Get(x, y);
                        inOutResult.MaterialMasks[sIdx].Set(x, y, std::clamp(currentMask + finalMask, 0.0f, 1.0f));
                    } else {
                        Stratums[i].Set(x, y, 0.0f);
                        
                        if (layer.StratumIndex < params.Stratums.size() && params.Stratums[layer.StratumIndex].UseImportedMask && !params.Stratums[layer.StratumIndex].ImportedMaskData.empty()) {
                            int texSize = params.MapSize;
                            int sx = std::min(x, texSize - 1);
                            int sy = std::min(y, texSize - 1);
                            float finalMask = params.Stratums[layer.StratumIndex].ImportedMaskData[sy * texSize + sx];
                            
                            int sIdx = std::clamp(layer.StratumIndex, 0, 8);
                            float currentMask = inOutResult.MaterialMasks[sIdx].Get(x, y);
                            inOutResult.MaterialMasks[sIdx].Set(x, y, std::clamp(currentMask + finalMask, 0.0f, 1.0f));
                        }
                    }
                }
            }
        }
        } // End CPU/GPU split
        
            // Save cache
            inOutResult.CachedBlendHash = currentBlendHash;
            inOutResult.CachedBlendedMap = outMap;
            inOutResult.CachedBlendedStratums = Stratums;
            inOutResult.CachedBlendedMaterialMasks = inOutResult.MaterialMasks;
        }
        
        // --- Process Erosion Sequentially Layer-by-Layer ---
        size_t currentErosionHash = params.GetErosionHash(currentBlendHash);
        bool skipErosion = false;
        
        if (currentErosionHash == inOutResult.CachedErosionHash && inOutResult.CachedErodedMap.GetWidth() == vertSize) {
            skipErosion = true;
            outMap = inOutResult.CachedErodedMap;
            Stratums = inOutResult.CachedErodedStratums;
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
                    
                    // Use the layer itself to route the noise through EvaluateSymmetricNoise
                    for(int y=0; y<vertSize; ++y) {
                        for(int x=0; x<vertSize; ++x) {
                            float n = (EvaluateSymmetricNoise(x, y, vertSize, rainNoise, layer, &params) + 1.0f) * 0.5f;
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
                
                if (params.UseGPUHydraulic) {
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
            if (combinedMask != 0) ApplySymmetryBlur(outMap, vertSize, params.SymmetryBlurRadius, combinedMask, params.SpawnPointCount);
        }
        
            // Save cache
            inOutResult.CachedErosionHash = currentErosionHash;
            inOutResult.CachedErodedMap = outMap;
            inOutResult.CachedErodedStratums = Stratums;
        } // End !skipErosion
        
        inOutResult.Stratums = Stratums;
        
        float minH = 99999.0f, maxH = -99999.0f;
        for (int y = 0; y < vertSize; ++y) {
            for (int x = 0; x < vertSize; ++x) {
                float h = outMap.Get(x,y);
                if (h < minH) minH = h;
                if (h > maxH) maxH = h;
            }
        }
        inOutResult.TerrainMinHeight = minH;
        inOutResult.TerrainMaxHeight = maxH;
        
        size_t currentPlacementHash = params.GetPlacementHash(currentErosionHash);
        bool skipPlacement = false;
        
        if (currentPlacementHash == inOutResult.CachedPlacementHash) {
            skipPlacement = true;
            // GeneratedMarkers is already cached in inOutResult!
        }
        
        if (!skipPlacement) {
            // 1. Calculate slopemap for procedural rules
            FloatMask slopeMap(vertSize, vertSize, 0.0f);
            for (int y = 1; y < vertSize - 1; ++y) {
                for (int x = 1; x < vertSize - 1; ++x) {
                    float dx = (outMap.Get(x+1, y) - outMap.Get(x-1, y)) * 0.5f;
                    float dy = (outMap.Get(x, y+1) - outMap.Get(x, y-1)) * 0.5f;
                    float slope = std::sqrt(dx*dx + dy*dy) * 100.0f; // Multiplied by 100 for better human readable slope 0-90 approx
                    slopeMap.Set(x, y, slope);
                }
            }
            
            // 2. Generate Procedural Markers
            inOutResult.GeneratedMarkers.clear();
            if (params.EnableProceduralMarkers) {
                GenerateProceduralMarkers(params, outMap, slopeMap, inOutResult);
            }
            
            inOutResult.CachedPlacementHash = currentPlacementHash;
        }
        
        // If we are in FastPreviewMode, skip all heavy topology, flow, and erosion logic
        if (params.FastPreviewMode) {
            return;
        }
        
        inOutResult.FlowMap = FloatMask(vertSize, vertSize, 0.0f);
        inOutResult.AccumulationMap = FloatMask(vertSize, vertSize, params.FlowSettingsParams.Precipitation);
        
        // --- TOPOLOGICAL SORT DOWNHILL FLOW ACCUMULATION ---
        if (!params.UseGPUFlowMap) {
            // 1. O(N) Bucket Sort (God-Tier Topology)
            const int numBuckets = 65536; // 16-bit precision bucket sort
            std::vector<int> bucketCounts(numBuckets, 0);
            std::vector<uint32_t> sortedIndices(vertSize * vertSize);
            
            size_t numPixels = vertSize * vertSize;
            float* mapData = outMap.GetMutableDataPtr();
            
            for (size_t i = 0; i < numPixels; ++i) {
                int b = static_cast<int>(mapData[i] * (numBuckets - 1));
                if (b < 0) b = 0;
                if (b >= numBuckets) b = numBuckets - 1;
                bucketCounts[b]++;
            }
            
            std::vector<int> bucketOffsets(numBuckets, 0);
            int currentOffset = 0;
            // Reverse order to go highest to lowest elevation
            for (int b = numBuckets - 1; b >= 0; --b) {
                bucketOffsets[b] = currentOffset;
                currentOffset += bucketCounts[b];
            }
            
            for (size_t i = 0; i < numPixels; ++i) {
                int b = static_cast<int>(mapData[i] * (numBuckets - 1));
                if (b < 0) b = 0;
                if (b >= numBuckets) b = numBuckets - 1;
                sortedIndices[bucketOffsets[b]++] = static_cast<uint32_t>(i);
            }
        
        // 2. Linearly process DAG topological sequence
            for (uint32_t idx : sortedIndices) {
                int x = idx % vertSize;
                int y = idx / vertSize;
                
                float h = outMap.Get(x, y);
                float currentAcc = inOutResult.AccumulationMap.Get(x, y);
                float currentVel = inOutResult.FlowMap.Get(x, y); 
                
                float maxDrop = 0.0f;
                int lowestX = -1, lowestY = -1;
                
                // D8 flow evaluation
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        if (dx == 0 && dy == 0) continue;
                        int nx = x + dx;
                        int ny = y + dy;
                        if (nx >= 0 && nx < vertSize && ny >= 0 && ny < vertSize) {
                            float nh = outMap.Get(nx, ny);
                            float drop = h - nh;
                            if (drop > maxDrop) {
                                maxDrop = drop;
                                lowestX = nx;
                                lowestY = ny;
                            }
                        }
                    }
                }
                
                if (maxDrop > 0.0f && lowestX != -1) {
                    // Accumulation logic (volume pooling)
                    // We add our accumulation to the lowest neighbor
                    float nextAcc = inOutResult.AccumulationMap.Get(lowestX, lowestY) + currentAcc;
                    inOutResult.AccumulationMap.Set(lowestX, lowestY, nextAcc);
                    
                    // Velocity logic (acceleration over distance)
                    // Add our velocity to neighbor, plus gravitational acceleration from the drop
                    float nextVel = inOutResult.FlowMap.Get(lowestX, lowestY) + currentVel + (maxDrop * 100.0f);
                    inOutResult.FlowMap.Set(lowestX, lowestY, nextVel);
                }
            } // End sortedIndices loop
        } // End CPU FlowMap

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
    }

    void TerrainGenerator::ApplySymmetryBlur(FloatMask& map, int mapSize, float blurRadius, int symmetryMask, int spawnPointCount) {
        // Simple Box Blur (or Gaussian) focused on the seams
        int r = static_cast<int>(blurRadius);
        if (r <= 0) return;
        
        FloatMask tempMap(mapSize, mapSize, 0.0f);
        int halfSize = mapSize / 2;
        
        for (int y = 0; y < mapSize; ++y) {
            for (int x = 0; x < mapSize; ++x) {
                bool nearSeam = false;
                
                // Check X Seam
                if (symmetryMask & Symmetry_X) {
                    if (std::abs(x - halfSize) <= r) nearSeam = true;
                }
                // Check Z Seam
                if (symmetryMask & Symmetry_Z) {
                    if (std::abs(y - halfSize) <= r) nearSeam = true;
                }
                // Check Radial Seams
                if (symmetryMask & Symmetry_Radial && spawnPointCount > 1) {
                    float dx = static_cast<float>(x - halfSize);
                    float dy = static_cast<float>(y - halfSize);
                    float angle = std::atan2(dy, dx);
                    if (angle < 0.0f) angle += 2.0f * 3.14159265f;
                    float wedgeAngle = (2.0f * 3.14159265f) / static_cast<float>(spawnPointCount);
                    float foldedAngle = std::fmod(angle, wedgeAngle);
                    
                    // The seam is at foldedAngle = 0 and foldedAngle = wedgeAngle
                    // Calculate linear distance to the angle ray
                    float distTo0 = std::abs(std::sin(foldedAngle) * std::sqrt(dx*dx + dy*dy));
                    float distToWedge = std::abs(std::sin(wedgeAngle - foldedAngle) * std::sqrt(dx*dx + dy*dy));
                    
                    if (distTo0 <= r || distToWedge <= r) nearSeam = true;
                }
                
                if (nearSeam) {
                    float sum = 0.0f;
                    float weightSum = 0.0f;
                    
                    for (int dy = -r; dy <= r; ++dy) {
                        for (int dx = -r; dx <= r; ++dx) {
                            int nx = x + dx;
                            int ny = y + dy;
                            if (nx >= 0 && nx < mapSize && ny >= 0 && ny < mapSize) {
                                // Simple Gaussian-ish weight
                                float distSq = static_cast<float>(dx*dx + dy*dy);
                                if (distSq <= r*r) {
                                    float weight = std::exp(-distSq / (2.0f * (r/2.0f)*(r/2.0f)));
                                    sum += map.Get(nx, ny) * weight;
                                    weightSum += weight;
                                }
                            }
                        }
                    }
                    tempMap.Set(x, y, weightSum > 0.0f ? (sum / weightSum) : map.Get(x, y));
                } else {
                    tempMap.Set(x, y, map.Get(x, y));
                }
            }
        }
        
        // Copy back
        for (int y = 0; y < mapSize; ++y) {
            for (int x = 0; x < mapSize; ++x) {
                map.Set(x, y, tempMap.Get(x, y));
            }
        }
    }

    struct MarkerCandidate {
        int x, y;
        int maxFlatRadius;
        float variance;
    };

    // Returns {MaxFlatRadius, Variance}
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

    void TerrainGenerator::GenerateProceduralMarkers(const GenerationParams& params, const FloatMask& heightmap, const FloatMask& slopeMap, GenerationResult& inOutResult) {
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

void TerrainGenerator::CalculateMarkerSymmetryGroups(GenerationParams& params) {
    // Reset all symmetry IDs
    for (auto& [key, marker] : params.MarkersList) {
        marker.SymmetryId = 0;
    }
    
    float mapSize = static_cast<float>(params.MapSize);
    uint32_t nextSymId = 1;
    
    std::vector<std::string> unprocessed;
    for (const auto& [key, marker] : params.MarkersList) {
        unprocessed.push_back(key);
    }
    
    while (!unprocessed.empty()) {
        std::string currentKey = unprocessed.front();
        unprocessed.erase(unprocessed.begin());
        
        auto& currentMarker = params.MarkersList[currentKey];
        if (currentMarker.SymmetryId != 0) continue;
        
        int mask = currentMarker.SymmetryUseGlobal ? params.GlobalSymmetryMask : currentMarker.SymmetryMask;
        if (mask == 0) continue;
        
        std::vector<std::string> group;
        group.push_back(currentKey);
        
        std::vector<std::pair<float, float>> expectedReflections;
        float x = currentMarker.Position[0];
        float z = currentMarker.Position[2];
        
        if (mask & Symmetry_Point) {
            expectedReflections.push_back({mapSize - x, mapSize - z});
        }
        if (mask & Symmetry_X) {
            expectedReflections.push_back({mapSize - x, z});
        }
        if (mask & Symmetry_Z) {
            expectedReflections.push_back({x, mapSize - z});
        }
        if (mask & Symmetry_XY) {
            expectedReflections.push_back({mapSize - z, mapSize - x});
        }
        if (mask & Symmetry_Radial) {
            expectedReflections.push_back({mapSize - x, mapSize - z});
            expectedReflections.push_back({mapSize - z, x});
            expectedReflections.push_back({z, mapSize - x});
        }
        
        float tolerance = params.SnapImperfectSymmetry ? 5.0f : 1.0f;
        
        for (auto expected : expectedReflections) {
            std::string bestMatch = "";
            float bestDist = tolerance * tolerance;
            
            for (auto it = unprocessed.begin(); it != unprocessed.end(); ++it) {
                auto& otherMarker = params.MarkersList[*it];
                if (otherMarker.Type != currentMarker.Type) continue;
                
                float dx = otherMarker.Position[0] - expected.first;
                float dz = otherMarker.Position[2] - expected.second;
                float distSq = dx*dx + dz*dz;
                
                if (distSq < bestDist) {
                    bestDist = distSq;
                    bestMatch = *it;
                }
            }
            
            if (!bestMatch.empty()) {
                group.push_back(bestMatch);
                unprocessed.erase(std::remove(unprocessed.begin(), unprocessed.end(), bestMatch), unprocessed.end());
                
                if (params.SnapImperfectSymmetry) {
                    auto& matchMarker = params.MarkersList[bestMatch];
                    matchMarker.Position[0] = expected.first;
                    matchMarker.Position[2] = expected.second;
                    matchMarker.Position[1] = currentMarker.Position[1];
                }
            }
        }
        
        if (group.size() > 1) {
            for (const auto& key : group) {
                params.MarkersList[key].SymmetryId = nextSymId;
            }
            nextSymId++;
        }
    }
}
} // namespace SanmapGen

