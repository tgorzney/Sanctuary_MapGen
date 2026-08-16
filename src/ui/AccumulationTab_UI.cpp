// AccumulationTab_UI.cpp — the imgui composition of the Accumulation tab. Layer: UI.
// Show-overlay tick, the ordered-spillover settings (bulk-written across the strata), and the
// shared GradientEditor over the accumulation ramp.
#include "AccumulationTab_UI.h"
#include "Checkbox_UI.h"
#include "../pipeline/PreviewDriver_PIPELINE.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

void NotifyChange(bool bCommitted, Pipeline::PreviewDriver* previewDriver) {
    if (bCommitted && previewDriver != nullptr) previewDriver->NotifyParametersChanged();
}

// The mirror is refreshed from the stage only while nothing is deferring a commit, so a drag in
// flight is never overwritten by the value it has not reached yet.
void DrawSpilloverSettings(AccumulationTabState& state,
                           Pipeline::GenerationAssembler& generationAssembler,
                           Pipeline::PreviewDriver* previewDriver) {
    if (!state.spilloverThresholdToggle.IsCommitDeferred()
        && !state.spilloverShareToggle.IsCommitDeferred())
        state.spilloverSettings = AccumulationSettingsOfAssembler(generationAssembler);

    WidgetChange change = DrawCheckbox("Accurate Simultaneous Accumulation",
                                       state.spilloverSettings.bAccurateSimultaneousAccumulation);
    bool bStageMoved = change.bValueChanged
        && ApplyAccumulationSettingsToErosion(state.spilloverSettings, generationAssembler);
    NotifyChange(bStageMoved && change.bCommitted, previewDriver);

    change = DrawSliderScalar("Spillover Threshold", state.spilloverSettings.spilloverThreshold,
                              state.spilloverThresholdRange, state.spilloverThresholdToggle,
                              WidgetStyle(), "%.4f");
    bStageMoved = change.bValueChanged
        && ApplyAccumulationSettingsToErosion(state.spilloverSettings, generationAssembler);
    NotifyChange(bStageMoved && change.bCommitted, previewDriver);

    change = DrawSliderScalar("Spillover Share", state.spilloverSettings.spilloverShare,
                              state.spilloverShareRange, state.spilloverShareToggle,
                              WidgetStyle(), "%.4f");
    bStageMoved = change.bValueChanged
        && ApplyAccumulationSettingsToErosion(state.spilloverSettings, generationAssembler);
    NotifyChange(bStageMoved && change.bCommitted, previewDriver);
}

} // namespace

void DrawAccumulationTab(PreviewCompositeSettings& compositeSettings, AccumulationTabState& state,
                         Pipeline::GenerationAssembler* generationAssembler,
                         Pipeline::PreviewDriver* previewDriver) {
    ImGui::PushID("accumulationTab");
    PreviewFieldLayer* const layer =
        PreviewFieldLayerOfKind(compositeSettings, PreviewLayerKind::Accumulation);
    if (layer != nullptr)
        NotifyChange(DrawCheckbox("Show Overlay", layer->bEnabled).bCommitted, previewDriver);
    else ImGui::TextUnformatted("The composite carries no Accumulation layer to show.");

    if (generationAssembler != nullptr)
        DrawSpilloverSettings(state, *generationAssembler, previewDriver);
    else
        ImGui::TextUnformatted("No pipeline bound - the spillover settings have nothing to edit.");

    Params::GradientRamp* const ramp = layer != nullptr
        ? PreviewRampOfFieldLayer(compositeSettings, *layer) : nullptr;
    if (ramp != nullptr && DrawGradientEditor("Accumulation Gradient", *ramp, state.gradientEditor))
        NotifyChange(true, previewDriver);
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
