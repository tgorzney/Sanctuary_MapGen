#pragma once
#include "Mask2D.h"
#include <memory>
#include "Parameters.h"
#include <vector>

class FastNoiseLite;

namespace SanmapGen {

    struct GenerationResult {
        std::vector<FloatMask> Stratums; // Per-layer geometry deltas (for Erosion)
        std::vector<FloatMask> MaterialMasks; // 9 Texture blending weights
        FloatMask FlowMap;
        FloatMask AccumulationMap;
        
        float TerrainMinHeight = 0.0f;
        float TerrainMaxHeight = 128.0f;
        
        std::vector<FloatMask> CachedRawNoise;
        std::vector<size_t> CachedNoiseHashes;
        
        // --- NEW CACHE PIPELINE ---
        size_t CachedBlendHash = 0;
        FloatMask CachedBlendedMap;
        std::vector<FloatMask> CachedBlendedStratums;
        std::vector<FloatMask> CachedBlendedMaterialMasks;
        
        size_t CachedErosionHash = 0;
        FloatMask CachedErodedMap;
        std::vector<FloatMask> CachedErodedStratums;
        
        size_t CachedPlacementHash = 0;
        // --------------------------
        
        std::map<std::string, MarkerTransform> GeneratedMarkers;
        
        GenerationResult() : FlowMap(0, 0), AccumulationMap(0, 0), CachedBlendedMap(0, 0), CachedErodedMap(0, 0) {}
    };

    class TerrainGenerator {
    public:
        // Main entry point for generating the terrain heightmap. Returns individual stratum masks and flow data.
        static void GenerateMap(FloatMask& outMap, const GenerationParams& params, GenerationResult& inOutResult);
        
        

    private:
        static FloatMask SymmetrizeErodedTerrain(const FloatMask& terrainMap, const NoiseLayer& layer, const GenerationParams& params);
    };

} // namespace SanmapGen
