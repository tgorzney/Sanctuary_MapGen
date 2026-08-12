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
        
        float StatisticalMinHeight = 0.0f;
        float StatisticalMaxHeight = 1.0f;
        float MapMinSlope = 0.0f;
        float MapMaxSlope = 90.0f;
        
        std::vector<FloatMask> CachedRawNoise;
        std::vector<size_t> CachedNoiseHashes;
        
        // --- NEW CACHE PIPELINE ---
        size_t CachedBlendHash = 0;
        FloatMask CachedBlendedMap;
        std::vector<FloatMask> CachedBlendedStratums;
        
        size_t CachedErosionHash = 0;
        FloatMask CachedErodedMap;
        std::vector<FloatMask> CachedErodedStratums;
        std::vector<FloatMask> CachedErodedMaterialMasks;
        
        size_t CachedSlopeHash = 0;
        FloatMask CachedSlopeMap;
        
        size_t CachedFlowHash = 0;
        FloatMask CachedFlowMap;
        FloatMask CachedAccumulationMap;
        
        size_t CachedPlacementHash = 0;
        // --------------------------
        
        std::map<std::string, MarkerTransform> GeneratedMarkers;
        
        GenerationResult() : FlowMap(0, 0), AccumulationMap(0, 0), CachedBlendedMap(0, 0), CachedErodedMap(0, 0), CachedSlopeMap(0, 0), CachedFlowMap(0, 0), CachedAccumulationMap(0, 0) {}
    };

    class TerrainGenerator {
    public:
        // Main entry point for generating the terrain heightmap. Returns individual stratum masks and flow data.
        static void GenerateMap(FloatMask& outMap, const GenerationParams& params, GenerationResult& inOutResult);
        
        // Bake support
        static void BakeLayer(const GenerationParams& params, NoiseLayer* layer);
        static void ClearBakedLayer(NoiseLayer* layer);

    private:
        
    };

} // namespace SanmapGen
