// PropsTab_Rules_UI.cpp — the Gates and Affinities sections of one prop rule. Layer: UI.
// Shared widgets only: Checkbox / RangeSlider / LabelledDial / Section. No ImGui::SliderFloat /
// DragFloat / VSliderFloat in this file.
#include "PropsTab_UI.h"
#include "Checkbox_UI.h"
#include "imgui.h"
#include <string>

namespace SanmapGen {
namespace Ui {
namespace {

// The bounded tpId-buffer -> std::string conversion every wire-mapping/lookup site already
// re-implements locally (MapExporter_ScatterTransform_IO.cpp's own copy is the closest precedent;
// UI cannot reuse IO's copy, and this is small enough to stay file-local like every other one).
std::string BoundedTemplateIdentifierText(const char (&templateIdentifier)[8]) {
    std::size_t length = 0;
    while (length < sizeof(templateIdentifier) && templateIdentifier[length] != '\0') ++length;
    return std::string(templateIdentifier, length);
}

} // namespace

// What terrain a prop may land on, and how densely it lands.
void DrawPropRuleGates(Params::PropRule& rule, PropsTabState& state,
                       Pipeline::PreviewDriver* previewDriver) {
    if (!DrawSectionBegin("Gates", state.ruleDetail.gateSection)) return;
    DrawPlacementSlopeHeightGate(state.slopeValues, state.slopeBounds, state.slopeToggle,
                                 state.heightValues, state.heightBounds, state.heightToggle,
                                 "Slope Range (degrees)", "Height Range (normalized)",
                                 [&]{ StorePropRuleValues(state, rule); }, previewDriver);
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

// STEP96_FootprintBakeAndStalenessCheck_IO.md §2 — a discrete, per-rule bake button, never automatic:
// fires only on click, never inside dirty-hash recompute, never notifies PreviewDriver (the baked
// fields carry no accuracy class of their own and no stage reads them yet — §18.2 rule 2).
void DrawResolvePropFootprintButton(Params::PropRule& rule, PropsTabState& state,
                                    const Io::TemplateIngestReport* templateIngestReport) {
    const bool bHasTemplateIdentifier = PropRuleHasTemplateIdentifier(rule);
    ImGui::BeginDisabled(!bHasTemplateIdentifier);
    const bool bClicked = ImGui::Button("Resolve Footprint");
    ImGui::EndDisabled();
    if (bClicked && bHasTemplateIdentifier) {
        const std::string templateIdentifier =
            BoundedTemplateIdentifierText(rule.transform.templateIdentifier);
        const Io::TemplateFootprintRecord* const record = templateIngestReport == nullptr
            ? nullptr : templateIngestReport->FindByTemplateIdentifier(templateIdentifier);
        if (ApplyResolvedFootprintBake(rule, record)) {
            state.bakeFootprintMessage.clear();
        } else {
            state.bakeFootprintMessage = "No ingested data for tpId '" + templateIdentifier
                + "'. Ingest game templates in the System tab, or enter a value by hand.";
        }
    }
    if (!state.bakeFootprintMessage.empty()) ImGui::TextWrapped("%s", state.bakeFootprintMessage.c_str());
}

} // namespace Ui
} // namespace SanmapGen
