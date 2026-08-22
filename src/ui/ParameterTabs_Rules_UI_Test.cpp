// ParameterTabs_Rules_UI_Test.cpp — the Markers and Props tab checks (M5-6 acceptance).
// Both tabs edit a rule ARRAY through one selection, so each check also proves the selected-rule
// resolution and the mirror -> rule store, not just the widget arithmetic.
#include "MarkersTab_UI.h"
#include "PropsTab_UI.h"
#include "ParameterTabs_TestSupport_UI.h"
#include "../params/MapRecipe_PARAMS.h"

using namespace SanmapGen;
using namespace SanmapGen::Ui;

void RunMarkersTabChecks(Params::MapRecipe& recipe) {
    MarkersTabState state;
    Params::MarkerRule* const rule = SelectedMarkerRule(recipe.markerRuleLayers, state);
    Check(rule != nullptr, "the marker selection resolves to a rule");
    if (rule == nullptr) return;
    LoadMarkerRuleValues(*rule, state);
    Check(state.slopeValues.maximumValue == rule->maxSlope, "the slope mirror loaded from the rule");

    // The slope gate: one handle of the shared range slider, dragged and released.
    WidgetChange change = StepRangeSliderInteraction(state.slopeToggle, state.slopeValues,
                                                     state.slopeBounds,
                                                     GrabRangeHandle(RangeSliderHandle::Maximum, 30.0f));
    Check(change.bValueChanged && !change.bCommitted,
          "RT off: the gate moves live and defers the commit");
    Check(StoreMarkerRuleValues(state, *rule), "the store reports the rule moved");
    Check(rule->maxSlope == 30.0f, "the slope gate reached the recipe");
    Check(rule->minSlope == state.slopeValues.minimumValue, "the partner handle is unchanged");
    change = StepRangeSliderInteraction(state.slopeToggle, state.slopeValues, state.slopeBounds,
                                        ReleaseRangeHandle());
    Check(change.bCommitted, "the commit lands on release");

    // The height gate, and the integer count through its float mirror.
    StepRangeSliderInteraction(state.heightToggle, state.heightValues, state.heightBounds,
                               GrabRangeHandle(RangeSliderHandle::Minimum, 0.25f));
    StoreMarkerRuleValues(state, *rule);
    Check(rule->minHeight == 0.25f, "the height gate reached the recipe");

    const int settledCount = rule->count;
    StepDialInteraction(state.countToggle, state.countValue, state.countRange, DialDrag(-100.0f));
    Check(StoreMarkerRuleValues(state, *rule) && rule->count > settledCount,
          "the count mirror reached the recipe");
    Check(rule->count <= static_cast<int>(state.countRange.maximumValue),
          "and stayed inside the tab's declared limits");

    // The tpId the IconGrid picker sits beside is a plain PARAMS field the tab writes directly.
    rule->transform.templateIdentifier[0] = 'z';
    Check(recipe.markerRuleLayers[0].rules[0].transform.templateIdentifier[0] == 'z',
          "the template id is edited in the recipe's own rule, not a copy");
}

void RunPropsTabChecks(Params::MapRecipe& recipe) {
    PropsTabState state;
    Params::PropRule* const rule = SelectedPropRule(recipe.propRules, state);
    Check(rule != nullptr, "the prop selection resolves to a rule");
    if (rule == nullptr) return;
    LoadPropRuleValues(*rule, state);

    StepRangeSliderInteraction(state.heightToggle, state.heightValues, state.heightBounds,
                               GrabRangeHandle(RangeSliderHandle::Maximum, 0.75f));
    Check(StorePropRuleValues(state, *rule), "the store reports the prop rule moved");
    Check(rule->maxHeight == 0.75f, "the height gate reached the recipe");

    const float settledDensity = rule->density;
    WidgetChange change = StepDialInteraction(state.densityToggle, rule->density,
                                              state.densityRange, DialDrag(-40.0f));
    Check(change.bValueChanged && rule->density > settledDensity,
          "the density dial writes the rule in place");
    Check(rule->density <= state.densityRange.maximumValue, "and stays inside its limits");

    const float settledSpacing = rule->spacingMinimum;
    StepDialInteraction(state.spacingToggle, rule->spacingMinimum, state.spacingRange, DialDrag(-60.0f));
    Check(rule->spacingMinimum > settledSpacing, "the Poisson spacing reached the recipe");

    // Selection is tab state: pointing past the end resolves to nothing rather than to garbage.
    state.selectedRuleIndex = static_cast<int>(recipe.propRules.size());
    Check(SelectedPropRule(recipe.propRules, state) == nullptr,
          "an out-of-range selection resolves to no rule");
    state.selectedRuleIndex = -1;
    Check(SelectedPropRule(recipe.propRules, state) == nullptr,
          "and so does an empty selection");
}
