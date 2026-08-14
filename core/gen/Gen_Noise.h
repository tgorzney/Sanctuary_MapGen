#pragma once
#include "Parameters.h"
#include "Mask2D.h"
#include "FastNoiseLite.h"
#include <cstdint>
#include "../../src/math/Morton_MATH.h"

namespace SanmapGen {

    class Gen_Noise {
    public:
        // Morton Z-Curve index for cache locality — forwards to the canonical
        // Morton_MATH module (single definition; Work-Order M0-1).
        static inline uint32_t EncodeMorton2D(uint32_t x, uint32_t y) {
            return SanmapGen::Math::EncodeMorton2D(x, y);
        }

        static inline void DecodeMorton2D(uint32_t code, uint32_t& x, uint32_t& y) {
            SanmapGen::Math::DecodeMorton2D(code, x, y);
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
