// MapImporter_UnitsStack_IO.cpp — the top-level `UnitsStack` array -> `recipe.unitRules`.
// Layer: IO. The exact inverse of MapExporter_UnitsStack_IO.cpp. `ReadUnitRuleJson`'s body is
// relocated verbatim from the deleted MapImporter_Rules_IO.cpp; only its container changed. Same
// tier/calling contract as `areas`/`armies`/`PropGroups`/etc.: takes the top-level `document`
// directly and is called unconditionally, BEFORE the `mapGeneratorData` presence gate.
#include "MapImporter_Recipe_IO.h"
#include "MapImporter_ScatterTransform_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

void ReadUnitRuleJson(const nlohmann::json& json, Params::UnitRule& rule) {
    ReadSharedRuleGates(json, rule);
    ReadJsonInteger(json, "ArmyIndex", rule.armyIndex);
    ReadJsonInteger(json, "Count", rule.count);
    ReadJsonFloat(json, "SpacingMinimum", rule.spacingMinimum);
    ReadJsonBoolean(json, "SymmetryUseGlobal", rule.bSymmetryUseGlobal);
    ReadJsonInteger(json, "SymmetryMask", rule.symmetryMask);
    ReadJsonInteger(json, "RadialSymmetryRepeatCount", rule.radialSymmetryRepeatCount);
}

} // namespace

void ReadUnitsStackJson(const nlohmann::json& document, Params::MapRecipe& outRecipe) {
    ReadRuleArray(document, "UnitsStack", outRecipe.unitRules, ReadUnitRuleJson);
}

} // namespace Io
} // namespace SanmapGen
