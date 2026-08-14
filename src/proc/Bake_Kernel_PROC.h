// Bake_Kernel_PROC.h — the one bake kernel contract shared by both backends.
// Layer: PROC. Declares (a) every tweakable stage constant (Constitution §8 — nothing the
// kernels use is baked into code or shader), (b) the per-stratum settings the caller
// supplies, (c) the flattened per-stratum record whose field order/type IS the std430
// layout the GLSL twin mirrors EXACTLY (DISPATCH_INTERFACE_SPEC §4), and (d) the baked
// texture set the stage writes.
// The stratum settings live here (not in PARAMS) only because `Stratum_PARAMS` is owned by
// another work-order; StratumBakeSource is the record it will populate, field for field.
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

// One stratum's bake settings: its albedo texture, preview tint, tiling and mask remap.
// `albedoPixels` is RGBA8 packed little-endian (red in bits 0..7); null = flat tint only.
// `textureVersion` is bumped by the asset loader when the pixels change — the parameter
// hash reads it instead of walking megabytes of texels every Run().
struct StratumBakeSource {
    const unsigned int* albedoPixels     = nullptr;
    int   albedoWidth                    = 0;
    int   albedoHeight                   = 0;
    int   textureVersion                 = 0;
    float tintRed                        = 1.0f;   // previewColor / diffuseRemap
    float tintGreen                      = 1.0f;
    float tintBlue                       = 1.0f;
    float tileCount                      = 1.0f;   // texture repeats across the map (tileSize)
    float maskRemapMinimum               = 0.0f;   // maskRemapMin (SANMAP_FORMAT_SPEC, .x channel)
    float maskRemapMaximum               = 1.0f;   // maskRemapMax
    bool  bEnabled                       = true;
};

// One flattened stratum, ready for either backend. 12 scalars = 48 bytes; that is a
// 16-byte multiple, so the std430 array stride matches without trailing padding. Order is
// load-bearing. The stage-wide floats are copied into every record because the SYS seam
// exposes integer uniforms only.
struct StratumKernelConfiguration {
    int   albedoPixelOffset        = 0;    // first texel of this stratum in the shared texel buffer
    int   albedoWidth              = 0;    // 0 = no texture, use the tint as a flat color
    int   albedoHeight             = 0;
    int   bEnabled                 = 1;
    float tintRed                  = 1.0f;
    float tintGreen                = 1.0f;
    float tintBlue                 = 1.0f;
    float tileCount                = 1.0f;
    float maskRemapMinimum         = 0.0f;
    float maskRemapRangeReciprocal = 1.0f; // 1/(max-min); reciprocal multiply, never divide
    float weightEpsilon            = 0.0001f;
    int   bNormalizeWeights        = 1;
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
