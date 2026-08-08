#include "ErosionSimulator.h"
#include "gen/Gen_Hydraulic.h"
#include "gen/Gen_Thermal.h"
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

            Gen_Hydraulic::ProcessDroplets(threadStratums, threadTotalHeight, spawns, dropStart, dropCount,
                                           settings, params, mapSize, currentLayerIdx, activeLayers, flatLayers);

            // Cohesion Pass (Talus Angle slippage)
            std::vector<size_t> cohesionLayers;
            if (currentLayer.ErodeBeneath) {
                for (size_t i = 0; i <= (size_t)currentLayerIdx; ++i) {
                    if (flatLayers[i]->Enabled) cohesionLayers.push_back(i);
                }
            } else {
                if (currentLayer.Enabled) cohesionLayers.push_back(currentLayerIdx);
            }

            Gen_Thermal::ProcessCohesion(threadStratums, threadTotalHeight, mapSize, cohesionLayers, flatLayers);

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
