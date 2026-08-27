// MarkersTab_Area_UI.cpp — the Area and Focus Gradient sections of one marker rule. Layer: UI.
// Split out of MarkersTab_Rules_UI.cpp purely to stay inside the ARCH §1.5 file ceilings; the two
// sections share the same MarkerRuleDetailState the rest of the tab holds.
#include "MarkersTab_Rules_UI.h"
#include "Checkbox_UI.h"
#include "Combo_UI.h"

namespace SanmapGen {
namespace Ui {

// The area a marker claims: how large a flat patch it needs, and — when the maximum is checked —
// how large a patch disqualifies it. Human's own bug report — this pair had no min-cannot-cross-max
// protection at all (two independent sliders): with the maximum checked, ONE DrawRangeSlider now
// edits both ends together, enforcing min<=max and the "Min Delta" gap (areaRadiusBounds.
// minimumSeparation) the same way every other min/max pair in the app already does.
void DrawMarkerRuleArea(Params::MarkerRule& rule, MarkerRuleDetailState& state,
                        Pipeline::PreviewDriver* previewDriver) {
    if (!DrawSectionBegin("Area", state.areaSection)) return;
    NotifyPlacementChange(DrawCheckbox("Check Maximum Radius", rule.bCheckMaximumRadius).bCommitted,
                          previewDriver);
    if (rule.bCheckMaximumRadius) {
        state.areaRadiusValues.minimumValue = rule.areaRadiusMinimum;
        state.areaRadiusValues.maximumValue = rule.areaRadiusMaximum;
        const WidgetChange change = DrawRangeSlider("Area Radius", state.areaRadiusValues,
                                                     state.areaRadiusBounds, state.areaRadiusToggle,
                                                     WidgetStyle(), "%.1f");
        rule.areaRadiusMinimum = state.areaRadiusValues.minimumValue;
        rule.areaRadiusMaximum = state.areaRadiusValues.maximumValue;
        NotifyPlacementChange(change.bCommitted, previewDriver);
    } else {
        NotifyPlacementChange(DrawSliderScalar("Area Radius Minimum", rule.areaRadiusMinimum,
                                               ScalarSliderRange{ state.areaRadiusBounds.lowerLimit,
                                                                  state.areaRadiusBounds.upperLimit, 0.0f },
                                               state.areaRadiusToggle, WidgetStyle(), "%.1f").bCommitted,
                              previewDriver);
    }
    NotifyPlacementChange(DrawSliderScalar("Area Height Range", rule.areaHeightRange,
                                           state.areaHeightRange, state.areaHeightRangeToggle,
                                           WidgetStyle(), "%.2f").bCommitted, previewDriver);
    DrawSectionEnd();
}

// The spatial weighting: which part of the map a rule prefers, and how hard it prefers it. The
// three shaping scalars are hidden while the gradient is None — they cannot mean anything then.
void DrawMarkerRuleFocus(Params::MarkerRule& rule, MarkerRuleDetailState& state,
                         Pipeline::PreviewDriver* previewDriver) {
    if (!DrawSectionBegin("Focus Gradient", state.focusSection)) return;
    ComboOptions options;
    options.labels = markerFocusGradientLabels;
    options.count  = kMarkerFocusGradientCount;
    const WidgetChange change = DrawCombo("Focus Gradient", state.focusGradientIndex, options);
    if (change.bValueChanged)
        rule.focusGradient = static_cast<Params::FocusGradient>(state.focusGradientIndex);
    NotifyPlacementChange(change.bCommitted, previewDriver);
    if (rule.focusGradient == Params::FocusGradient::None) {
        DrawSectionEnd();
        return;
    }
    NotifyPlacementChange(DrawSliderScalar("Gradient Radius", rule.focusGradientRadius,
                                           state.focusRadiusRange, state.focusRadiusToggle,
                                           WidgetStyle(), "%.1f").bCommitted, previewDriver);
    NotifyPlacementChange(DrawSliderScalar("Gradient Strength", rule.focusGradientStrength,
                                           state.focusStrengthRange, state.focusStrengthToggle,
                                           WidgetStyle(), "%.2f").bCommitted, previewDriver);
    NotifyPlacementChange(DrawSliderScalar("Gradient Contrast", rule.focusGradientContrast,
                                           state.focusContrastRange, state.focusContrastToggle,
                                           WidgetStyle(), "%.2f").bCommitted, previewDriver);
    DrawSectionEnd();
}

} // namespace Ui
} // namespace SanmapGen
