#pragma once
#include "../Parameters.h"
#include "../Mask2D.h"

namespace SanmapGen {
    class Gen_Marker_Procedural {
    public:
        static void GenerateProceduralMarkers(const GenerationParams& params, const FloatMask& heightmap, const FloatMask& slopeMap, GenerationResult& inOutResult);
    };
}
