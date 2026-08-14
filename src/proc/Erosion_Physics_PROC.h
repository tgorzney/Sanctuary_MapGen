// Erosion_Physics_PROC.h — per-material soil physics + the stage-wide erosion constants.
// Layer: PROC. Every number the erosion kernels use lives here or in
// Erosion_Settings_PROC.h — nothing is baked into the .cpp or the .glsl (Constitution §8;
// this is where the old hardcoded shader erosion rate 0.3 and capacity "* 4.0" die).
// One record per stratum index, exactly the palette LAYER_SYSTEM_SPEC describes; the sim
// looks physics up by the column's current top material.
#pragma once

namespace SanmapGen {
namespace Proc {

// Soil physics of one stratum (SIM_ALGORITHMS_SPEC "per-material physics").
struct MaterialPhysics {
    float hardness           = 0.2f;   // 0 = mud, 1 = bedrock; scales the erosion rate
    float friction           = 0.8f;   // feeds droplet inertia
    float cohesion           = 0.5f;   // talus/repose limit (read by the thermal stage)
    float capacityMultiplier = 2.0f;   // how much sediment this material lets water carry
    float absorptionRate     = 0.01f;  // fraction of water the soil drinks per step
    bool  bErodable          = true;   // false = the droplet cannot carve this stratum
};

// Stage-wide constants shared by every layer pass and both backends.
struct ErosionConstants {
    // Fixed-point erosion state (DETERMINISM_SPEC "integer accumulation for the
    // feedback-sensitive erosion state"). 2^20 ticks per height unit: ~1e-6 resolution and
    // room for a height of ~2047 before an int32 overflows. Integer addition is exact and
    // order-independent, which is ALSO what makes the GPU atomic scatter race-free.
    float heightFixedPointScale = 1048576.0f;

    // Droplets die this far from the border (the old hardcoded 1.0 / mapSize-2 margin).
    float boundaryMargin = 1.0f;

    // Rejection sampling gives up and accepts a uniform spawn after this many rejections.
    int spawnRejectionSafetyLimit = 1000000;

    // Floor used when normalising the rain map, so an all-dry map cannot divide by zero.
    float rainMapMinimumMaximum = 0.001f;

    // Seed offsets keeping the rain field, the spawn sampler and the droplet meander on
    // independent random streams.
    int rainNoiseSeedOffset = 9999;
    int spawnSeedOffset     = 4242;
    int meanderSeedOffset   = 1337;

    float HeightFixedPointInverse() const { return 1.0f / heightFixedPointScale; }
};

} // namespace Proc
} // namespace SanmapGen
