// MapExporter_PropsStack_IO.cpp — `recipe.propRules` -> the top-level `PropsStack` array.
// Layer: IO. SANMAP_FORMAT_SPEC Correction 7 (ruling #1: a bare top-level array, same shape as
// `PropGroups`/`DecalGroups`/`StratumGenerationSettings`). `BuildPropRuleJson`'s body is relocated
// verbatim from the deleted MapExporter_Rules_IO.cpp; only its container changed.
#include "MapExporter_Recipe_IO.h"
#include "MapExporter_ScatterTransform_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

nlohmann::ordered_json BuildPropRuleJson(const Params::PropRule& rule) {
    nlohmann::ordered_json json;
    json["Enabled"] = rule.bEnabled;   json["Density"] = rule.density;
    json["MinSlope"] = rule.minSlope;  json["MaxSlope"] = rule.maxSlope;
    json["MinHeight"] = rule.minHeight; json["MaxHeight"] = rule.maxHeight;
    json["AvoidWater"] = rule.bAvoidWater; json["NearCliffs"] = rule.bNearCliffs;
    json["Reclaimable"] = rule.bReclaimable;
    json["SpacingMinimum"] = rule.spacingMinimum;
    json["MapEdgePadding"] = rule.mapEdgePadding;
    json["MaskStratumIndex"] = rule.maskStratumIndex;
    json["MaskWeightMinimum"] = rule.maskWeightMinimum;
    json["ObstacleDistanceMinimum"] = rule.obstacleDistanceMinimum;
    json["NearCliffDistanceMaximum"] = rule.nearCliffDistanceMaximum;
    json["SymmetryUseGlobal"] = rule.bSymmetryUseGlobal;
    json["SymmetryMask"] = rule.symmetryMask;
    json["RadialSymmetryRepeatCount"] = rule.radialSymmetryRepeatCount;
    json["Transform"] = BuildScatterTransformJson(rule.transform);
    return json;
}

} // namespace

nlohmann::ordered_json BuildPropsStackJson(const Params::MapRecipe& recipe) {
    nlohmann::ordered_json propsStack = nlohmann::ordered_json::array();
    for (const Params::PropRule& rule : recipe.propRules)
        propsStack.push_back(BuildPropRuleJson(rule));
    return propsStack;
}

} // namespace Io
} // namespace SanmapGen
