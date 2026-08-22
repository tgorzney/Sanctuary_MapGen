// Placement_MarkerRules_PROC.cpp — flattens recipe.markerRuleLayers (the two-level marker
// family) into ScatterRuleConfiguration records. Split out of Placement_Rules_PROC.cpp
// (ARCH_01_05_FileSizeCeilings.md §1.5) because the marker family alone carries a layer tier;
// props/units/decals stay flat single-level arrays in the sibling file.
#include "Placement_RuleAppend_PROC.h"
#include "Placement_RuleBuild_PROC.h"

namespace SanmapGen {
namespace Proc {
namespace {

inline float ReciprocalOrZero(float value, float epsilon) {
    return value > epsilon ? 1.0f / value : 1.0f / epsilon;
}

// Today's per-rule loop body, unchanged except the symmetry pair now sources from the
// LAYER's SymmetrySetting rather than the rule itself (ARCH_16_01_NewParamsShapes.md §16.1).
void AppendMarkerRuleConfiguration(const PlacementConstants& constants, const Params::MapRecipe& recipe,
                                   const Params::MarkerRule& rule, const Params::SymmetrySetting& symmetry,
                                   int flatRuleIndex, std::vector<ScatterRuleConfiguration>& configurations,
                                   std::vector<Data::TemplateIdentifier>& identifiers,
                                   std::vector<int>& radialSymmetryRepeatCounts) {
    ScatterRuleConfiguration configuration = MakeCommonConfiguration(
        constants, recipe.geometry, recipe.water, rule, flatRuleIndex, 0);
    configuration.category          = static_cast<int>(rule.category);
    configuration.priorityMode      = static_cast<int>(rule.priority);
    configuration.focusGradientMode = static_cast<int>(rule.focusGradient);
    configuration.symmetryMask      = ResolveSymmetryMask(symmetry.bSymmetryUseGlobal, symmetry.symmetryMask,
                                                          recipe.globalSymmetryMask);
    const int radialSymmetryRepeatCount = ResolveRadialSymmetryRepeatCount(
        symmetry.bSymmetryUseGlobal, symmetry.radialSymmetryRepeatCount, recipe.radialSymmetryRepeatCount);
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

} // namespace

// The two-level walk (ARCH_16_01_NewParamsShapes.md §16.1): recipe.markerRuleLayers -> layer.rules.
// ONE flat counter threaded through both loops, advancing for every rule encountered —
// including suppressed ones — so ruleIndex/ruleSeed keep today's exact flat numbering
// (Constitution §4, the determinism guard this ticket exists to preserve).
void AppendMarkerRules(const PlacementConstants& constants, const Params::MapRecipe& recipe,
                       std::vector<ScatterRuleConfiguration>& configurations,
                       std::vector<Data::TemplateIdentifier>& identifiers,
                       std::vector<int>& radialSymmetryRepeatCounts) {
    int ruleIndex = 0;
    for (const Params::MarkerRuleLayer& layer : recipe.markerRuleLayers) {
        const bool bLayerSuppressed = !layer.bEnabled && !layer.bHidden;
        for (const Params::MarkerRule& rule : layer.rules) {
            const int flatRuleIndex = ruleIndex++;
            if (bLayerSuppressed || (!rule.bEnabled && !rule.bHidden)) continue;
            AppendMarkerRuleConfiguration(constants, recipe, rule, layer.symmetry, flatRuleIndex,
                                          configurations, identifiers, radialSymmetryRepeatCounts);
        }
    }
}

} // namespace Proc
} // namespace SanmapGen
