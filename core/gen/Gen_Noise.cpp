#include "Gen_Noise.h"
#include "../TerrainGenerator.h"
#include <cmath>
#include <omp.h>

namespace SanmapGen {

void Gen_Noise::ProcessLayerChunk(ChunkTask task) {
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

float Gen_Noise::EvaluateSymmetricNoise(int px, int py, int mapSize, FastNoiseLite& noise, const NoiseLayer& layer, const GenerationParams* params) {
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

float Gen_Noise::BilinearGet(const FloatMask& map, float x, float y) {
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

void Gen_Noise::ApplySymmetryBlur(FloatMask& map, int mapSize, float blurRadius, int symmetryMask, int spawnPointCount) {
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

}
