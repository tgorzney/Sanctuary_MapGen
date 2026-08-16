// StratumSoilPhysics_PARAMS.h — the soil-physics settings of ONE stratum. Layer: PARAMS.
// A MEMBER file of `Stratum_PARAMS.h` (ARCH §7.1: "Composition is allowed; rival top-level types
// are not"), split out only because the §1.5 ceiling forbids one fat stratum header. No stage
// reaches this type independently — it is read as `Params::Stratum::soilPhysics`.
//
// THIS IS THE SETTINGS HOME the ARCH names: §7.1 lists "the soil physics" among the things reached
// through `Params::Stratum`. `Proc::MaterialPhysics` (Erosion_Physics_PROC.h) is the RUNTIME record
// the sim kernels read; it carries the same five numbers and the same defaults so the copy is
// literal and cannot transpose two values. Until PIPELINE seeds that record from the recipe, the UI
// pushes settings -> record (StratumsTab_SoilPhysics_UI.h) — one direction, one authority.
//
// Settings only: no behavior, no computed data (ARCH §3.2).
#pragma once

namespace SanmapGen {
namespace Params {

struct StratumSoilPhysics {
    float hardness           = 0.2f;   // 0 = mud, 1 = bedrock; scales the erosion rate
    float friction           = 0.8f;   // feeds droplet inertia
    float cohesion           = 0.5f;   // talus / angle-of-repose limit (read by the thermal stage)
    float capacityMultiplier = 2.0f;   // how much sediment this material lets water carry
    float absorptionRate     = 0.01f;  // fraction of water the soil drinks per step
    bool  bErodable          = true;   // false = a droplet cannot carve this stratum
};

} // namespace Params
} // namespace SanmapGen
