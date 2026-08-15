// NoiseBlend_Kernel_PROC.h — the one noise/blend kernel contract shared by both backends.
// Layer: PROC. Declares (a) every tweakable stage constant (Constitution §8 — nothing the
// kernels use is baked into code or shader) and (b) the per-layer configuration record,
// whose field order/type is the std430 layout the GLSL twin mirrors EXACTLY. Declared once
// here so the CPU struct and the shader block can never drift (DISPATCH_INTERFACE_SPEC §4).
#pragma once

namespace SanmapGen {
namespace Proc {

// The kernel runs the same program twice: one pass fills a single layer's raw noise (so the
// two-level dirty hash works on the Gpu as well), the other blends the whole stack.
namespace NoiseBlendPassMode {
    constexpr int Noise = 0;
    constexpr int Blend = 1;
}

// std430 binding points, declared here once and injected into the GLSL as #defines so the
// C++ BindBuffer calls and the shader layout qualifiers can never disagree.
namespace NoiseBlendBinding {
    constexpr unsigned layerConfigurations = 0;
    constexpr unsigned rawNoise            = 1;
    constexpr unsigned heightField         = 2;
    constexpr unsigned materialProportions = 3;
    constexpr unsigned layerThickness      = 4;   // blend-pass scratch (see NoiseBlend_PROC.glsl)
}

// Stage constants — defaults only; every one is settable per project (§8).
struct NoiseBlendConstants {
    float landDensityScale       = 2.0f;    // reshape: value *= landDensity * this
    float terraceCountBase       = 3.0f;    // plateau terracing: count = base + density * range
    float terraceCountRange      = 27.0f;
    float rawNoiseOffset         = 1.0f;    // raw noise remap to 0..1: (noise + offset) * scale
    float rawNoiseScale          = 0.5f;
    float heightMinimum          = 0.0f;    // blended-height clamp window
    float heightMaximum          = 1.0f;
    float occlusionWindowEpsilon = 0.001f;  // MASKING_SPEC swap guard on an empty window
    int   layerSeedStride        = 1;       // layer seed = geometry.seed + index * this
    int   maximumGpuLayerCount   = 32;      // deeper stacks fall back to the Cpu; also bounds the
                                            // blend pass's per-layer scratch buffer
    int   gpuFenceMaximumPollCount = 2000000; // fence-wait budget; the poll loop YIELDS between
                                             // polls, never busy-spins (a hot spin starves the
                                             // ThreadPool workers sharing the cores)
};

// One flattened layer, ready for either backend. 32 scalars = 128 bytes; the trailing
// padding keeps the std430 array stride a 16-byte multiple. Order is load-bearing.
struct LayerKernelConfiguration {
    int   noiseType              = 0;   // Params::NoiseType   as int
    int   fractalType            = 0;   // Params::FractalType as int
    int   layerSeed              = 0;
    int   octaves                = 1;
    float frequency              = 0.0f;
    float gain                   = 0.0f;
    float lacunarity             = 0.0f;
    float weightedStrength       = 0.0f;
    float pingPongStrength       = 0.0f;
    float cellularJitter         = 0.0f;
    float fractalBounding        = 1.0f;   // FastNoiseLite's 1/sum(amplitudes), precomputed
    float landDensityMultiplier  = 1.0f;   // landDensity * landDensityScale
    float mountainDensity        = 0.0f;
    float plateauDensity         = 0.0f;
    float rampDensity            = 0.0f;
    float terraceHeight          = 1.0f;
    float terraceHeightReciprocal = 1.0f;  // reciprocal multiply, never divide in the loop
    float levelsShadows          = 0.0f;
    float levelsMidtones         = 1.0f;
    float levelsHighlights       = 1.0f;
    float levelsRangeReciprocal  = 1.0f;   // 1/(highlights-shadows); 0 marks a degenerate range
    float levelsOutputBlack      = 0.0f;
    float levelsOutputWhite      = 1.0f;
    int   blendMode              = 0;   // Params::HeightBlendMode as int
    int   stratumIndex           = 0;
    float opacity                = 1.0f;
    float heightBlendContrast    = 1.0f;
    float occlusionWindowLow     = 0.0f;   // swap-guarded heightBlendMinimum
    float occlusionWindowHigh    = 1.0f;   // swap-guarded heightBlendMaximum
    float heightMinimum          = 0.0f;   // copies of the clamp window, so the shader needs
    float heightMaximum          = 1.0f;   // no float uniforms (SYS exposes int uniforms only)
    float padding                = 0.0f;
};

} // namespace Proc
} // namespace SanmapGen
