#pragma once
#include "Parameters.h"
#include "Mask2D.h"
#include "FastNoiseLite.h"
#include <cstdint>

namespace SanmapGen {
    
    class Gen_Noise {
    public:
        // Converts X,Y into a Morton Z-Curve index for cache locality
        static inline uint32_t EncodeMorton2D(uint32_t x, uint32_t y) {
            x = (x | (x << 8)) & 0x00FF00FF;
            x = (x | (x << 4)) & 0x0F0F0F0F;
            x = (x | (x << 2)) & 0x33333333;
            x = (x | (x << 1)) & 0x55555555;
            
            y = (y | (y << 8)) & 0x00FF00FF;
            y = (y | (y << 4)) & 0x0F0F0F0F;
            y = (y | (y << 2)) & 0x33333333;
            y = (y | (y << 1)) & 0x55555555;
            
            return x | (y << 1);
        }
        
        static inline void DecodeMorton2D(uint32_t code, uint32_t& x, uint32_t& y) {
            x = code & 0x55555555;
            x = (x | (x >> 1)) & 0x33333333;
            x = (x | (x >> 2)) & 0x0F0F0F0F;
            x = (x | (x >> 4)) & 0x00FF00FF;
            x = (x | (x >> 8)) & 0x0000FFFF;
            
            y = (code >> 1) & 0x55555555;
            y = (y | (y >> 1)) & 0x33333333;
            y = (y | (y >> 2)) & 0x0F0F0F0F;
            y = (y | (y >> 4)) & 0x00FF00FF;
            y = (y | (y >> 8)) & 0x0000FFFF;
        }

        struct ChunkTask {
            uint32_t StartZ;
            uint32_t EndZ;
            const GenerationParams* Params;
            const NoiseLayer* Layer;
            FastNoiseLite* Noise;
            FloatMask* OutputMap;
        };

        static void ProcessLayerChunk(ChunkTask task);
        
        static float EvaluateSymmetricNoise(int px, int py, int mapSize, FastNoiseLite& noise, const NoiseLayer& layer, const GenerationParams* params);
        
        static float BilinearGet(const FloatMask& map, float x, float y);
        static void ApplySymmetryBlur(FloatMask& map, int mapSize, float blurRadius, int symmetryMask, int spawnPointCount);
    };

}
