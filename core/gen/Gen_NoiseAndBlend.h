#pragma once
#include <vector>
#include "../Parameters.h"
#include "../Mask2D.h"

namespace SanmapGen {
    struct GenerationResult;

    class Gen_NoiseAndBlend {
    public:
        static void Process(FloatMask& outMap, std::vector<FloatMask>& Stratums, const GenerationParams& params, GenerationResult& inOutResult, size_t& outBlendHash);
    };
}
