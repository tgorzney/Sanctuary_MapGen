// MarkersTab_Rules_UI.cpp — the Gates and Quantity sections of one marker rule. Layer: UI.
// Shared widgets only: Combo / Checkbox / RangeSlider / LabelledDial / Section. No
// ImGui::SliderFloat / DragFloat / VSliderFloat in this file.
#include "MarkersTab_UI.h"
#include "Checkbox_UI.h"
#include "Combo_UI.h"
#include "../pipeline/PreviewDriver_PIPELINE.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// One enum row, over the mirror index Combo_UI edits. Returns the picked index unchanged when the
// dropdown reported nothing, so an untouched frame costs nothing.
bool DrawEnumRow(const char* label, int& mirrorIndex, const char* const* labels, int labelCount,
                 Pipeline::PreviewDriver* previewDriver) {
    ComboOptions options;
    options.labels = labels;
    options.count  = labelCount;
    const WidgetChange change = DrawCombo(label, mirrorIndex, options);
    NotifyPlacementChange(change.bCommitted, previewDriver);
    return change.bValueChanged;
}

} // namespace

// What terrain a marker may land on: its category, its visibility, and the three gates the
// Placement stage tests before it accepts a position.
void DrawMarkerRuleGates(Params::MarkerRule& rule, MarkersTabState& state,
                         Pipeline::PreviewDriver* previewDriver) {
    if (!DrawSectionBegin("Gates", state.ruleDetail.gateSection)) return;
    if (DrawEnumRow("Type", state.ruleDetail.categoryIndex, markerCategoryLabels,
                    kMarkerCategoryCount, previewDriver))
        rule.category = static_cast<Params::MarkerCategory>(state.ruleDetail.categoryIndex);
    NotifyPlacementChange(DrawCheckbox("Hidden (still generated for clearance/fairness)",
                                       rule.bHidden).bCommitted, previewDriver);
    WidgetChange change = DrawRangeSlider("Slope Bounds (degrees)", state.slopeValues,
                                          state.slopeBounds, state.slopeToggle, WidgetStyle(), "%.1f");
    if (change.bValueChanged) StoreMarkerRuleValues(state, rule);
    NotifyPlacementChange(change.bCommitted, previewDriver);
    change = DrawRangeSlider("Height Bounds (normalized)", state.heightValues, state.heightBounds,
                             state.heightToggle);
    if (change.bValueChanged) StoreMarkerRuleValues(state, rule);
    NotifyPlacementChange(change.bCommitted, previewDriver);
    NotifyPlacementChange(DrawLabelledDial("Obstacle Distance Minimum", rule.obstacleDistanceMinimum,
                                           state.obstacleDistanceRange, state.obstacleDistanceToggle,
                                           WidgetStyle(), "%.2f").bCommitted, previewDriver);
    DrawSectionEnd();
}

// How many markers land, how they are chosen, and how far apart they must stay.
void DrawMarkerRuleQuantity(Params::MarkerRule& rule, MarkersTabState& state,
                            Pipeline::PreviewDriver* previewDriver) {
    if (!DrawSectionBegin("Quantity & Selection", state.ruleDetail.quantitySection)) return;
    NotifyPlacementChange(DrawCheckbox("Use Density (off = fixed count)", rule.bUseDensity).bCommitted,
                          previewDriver);
    WidgetChange change = DrawLabelledDial("Count", state.countValue, state.countRange,
                                           state.countToggle, WidgetStyle(), "%.0f");
    if (change.bValueChanged) StoreMarkerRuleValues(state, rule);
    NotifyPlacementChange(change.bCommitted, previewDriver);
    NotifyPlacementChange(DrawLabelledDial("Density", rule.density, state.densityRange,
                                           state.densityToggle, WidgetStyle(), "%.4f").bCommitted,
                          previewDriver);
    NotifyPlacementChange(DrawLabelledDial("Clearance Spacing", rule.clearanceSpacing,
                                           state.clearanceSpacingRange, state.clearanceSpacingToggle,
                                           WidgetStyle(), "%.2f").bCommitted, previewDriver);
    NotifyPlacementChange(DrawCheckbox("Use All Positions", rule.bUseAllPositions).bCommitted,
                          previewDriver);
    NotifyPlacementChange(DrawCheckbox("Random Selection", rule.bRandomSelection).bCommitted,
                          previewDriver);
    if (DrawEnumRow("Priority", state.ruleDetail.priorityIndex, markerPriorityLabels,
                    kMarkerPriorityCount, previewDriver))
        rule.priority = static_cast<Params::MarkerPriority>(state.ruleDetail.priorityIndex);
    DrawSectionEnd();
}

} // namespace Ui
} // namespace SanmapGen
