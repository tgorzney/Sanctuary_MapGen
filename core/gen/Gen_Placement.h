#pragma once
#include <vector>
#include "../Parameters.h"
#include "../Mask2D.h"

namespace SanmapGen {
    struct GenerationResult;

    class Gen_Placement {
    public:
        static void Process(const FloatMask& outMap, const GenerationParams& params, GenerationResult& inOutResult, size_t currentErosionHash, size_t currentFlowHash);
    };
}
