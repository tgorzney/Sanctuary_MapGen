#include "ErosionSimulator.h"
#include <cmath>
#include <algorithm>
#include <random>
#include <future>
#include <vector>

namespace SanmapGen {

    struct StratumPhysics {
        float Hardness;
        float Friction;
        float Cohesion;
        float CapacityMult;
    };

    void ErosionSimulator::CalculateGradient(const FloatMask& map, float x, float y, float& height, float& gradX, float& gradY) {
        int coordX = static_cast<int>(x);
        int coordY = static_cast<int>(y);
        float u = x - coordX;
        float v = y - coordY;

        int mapSize = map.GetWidth();
        
        int x0 = std::clamp(coordX, 0, mapSize - 1);
        int y0 = std::clamp(coordY, 0, mapSize - 1);
        int x1 = std::clamp(coordX + 1, 0, mapSize - 1);
        int y1 = std::clamp(coordY + 1, 0, mapSize - 1);

        float h00 = map.Get(x0, y0);
        float h10 = map.Get(x1, y0);
        float h01 = map.Get(x0, y1);
        float h11 = map.Get(x1, y1);

        gradX = (h10 - h00) * (1.0f - v) + (h11 - h01) * v;
        gradY = (h01 - h00) * (1.0f - u) + (h11 - h10) * u;
        
        height = h00 * (1.0f - u) * (1.0f - v) + h10 * u * (1.0f - v) + h01 * (1.0f - u) * v + h11 * u * v;
    }

    void ErosionSimulator::SimulateStratifiedErosionDelta(std::vector<FloatMask>& stratums, const std::vector<DropletSpawn>& spawns, const ErosionSettings& settings, const GenerationParams& params, int mapSize, int currentLayerIdx) {
        auto flatLayers = params.GetFlatLayers();
        if (!settings.Enabled || stratums.empty() || spawns.empty() || currentLayerIdx < 0 || currentLayerIdx >= flatLayers.size()) return;

        unsigned int numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) numThreads = 4;
        
        int dropletsPerThread = settings.DropletCount / numThreads;
        if (dropletsPerThread < 1) {
            dropletsPerThread = settings.DropletCount;
            numThreads = 1;
        }

        // Pre-calculate TotalHeight of layers up to currentLayerIdx
        FloatMask initialTotalHeight(mapSize, mapSize, 0.0f);
        for (int y = 0; y < mapSize; ++y) {
            for (int x = 0; x < mapSize; ++x) {
                float th = 0.0f;
                for (size_t i = 0; i <= (size_t)currentLayerIdx; ++i) {
                    if (flatLayers[i]->Enabled) th += stratums[i].Get(x, y);
                }
                initialTotalHeight.Set(x, y, th);
            }
        }

