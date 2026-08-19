// MapExporter_UnitsStack_IO.cpp — `recipe.unitRules` -> the top-level `UnitsStack` array.
// Layer: IO. SANMAP_FORMAT_SPEC Correction 7 (ruling #1: a bare top-level array, same shape as
// `PropGroups`/`DecalGroups`/`StratumGenerationSettings`). `BuildUnitRuleJson`'s body is relocated
// verbatim from the deleted MapExporter_Rules_IO.cpp; only its container changed.
#include "MapExporter_Recipe_IO.h"
#include "MapExporter_ScatterTransform_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

nlohmann::ordered_json BuildUnitRuleJson(const Params::UnitRule& rule) {
    nlohmann::ordered_json json;
    json["Enabled"] = rule.bEnabled;   json["ArmyIndex"] = rule.armyIndex;
    json["Count"] = rule.count;
    json["MinSlope"] = rule.minSlope;  json["MaxSlope"] = rule.maxSlope;
    json["MinHeight"] = rule.minHeight; json["MaxHeight"] = rule.maxHeight;
    json["SpacingMinimum"] = rule.spacingMinimum;
    json["MapEdgePadding"] = rule.mapEdgePadding;
    json["MaskStratumIndex"] = rule.maskStratumIndex;
    json["MaskWeightMinimum"] = rule.maskWeightMinimum;
    json["SymmetryUseGlobal"] = rule.bSymmetryUseGlobal;
    json["SymmetryMask"] = rule.symmetryMask;
    json["RadialSymmetryRepeatCount"] = rule.radialSymmetryRepeatCount;
    json["Transform"] = BuildScatterTransformJson(rule.transform);
    return json;
}

} // namespace

nlohmann::ordered_json BuildUnitsStackJson(const Params::MapRecipe& recipe) {
    nlohmann::ordered_json unitsStack = nlohmann::ordered_json::array();
    for (const Params::UnitRule& rule : recipe.unitRules)
        unitsStack.push_back(BuildUnitRuleJson(rule));
    return unitsStack;
}

} // namespace Io
} // namespace SanmapGen
