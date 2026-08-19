// MapExporter_StratumGeneration_IO.cpp — `recipe.strata` -> the top-level `StratumGenerationSettings`
// array. Layer: IO. SANMAP_FORMAT_SPEC Correction 12: per-stratum soil physics (`Params::Stratum::
// soilPhysics`, 6 fields — genuinely new writes, nothing serializes them today) plus the 8
// slope-gate fields RELOCATED (not duplicated) from the legacy `mapGeneratorData.Stratums` blob's
// `BuildStratumJson` (MapExporter_Layers_IO.cpp), plus the new `SlopeUseGlobal`
// (`Stratum::bSlopeUseGlobal`, MASKING_SPEC.md §1.7). Sibling of `stratumLayers`, index-aligned with
// it, same cardinality pattern as `BuildStratumLayersJson` (MapExporter_Recipe_IO.cpp) — always
// exactly `sanmapStratumCount` entries, padding past `recipe.strata.size()` with `Params::Stratum()`
// defaults.
#include "MapExporter_Recipe_IO.h"
#include "MapExporter_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {

nlohmann::ordered_json BuildStratumGenerationSettingsJson(const Params::MapRecipe& recipe) {
    nlohmann::ordered_json settings = nlohmann::ordered_json::array();
    for (int stratumIndex = 0; stratumIndex < sanmapStratumCount; ++stratumIndex) {
        const bool bHasSettings = stratumIndex < static_cast<int>(recipe.strata.size());
        const Params::Stratum stratum = bHasSettings ? recipe.strata[stratumIndex] : Params::Stratum();
        const Params::StratumSoilPhysics& soilPhysics = stratum.soilPhysics;
        nlohmann::ordered_json entry;
        // Soil physics — new writes (Params::Stratum::soilPhysics, 6 fields).
        entry["Hardness"]           = soilPhysics.hardness;
        entry["Friction"]           = soilPhysics.friction;
        entry["Cohesion"]           = soilPhysics.cohesion;
        entry["CapacityMultiplier"] = soilPhysics.capacityMultiplier;
        entry["AbsorptionRate"]     = soilPhysics.absorptionRate;
        entry["Erodable"]           = soilPhysics.bErodable;
        // Slope gate — 1 new (`SlopeUseGlobal`) + 8 relocated verbatim from the legacy
        // `mapGeneratorData.Stratums` blob's `BuildStratumJson`.
        entry["SlopeUseGlobal"]          = stratum.bSlopeUseGlobal;
        entry["SlopeGateEnabled"]        = stratum.bSlopeGateEnabled;
        entry["MinimumSlopeDegrees"]     = stratum.minimumSlopeDegrees;
        entry["MaximumSlopeDegrees"]     = stratum.maximumSlopeDegrees;
        entry["SlopeFeatherDegreesLow"]  = stratum.slopeFeatherDegreesLow;
        entry["SlopeFeatherDegreesHigh"] = stratum.slopeFeatherDegreesHigh;
        entry["UseSmoothstep"]           = stratum.bUseSmoothstep;
        entry["InvertSlopeGate"]         = stratum.bInvertSlopeGate;
        entry["SlopeGateStrength"]       = stratum.slopeGateStrength;
        settings.push_back(entry);
    }
    return settings;
}

} // namespace Io
} // namespace SanmapGen
