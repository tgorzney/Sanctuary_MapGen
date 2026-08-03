#include "TerrainGenerator.h"
#include "FastNoiseLite.h"
#include <future>
#include <thread>
#include <algorithm>
#include <cmath>

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
        int symMask = layer.SymmetryMask;
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
        else if (alg == SymmetryAlgorithm::CrossFade && (symMask & Symmetry_Radial) && spawnCount > 1) {
            float dx = static_cast<float>(px - halfSize);
            float dy = static_cast<float>(py - halfSize);
            float radius = std::sqrt(dx*dx + dy*dy);
            float angle = std::atan2(dy, dx);
            if (angle < 0.0f) angle += 2.0f * 3.14159265f;
            
            float wedgeAngle = (2.0f * 3.14159265f) / static_cast<float>(spawnCount);
            float foldedAngle = std::fmod(angle, wedgeAngle);
            
            // Primary Noise Evaluation
            float dx1 = radius * std::cos(foldedAngle);
            float dy1 = radius * std::sin(foldedAngle);
            int mx1 = halfSize + static_cast<int>(std::round(dx1));
            int my1 = halfSize + static_cast<int>(std::round(dy1));
            // Apply other mirrors if needed
            if (symMask & Symmetry_X) { if (mx1 > halfSize) mx1 = mapSize - mx1 - 1; }
            if (symMask & Symmetry_Z) { if (my1 > halfSize) my1 = mapSize - my1 - 1; }
            float noise1 = noise.GetNoise(static_cast<float>(mx1), static_cast<float>(my1), 0.0f);
            
            float blendWidth = params->CrossFadeWidth;
            if (foldedAngle < blendWidth || foldedAngle > wedgeAngle - blendWidth) {
                // Secondary Noise (Wrap around the wedge to seamlessly loop)
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
                t = t * t * (3.0f - 2.0f * t); // smoothstep
                return (noise1 * t) + (noise2 * (1.0f - t));
            }
            return noise1;
        } 
        else {
            // Fold or Blur
            int mx = px;
            int my = py;
            fold2D(mx, my);
            return noise.GetNoise(static_cast<float>(mx), static_cast<float>(my), 0.0f);
        }
    }

    void TerrainGenerator::ProcessChunk(ChunkTask task) {
        int mapSize = task.Params->MapSize;
        
        // Prepare Noise Instances (AoSoA approach: pre-initialize noise per layer)
        std::vector<FastNoiseLite> layerNoises;
        for (const auto& layer : task.Params->Layers) {
            FastNoiseLite noise;
            noise.SetSeed(task.Params->Seed);
            
            // Map our Enums to FastNoiseLite Enums
            switch (layer.Type) {
                case NoiseType::OpenSimplex2: noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2); break;
                case NoiseType::OpenSimplex2S: noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S); break;
                case NoiseType::Cellular: noise.SetNoiseType(FastNoiseLite::NoiseType_Cellular); break;
                case NoiseType::Perlin: noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin); break;
                case NoiseType::ValueCubic: noise.SetNoiseType(FastNoiseLite::NoiseType_ValueCubic); break;
                case NoiseType::Value: noise.SetNoiseType(FastNoiseLite::NoiseType_Value); break;
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
            
            if (task.Params->SymAlgorithm == SymmetryAlgorithm::NativeHash) {
                bool symX = (layer.SymmetryMask & Symmetry_X) != 0;
                bool symZ = (layer.SymmetryMask & Symmetry_Z) != 0;
                bool symPoint = (layer.SymmetryMask & Symmetry_Point) != 0;
                noise.SetNativeSymmetry(symX, symZ, symPoint);
            }
            
            layerNoises.push_back(noise);
        }

        // Execute Morton Z-Curve Iteration for this chunk
        for (uint32_t z = task.StartZ; z < task.EndZ; ++z) {
            uint32_t px, py;
            DecodeMorton2D(z, px, py);
            
            // Skip out of bounds (since Morton curve maps to a square power of 2,
            // if mapSize isn't a power of 2, some indices are out of bounds)
            if (px >= (uint32_t)mapSize || py >= (uint32_t)mapSize) continue;

            float finalValue = 0.0f; // Base background
            
            // Blend Layers (Laplacian/AoSoA Interleaved Execution)
            for (size_t i = 0; i < task.Params->Layers.size(); ++i) {
                const auto& layer = task.Params->Layers[i];
                if (!layer.Enabled) continue;

                // Evaluate the noise with the chosen Symmetry Algorithm
                float noiseVal = (EvaluateSymmetricNoise(px, py, mapSize, layerNoises[i], layer, task.Params) + 1.0f) * 0.5f;
                
                // --- Apply Terrain Density Shaping ---
                
                // 1. Land Density (Overall Scale/Bias)
                // Multiply noise by density (e.g. 0.5 density = normal size, 1.0 = double size clamped)
                noiseVal = noiseVal * (layer.LandDensity * 2.0f);
                
                // 2. Mountain Density (Clamp the tops)
                // If the noise exceeds the mountain threshold, flatten it out
                float mountainThreshold = 1.0f - layer.MountainDensity;
                if (mountainThreshold < 1.0f) {
                    if (noiseVal > mountainThreshold) {
                        noiseVal = 1.0f; // Simplified mountain flatten logic for now
                    }
                }
                
                // 3. Plateau Density (Terracing / Stepping)
                if (layer.PlateauDensity > 0.0f) {
                    // Number of terraces (more density = fewer, wider terraces vs many thin ones)
                    // Let's say max 20 terraces down to 2 terraces based on density
                    float terraces = 20.0f - (layer.PlateauDensity * 18.0f);
                    float terraceHeight = 1.0f / terraces;
                    noiseVal = std::round(noiseVal / terraceHeight) * terraceHeight;
                }
                
                // 4. Ramp Density (Smoothing out the terraces or steep cliffs)
                if (layer.RampDensity > 0.0f) {
                    // Simple simulated ramp: blend the original noise with the terraced/clamped noise
                    // based on Ramp Density
                    // For the ramp density, evaluate the symmetry again
                    float origNoise = (EvaluateSymmetricNoise(px, py, mapSize, layerNoises[i], layer, task.Params) + 1.0f) * 0.5f;
                    origNoise = origNoise * (layer.LandDensity * 2.0f);
                    noiseVal = (noiseVal * (1.0f - layer.RampDensity)) + (origNoise * layer.RampDensity);
                }
                
                // Final Weighting
                float weightedVal = noiseVal * layer.Opacity;
                
                // Blend Math
                switch (layer.Blend) {
                    case BlendMode::Add:
                        finalValue += weightedVal;
                        break;
                    case BlendMode::Subtract:
                        finalValue -= weightedVal;
                        break;
                    case BlendMode::Multiply:
                        finalValue *= (1.0f - layer.Opacity) + (noiseVal * layer.Opacity);
                        break;
                    case BlendMode::Overlay:
                        if (finalValue < 0.5f) {
                            finalValue = 2.0f * finalValue * weightedVal;
                        } else {
                            finalValue = 1.0f - 2.0f * (1.0f - finalValue) * (1.0f - weightedVal);
                        }
                        break;
                }
            }
            
            // Clamp final value
            if (finalValue < 0.0f) finalValue = 0.0f;
            if (finalValue > 1.0f) finalValue = 1.0f;
            
            // Write to output mask
            task.OutputMap->Set(px, py, finalValue);
        }
    }

    void TerrainGenerator::GenerateMap(FloatMask& outMap, const GenerationParams& params) {
        int mapSize = params.MapSize;
        
        // Find the next power of 2 to guarantee the Morton curve encompasses the whole grid
        uint32_t pow2Size = 1;
        while (pow2Size < (uint32_t)mapSize) pow2Size <<= 1;
        
        uint32_t totalMortonCells = pow2Size * pow2Size;
        
        // Multithreading Setup
        unsigned int numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) numThreads = 4; // Fallback
        
        uint32_t cellsPerThread = totalMortonCells / numThreads;
        
        std::vector<std::future<void>> futures;
        
        for (unsigned int i = 0; i < numThreads; ++i) {
            ChunkTask task;
            task.StartZ = i * cellsPerThread;
            task.EndZ = (i == numThreads - 1) ? totalMortonCells : (i + 1) * cellsPerThread;
            task.Params = &params;
            task.OutputMap = &outMap;
            
            futures.push_back(std::async(std::launch::async, ProcessChunk, task));
        }
        
        // Wait for all threads to finish
        for (auto& f : futures) {
            f.get();
        }
        
        // Post-Process 2-Pass Blur if Blur Algorithm is selected
        if (params.SymAlgorithm == SymmetryAlgorithm::Blur && params.SymmetryBlurRadius > 0.0f) {
            // Determine if ANY layer uses radial or mirror symmetry
            int combinedMask = 0;
            for (const auto& layer : params.Layers) combinedMask |= layer.SymmetryMask;
            
            if (combinedMask != 0) {
                ApplySymmetryBlur(outMap, params.MapSize, params.SymmetryBlurRadius, combinedMask, params.SpawnPointCount);
            }
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

} // namespace SanmapGen