        auto erosionWorker = [&](int threadIdx, int dropStart, int dropCount) -> std::vector<FloatMask> {
            // Thread-Local copy of all thicknesses (stratums) up to currentLayerIdx
            std::vector<FloatMask> threadStratums = stratums;
            FloatMask threadTotalHeight = initialTotalHeight;

            // Determine which layers can be carved into
            std::vector<size_t> activeLayers;
            const auto& currentLayer = *flatLayers[currentLayerIdx];
            if (currentLayer.ErodeBeneath) {
                for (size_t i = 0; i <= (size_t)currentLayerIdx; ++i) {
                    if (flatLayers[i]->Enabled) activeLayers.push_back(i);
                }
            } else {
                if (currentLayer.Enabled) activeLayers.push_back(currentLayerIdx);
            }

            for (int i = 0; i < dropCount; ++i) {
                float posX = spawns[dropStart + i].x;
                float posY = spawns[dropStart + i].y;
                float dirX = 0.0f;
                float dirY = 0.0f;
                float speed = 1.0f;
                float water = 1.0f;
                float sediment = settings.DepositionMode ? settings.InitialSedimentLoad : 0.0f;
                
                for (int life = 0; life < settings.MaxLifetime; ++life) {
                    int nodeX = static_cast<int>(posX);
                    int nodeY = static_cast<int>(posY);
                    
                    float h, gradX, gradY;
                    CalculateGradient(threadTotalHeight, posX, posY, h, gradX, gradY);

                    // If DepositionMode has height limits, check if we've fallen below them?
                    // Actually, height limits apply to SPAWN, not flowing. The DropletSpawn array is already filtered.

                    // Physics based on the top-most ACTIVE stratum at this node
                    float topHardness = 0.2f, topFriction = 0.8f, topCohesion = 0.5f, topCapacityMult = 2.0f;
                    int topLayerIdx = -1;
                    
                    for (int l = (int)activeLayers.size() - 1; l >= 0; --l) {
                        if (threadStratums[activeLayers[l]].Get(nodeX, nodeY) > 0.0001f) {
                            topLayerIdx = activeLayers[l];
                            const auto& layer = (*flatLayers[topLayerIdx]);
                            topHardness = layer.Hardness;
                            topFriction = layer.Friction;
                            topCohesion = layer.Cohesion;
                            topCapacityMult = layer.CapacityMult;
                            break;
                        }
                    }

                    // Inertia modified by friction
                    float inertia = 0.05f + (1.0f - topFriction) * 0.1f;
                    
                    dirX = (dirX * inertia) - (gradX * (1.0f - inertia));
                    dirY = (dirY * inertia) - (gradY * (1.0f - inertia));

                    float len = std::sqrt(dirX * dirX + dirY * dirY);
                    if (len != 0.0f) { dirX /= len; dirY /= len; }

                    posX += dirX;
                    posY += dirY;

                    if ((dirX == 0.0f && dirY == 0.0f) || posX < 1.0f || posX >= mapSize - 2 || posY < 1.0f || posY >= mapSize - 2) {
                        break;
                    }

                    float newH, dummyX, dummyY;
                    CalculateGradient(threadTotalHeight, posX, posY, newH, dummyX, dummyY);
                    float deltaHeight = newH - h;
                    
                    // Capacity driven by global setting * stratum mult
                    float capacity = std::max(-deltaHeight * speed * water * 4.0f * topCapacityMult, 0.01f);
                    if (settings.DepositionMode) {
                        capacity = std::max(capacity, sediment * std::clamp(-deltaHeight * 100.0f, 0.0f, 1.0f));
                    }

                    if (sediment > capacity || deltaHeight > 0.0f) {
                        // DEPOSIT
                        float amountToDeposit = (deltaHeight > 0.0f) ? std::min(deltaHeight, sediment) : (sediment - capacity) * 0.3f;
                        sediment -= amountToDeposit;
                        
                        // Always deposit into the current layer being generated, regardless of what's beneath it!
                        int depIdx = currentLayerIdx;
                        
                        float u = posX - static_cast<int>(posX);
                        float v = posY - static_cast<int>(posY);
                        
                        float d00 = amountToDeposit * (1-u)*(1-v);
                        float d10 = amountToDeposit * u*(1-v);
                        float d01 = amountToDeposit * (1-u)*v;
                        float d11 = amountToDeposit * u*v;
                        
                        threadStratums[depIdx].Set(nodeX, nodeY, threadStratums[depIdx].Get(nodeX, nodeY) + d00);
                        threadStratums[depIdx].Set(nodeX+1, nodeY, threadStratums[depIdx].Get(nodeX+1, nodeY) + d10);
                        threadStratums[depIdx].Set(nodeX, nodeY+1, threadStratums[depIdx].Get(nodeX, nodeY+1) + d01);
                        threadStratums[depIdx].Set(nodeX+1, nodeY+1, threadStratums[depIdx].Get(nodeX+1, nodeY+1) + d11);
                        
                        threadTotalHeight.Set(nodeX, nodeY, threadTotalHeight.Get(nodeX, nodeY) + d00);
                        threadTotalHeight.Set(nodeX+1, nodeY, threadTotalHeight.Get(nodeX+1, nodeY) + d10);
                        threadTotalHeight.Set(nodeX, nodeY+1, threadTotalHeight.Get(nodeX, nodeY+1) + d01);
                        threadTotalHeight.Set(nodeX+1, nodeY+1, threadTotalHeight.Get(nodeX+1, nodeY+1) + d11);

                    } else if (!settings.DepositionMode) {
                        // ERODE (Only if not in Deposition Mode)
                        float erosionRate = 0.3f * (1.0f - topHardness); // Harder soil erodes slower
                        float amountToErode = std::min((capacity - sediment) * erosionRate, -deltaHeight);
                        
                        if (amountToErode > 0.0f && topLayerIdx != -1) {
                            sediment += amountToErode;
                            
                            float u = posX - static_cast<int>(posX);
                            float v = posY - static_cast<int>(posY);
                            
                            float e00 = amountToErode * (1-u)*(1-v);
                            float e10 = amountToErode * u*(1-v);
                            float e01 = amountToErode * (1-u)*v;
                            float e11 = amountToErode * u*v;
                            
                            auto erodePixel = [&](int nx, int ny, float amount) {
                                float rem = amount;
                                // Dig down through active layers until erosion is spent or we hit a non-erodable floor
                                for (int l = (int)activeLayers.size() - 1; l >= 0 && rem > 0; --l) {
                                    int idx = activeLayers[l];
                                    if (!(*flatLayers[idx]).Erodable) continue;
                                    
                                    float th = threadStratums[idx].Get(nx, ny);
                                    if (th > 0) {
                                        float sub = std::min(th, rem);
                                        threadStratums[idx].Set(nx, ny, th - sub);
                                        threadTotalHeight.Set(nx, ny, threadTotalHeight.Get(nx, ny) - sub);
                                        rem -= sub;
                                    }
                                }
                            };
                            
                            erodePixel(nodeX, nodeY, e00);
                            erodePixel(nodeX+1, nodeY, e10);
                            erodePixel(nodeX, nodeY+1, e01);
                            erodePixel(nodeX+1, nodeY+1, e11);
                        }
                    }

                    speed = std::sqrt(std::max(0.0f, speed * speed + deltaHeight * settings.Gravity));
                    water *= (1.0f - settings.EvaporationRate);
                }
            }

            // Cohesion Pass (Talus Angle slippage)
            std::vector<size_t> cohesionLayers;
            if (currentLayer.ErodeBeneath) {
                for (size_t i = 0; i <= (size_t)currentLayerIdx; ++i) {
                    if (flatLayers[i]->Enabled) cohesionLayers.push_back(i);
                }
            } else {
                if (currentLayer.Enabled) cohesionLayers.push_back(currentLayerIdx);
            }

            for (int p = 0; p < 2; ++p) { // 2 passes
                for (int y = 1; y < mapSize - 1; ++y) {
                    for (int x = 1; x < mapSize - 1; ++x) {
                        for (int l = (int)cohesionLayers.size() - 1; l >= 0; --l) {
                            int idx = cohesionLayers[l];
                            if (!(*flatLayers[idx]).Erodable) continue;
                            
                            float thickness = threadStratums[idx].Get(x, y);
                            if (thickness > 0.001f) {
                                const auto& layer = (*flatLayers[idx]);
                                float maxSlope = layer.Cohesion;
                                
                                float h = threadTotalHeight.Get(x, y);
                                int bestNX = x, bestNY = y;
                                float lowestH = h;
                                
                                const int dx[] = { -1, 1, 0, 0 };
                                const int dy[] = { 0, 0, -1, 1 };
                                for (int d = 0; d < 4; ++d) {
                                    float nh = threadTotalHeight.Get(x + dx[d], y + dy[d]);
                                    if (nh < lowestH) {
                                        lowestH = nh;
                                        bestNX = x + dx[d];
                                        bestNY = y + dy[d];
                                    }
                                }
                                
                                float diff = h - lowestH;
                                if (diff > maxSlope) {
                                    float slideAmount = (diff - maxSlope) / 2.0f;
                                    slideAmount = std::min(slideAmount, thickness); 
                                    
                                    threadStratums[idx].Set(x, y, thickness - slideAmount);
                                    threadStratums[idx].Set(bestNX, bestNY, threadStratums[idx].Get(bestNX, bestNY) + slideAmount);
                                    
                                    threadTotalHeight.Set(x, y, threadTotalHeight.Get(x, y) - slideAmount);
                                    threadTotalHeight.Set(bestNX, bestNY, threadTotalHeight.Get(bestNX, bestNY) + slideAmount);
                                }
                            }
                        }
                    }
                }
            }

            // Delta
            std::vector<FloatMask> threadDelta = threadStratums;
            for (size_t i = 0; i <= (size_t)currentLayerIdx; ++i) {
                for (int y = 0; y < mapSize; ++y) {
                    for (int x = 0; x < mapSize; ++x) {
                        threadDelta[i].Set(x, y, threadStratums[i].Get(x, y) - stratums[i].Get(x, y));
                    }
                }
            }
            return threadDelta;
        };

        std::vector<std::future<std::vector<FloatMask>>> futures;
        int currentDrop = 0;
        for (unsigned int t = 0; t < numThreads; ++t) {
            int drops = (t == numThreads - 1) ? (settings.DropletCount - currentDrop) : dropletsPerThread;
            futures.push_back(std::async(std::launch::async, erosionWorker, t, currentDrop, drops));
            currentDrop += drops;
        }

        for (auto& f : futures) {
            std::vector<FloatMask> threadDelta = f.get();
            for (size_t i = 0; i <= (size_t)currentLayerIdx; ++i) {
                for (int y = 0; y < mapSize; ++y) {
                    for (int x = 0; x < mapSize; ++x) {
                        float oldVal = stratums[i].Get(x, y);
                        stratums[i].Set(x, y, std::max(0.0f, oldVal + threadDelta[i].Get(x, y))); // no negative thickness
                    }
                }
            }
        }
    }
}
