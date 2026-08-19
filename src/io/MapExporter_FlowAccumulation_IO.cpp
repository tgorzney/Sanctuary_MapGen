// MapExporter_FlowAccumulation_IO.cpp — `recipe.flow`/`recipe.accumulation` -> the top-level
// `Flow`/`Accumulation` `.sanmap` objects. Layer: IO. SANMAP_FORMAT_SPEC Correction 6: two flat
// objects, siblings of `armies`/`atmosphere`/`SlopeDefaults`/etc., NOT nested in `mapGeneratorData`.
// `FlowMapColor` reuses the `{r,g,b,a}` shape already shipped for `armyColor` (Step 2). `Accumulation`
// writes as a genuinely empty object — the spec has no field list for it yet.
#include "MapExporter_Recipe_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {

nlohmann::ordered_json BuildFlowJson(const Params::MapRecipe& recipe) {
    const Params::Flow& flow = recipe.flow;
    nlohmann::ordered_json json;
    json["FlowMapColor"] = { { "r", flow.flowMapColor[0] }, { "g", flow.flowMapColor[1] },
                              { "b", flow.flowMapColor[2] }, { "a", flow.flowMapColor[3] } };
    return json;
}

nlohmann::ordered_json BuildAccumulationJson(const Params::MapRecipe& /*recipe*/) {
    // Genuinely empty — Params::Accumulation has no fields yet (Correction 6: "TBD" means TBD).
    return nlohmann::ordered_json::object();
}

} // namespace Io
} // namespace SanmapGen
