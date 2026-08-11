#pragma once
#include "../Parameters.h"

namespace SanmapGen {
    class LayerBakeCompute {
    public:
        // Dispatches the compute shader for baking a specific layer
        // Applies zero-branching math (lerp, step) for optimal GPU execution
        static void Dispatch(const GenerationParams& params, NoiseLayer* layer);
    };
}
