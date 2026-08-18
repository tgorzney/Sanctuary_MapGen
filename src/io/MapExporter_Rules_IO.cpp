// MapExporter_Rules_IO.cpp — the four placement rule vectors as `mapGeneratorData` JSON.
// Layer: IO. These are the procedural RULES (PARAMS), not the resolved instances
// (`Data::PlacementResults`) — the recipe regenerates the instances, so the rules are what the
// deterministic shared-generation mode transmits (Constitution §4).
#include "MapExporter_Recipe_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

nlohmann::ordered_json BuildScatterTransformJson(const Params::ScatterTransform& transform) {
    nlohmann::ordered_json json;
    json["ScaleMinimum"]           = transform.scaleMinimum;
    json["ScaleMaximum"]           = transform.scaleMaximum;
    json["RotationMinimumDegrees"] = transform.rotationMinimumDegrees;
    json["RotationMaximumDegrees"] = transform.rotationMaximumDegrees;
    json["AlignToTerrainNormal"]   = transform.bAlignToTerrainNormal;
    json["Collidable"]             = transform.bCollidable;
    // `tpId` is a game-dictated identifier kept verbatim (ARCH §1.1). Stored as a bounded string
    // so a non-terminated buffer can never run off the end (Constitution §6).
    const char* identifier = transform.templateIdentifier;
    std::size_t identifierLength = 0;
    while (identifierLength < sizeof(transform.templateIdentifier) && identifier[identifierLength] != '\0')
        ++identifierLength;
    json["TemplateIdentifier"] = std::string(identifier, identifierLength);
    return json;
}

nlohmann::ordered_json BuildMarkerRuleJson(const Params::MarkerRule& rule) {
    nlohmann::ordered_json json;
    json["Enabled"] = rule.bEnabled;   json["Hidden"] = rule.bHidden;
    json["Category"] = static_cast<int>(rule.category);
    json["MinSlope"] = rule.minSlope;  json["MaxSlope"] = rule.maxSlope;
    json["MinHeight"] = rule.minHeight; json["MaxHeight"] = rule.maxHeight;
    json["MaskStratumIndex"] = rule.maskStratumIndex;
    json["MaskWeightMinimum"] = rule.maskWeightMinimum;
    json["ObstacleDistanceMinimum"] = rule.obstacleDistanceMinimum;
    json["ClearanceSpacing"] = rule.clearanceSpacing;
    json["MapEdgePadding"] = rule.mapEdgePadding;
    json["AreaRadiusMinimum"] = rule.areaRadiusMinimum;
    json["AreaRadiusMaximum"] = rule.areaRadiusMaximum;
    json["CheckMaximumRadius"] = rule.bCheckMaximumRadius;
    json["AreaHeightRange"] = rule.areaHeightRange;
    json["UseDensity"] = rule.bUseDensity;  json["Density"] = rule.density;
    json["Count"] = rule.count;
    json["UseAllPositions"] = rule.bUseAllPositions;
    json["RandomSelection"] = rule.bRandomSelection;
    json["Priority"] = static_cast<int>(rule.priority);
    json["FocusGradient"] = static_cast<int>(rule.focusGradient);
    json["FocusGradientRadius"] = rule.focusGradientRadius;
    json["FocusGradientStrength"] = rule.focusGradientStrength;
    json["FocusGradientContrast"] = rule.focusGradientContrast;
    json["SymmetryUseGlobal"] = rule.bSymmetryUseGlobal;
    json["SymmetryMask"] = rule.symmetryMask;
    json["Transform"] = BuildScatterTransformJson(rule.transform);
    return json;
}

nlohmann::ordered_json BuildPropRuleJson(const Params::PropRule& rule) {
    nlohmann::ordered_json json;
    json["Enabled"] = rule.bEnabled;   json["Density"] = rule.density;
    json["MinSlope"] = rule.minSlope;  json["MaxSlope"] = rule.maxSlope;
    json["MinHeight"] = rule.minHeight; json["MaxHeight"] = rule.maxHeight;
    json["AvoidWater"] = rule.bAvoidWater; json["NearCliffs"] = rule.bNearCliffs;
    json["SpacingMinimum"] = rule.spacingMinimum;
    json["MapEdgePadding"] = rule.mapEdgePadding;
    json["MaskStratumIndex"] = rule.maskStratumIndex;
    json["MaskWeightMinimum"] = rule.maskWeightMinimum;
    json["ObstacleDistanceMinimum"] = rule.obstacleDistanceMinimum;
    json["NearCliffDistanceMaximum"] = rule.nearCliffDistanceMaximum;
    json["SymmetryUseGlobal"] = rule.bSymmetryUseGlobal;
    json["SymmetryMask"] = rule.symmetryMask;
    json["Transform"] = BuildScatterTransformJson(rule.transform);
    return json;
}

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
    json["Transform"] = BuildScatterTransformJson(rule.transform);
    return json;
}

} // namespace

nlohmann::ordered_json BuildPlacementRulesJson(const Params::MapRecipe& recipe) {
    nlohmann::ordered_json markers = nlohmann::ordered_json::array();
    for (const Params::MarkerRule& rule : recipe.markerRules) markers.push_back(BuildMarkerRuleJson(rule));
    nlohmann::ordered_json props = nlohmann::ordered_json::array();
    for (const Params::PropRule& rule : recipe.propRules) props.push_back(BuildPropRuleJson(rule));
    nlohmann::ordered_json decals = nlohmann::ordered_json::array();
    for (const Params::DecalRule& rule : recipe.decalRules) decals.push_back(BuildDecalRuleJson(rule));
    nlohmann::ordered_json units = nlohmann::ordered_json::array();
    for (const Params::UnitRule& rule : recipe.unitRules) units.push_back(BuildUnitRuleJson(rule));
    nlohmann::ordered_json json;
    json["MarkerRules"] = markers;
    json["PropRules"]   = props;
    json["DecalRules"]  = decals;
    json["UnitRules"]   = units;
    return json;
}

} // namespace Io
} // namespace SanmapGen
