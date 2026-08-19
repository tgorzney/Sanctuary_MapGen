// MapExporter_DecalsStack_IO.cpp — `recipe.decalRules` -> the top-level `DecalsStack` array.
// Layer: IO. SANMAP_FORMAT_SPEC Correction 7 (ruling #1: a bare top-level array, same shape as
// `PropGroups`/`DecalGroups`/`StratumGenerationSettings`). `BuildDecalRuleJson`'s body is relocated
// verbatim from the deleted MapExporter_Rules_IO.cpp; only its container changed.
#include "MapExporter_Recipe_IO.h"
#include "MapExporter_ScatterTransform_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

nlohmann::ordered_json BuildDecalRuleJson(const Params::DecalRule& rule) {
    nlohmann::ordered_json json;
    json["Enabled"] = rule.bEnabled;   json["Density"] = rule.density;
    json["MinSlope"] = rule.minSlope;  json["MaxSlope"] = rule.maxSlope;
    json["MinHeight"] = rule.minHeight; json["MaxHeight"] = rule.maxHeight;
    json["SpacingMinimum"] = rule.spacingMinimum;
    json["MapEdgePadding"] = rule.mapEdgePadding;
    json["MaskStratumIndex"] = rule.maskStratumIndex;
    json["MaskWeightMinimum"] = rule.maskWeightMinimum;
    json["SymmetryUseGlobal"] = rule.bSymmetryUseGlobal;
    json["SymmetryMask"] = rule.symmetryMask;
    json["Transform"] = BuildScatterTransformJson(rule.transform);
    return json;
}

} // namespace

nlohmann::ordered_json BuildDecalsStackJson(const Params::MapRecipe& recipe) {
    nlohmann::ordered_json decalsStack = nlohmann::ordered_json::array();
    for (const Params::DecalRule& rule : recipe.decalRules)
        decalsStack.push_back(BuildDecalRuleJson(rule));
    return decalsStack;
}

} // namespace Io
} // namespace SanmapGen
