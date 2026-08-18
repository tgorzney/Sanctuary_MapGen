// MapExporter_SlopeDefaults_IO.cpp — `recipe.slopeDefaults` -> the top-level `.sanmap`
// `SlopeDefaults` object. Layer: IO. STEP10_SlopeDefaults_Mechanism: one flat object, sibling of
// `armies`/`atmosphere`/etc., NOT nested in `mapGeneratorData` — the PARAMS-side shared-default
// layer MASKING_SPEC.md §1.7 describes. Same 7 field names as `Params::Stratum`'s own slope-gate
// block (SlopeDefaults_PARAMS.h), so the JSON keys match the stratum settings' own spelling
// verbatim (no format-dictated rename here — this is a SanGen-owned section, not a legacy C# one).
#include "MapExporter_Recipe_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {

nlohmann::ordered_json BuildSlopeDefaultsJson(const Params::MapRecipe& recipe) {
    const Params::SlopeDefaults& slopeDefaults = recipe.slopeDefaults;
    nlohmann::ordered_json json;
    json["bSlopeGateEnabled"]       = slopeDefaults.bSlopeGateEnabled;
    json["minimumSlopeDegrees"]     = slopeDefaults.minimumSlopeDegrees;
    json["maximumSlopeDegrees"]     = slopeDefaults.maximumSlopeDegrees;
    json["slopeFeatherDegreesLow"]  = slopeDefaults.slopeFeatherDegreesLow;
    json["slopeFeatherDegreesHigh"] = slopeDefaults.slopeFeatherDegreesHigh;
    json["bUseSmoothstep"]          = slopeDefaults.bUseSmoothstep;
    json["bInvertSlopeGate"]        = slopeDefaults.bInvertSlopeGate;
    json["slopeGateStrength"]       = slopeDefaults.slopeGateStrength;
    return json;
}

} // namespace Io
} // namespace SanmapGen
