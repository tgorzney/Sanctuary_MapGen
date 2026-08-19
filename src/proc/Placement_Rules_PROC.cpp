// Placement_Rules_PROC.cpp — flattens the recipe's four rule families into the single
// ScatterRuleConfiguration record both backends consume. This is where "three divergent
// mechanisms" become one: markers, props, units and decals differ only in the selection
// fields they fill in; from here on the scatter code knows nothing about rule families.
#include "Placement_PROC.h"
#include "Placement_RuleBuild_PROC.h"

namespace SanmapGen {
namespace Proc {
namespace {

inline float ReciprocalOrZero(float value, float epsilon) {
    return value > epsilon ? 1.0f / value : 1.0f / epsilon;
}

void AppendMarkerRules(const PlacementConstants& constants, const Params::MapRecipe& recipe,
                       std::vector<ScatterRuleConfiguration>& configurations,
                       std::vector<Data::TemplateIdentifier>& identifiers,
                       std::vector<int>& radialSymmetryRepeatCounts) {
    for (std::size_t index = 0; index < recipe.markerRules.size(); ++index) {
        const Params::MarkerRule& rule = recipe.markerRules[index];
        // A hidden rule still generates: its markers hold clearance/fairness even when the
        // rule is switched off in the UI (PLACEMENT_SCATTER_SPEC "Transform & symmetry").
        if (!rule.bEnabled && !rule.bHidden) continue;
        ScatterRuleConfiguration configuration = MakeCommonConfiguration(
            constants, recipe.geometry, recipe.water, rule, static_cast<int>(index), 0);
        configuration.category          = static_cast<int>(rule.category);
        configuration.priorityMode      = static_cast<int>(rule.priority);
        configuration.focusGradientMode = static_cast<int>(rule.focusGradient);
        configuration.symmetryMask      = ResolveSymmetryMask(rule.bSymmetryUseGlobal, rule.symmetryMask,
                                                              recipe.globalSymmetryMask);
        const int radialSymmetryRepeatCount = ResolveRadialSymmetryRepeatCount(
            rule.bSymmetryUseGlobal, rule.radialSymmetryRepeatCount, recipe.radialSymmetryRepeatCount);
        configuration.targetCount             = rule.count;
        configuration.density                 = rule.density;
        configuration.spacingMinimum          = rule.clearanceSpacing;
        configuration.clearanceRadiusMinimum  = rule.areaRadiusMinimum;
        configuration.clearanceRadiusMaximum  = rule.areaRadiusMaximum;
        configuration.obstacleDistanceMinimum = rule.obstacleDistanceMinimum;
        if (rule.areaHeightRange > 0.0f) configuration.clearanceHeightTolerance = rule.areaHeightRange;
        configuration.focusGradientRadiusReciprocal =
            ReciprocalOrZero(rule.focusGradientRadius, constants.focusGradientEpsilon);
        configuration.focusGradientStrength = rule.focusGradientStrength;
        configuration.focusGradientContrast = rule.focusGradientContrast;
        if (rule.bUseDensity)         configuration.selectionFlags |= ScatterSelectionFlag::UseDensity;
        if (rule.bUseAllPositions)    configuration.selectionFlags |= ScatterSelectionFlag::UseAllPositions;
        if (rule.bRandomSelection)    configuration.selectionFlags |= ScatterSelectionFlag::RandomSelection;
        if (rule.bCheckMaximumRadius) configuration.selectionFlags |= ScatterSelectionFlag::CheckMaximumRadius;
        if (rule.bHidden)             configuration.selectionFlags |= ScatterSelectionFlag::Hidden;
        configurations.push_back(configuration);
        identifiers.push_back(Data::MakeTemplateIdentifier(rule.transform.templateIdentifier));
        radialSymmetryRepeatCounts.push_back(radialSymmetryRepeatCount);
    }
}

void AppendPropRules(const PlacementConstants& constants, const Params::MapRecipe& recipe,
                     std::vector<ScatterRuleConfiguration>& configurations,
                     std::vector<Data::TemplateIdentifier>& identifiers,
                     std::vector<int>& radialSymmetryRepeatCounts) {
    for (std::size_t index = 0; index < recipe.propRules.size(); ++index) {
        const Params::PropRule& rule = recipe.propRules[index];
        if (!rule.bEnabled) continue;
        ScatterRuleConfiguration configuration = MakeCommonConfiguration(
            constants, recipe.geometry, recipe.water, rule, static_cast<int>(index), 1);
        configuration.symmetryMask = ResolveSymmetryMask(rule.bSymmetryUseGlobal, rule.symmetryMask,
                                                         recipe.globalSymmetryMask);
        const int radialSymmetryRepeatCount = ResolveRadialSymmetryRepeatCount(
            rule.bSymmetryUseGlobal, rule.radialSymmetryRepeatCount, recipe.radialSymmetryRepeatCount);
        configuration.density                  = rule.density;
        configuration.spacingMinimum           = rule.spacingMinimum;
        configuration.obstacleDistanceMinimum  = rule.obstacleDistanceMinimum;
        configuration.nearCliffDistanceMaximum = rule.nearCliffDistanceMaximum;
        configuration.selectionFlags |= ScatterSelectionFlag::UseDensity;
        if (rule.bAvoidWater) configuration.selectionFlags |= ScatterSelectionFlag::AvoidWater;
        if (rule.bNearCliffs) configuration.selectionFlags |= ScatterSelectionFlag::NearCliffs;
        configurations.push_back(configuration);
        identifiers.push_back(Data::MakeTemplateIdentifier(rule.transform.templateIdentifier));
        radialSymmetryRepeatCounts.push_back(radialSymmetryRepeatCount);
    }
}

void AppendUnitRules(const PlacementConstants& constants, const Params::MapRecipe& recipe,
                     std::vector<ScatterRuleConfiguration>& configurations,
                     std::vector<Data::TemplateIdentifier>& identifiers,
                     std::vector<int>& radialSymmetryRepeatCounts) {
    for (std::size_t index = 0; index < recipe.unitRules.size(); ++index) {
        const Params::UnitRule& rule = recipe.unitRules[index];
        if (!rule.bEnabled) continue;
        ScatterRuleConfiguration configuration = MakeCommonConfiguration(
            constants, recipe.geometry, recipe.water, rule, static_cast<int>(index), 2);
        configuration.symmetryMask = ResolveSymmetryMask(rule.bSymmetryUseGlobal, rule.symmetryMask,
                                                         recipe.globalSymmetryMask);
        const int radialSymmetryRepeatCount = ResolveRadialSymmetryRepeatCount(
            rule.bSymmetryUseGlobal, rule.radialSymmetryRepeatCount, recipe.radialSymmetryRepeatCount);
        configuration.armyIndex      = rule.armyIndex;
        configuration.targetCount    = rule.count;
        configuration.spacingMinimum = rule.spacingMinimum;
        configurations.push_back(configuration);
        identifiers.push_back(Data::MakeTemplateIdentifier(rule.transform.templateIdentifier));
        radialSymmetryRepeatCounts.push_back(radialSymmetryRepeatCount);
    }
}

void AppendDecalRules(const PlacementConstants& constants, const Params::MapRecipe& recipe,
                      std::vector<ScatterRuleConfiguration>& configurations,
                      std::vector<Data::TemplateIdentifier>& identifiers,
                      std::vector<int>& radialSymmetryRepeatCounts) {
    for (std::size_t index = 0; index < recipe.decalRules.size(); ++index) {
        const Params::DecalRule& rule = recipe.decalRules[index];
        if (!rule.bEnabled) continue;
        ScatterRuleConfiguration configuration = MakeCommonConfiguration(
            constants, recipe.geometry, recipe.water, rule, static_cast<int>(index), 3);
        configuration.symmetryMask = ResolveSymmetryMask(rule.bSymmetryUseGlobal, rule.symmetryMask,
                                                         recipe.globalSymmetryMask);
        const int radialSymmetryRepeatCount = ResolveRadialSymmetryRepeatCount(
            rule.bSymmetryUseGlobal, rule.radialSymmetryRepeatCount, recipe.radialSymmetryRepeatCount);
        configuration.density        = rule.density;
        configuration.spacingMinimum = rule.spacingMinimum;
        configuration.selectionFlags |= ScatterSelectionFlag::UseDensity;
        configurations.push_back(configuration);
        identifiers.push_back(Data::MakeTemplateIdentifier(rule.transform.templateIdentifier));
        radialSymmetryRepeatCounts.push_back(radialSymmetryRepeatCount);
    }
}

} // namespace

void PlacementStage::BuildRuleConfigurations() {
    ruleConfigurations.clear();
    ruleTemplateIdentifiers.clear();
    ruleRadialSymmetryRepeatCounts.clear();
    AppendMarkerRules(constants, recipe, ruleConfigurations, ruleTemplateIdentifiers,
                      ruleRadialSymmetryRepeatCounts);
    AppendPropRules(constants, recipe, ruleConfigurations, ruleTemplateIdentifiers,
                    ruleRadialSymmetryRepeatCounts);
    AppendUnitRules(constants, recipe, ruleConfigurations, ruleTemplateIdentifiers,
                    ruleRadialSymmetryRepeatCounts);
    AppendDecalRules(constants, recipe, ruleConfigurations, ruleTemplateIdentifiers,
                     ruleRadialSymmetryRepeatCounts);
}

} // namespace Proc
} // namespace SanmapGen
