// NoiseBlend_Baked_PROC_Test.cpp — STEP100 acceptance: a bBaked layer reads its frozen
// Data::BakedLayerImage instead of regenerating live noise. Its own binary rather than a
// fourth translation unit of NoiseBlend_PROC_Test's group (NoiseBlend_FeatureScale_PROC_Test's
// precedent) — it drives only the Cpu path, so it needs neither a GL context nor the parity
// fixtures those files carry. The Gpu-fallback-with-a-working-GL-program half of this ticket's
// acceptance test lives in NoiseBlend_GpuBlend_PROC_Test.cpp's CheckBakedLayerForcesCpuFallback
// (it needs a real compiled program to prove the refusal is deliberate, not merely "no manager").
#include "NoiseBlend_PROC.h"
#include <cmath>
#include <cstdio>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

Params::LayerStack OneBakedLayerStack(int layerIdentifier) {
    Params::Layer layer;
    layer.bBaked = true;
    layer.layerIdentifier = layerIdentifier;
    Params::GeoLayer group;
    group.layers.push_back(layer);
    Params::LayerStack stack;
    stack.geoLayers.push_back(group);
    return stack;
}

// A baked layer at the SAME resolution as the generated grid reproduces its frozen pixels
// exactly, and a second, unchanged run is a cache-skip no-op (matching the existing cache-skip
// posture in NoiseBlend_PROC_Test.cpp).
void CheckBakedExactReproductionAndCacheSkip() {
    Params::Geometry geometry;
    geometry.mapSize = 15;   // vertexSize 16
    const int vertexSize = geometry.VertexSize();

    Data::FloatField pattern(vertexSize, vertexSize);
    for (int y = 0; y < vertexSize; ++y)
        for (int x = 0; x < vertexSize; ++x)
            pattern.Set(x, y, static_cast<float>((x * 7 + y * 13) % 101) / 100.0f);

    Params::LayerStack stack = OneBakedLayerStack(0);
    std::vector<Data::BakedLayerImage> bakedImages(1);
    bakedImages[0].layerIdentifier = 0;
    bakedImages[0].image = pattern;

    Data::MapFields fields;
    Proc::NoiseBlendStage stage(geometry, stack, fields, bakedImages);
    stage.RunOnCpu();

    bool bExact = true;
    for (int y = 0; y < vertexSize; ++y)
        for (int x = 0; x < vertexSize; ++x)
            if (fields.heightfield.Get(x, y) != pattern.Get(x, y)) bExact = false;
    Check(bExact, "a baked layer at matching resolution reproduces the frozen pixels exactly");

    stage.RunOnCpu();
    Check(stage.WasLastRunSkipped(), "an unchanged re-run of a baked stack is skipped (cache-skip)");
}

// The map was resized since the layer was baked: the frozen image is bilinear-resampled onto
// the new grid, the ONE resampler (mirrors MASKING_SPEC 1.8's mask-import posture).
void CheckBakedBilinearResampleOnResize() {
    Params::Geometry geometry;
    geometry.mapSize = 15;   // vertexSize 16, baked at a different, smaller resolution
    const int vertexSize = geometry.VertexSize();
    const int bakedSize = 9;

    Data::FloatField pattern(bakedSize, bakedSize);
    for (int y = 0; y < bakedSize; ++y)
        for (int x = 0; x < bakedSize; ++x)
            pattern.Set(x, y, static_cast<float>((x * 3 + y * 5) % 17) / 16.0f);

    Params::LayerStack stack = OneBakedLayerStack(0);
    std::vector<Data::BakedLayerImage> bakedImages(1);
    bakedImages[0].layerIdentifier = 0;
    bakedImages[0].image = pattern;

    Data::MapFields fields;
    Proc::NoiseBlendStage stage(geometry, stack, fields, bakedImages);
    stage.RunOnCpu();

    const float scale = (bakedSize - 1) / static_cast<float>(vertexSize - 1);
    bool bMatches = true;
    for (int y = 0; y < vertexSize; ++y)
        for (int x = 0; x < vertexSize; ++x) {
            const float expected = pattern.SampleBilinear(x * scale, y * scale);
            if (std::fabs(fields.heightfield.Get(x, y) - expected) > 1e-6f) bMatches = false;
        }
    Check(bMatches, "a baked layer resampled after a map resize matches bilinear sampling");
}

