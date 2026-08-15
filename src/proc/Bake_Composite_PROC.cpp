// Bake_Composite_PROC.cpp — the CPU twin of the bake kernel: for every output texel, sample
// each stratum's SURFACE weight, sample that stratum's tiled albedo (or its flat preview tint
// when it has no texture), and accumulate the weighted composite. Mirrors Bake_PROC.glsl
// expression for expression — same tiling, same bilinear filters, same round-to-nearest
// quantization — so the Visual-class backends agree (ARCH §6.1).
// No remap here: the weights arrive already remapped from the Mask stage (ARCH §7.2.5).
#include "Bake_PROC.h"
#include "Bake_Sampling_PROC.h"
#include "../sys/ThreadPool_SYS.h"

namespace SanmapGen {
namespace Proc {
namespace {

// Everything one output texel needs from the stage, gathered once so the per-texel kernel
// stays a pure function of its inputs (and short enough to read, ARCH §1.5).
struct TexelKernelInputs {
    const StratumKernelConfiguration* configurations;
    const unsigned int* const*        albedoTexels;      // one borrowed pointer per stratum
    const Data::FloatField*           surfaceStratumWeights;
    const BakeConstants*              constants;
    float weightScale;
    int   baseStratumIndex;
};

// One texel of the composite: weights out, weighted colour out. Twin of compositeTexel() in
// Bake_PROC.glsl.
void CompositeTexel(const TexelKernelInputs& inputs, float mapU, float mapV,
                    float* weights, float& red, float& green, float& blue) {
    float weightTotal = 0.0f;
    red = 0.0f; green = 0.0f; blue = 0.0f;
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum) {
        const StratumKernelConfiguration& configuration = inputs.configurations[stratum];
        const float weight = configuration.bEnabled != 0
            ? inputs.surfaceStratumWeights[stratum].SampleBilinear(mapU * inputs.weightScale,
                                                                   mapV * inputs.weightScale)
            : 0.0f;
        weights[stratum] = weight;
        if (weight > 0.0f) {
            float stratumRed = 0.0f, stratumGreen = 0.0f, stratumBlue = 0.0f;
            SampleStratumColor(configuration, inputs.albedoTexels[stratum],
                               mapU, mapV, stratumRed, stratumGreen, stratumBlue);
            red += weight * stratumRed; green += weight * stratumGreen; blue += weight * stratumBlue;
        }
        weightTotal += weight;
    }
    if (weightTotal <= inputs.constants->weightEpsilon) {
        const int base = inputs.baseStratumIndex;
        SampleStratumColor(inputs.configurations[base], inputs.albedoTexels[base],
                           mapU, mapV, red, green, blue);
    } else if (inputs.constants->bNormalizeWeights) {
        const float weightReciprocal = 1.0f / weightTotal;
        red *= weightReciprocal; green *= weightReciprocal; blue *= weightReciprocal;
    }
}

} // namespace

void BakeStage::RunOnCpu() {
    PrepareRun();
    CompositeCpu();
}

void BakeStage::CompositeCpu() {
    const int resolution = bakedTextures.resolution;
    if (resolution <= 0 || !mapFields.IsSized() || stratumConfigurations.empty()) return;
    const int stratumCount = Data::MapFields::stratumCount;
    const unsigned int* albedoTexels[Data::MapFields::stratumCount] = {};
    for (int stratum = 0; stratum < stratumCount; ++stratum)
        albedoTexels[stratum] = static_cast<std::size_t>(stratum) < stratumArt.size()
                              ? stratumArt[stratum].albedoTexels : nullptr;

    TexelKernelInputs inputs;
    inputs.configurations         = stratumConfigurations.data();
    inputs.albedoTexels           = albedoTexels;
    inputs.surfaceStratumWeights  = mapFields.surfaceStratumWeights;
    inputs.constants              = &constants;
    inputs.weightScale            = static_cast<float>(mapFields.VertexSize() - 1);
    inputs.baseStratumIndex = constants.baseStratumIndex < 0 || constants.baseStratumIndex >= stratumCount
                            ? 0 : constants.baseStratumIndex;
    const float resolutionReciprocal = 1.0f / static_cast<float>(resolution);
    const float alphaValue = constants.compositeAlphaValue;
    BakedTextureSet& textures = bakedTextures;

    // One output row per task: rows are independent, so the threaded and serial bakes are
    // identical texel for texel. Each row owns its weight scratch.
    const auto bakeRow = [&inputs, &textures, resolution, resolutionReciprocal, alphaValue](int pixelY) {
        float weights[Data::MapFields::stratumCount];
        const float mapV = (static_cast<float>(pixelY) + 0.5f) * resolutionReciprocal;
        for (int pixelX = 0; pixelX < resolution; ++pixelX) {
            const float mapU = (static_cast<float>(pixelX) + 0.5f) * resolutionReciprocal;
            float red = 0.0f, green = 0.0f, blue = 0.0f;
            CompositeTexel(inputs, mapU, mapV, weights, red, green, blue);
            const int texelIndex = pixelY * resolution + pixelX;
            textures.compositeAlbedo[texelIndex] = PackRgba8(red, green, blue, alphaValue);
            textures.stratumMaskLow[texelIndex]  = PackRgba8(weights[1], weights[2], weights[3], weights[4]);
            textures.stratumMaskHigh[texelIndex] = PackRgba8(weights[5], weights[6], weights[7], weights[8]);
        }
    };

    if (threadPool != nullptr) threadPool->ParallelFor(0, resolution, bakeRow);
    else for (int pixelY = 0; pixelY < resolution; ++pixelY) bakeRow(pixelY);
}

} // namespace Proc
} // namespace SanmapGen
