#include "gen/Gen_Placement.h"
#include "gen/Gen_FlowAndAccumulation.h"
#include "gen/Gen_Erosion.h"
#include "gen/Gen_NoiseAndBlend.h"
#include "TerrainGenerator.h"
#include "gen/Gen_Noise.h"
#include "gen/Gen_Marker_Procedural.h"
#include "FastNoiseLite.h"
#include <random>
#include <future>
#include <thread>
#include <algorithm>
#include <cmath>
#include "ErosionSimulator.h"
#include "TerrainCompute.h"
#include "gen/Gen_Mask_Slope.h"
#include "gen/Gen_Marker_Placement.h"
#include "gen/Gen_Mask_Height.h"
#include "ErosionCompute.h"
#include "PlacementRules.h"
#include "math/Sanmath_SIMD.h"
#include "gen/LayerBakeCompute.h"

namespace SanmapGen {

    // (Morton Part1By1/Compact1By1 helpers removed — dead duplicate; the canonical
    //  Morton lives in src/math/Morton_MATH.h. Work-Order M0-1.)

    void TerrainGenerator::GenerateMap(FloatMask& outMap, const GenerationParams& params, GenerationResult& inOutResult) {
        int vertSize = params.MapSize + 1;
        outMap.Resize(vertSize, vertSize, 0.0f);

        std::vector<FloatMask> Stratums;
        auto flatLayers = params.GetFlatLayers();
        for (size_t i = 0; i < flatLayers.size(); ++i) {
            Stratums.push_back(FloatMask(vertSize, vertSize, 0.0f));
        }

        size_t currentBlendHash = 0;
        Gen_NoiseAndBlend::Process(outMap, Stratums, params, inOutResult, currentBlendHash);

        size_t currentErosionHash = 0;
        Gen_Erosion::Process(outMap, Stratums, params, inOutResult, currentBlendHash, currentErosionHash);

        size_t currentFlowHash = 0;
        Gen_FlowAndAccumulation::Process(outMap, params, inOutResult, currentErosionHash, currentFlowHash);

        Gen_Placement::Process(outMap, params, inOutResult, currentErosionHash, currentFlowHash);
    }

    void TerrainGenerator::BakeLayer(const GenerationParams& params, NoiseLayer* layer) {
        if (!layer) return;
        layer->IsBaked = true;
        layer->BakeRequested = false;

        LayerBakeCompute::Dispatch(params, layer);
    }

    void TerrainGenerator::ClearBakedLayer(NoiseLayer* layer) {
        if (!layer) return;
        layer->IsBaked = false;
        layer->BakeRequested = false;
        layer->BakedImageData.clear();
    }

} // namespace SanmapGen
