// MapExporter_PropsStack_IO.cpp — `recipe.propRules` -> the top-level `PropsStack` array.
// Layer: IO. SANMAP_FORMAT_SPEC Correction 7 (ruling #1: a bare top-level array, same shape as
// `PropGroups`/`DecalGroups`/`StratumGenerationSettings`). `BuildPropRuleJson`'s body is relocated
// verbatim from the deleted MapExporter_Rules_IO.cpp; only its container changed.
#include "FootprintBakeFingerprint_IO.h"
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
    json["BaseFootprintWidth"] = rule.baseFootprintWidth;
    json["BaseFootprintDepth"] = rule.baseFootprintDepth;
    json["FootprintBakeFingerprint"] = BuildFootprintBakeFingerprintJson(rule.footprintBakeFingerprint);
    json["SymmetryUseGlobal"] = rule.symmetry.bSymmetryUseGlobal;
    json["SymmetryMask"] = rule.symmetry.symmetryMask;
    json["RadialSymmetryRepeatCount"] = rule.symmetry.radialSymmetryRepeatCount;
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

// `GlobalPropSettings` — its own top-level key, a sibling of `PropsStack` (ARCH §20, mirrors
// `BuildGlobalMarkerSettingsJson`'s placement beside `MarkersStack`), not nested inside it.
nlohmann::ordered_json BuildGlobalPropSettingsJson(const Params::MapRecipe& recipe) {
    const Params::GlobalPropSettings& settings = recipe.globalPropSettings;
    nlohmann::ordered_json json;
    json["ColorProp"] = { { "r", settings.colorProp[0] }, { "g", settings.colorProp[1] },
                          { "b", settings.colorProp[2] }, { "a", settings.colorProp[3] } };
    json["ColorReclaim"] = { { "r", settings.colorReclaim[0] }, { "g", settings.colorReclaim[1] },
                             { "b", settings.colorReclaim[2] }, { "a", settings.colorReclaim[3] } };
    return json;
}

} // namespace Io
} // namespace SanmapGen
