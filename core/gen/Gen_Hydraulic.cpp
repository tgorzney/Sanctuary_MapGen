#include "Gen_Hydraulic.h"
#include "../ErosionSimulator.h"
#include <cmath>
#include <algorithm>

namespace SanmapGen {

    // (This is duplicated locally from ErosionSimulator to decouple it without header tangling, or you can expose it)
    static void CalculateGradient(const FloatMask& map, float x, float y, float& height, float& gradX, float& gradY) {
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

    void Gen_Hydraulic::ProcessDroplets(std::vector<FloatMask>& threadStratums, FloatMask& threadTotalHeight,
                                        const std::vector<DropletSpawn>& spawns, int dropStart, int dropCount,
                                        const ErosionSettings& settings, const GenerationParams& params,
                                        int mapSize, int currentLayerIdx, const std::vector<size_t>& activeLayers,
                                        const std::vector<const NoiseLayer*>& flatLayers) {
        
        for (int i = 0; i < dropCount; ++i) {
            float posX = spawns[dropStart + i].x;
            float posY = spawns[dropStart + i].y;
            float dirX = 0.0f;
            float dirY = 0.0f;
            float speed = 1.0f;
            float water = 1.0f;
            float sediment = settings.DepositionMode ? settings.InitialSedimentLoad : 0.0f;
            
            for (int life = 0; life < settings.MaxLifetime; ++life) {
                float oldPosX = posX;
                float oldPosY = posY;
                int nodeX = static_cast<int>(oldPosX);
                int nodeY = static_cast<int>(oldPosY);
                
                float h, gradX, gradY;
                CalculateGradient(threadTotalHeight, posX, posY, h, gradX, gradY);

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
                
                float capacity = std::max(-deltaHeight * speed * water * 4.0f * topCapacityMult, 0.01f);
                if (settings.DepositionMode) {
                    capacity = std::max(capacity, sediment * std::clamp(-deltaHeight * 100.0f, 0.0f, 1.0f));
                }

                if (sediment > capacity || deltaHeight > 0.0f) {
                    float amountToDeposit = (deltaHeight > 0.0f) ? std::min(deltaHeight, sediment) : (sediment - capacity) * 0.3f;
                    sediment -= amountToDeposit;
                    
                    int depIdx = currentLayerIdx;
                    
                    float u = oldPosX - static_cast<int>(oldPosX);
                    float v = oldPosY - static_cast<int>(oldPosY);
                    
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
                    float erosionRate = 0.3f * (1.0f - topHardness); 
                    float amountToErode = std::min((capacity - sediment) * erosionRate, -deltaHeight);
                    
                    if (amountToErode > 0.0f && topLayerIdx != -1) {
                        sediment += amountToErode;
                        
                        float u = oldPosX - static_cast<int>(oldPosX);
                        float v = oldPosY - static_cast<int>(oldPosY);
                        
                        float e00 = amountToErode * (1-u)*(1-v);
                        float e10 = amountToErode * u*(1-v);
                        float e01 = amountToErode * (1-u)*v;
                        float e11 = amountToErode * u*v;
                        
                        auto erodePixel = [&](int nx, int ny, float amount) {
                            float rem = amount;
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
    }
}
