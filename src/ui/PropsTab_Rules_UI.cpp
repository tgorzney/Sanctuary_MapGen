// PropsTab_Rules_UI.cpp — the Gates and Affinities sections of one prop rule. Layer: UI.
// Shared widgets only: Checkbox / RangeSlider / LabelledDial / Section. No ImGui::SliderFloat /
// DragFloat / VSliderFloat in this file.
#include "PropsTab_UI.h"
#include "Checkbox_UI.h"

namespace SanmapGen {
namespace Ui {

// What terrain a prop may land on, and how densely it lands.
void DrawPropRuleGates(Params::PropRule& rule, PropsTabState& state,
                       Pipeline::PreviewDriver* previewDriver) {
    if (!DrawSectionBegin("Gates", state.ruleDetail.gateSection)) return;
    WidgetChange change = DrawRangeSlider("Slope Range (degrees)", state.slopeValues,
                                          state.slopeBounds, state.slopeToggle, WidgetStyle(), "%.1f");
    if (change.bValueChanged) StorePropRuleValues(state, rule);
    NotifyPlacementChange(change.bCommitted, previewDriver);
    change = DrawRangeSlider("Height Range (normalized)", state.heightValues, state.heightBounds,
                             state.heightToggle);
    if (change.bValueChanged) StorePropRuleValues(state, rule);
    NotifyPlacementChange(change.bCommitted, previewDriver);
    NotifyPlacementChange(DrawLabelledDial("Density", rule.density, state.densityRange,
                                           state.densityToggle, WidgetStyle(), "%.4f").bCommitted,
                          previewDriver);
    NotifyPlacementChange(DrawLabelledDial("Spacing Minimum (cells)", rule.spacingMinimum,
                                           state.spacingRange, state.spacingToggle, WidgetStyle(),
                                           "%.2f").bCommitted, previewDriver);
    NotifyPlacementChange(DrawLabelledDial("Obstacle Distance Minimum", rule.obstacleDistanceMinimum,
                                           state.obstacleDistanceRange, state.obstacleDistanceToggle,
                                           WidgetStyle(), "%.2f").bCommitted, previewDriver);
    DrawSectionEnd();
}

// The water and cliff affinities. The cliff distance is hidden while the affinity is off — it
// cannot mean anything then.
void DrawPropRuleAffinities(Params::PropRule& rule, PropsTabState& state,
                            Pipeline::PreviewDriver* previewDriver) {
    if (!DrawSectionBegin("Affinities", state.ruleDetail.affinitySection)) return;
    NotifyPlacementChange(DrawCheckbox("Avoid Water", rule.bAvoidWater).bCommitted, previewDriver);
    NotifyPlacementChange(DrawCheckbox("Near Cliffs Only", rule.bNearCliffs).bCommitted, previewDriver);
    if (rule.bNearCliffs)
        NotifyPlacementChange(DrawLabelledDial("Near Cliff Distance Maximum",
                                               rule.nearCliffDistanceMaximum,
                                               state.nearCliffDistanceRange,
                                               state.nearCliffDistanceToggle, WidgetStyle(),
                                               "%.2f").bCommitted, previewDriver);
    DrawSectionEnd();
}

} // namespace Ui
} // namespace SanmapGen
