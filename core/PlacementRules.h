#pragma once
#include "Parameters.h"
#include "Mask2D.h"
#include <vector>

namespace SanmapGen {

    struct FlatZone {
        std::vector<Point2D> Points;
        int CenterX;
        int CenterY;
        float AverageHeight;
        size_t Area;
    };

    class PlacementRules {
    public:
        // Main entry point for post-generation placement
        static void ExecutePlacement(const FloatMask& heightmap, GenerationParams& params);

    private:
        static FloatMask CalculateSlopeMask(const FloatMask& heightmap);
        static void DetectSpawns(const FloatMask& heightmap, const FloatMask& slopeMap, GenerationParams& params);
        static void PlaceResources(const FloatMask& heightmap, const FloatMask& slopeMap, GenerationParams& params);
        
        static void GenerateExclusionMask(BooleanMask& outMask, const GenerationParams& params, int mapSize);
        static void PlaceProps(const FloatMask& heightmap, const FloatMask& slopeMap, const BooleanMask& exclusionMask, GenerationParams& params);
    };

} // namespace SanmapGen
