// MapImporter_StratumGeneration_IO.cpp — the top-level `StratumGenerationSettings[9]` array ->
// `Params::Stratum::soilPhysics` + the 9 slope-gate fields. Layer: IO. The exact inverse of
// `BuildStratumGenerationSettingsJson` (MapExporter_StratumGeneration_IO.cpp), SANMAP_FORMAT_SPEC
// Correction 12.
//
// Growing only (never shrinking, Constitution §6), same pattern `ReadStratumLayersJson` (Step 11)
// already established: this runs at the same top-level tier as that sibling reader and must not
// clear-and-rebuild `outRecipe.strata`, or it would stomp whatever `ReadStratumLayersJson` already
// wrote there.
#include "MapImporter_Recipe_IO.h"
#include "MapImporter_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

// One `StratumGenerationSettings[]` entry -> one `Params::Stratum`'s soil physics + slope gate.
// Total per Constitution §6: a missing/wrong-typed sub-key leaves that one field on whatever it
// already held.
void ReadStratumGenerationSettingsEntryJson(const nlohmann::json& entryJson, Params::Stratum& stratum) {
    Params::StratumSoilPhysics& soilPhysics = stratum.soilPhysics;
    ReadJsonFloat(entryJson, "Hardness", soilPhysics.hardness);
    ReadJsonFloat(entryJson, "Friction", soilPhysics.friction);
    ReadJsonFloat(entryJson, "Cohesion", soilPhysics.cohesion);
    ReadJsonFloat(entryJson, "CapacityMultiplier", soilPhysics.capacityMultiplier);
    ReadJsonFloat(entryJson, "AbsorptionRate", soilPhysics.absorptionRate);
    ReadJsonBoolean(entryJson, "Erodable", soilPhysics.bErodable);

    ReadJsonBoolean(entryJson, "SlopeUseGlobal", stratum.bSlopeUseGlobal);
    ReadJsonBoolean(entryJson, "SlopeGateEnabled", stratum.bSlopeGateEnabled);
    ReadJsonFloat(entryJson, "MinimumSlopeDegrees", stratum.minimumSlopeDegrees);
    ReadJsonFloat(entryJson, "MaximumSlopeDegrees", stratum.maximumSlopeDegrees);
    ReadJsonFloat(entryJson, "SlopeFeatherDegreesLow", stratum.slopeFeatherDegreesLow);
    ReadJsonFloat(entryJson, "SlopeFeatherDegreesHigh", stratum.slopeFeatherDegreesHigh);
    ReadJsonBoolean(entryJson, "UseSmoothstep", stratum.bUseSmoothstep);
    ReadJsonBoolean(entryJson, "InvertSlopeGate", stratum.bInvertSlopeGate);
    ReadJsonFloat(entryJson, "SlopeGateStrength", stratum.slopeGateStrength);
}

} // namespace

void ReadStratumGenerationSettingsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe,
                                       MapImportResult& result) {
    if (!document.contains("StratumGenerationSettings")
        || !document["StratumGenerationSettings"].is_array()) return;
    const nlohmann::json& settings = document["StratumGenerationSettings"];

    // Cardinality rule (Correction 12): compare against `stratumLayers`'s own actual array length —
    // the two SanGen-owned arrays, to each other — not each independently against
    // `sanmapStratumCount`. Read straight off the document rather than `outRecipe.strata.size()`,
    // which may already have been grown by `ReadStratumLayersJson` running earlier.
    std::size_t stratumLayersLength = 0;
    if (document.contains("stratumLayers") && document["stratumLayers"].is_array())
        stratumLayersLength = document["stratumLayers"].size();
    if (settings.size() != stratumLayersLength) {
        result.Warn("StratumGenerationSettings has " + std::to_string(settings.size())
                    + " entries, but stratumLayers has " + std::to_string(stratumLayersLength)
                    + "; the two arrays should stay index-aligned.");
    }

    if (outRecipe.strata.size() < settings.size())
        outRecipe.strata.resize(settings.size());
    for (std::size_t index = 0; index < settings.size(); ++index) {
        if (settings[index].is_object())
            ReadStratumGenerationSettingsEntryJson(settings[index], outRecipe.strata[index]);
    }
}

} // namespace Io
} // namespace SanmapGen
