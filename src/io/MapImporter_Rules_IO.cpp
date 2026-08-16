// MapImporter_Rules_IO.cpp — `mapGeneratorData.PlacementRules` -> the recipe's four rule vectors.
// Layer: IO. The exact inverse of MapExporter_Rules_IO.cpp.
#include "MapImporter_Recipe_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

constexpr int markerCategoryCount = 4;   // Generic, Spawn, Alloys, Expansion
constexpr int markerPriorityCount = 3;   // LargestArea, SmallestArea, LeastVariance
constexpr int focusGradientCount  = 4;   // None, CenterFocus, EdgeFocus, Torus

void ReadScatterTransformJson(const nlohmann::json& parent, Params::ScatterTransform& transform) {
    if (!parent.contains("Transform") || !parent["Transform"].is_object()) return;
    const nlohmann::json& json = parent["Transform"];
    ReadJsonFloat(json, "ScaleMinimum", transform.scaleMinimum);
    ReadJsonFloat(json, "ScaleMaximum", transform.scaleMaximum);
    ReadJsonFloat(json, "RotationMinimumDegrees", transform.rotationMinimumDegrees);
    ReadJsonFloat(json, "RotationMaximumDegrees", transform.rotationMaximumDegrees);
    ReadJsonBoolean(json, "AlignToTerrainNormal", transform.bAlignToTerrainNormal);
    ReadJsonBoolean(json, "Collidable", transform.bCollidable);
    std::string templateIdentifier;
    if (!ReadJsonText(json, "TemplateIdentifier", templateIdentifier)) return;
    // The buffer is fixed width and must stay NUL-terminated whatever the document claimed.
    const std::size_t capacity = sizeof(transform.templateIdentifier) - 1u;
    const std::size_t copyLength = templateIdentifier.size() < capacity ? templateIdentifier.size()
                                                                        : capacity;
    for (std::size_t index = 0; index < sizeof(transform.templateIdentifier); ++index)
        transform.templateIdentifier[index] = index < copyLength ? templateIdentifier[index] : '\0';
}

// The gates every rule family shares, so the four readers below stay inside the ARCH §1.5 ceiling.
template <typename RuleType>
void ReadSharedRuleGates(const nlohmann::json& json, RuleType& rule) {
    ReadJsonBoolean(json, "Enabled", rule.bEnabled);
    ReadJsonFloat(json, "MinSlope", rule.minSlope);
    ReadJsonFloat(json, "MaxSlope", rule.maxSlope);
    ReadJsonFloat(json, "MinHeight", rule.minHeight);
    ReadJsonFloat(json, "MaxHeight", rule.maxHeight);
    ReadJsonInteger(json, "MapEdgePadding", rule.mapEdgePadding);
    ReadJsonInteger(json, "MaskStratumIndex", rule.maskStratumIndex);
    ReadJsonFloat(json, "MaskWeightMinimum", rule.maskWeightMinimum);
    ReadScatterTransformJson(json, rule.transform);
}

void ReadMarkerRuleJson(const nlohmann::json& json, Params::MarkerRule& rule) {
    ReadSharedRuleGates(json, rule);
    ReadJsonBoolean(json, "Hidden", rule.bHidden);
    int enumerationValue = static_cast<int>(rule.category);
    if (ReadJsonEnumeration(json, "Category", markerCategoryCount, enumerationValue))
        rule.category = static_cast<Params::MarkerCategory>(enumerationValue);
    ReadJsonFloat(json, "ObstacleDistanceMinimum", rule.obstacleDistanceMinimum);
    ReadJsonFloat(json, "ClearanceSpacing", rule.clearanceSpacing);
    ReadJsonFloat(json, "AreaRadiusMinimum", rule.areaRadiusMinimum);
    ReadJsonFloat(json, "AreaRadiusMaximum", rule.areaRadiusMaximum);
    ReadJsonBoolean(json, "CheckMaximumRadius", rule.bCheckMaximumRadius);
    ReadJsonFloat(json, "AreaHeightRange", rule.areaHeightRange);
    ReadJsonBoolean(json, "UseDensity", rule.bUseDensity);
    ReadJsonFloat(json, "Density", rule.density);
    ReadJsonInteger(json, "Count", rule.count);
    ReadJsonBoolean(json, "UseAllPositions", rule.bUseAllPositions);
    ReadJsonBoolean(json, "RandomSelection", rule.bRandomSelection);
    enumerationValue = static_cast<int>(rule.priority);
    if (ReadJsonEnumeration(json, "Priority", markerPriorityCount, enumerationValue))
        rule.priority = static_cast<Params::MarkerPriority>(enumerationValue);
    enumerationValue = static_cast<int>(rule.focusGradient);
    if (ReadJsonEnumeration(json, "FocusGradient", focusGradientCount, enumerationValue))
        rule.focusGradient = static_cast<Params::FocusGradient>(enumerationValue);
    ReadJsonFloat(json, "FocusGradientRadius", rule.focusGradientRadius);
    ReadJsonFloat(json, "FocusGradientStrength", rule.focusGradientStrength);
    ReadJsonFloat(json, "FocusGradientContrast", rule.focusGradientContrast);
    ReadJsonBoolean(json, "SymmetryUseGlobal", rule.bSymmetryUseGlobal);
    ReadJsonInteger(json, "SymmetryMask", rule.symmetryMask);
}

