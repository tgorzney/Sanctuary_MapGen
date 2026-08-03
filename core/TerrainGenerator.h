#pragma once
#include "Mask2D.h"
#include <memory>
#include "Parameters.h"
#include <vector>

class FastNoiseLite;

namespace SanmapGen {

    class TerrainGenerator {
    public:
        // Main entry point for generating the terrain heightmap
        static void GenerateMap(FloatMask& outMap, const GenerationParams& params);

    private:
        // Converts X,Y into a Morton Z-Curve index for cache locality
        static inline uint32_t EncodeMorton2D(uint32_t x, uint32_t y);
        
        // Decodes a Morton Z-Curve index back to X,Y
        static inline void DecodeMorton2D(uint32_t code, uint32_t& x, uint32_t& y);
        
        // Internal struct for passing to worker threads
        struct ChunkTask {
            uint32_t StartZ;
            uint32_t EndZ;
            const GenerationParams* Params;
            const NoiseLayer* Layer;
            FastNoiseLite* Noise;
            FloatMask* OutputMap;
        };

        // Thread worker function
        static void ProcessLayerChunk(ChunkTask task);
        
        // Evaluates noise based on the chosen Symmetry Algorithm
        static float EvaluateSymmetricNoise(int px, int py, int mapSize, FastNoiseLite& noise, const NoiseLayer& layer, const GenerationParams* params);
        
        static float BilinearGet(const FloatMask& map, float x, float y);
        static FloatMask SymmetrizeErodedTerrain(const FloatMask& terrainMap, const NoiseLayer& layer, const GenerationParams& params);
        
        // 2-Pass Blur for Legacy Symmetry Hardlines
        static void ApplySymmetryBlur(FloatMask& map, int mapSize, float blurRadius, int symmetryMask, int spawnPointCount);
    };

} // namespace SanmapGen
