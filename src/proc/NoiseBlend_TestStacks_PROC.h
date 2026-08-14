// NoiseBlend_TestStacks_PROC.h — the layer stacks the M3-1 acceptance test runs on, shared by
// the Cpu-side test and the Gpu-parity test so BOTH backends are handed the identical layer
// configuration (the whole point of the stage: one config, two backends). Test support only.
#pragma once
#include <cstdio>
#include <vector>
#include "../params/LayerStack_PARAMS.h"

namespace SanmapGen {
namespace Proc {

// A layer whose shaped value is EXACTLY `value` for every cell: NoiseType::None makes the raw
// noise 0, and Levels output black == white maps whatever arrives to `value`. That turns the
// blend stack into hand-checkable arithmetic with no noise in the way.
inline Params::Layer MakeConstantLayer(float value, Params::HeightBlendMode mode, float opacity,
                                       int stratumIndex) {
    Params::Layer layer;
    layer.noiseType = Params::NoiseType::None;
    layer.fractalType = Params::FractalType::None;
    layer.levelsOutputBlack = value;
    layer.levelsOutputWhite = value;
    layer.blendMode = mode;
    layer.opacity = opacity;
    layer.stratumIndex = stratumIndex;
    return layer;
}

// Base 0.6 (Add onto height 0) then 0.5 combined with `mode` at `opacity`.
inline Params::LayerStack MakeConstantTwoLayerStack(Params::HeightBlendMode mode, float opacity) {
    Params::GeoLayer group;
    group.layers.push_back(MakeConstantLayer(0.6f, Params::HeightBlendMode::Add, 1.0f, 0));
    group.layers.push_back(MakeConstantLayer(0.5f, mode, opacity, 1));
    Params::LayerStack stack;
    stack.geoLayers.push_back(group);
    return stack;
}

struct BlendModeExpectation {
    Params::HeightBlendMode mode;
    float opacity;
    float height;
    const char* label;
};

// Hand-computed from base 0.6 / layer 0.5, clamped to the [0,1] height window.
inline const std::vector<BlendModeExpectation>& BlendModeExpectations() {
    static const std::vector<BlendModeExpectation> expectations = {
        { Params::HeightBlendMode::Add,      1.0f, 1.0f,  "Add: 0.6 + 0.5 clamps to 1.0" },
        { Params::HeightBlendMode::Subtract, 1.0f, 0.1f,  "Subtract: 0.6 - 0.5 = 0.1" },
        { Params::HeightBlendMode::Multiply, 1.0f, 0.3f,  "Multiply: 0.6 * 0.5 = 0.3" },
        { Params::HeightBlendMode::Overlay,  1.0f, 0.6f,  "Overlay: 1 - 2*(1-0.6)*(1-0.5) = 0.6" },
        { Params::HeightBlendMode::Maximum,  1.0f, 0.6f,  "Maximum: max(0.6, 0.5) = 0.6" },
        { Params::HeightBlendMode::Minimum,  1.0f, 0.5f,  "Minimum: min(0.6, 0.5) = 0.5" },
        { Params::HeightBlendMode::Multiply, 0.5f, 0.45f, "Opacity 0.5 on Multiply: 0.6 + (0.3-0.6)*0.5" },
    };
    return expectations;
}

// One layer of a chosen noise/fractal pair, everything else at its default — the isolation
// case for checking the GLSL FastNoiseLite port one generator at a time.
inline Params::LayerStack MakeSingleNoiseStack(Params::NoiseType noiseType,
                                               Params::FractalType fractalType) {
    Params::Layer layer;
    layer.noiseType = noiseType;
    layer.fractalType = fractalType;
    layer.frequency = 0.02f;
    layer.octaves = 3;
    layer.weightedStrength = 0.3f;
    layer.cellularJitter = 0.8f;
    layer.pingPongStrength = 1.6f;
    Params::GeoLayer group;
    group.layers.push_back(layer);
    Params::LayerStack stack;
    stack.geoLayers.push_back(group);
    return stack;
}

inline const char* NoiseCombinationLabel(Params::NoiseType noiseType, Params::FractalType fractalType) {
    static const char* const noiseNames[] = { "OpenSimplex2", "OpenSimplex2Smooth", "Cellular",
                                              "Perlin", "ValueCubic", "Value", "None" };
    static const char* const fractalNames[] = { "None", "FractionalBrownian", "Ridged", "PingPong" };
    static char label[64];
    std::snprintf(label, sizeof(label), "%s/%s", noiseNames[static_cast<int>(noiseType)],
                  fractalNames[static_cast<int>(fractalType)]);
    return label;
}

// --- The reference stack: three layers that exercise every config field the old Gpu path
// dropped on the floor (a different NoiseType and FractalType each, ping-pong, cellular
// jitter, three blend modes, partial opacity, Levels, the density reshapes) across three
// stratums. Built one layer per function so each stays inside the ARCH §1.5 40-line cap.
inline Params::Layer MakeReferenceBaseLayer() {
    Params::Layer base;
    base.noiseType = Params::NoiseType::OpenSimplex2;
    base.fractalType = Params::FractalType::FractionalBrownian;
    base.frequency = 0.008f;
    base.octaves = 4;
    base.stratumIndex = 0;
    return base;
}

inline Params::Layer MakeReferenceRidgedLayer() {
    Params::Layer ridged;
    ridged.noiseType = Params::NoiseType::Perlin;
    ridged.fractalType = Params::FractalType::Ridged;
    ridged.frequency = 0.02f;
    ridged.octaves = 3;
    ridged.weightedStrength = 0.4f;
    ridged.blendMode = Params::HeightBlendMode::Maximum;
    ridged.opacity = 0.7f;
    ridged.levelsShadows = 0.2f;
    ridged.levelsHighlights = 0.9f;
    ridged.mountainDensity = 0.35f;
    ridged.plateauDensity = 0.25f;      // terracing: a floor() knife-edge, on purpose
    ridged.heightBlendContrast = 1.4f;
    ridged.heightBlendMaximum = 0.8f;
    ridged.stratumIndex = 1;
    return ridged;
}

inline Params::Layer MakeReferenceCellularLayer() {
    Params::Layer cellular;
    cellular.noiseType = Params::NoiseType::Cellular;
    cellular.fractalType = Params::FractalType::PingPong;
    cellular.frequency = 0.03f;
    cellular.octaves = 2;
    cellular.pingPongStrength = 1.6f;
    cellular.cellularJitter = 0.8f;
    cellular.blendMode = Params::HeightBlendMode::Multiply;
    cellular.opacity = 0.45f;
    cellular.rampDensity = 0.3f;
    cellular.stratumIndex = 2;
    return cellular;
}

inline Params::LayerStack MakeRepresentativeStack() {
    Params::GeoLayer group;
    group.layers.push_back(MakeReferenceBaseLayer());
    group.layers.push_back(MakeReferenceRidgedLayer());
    group.layers.push_back(MakeReferenceCellularLayer());
    Params::LayerStack stack;
    stack.geoLayers.push_back(group);
    return stack;
}

} // namespace Proc
} // namespace SanmapGen
