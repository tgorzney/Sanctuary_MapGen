// Erosion_Settings_PROC.h — one erosion (simulation) layer's settings.
// Layer: PROC. Erosion is per-layer `ErodeBeneath`, NOT one global pass: the stage holds
// one of these per stratum slot, so a pass carves top-down through the stack and deposits
// into its own stratum (LAYER_SYSTEM_SPEC "Simulation layer"). Every rate, threshold and
// iteration count is a field — Constitution §8, no designer-unreachable constant.
// (Follow-up work-order: relocate this record to `ErosionFlow_PARAMS` once that PARAMS
// module exists; the field set is already the serialisable recipe half.)
#pragma once

namespace SanmapGen {
namespace Proc {

struct ErosionLayerSettings {
    bool bEnabled = false;

    // Droplet population and lifetime
    int dropletCount    = 20000;
    int maximumLifetime = 30;

    // Rates (the shader's old hardcoded 0.3 pair, now settable)
    float baseErosionRate    = 0.3f;
    float baseDepositionRate = 0.3f;
    float evaporationRate    = 0.02f;
    float gravity            = 4.0f;

    // Carrying capacity: max(-deltaHeight * speed * water * capacityBaseMultiplier *
    //                        materialCapacityMultiplier * carryingCapacityScale, capacityMinimum)
    float capacityBaseMultiplier = 4.0f;
    float carryingCapacityScale  = 1.0f;
    float capacityMinimum        = 0.01f;

    // Direction blend: inertia = (inertiaBase + (1 - friction) * inertiaFrictionScale) / viscosity
    float inertiaBase          = 0.05f;
    float inertiaFrictionScale = 0.1f;
    float fluidViscosity       = 1.0f;
    float minimumViscosity     = 0.1f;

    // Meander / divergence — the CPU-only term the old GPU path lacked. Now shared: the
    // random stream is an integer hash of (droplet, step, seed), identical on both backends.
    float meanderStrength     = 0.0f;
    float slopeAdherence      = 1.0f;   // 1 = follow the gradient exactly, 0 = free to wander
    float divergenceThreshold = 1.0f;   // steeper than this and meander is suppressed

    // Termination / bookkeeping
    float waterMinimum            = 0.001f;
    float sedimentMinimum         = 0.0001f;
    float thicknessEpsilon        = 0.0001f;
    bool  bConserveSedimentAtExit = true;   // dump the load at the last valid cell (mass sanity)

    // Stack coupling
    bool  bErodeBeneath       = true;    // carve the whole stack, not just this stratum
    bool  bDepositionMode     = false;   // deposit-only droplets (dune/alluvium mode)
    float initialSedimentLoad = 0.0f;    // deposition-mode starting load
    float depositionModeCapacityGain = 100.0f;

    // Rain field (spawn distribution)
    bool  bUseRainNoise      = false;
    float rainNoiseFrequency = 0.01f;
    int   rainNoiseOctaves   = 4;
    float rainNoiseGain      = 0.5f;
    float rainNoiseLacunarity = 2.0f;
    float rainNoiseThreshold = 0.3f;

    // Orographic rain shadow (wind vs slope)
    bool  bUseOrographicRain  = false;
    float windAngleDegrees    = 0.0f;
    float orographicStrength  = 100.0f;
    float orographicMinimum   = 0.1f;
    float orographicMaximum   = 2.0f;
    float rainHeightScale     = 2.0f;
    float rainHeightMinimum   = 0.5f;
    float rainHeightMaximum   = 1.5f;

    // Deposition-mode spawn height band
    float spawnMinimumHeight = 0.0f;
    float spawnMaximumHeight = 1.0f;

    // Accumulation DAG (CPU-only ordered spillover valley fill)
    bool  bAccurateSimultaneousAccumulation = false;
    float spilloverThreshold = 0.05f;
    float spilloverShare     = 0.5f;   // fraction of the height difference a cell may pass on
};

} // namespace Proc
} // namespace SanmapGen
