// NoiseBlend_Blend_PROC.cpp — the CPU accuracy path: reshape each cached layer, blend the
// stack into the heightfield per Params::HeightBlendMode, and turn each layer's thickness
// into per-stratum material proportions by top-down occlusion (LAYER_SYSTEM_SPEC, via
// HeightOcclusion_MATH). Twin of the blend pass in NoiseBlend_PROC.glsl. This stage SEEDS the
// physical field; the visible surface weights are the Mask stage's output (ARCH §7.2).
#include "NoiseBlend_PROC.h"
#include "NoiseBlend_Shape_PROC.h"
#include "../math/HeightOcclusion_MATH.h"
#include "../sys/ThreadPool_SYS.h"

namespace SanmapGen {
namespace Proc {
namespace {

// Blends one cell bottom-up, recording how much height each layer actually contributed.
float BlendCell(const std::vector<LayerKernelConfiguration>& configurations,
                const std::vector<Data::FloatField>& rawNoise, int x, int y,
                float startHeight, std::vector<float>& outThickness) {
    float height = startHeight;
    for (std::size_t index = 0; index < configurations.size(); ++index) {
        const LayerKernelConfiguration& configuration = configurations[index];
        const float shaped = ReshapeLayerValue(rawNoise[index].Get(x, y), configuration);
        const float blended = ApplyLayerToHeight(height, shaped, configuration);
        outThickness[index] = blended - height;
        height = blended;
    }
    return height;
}

// Top-down occlusion: the topmost layer claims coverage first, each layer takes only what is
// still visible, and any leftover falls through to the bottom layer's stratum.
void AccumulateCellProportions(const std::vector<LayerKernelConfiguration>& configurations,
                         const std::vector<float>& thickness, Data::MapFields& fields, int x, int y) {
    float remainingVisibility = 1.0f;
    for (std::size_t reverse = configurations.size(); reverse > 0; --reverse) {
        if (remainingVisibility <= 0.0f) break;
        const LayerKernelConfiguration& configuration = configurations[reverse - 1];
        const float layerThickness = thickness[reverse - 1];
        if (layerThickness <= 0.0f) continue;
        const float alpha = Math::OcclusionAlpha(layerThickness, configuration.heightBlendContrast,
                                                 configuration.occlusionWindowLow,
                                                 configuration.occlusionWindowHigh, configuration.opacity);
        const float contribution = Math::OcclusionContribution(alpha, remainingVisibility);
        fields.materialProportions[configuration.stratumIndex].At(x, y) += contribution;
        remainingVisibility -= contribution;
    }
    if (remainingVisibility > 0.0f)
        fields.materialProportions[configurations[0].stratumIndex].At(x, y) += remainingVisibility;
}

} // namespace

void NoiseBlendStage::BlendLayersCpu() {
    ClearMaterialProportions();
    const int vertexSize = geometry.VertexSize();
    if (layerConfigurations.empty()) {
        mapFields.heightfield.Fill(constants.heightMinimum);
        return;
    }
    const float startHeight = constants.heightMinimum;
    const auto blendRow = [&](int y) {
        std::vector<float> thickness(layerConfigurations.size(), 0.0f);
        for (int x = 0; x < vertexSize; ++x) {
            const float height = BlendCell(layerConfigurations, cachedRawNoiseCpu, x, y, startHeight, thickness);
            mapFields.heightfield.Set(x, y, height);
            AccumulateCellProportions(layerConfigurations, thickness, mapFields, x, y);
        }
    };
    if (threadPool != nullptr) threadPool->ParallelFor(0, vertexSize, blendRow);
    else for (int y = 0; y < vertexSize; ++y) blendRow(y);
}

void NoiseBlendStage::RunOnCpu() {
    PrepareRun();
    const std::size_t blendHash = ComputeBlendHash();
    if (bBlendCacheValid && blendHash == cachedBlendHash && cachedBlendBackend == Sys::ComputeBackend::Cpu) {
        bLastRunSkipped = true;
        regeneratedLayerCount = 0;
        return;
    }
    bLastRunSkipped = false;
    regeneratedLayerCount = 0;
    for (std::size_t index = 0; index < layerConfigurations.size(); ++index) {
        const std::size_t structuralHash = ComputeStructuralNoiseHash(index);
        if (cachedStructuralHashesCpu[index] == structuralHash) continue;
        GenerateLayerNoiseCpu(index);
        cachedStructuralHashesCpu[index] = structuralHash;
        ++regeneratedLayerCount;
    }
    BlendLayersCpu();
    cachedBlendHash = blendHash;
    cachedBlendBackend = Sys::ComputeBackend::Cpu;
    bBlendCacheValid = true;
    lastBackend = Sys::ComputeBackend::Cpu;
}

} // namespace Proc
} // namespace SanmapGen
