// MapImporter_SlopeDefaults_IO.cpp — the top-level `.sanmap` `SlopeDefaults` object ->
// `recipe.slopeDefaults`. Layer: IO. The exact inverse of MapExporter_SlopeDefaults_IO.cpp.
// Same tier and calling contract as `areas`/`armies`/`atmosphere`: takes the top-level `document`
// directly and must be called unconditionally, BEFORE the `mapGeneratorData` presence gate
// (STEP10_SlopeDefaults_Mechanism, following the wiring-order rule every prior top-level-key
// ticket this session has used). Total per Constitution §6: a missing/non-object key leaves
// `outRecipe.slopeDefaults` on its own (Stratum-matching) defaults.
#include "JsonPrimitives_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {

void ReadSlopeDefaultsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe) {
    if (!document.contains("SlopeDefaults") || !document["SlopeDefaults"].is_object()) return;
    const nlohmann::json& json = document["SlopeDefaults"];
    Params::SlopeDefaults& slopeDefaults = outRecipe.slopeDefaults;
    ReadJsonBoolean(json, "bSlopeGateEnabled", slopeDefaults.bSlopeGateEnabled);
    ReadJsonFloat(json, "minimumSlopeDegrees", slopeDefaults.minimumSlopeDegrees);
    ReadJsonFloat(json, "maximumSlopeDegrees", slopeDefaults.maximumSlopeDegrees);
    ReadJsonFloat(json, "slopeFeatherDegreesLow", slopeDefaults.slopeFeatherDegreesLow);
    ReadJsonFloat(json, "slopeFeatherDegreesHigh", slopeDefaults.slopeFeatherDegreesHigh);
    ReadJsonBoolean(json, "bUseSmoothstep", slopeDefaults.bUseSmoothstep);
    ReadJsonBoolean(json, "bInvertSlopeGate", slopeDefaults.bInvertSlopeGate);
    ReadJsonFloat(json, "slopeGateStrength", slopeDefaults.slopeGateStrength);
}

} // namespace Io
} // namespace SanmapGen