void ReadPropRuleJson(const nlohmann::json& json, Params::PropRule& rule) {
    ReadSharedRuleGates(json, rule);
    ReadJsonFloat(json, "Density", rule.density);
    ReadJsonBoolean(json, "AvoidWater", rule.bAvoidWater);
    ReadJsonBoolean(json, "NearCliffs", rule.bNearCliffs);
    ReadJsonFloat(json, "SpacingMinimum", rule.spacingMinimum);
    ReadJsonFloat(json, "ObstacleDistanceMinimum", rule.obstacleDistanceMinimum);
    ReadJsonFloat(json, "NearCliffDistanceMaximum", rule.nearCliffDistanceMaximum);
    ReadJsonBoolean(json, "SymmetryUseGlobal", rule.bSymmetryUseGlobal);
    ReadJsonInteger(json, "SymmetryMask", rule.symmetryMask);
}

void ReadDecalRuleJson(const nlohmann::json& json, Params::DecalRule& rule) {
    ReadSharedRuleGates(json, rule);
    ReadJsonFloat(json, "Density", rule.density);
    ReadJsonFloat(json, "SpacingMinimum", rule.spacingMinimum);
}

void ReadUnitRuleJson(const nlohmann::json& json, Params::UnitRule& rule) {
    ReadSharedRuleGates(json, rule);
    ReadJsonInteger(json, "ArmyIndex", rule.armyIndex);
    ReadJsonInteger(json, "Count", rule.count);
    ReadJsonFloat(json, "SpacingMinimum", rule.spacingMinimum);
    ReadJsonBoolean(json, "SymmetryUseGlobal", rule.bSymmetryUseGlobal);
    ReadJsonInteger(json, "SymmetryMask", rule.symmetryMask);
}

// One rule array -> one recipe vector. `ReadOneRule` fills a default-constructed rule, so a
// non-object entry simply yields the default instead of aborting the whole import.
template <typename RuleType, typename ReadOneRuleFunction>
void ReadRuleArray(const nlohmann::json& parent, const char* key, std::vector<RuleType>& outRules,
                   ReadOneRuleFunction ReadOneRule) {
    if (!parent.contains(key) || !parent[key].is_array()) return;
    outRules.clear();
    for (const nlohmann::json& ruleJson : parent[key]) {
        RuleType rule;
        if (ruleJson.is_object()) ReadOneRule(ruleJson, rule);
        outRules.push_back(rule);
    }
}

} // namespace

void ReadPlacementRulesJson(const nlohmann::json& generatorData, Params::MapRecipe& outRecipe) {
    if (!generatorData.contains("PlacementRules") || !generatorData["PlacementRules"].is_object()) return;
    const nlohmann::json& rules = generatorData["PlacementRules"];
    ReadRuleArray(rules, "MarkerRules", outRecipe.markerRules, ReadMarkerRuleJson);
    ReadRuleArray(rules, "PropRules", outRecipe.propRules, ReadPropRuleJson);
    ReadRuleArray(rules, "DecalRules", outRecipe.decalRules, ReadDecalRuleJson);
    ReadRuleArray(rules, "UnitRules", outRecipe.unitRules, ReadUnitRuleJson);
}

} // namespace Io
} // namespace SanmapGen