// bBaked==true with no matching layerIdentifier in bakedLayerImages degrades to flat (all
// zero), never a crash (Constitution §6, Params::Layer::bBaked's own documented contract).
void CheckBakedMissingImageDegradesToFlat() {
    Params::Geometry geometry;
    geometry.mapSize = 7;
    Params::LayerStack stack = OneBakedLayerStack(99);
    std::vector<Data::BakedLayerImage> bakedImages;   // empty -- no matching identifier

    Data::MapFields fields;
    Proc::NoiseBlendStage stage(geometry, stack, fields, bakedImages);
    stage.RunOnCpu();

    bool bAllZero = true;
    for (std::size_t cell = 0; cell < fields.heightfield.CellCount(); ++cell)
        if (fields.heightfield.Data()[cell] != 0.0f) bAllZero = false;
    Check(bAllZero, "a baked layer with no matching layerIdentifier degrades to flat, never a crash");
}

// Baking is NOT one-way: a layer that ALSO carries live noise settings resumes generating
// noise identical to a never-baked layer with the same settings once bBaked flips back off.
void CheckUnbakeResumesLiveNoise() {
    Params::Geometry geometry;
    geometry.mapSize = 63;
    geometry.seed = 7u;

    Params::Layer liveLayer;
    liveLayer.noiseType = Params::NoiseType::OpenSimplex2;
    liveLayer.fractalType = Params::FractalType::FractionalBrownian;
    liveLayer.frequency = 0.02f;
    liveLayer.octaves = 3;

    Params::GeoLayer neverBakedGroup;
    neverBakedGroup.layers.push_back(liveLayer);
    Params::LayerStack neverBakedStack;
    neverBakedStack.geoLayers.push_back(neverBakedGroup);

    Data::MapFields neverBakedFields;
    std::vector<Data::BakedLayerImage> noBakedImages;
    Proc::NoiseBlendStage neverBakedStage(geometry, neverBakedStack, neverBakedFields, noBakedImages);
    neverBakedStage.RunOnCpu();

    Params::GeoLayer togglingGroup;
    togglingGroup.layers.push_back(liveLayer);
    Params::LayerStack togglingStack;
    togglingStack.geoLayers.push_back(togglingGroup);

    std::vector<Data::BakedLayerImage> bakedImages;
    Data::MapFields togglingFields;
    Proc::NoiseBlendStage togglingStage(geometry, togglingStack, togglingFields, bakedImages);
    togglingStage.RunOnCpu();

    // Bake it: a frozen image now stands in for the live noise.
    Data::FloatField frozen(geometry.VertexSize(), geometry.VertexSize(), 0.75f);
    togglingStack.geoLayers[0].layers[0].bBaked = true;
    togglingStack.geoLayers[0].layers[0].layerIdentifier = 0;
    bakedImages.push_back(Data::BakedLayerImage{ 0, frozen });
    togglingStage.RunOnCpu();
    Check(std::fabs(togglingFields.heightfield.Get(3, 3) - 0.75f) < 1e-6f,
          "baking a live layer switches its output to the frozen pixels");

    // Unbake: the SAME recipe resumes live generation, identical to a never-baked layer.
    togglingStack.geoLayers[0].layers[0].bBaked = false;
    togglingStage.RunOnCpu();

    bool bIdentical = true;
    for (std::size_t cell = 0; cell < neverBakedFields.heightfield.CellCount(); ++cell)
        if (neverBakedFields.heightfield.Data()[cell] != togglingFields.heightfield.Data()[cell])
            bIdentical = false;
    Check(bIdentical, "unbaking resumes live noise identical to a never-baked layer with the same settings");
}

} // namespace

int main() {
    CheckBakedExactReproductionAndCacheSkip();
    CheckBakedBilinearResampleOnResize();
    CheckBakedMissingImageDegradesToFlat();
    CheckUnbakeResumesLiveNoise();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
