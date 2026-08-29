// PreviewComposite_TestScene_UI.h — the synthetic baked scene both composite acceptance tests
// run against, plus the shared check/inspect helpers. Test-support only; no GL.
// The fields are deliberately uniform so every expected color is exact arithmetic rather than a
// golden image, and the map is tiny (5x5 vertices -> a 4x4 preview) so a failure can be read
// pixel by pixel.
#pragma once
#include "PreviewComposite_UI.h"
#include <cstdio>

namespace SanmapGen {
namespace Ui {

struct PreviewTestScene {
    Params::Geometry             geometry;
    Params::Water                water;
    std::vector<Params::Stratum> strata;
    // ARCH §14.17 — empty by default; BuildMapAreaConfigurations pushes a degenerate sentinel
    // rectangle for an empty list, so leaving this untouched is a legal, no-op scene.
    std::vector<Params::MapArea> areas;
    Data::MapFields              fields;
    Data::PlacementInstances     instances;
    Data::EntityIdBuffer         entityIdentifiers;
};

inline int previewTestFailureCount = 0;

inline void CheckPreviewExpectation(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++previewTestFailureCount; }
}

inline int ChannelByte(unsigned int packedTexel, int channel) {
    return static_cast<int>((packedTexel >> (channel * 8)) & 0xFFu);
}

// The composite writes bytes, so an expectation computed in floats is compared with one
// quantization step of slack (the Visual-class tolerance the bake test also uses).
inline bool ChannelNear(unsigned int packedTexel, int channel, float expected) {
    const int expectedByte = static_cast<int>(ClampUnit(expected) * 255.0f + 0.5f);
    const int difference = ChannelByte(packedTexel, channel) - expectedByte;
    return difference <= 1 && difference >= -1;
}

// One layer's worth of the byte round-trip the composite performs between passes, so a test can
// re-derive an expected pixel independently of the library's blend implementation.
inline float QuantizeChannel(float value) {
    return static_cast<float>(static_cast<int>(ClampUnit(value) * 255.0f + 0.5f)) * (1.0f / 255.0f);
}

// A 5x5-vertex bake: flat height 0.25, flow 1.0, stratum 0 covering half the surface, one
// resolved instance at the map centre. Nothing here is a sim input — these are baked results.
inline void BuildPreviewTestScene(PreviewTestScene& scene) {
    scene.geometry.mapSize = 4;
    scene.geometry.terrainMaxHeight = 100.0f;
    scene.fields.Resize(scene.geometry.VertexSize(), 0.0f);
    scene.fields.heightfield.Fill(0.25f);
    scene.fields.flow.Fill(1.0f);
    scene.fields.surfaceStratumWeights[0].Fill(0.5f);
    scene.fields.materialProportions[0].Fill(1.0f);      // the sim-owned field the preview ignores
    scene.strata.assign(2, Params::Stratum());
    scene.strata[0].tintRed = 0.8f; scene.strata[0].tintGreen = 0.2f; scene.strata[0].tintBlue = 0.1f;
    scene.strata[1].bEnabled = false;
    Data::PlacementInstance instance;
    instance.positionX = 2.0f;                            // world units; worldUnitsPerCell = 1
    instance.positionZ = 2.0f;
    scene.instances.Clear();
    scene.instances.Append(instance);
}

inline Params::GradientRamp MakeBlackToWhiteRamp() {
    Params::GradientRamp ramp;
    Params::GradientStop low;  low.color[0] = 0.0f; low.color[1] = 0.0f; low.color[2] = 0.0f;
    Params::GradientStop high; high.location = 1.0f;
    ramp.stops.push_back(low);
    ramp.stops.push_back(high);
    ramp.bSmoothInterpolation = false;
    return ramp;
}

// A one-stop ramp bakes a constant table, so a layer using it contributes one exact color.
inline Params::GradientRamp MakeConstantRamp(float red, float green, float blue, float alpha) {
    Params::GradientRamp ramp;
    Params::GradientStop stop;
    stop.color[0] = red; stop.color[1] = green; stop.color[2] = blue; stop.color[3] = alpha;
    ramp.stops.push_back(stop);
    return ramp;
}

inline PreviewFieldLayer MakeLayer(PreviewLayerKind kind, PreviewBlendMode blendMode, int rampIndex,
                                   float domainMinimum, float domainMaximum) {
    PreviewFieldLayer layer;
    layer.kind = kind;
    layer.blendMode = blendMode;
    layer.gradientRampIndex = rampIndex;
    layer.domainMinimum = domainMinimum;
    layer.domainMaximum = domainMaximum;
    return layer;
}

// Height ramp -> stratum splat -> flow, at a 4x4 preview: the three colorization paths the
// work-order names (height shading, the surface-weight splat, a LUT-colorized field).
inline void ConfigurePreviewSettings(PreviewCompositeSettings& settings) {
    settings.previewResolution = 4;
    settings.gradientRamps.push_back(MakeBlackToWhiteRamp());
    settings.gradientRamps.push_back(MakeConstantRamp(0.0f, 0.0f, 1.0f, 0.4f));
    settings.fieldLayers.push_back(
        MakeLayer(PreviewLayerKind::HeightRamp, PreviewBlendMode::AlphaBlend, 0, 0.0f, 1.0f));
    settings.fieldLayers.push_back(
        MakeLayer(PreviewLayerKind::StratumSplat, PreviewBlendMode::AlphaBlend, -1, 0.0f, 1.0f));
    settings.fieldLayers.push_back(
        MakeLayer(PreviewLayerKind::Flow, PreviewBlendMode::Add, 1, 0.0f, 2.0f));
    settings.entityMarkRadiusPixels = 0.9f;
    settings.entityMarkColor[0] = 1.0f; settings.entityMarkColor[1] = 0.0f;
    settings.entityMarkColor[2] = 0.0f; settings.entityMarkColor[3] = 1.0f;
}

} // namespace Ui
} // namespace SanmapGen
