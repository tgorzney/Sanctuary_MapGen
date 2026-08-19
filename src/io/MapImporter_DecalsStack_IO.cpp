// MapImporter_DecalsStack_IO.cpp — the top-level `DecalsStack` array -> `recipe.decalRules`.
// Layer: IO. The exact inverse of MapExporter_DecalsStack_IO.cpp. `ReadDecalRuleJson`'s body is
// relocated verbatim from the deleted MapImporter_Rules_IO.cpp; only its container changed. Same
// tier/calling contract as `areas`/`armies`/`PropGroups`/etc.: takes the top-level `document`
// directly and is called unconditionally, BEFORE the `mapGeneratorData` presence gate.
#include "MapImporter_Recipe_IO.h"
#include "MapImporter_ScatterTransform_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

void ReadDecalRuleJson(const nlohmann::json& json, Params::DecalRule& rule) {
    ReadSharedRuleGates(json, rule);
    ReadJsonFloat(json, "Density", rule.density);
    ReadJsonFloat(json, "SpacingMinimum", rule.spacingMinimum);
    ReadJsonBoolean(json, "SymmetryUseGlobal", rule.bSymmetryUseGlobal);
    ReadJsonInteger(json, "SymmetryMask", rule.symmetryMask);
    ReadJsonIntegerClamped(json, "RadialSymmetryRepeatCount", Params::radialSymmetryRepeatCountMinimum,
                          Params::radialSymmetryRepeatCountMaximum, rule.radialSymmetryRepeatCount);
}

} // namespace

void ReadDecalsStackJson(const nlohmann::json& document, Params::MapRecipe& outRecipe) {
    ReadRuleArray(document, "DecalsStack", outRecipe.decalRules, ReadDecalRuleJson);
}

} // namespace Io
} // namespace SanmapGen
