// Bake_Kernel_PROC.h — the one bake kernel contract shared by both backends.
// Layer: PROC. Declares (a) every tweakable stage constant (Constitution §8 — nothing the
// kernels use is baked into code or shader), (b) the flattened per-stratum record whose field
// order/type IS the std430 layout the GLSL twin mirrors EXACTLY (DISPATCH_INTERFACE_SPEC §4),
// and (c) the baked texture set the stage writes.
// There is NO per-stratum settings type here: the settings live in `Params::Stratum` and the
// loaded pixels in `Data::StratumArt` (ARCH §7.1). Only the flattened GPU-layout record stays
// in PROC. Bake also has no remap of its own — the ONE remap happened in Mask (ARCH §7.2.5);
// Bake consumes `surfaceStratumWeights` verbatim.
#pragma once
#include <vector>
#include <cstddef>

namespace SanmapGen {
namespace Proc {

// Stage constants — defaults only; every one is settable per project (§8).
struct BakeConstants {
    int   outputResolutionMultiplier = 2;       // texture side = mapSize * this (LAYER_SYSTEM_SPEC
    int   minimumOutputResolution    = 64;      // "~2x map res, <= 4096")
    int   maximumOutputResolution    = 4096;
    int   baseStratumIndex           = 0;       // the always-present base (no mask of its own)
    float weightEpsilon              = 0.0001f; // total weight at/below this -> base stratum only
    bool  bNormalizeWeights          = true;    // weighted average (else straight accumulation)
    float compositeAlphaValue        = 1.0f;    // alpha written into the composite albedo
};

// One flattened stratum, ready for either backend. 10 live scalars + 2 explicit padding words
// = 48 bytes, a 16-byte multiple, so the std430 array stride matches the C++ layout with no
// implicit trailing padding (the pad replaces the two deleted remap floats, ARCH §7.2.5).
// Order is load-bearing. The stage-wide floats are copied into every record because the SYS
// seam exposes integer uniforms only.
struct StratumKernelConfiguration {
    int   albedoPixelOffset        = 0;    // first texel of this stratum in the shared texel buffer
    int   albedoWidth              = 0;    // 0 = no texture, use the tint as a flat color
    int   albedoHeight             = 0;
    int   bEnabled                 = 1;
    float tintRed                  = 1.0f;
    float tintGreen                = 1.0f;
    float tintBlue                 = 1.0f;
    float tileCount                = 1.0f;
    float weightEpsilon            = 0.0001f;
    int   bNormalizeWeights        = 1;
    int   paddingFirst             = 0;    // std430 stride pad: 40 -> 48 bytes (16-byte multiple)
    int   paddingSecond            = 0;
};

// The baked output set: the composite albedo plus the two packed stratum-mask textures the
// export writes as stratums_1_4 / stratums_5_8 (LAYER_SYSTEM_SPEC). All RGBA8, row-major,
// `resolution` square. Writing them to disk is the IO export work-order's job.
struct BakedTextureSet {
    int resolution = 0;
    std::vector<unsigned int> compositeAlbedo;
    std::vector<unsigned int> stratumMaskLow;   // strata 1..4 in R,G,B,A
    std::vector<unsigned int> stratumMaskHigh;  // strata 5..8 in R,G,B,A

    void Resize(int side) {
        resolution = side < 0 ? 0 : side;
        const std::size_t texelCount = static_cast<std::size_t>(resolution) * resolution;
        compositeAlbedo.assign(texelCount, 0u);
        stratumMaskLow.assign(texelCount, 0u);
        stratumMaskHigh.assign(texelCount, 0u);
    }
    bool IsSized() const { return resolution > 0 && !compositeAlbedo.empty(); }
};

} // namespace Proc
} // namespace SanmapGen
