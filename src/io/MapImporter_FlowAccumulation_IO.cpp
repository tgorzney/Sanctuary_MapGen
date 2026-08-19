// MapImporter_FlowAccumulation_IO.cpp — the top-level `.sanmap` `Flow`/`Accumulation` objects ->
// `recipe.flow`/`recipe.accumulation`. Layer: IO. The exact inverse of
// MapExporter_FlowAccumulation_IO.cpp. Same tier and calling contract as `areas`/`armies`/
// `atmosphere`/`SlopeDefaults`: takes the top-level `document` directly and must be called
// unconditionally, BEFORE the `mapGeneratorData` presence gate. Total per Constitution §6: a
// missing/non-object key leaves the destination on its own defaults.
//
// `ReadAccumulationJson` deliberately reads NOTHING out of `Accumulation` yet — the spec has no
// field list for it (Correction 6, "TBD" means TBD) — but still accepts the key being present with
// arbitrary/future content: an object with unknown keys is silently ignored, never a warning or a
// failure, so a later ticket that adds real Accumulation fields does not need this reader rewritten
// just to stop it from choking on them.
#include "JsonPrimitives_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {

void ReadFlowJson(const nlohmann::json& document, Params::MapRecipe& outRecipe) {
    if (!document.contains("Flow") || !document["Flow"].is_object()) return;
    const nlohmann::json& json = document["Flow"];
    if (!json.contains("FlowMapColor") || !json["FlowMapColor"].is_object()) return;
    const nlohmann::json& color = json["FlowMapColor"];
    Params::Flow& flow = outRecipe.flow;
    ReadJsonFloat(color, "r", flow.flowMapColor[0]);
    ReadJsonFloat(color, "g", flow.flowMapColor[1]);
    ReadJsonFloat(color, "b", flow.flowMapColor[2]);
    ReadJsonFloat(color, "a", flow.flowMapColor[3]);
}

void ReadAccumulationJson(const nlohmann::json& /*document*/, Params::MapRecipe& /*outRecipe*/) {
    // Nothing to read yet (Correction 6: no field list exists). A present-but-empty object, a
    // present object carrying unrecognized future keys, and a missing key are all equally fine —
    // none of them is a warning or a failure, matching this project's degrade-gracefully IO posture.
}

} // namespace Io
} // namespace SanmapGen
