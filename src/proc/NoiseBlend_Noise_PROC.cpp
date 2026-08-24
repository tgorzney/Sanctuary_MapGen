// NoiseBlend_Noise_PROC.cpp — CPU raw-noise generation for one layer (the accuracy path).
// Layer: PROC. Backed by the project's FastNoiseLite; its GPU twin is the FastNoiseLite
// port in NoiseBlend_Hash/Fractal/Simplex/SimplexSmooth/Lattice/Cellular_PROC.glsl, which
// carries the SAME full configuration (type, fractal, lacunarity, ping-pong, jitter) so a
// layer bakes the same shape on either backend (NOISE_BLEND_SPEC "CPU vs GPU").
// Output is the structural noise remapped to 0..1 and cached per layer; reshape and blend
// are deliberately NOT applied here (that is the second dirty-hash level).
#include "NoiseBlend_PROC.h"
#include "../sys/ThreadPool_SYS.h"
#include "FastNoiseLite.h"

namespace SanmapGen {
namespace Proc {
namespace {

FastNoiseLite::NoiseType TranslateNoiseType(Params::NoiseType noiseType) {
    switch (noiseType) {
        case Params::NoiseType::OpenSimplex2Smooth: return FastNoiseLite::NoiseType_OpenSimplex2S;
        case Params::NoiseType::Cellular:           return FastNoiseLite::NoiseType_Cellular;
        case Params::NoiseType::Perlin:             return FastNoiseLite::NoiseType_Perlin;
        case Params::NoiseType::ValueCubic:         return FastNoiseLite::NoiseType_ValueCubic;
        case Params::NoiseType::Value:              return FastNoiseLite::NoiseType_Value;
        default:                                    return FastNoiseLite::NoiseType_OpenSimplex2;
    }
}

FastNoiseLite::FractalType TranslateFractalType(Params::FractalType fractalType) {
    switch (fractalType) {
        case Params::FractalType::FractionalBrownian: return FastNoiseLite::FractalType_FBm;
        case Params::FractalType::Ridged:             return FastNoiseLite::FractalType_Ridged;
        case Params::FractalType::PingPong:           return FastNoiseLite::FractalType_PingPong;
        default:                                      return FastNoiseLite::FractalType_None;
    }
}

// Fills `target` with a baked layer's frozen pixels: an exact copy at matching resolution, a
// bilinear resample if the map size changed since baking, or a flat 0 contribution if no image
// has been recorded yet for this layerIdentifier (Constitution §6 — degrade, never crash).
void FillBakedLayerCpu(const Params::Layer& layer,
                       const std::vector<Data::BakedLayerImage>& bakedLayerImages,
                       Data::FloatField& target) {
    const Data::FloatField* baked = Data::FindBakedLayerImage(bakedLayerImages, layer.layerIdentifier);
    if (baked == nullptr) { target.Fill(0.0f); return; }
    const int vertexSize = target.Width();
    if (baked->Width() == vertexSize && baked->Height() == vertexSize) {
        target = *baked;
        return;
    }
    const float scaleX = vertexSize > 1 ? (baked->Width()  - 1) / static_cast<float>(vertexSize - 1) : 0.0f;
    const float scaleY = vertexSize > 1 ? (baked->Height() - 1) / static_cast<float>(vertexSize - 1) : 0.0f;
    for (int y = 0; y < vertexSize; ++y)
        for (int x = 0; x < vertexSize; ++x)
            target.Set(x, y, baked->SampleBilinear(x * scaleX, y * scaleY));
}

} // namespace

void NoiseBlendStage::GenerateLayerNoiseCpu(std::size_t layerIndex) {
    const std::vector<const Params::Layer*> flatLayers = layerStack.GetFlatLayers();
    Data::FloatField& target = cachedRawNoiseCpu[layerIndex];
    if (layerIndex < flatLayers.size() && flatLayers[layerIndex]->bBaked) {
        FillBakedLayerCpu(*flatLayers[layerIndex], bakedLayerImages, target);
        return;
    }

    const LayerKernelConfiguration& configuration = layerConfigurations[layerIndex];
    if (configuration.noiseType == static_cast<int>(Params::NoiseType::None)) {
        target.Fill(0.0f);
        return;
    }

    FastNoiseLite noise;
    noise.SetSeed(configuration.layerSeed);
    noise.SetNoiseType(TranslateNoiseType(static_cast<Params::NoiseType>(configuration.noiseType)));
    noise.SetFractalType(TranslateFractalType(static_cast<Params::FractalType>(configuration.fractalType)));
    noise.SetFrequency(configuration.frequency);
    noise.SetFractalOctaves(configuration.octaves);
    noise.SetFractalGain(configuration.gain);
    noise.SetFractalLacunarity(configuration.lacunarity);
    noise.SetFractalWeightedStrength(configuration.weightedStrength);
    noise.SetFractalPingPongStrength(configuration.pingPongStrength);
    noise.SetCellularJitter(configuration.cellularJitter);

    const int vertexSize = target.Width();
    const float noiseOffset = constants.rawNoiseOffset;
    const float noiseScale = constants.rawNoiseScale;
    // Rows are independent, so the partition is irrelevant to the result (deterministic).
    const auto fillRow = [&](int y) {
        for (int x = 0; x < vertexSize; ++x)
            target.Set(x, y, (noise.GetNoise(static_cast<float>(x), static_cast<float>(y)) + noiseOffset) * noiseScale);
    };
    if (threadPool != nullptr) threadPool->ParallelFor(0, vertexSize, fillRow);
    else for (int y = 0; y < vertexSize; ++y) fillRow(y);
}

} // namespace Proc
} // namespace SanmapGen
