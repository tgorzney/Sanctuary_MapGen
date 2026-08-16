// FlowTab_UI.cpp — the imgui composition of the Flow tab. Layer: UI.
// Routing constants and iteration budgets on the flow/accumulation stage (reached through
// PIPELINE), the per-stage backend override, and the flow overlay + its ramp.
#include "FlowTab_UI.h"
#include "Checkbox_UI.h"
#include "../pipeline/GenerationAssembler_PIPELINE.h"
#include "../pipeline/PreviewDriver_PIPELINE.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

void NotifyChange(bool bCommitted, Pipeline::PreviewDriver* previewDriver) {
    if (bCommitted && previewDriver != nullptr) previewDriver->NotifyParametersChanged();
}

// The per-cell routing numbers: how much water each cell contributes, how the path magnitude is
// scaled, and how much stochastic wander the single-flow-direction choice carries.
void DrawRoutingSettings(Proc::FlowAccumulationConstants& constants, FlowTabState& state,
                         Pipeline::PreviewDriver* previewDriver) {
    if (!DrawSectionBegin("Routing", state.routingSection)) return;
    NotifyChange(DrawSliderScalar("Precipitation Rate", constants.cellWeight,
                                  state.precipitationRateRange, state.precipitationRateToggle,
                                  WidgetStyle(), "%.3f").bCommitted, previewDriver);
    NotifyChange(DrawSliderScalar("Flow Volume Multiplier", constants.flowMagnitudeScale,
                                  state.flowVolumeMultiplierRange, state.flowVolumeMultiplierToggle,
                                  WidgetStyle(), "%.3f").bCommitted, previewDriver);
    NotifyChange(DrawSliderScalar("Stochastic Variance", constants.flowNoiseImpact,
                                  state.stochasticVarianceRange, state.stochasticVarianceToggle,
                                  WidgetStyle(), "%.4f").bCommitted, previewDriver);
    NotifyChange(DrawSliderScalar("Depression Fill Epsilon", constants.depressionFillEpsilon,
                                  state.depressionFillEpsilonRange, state.depressionFillEpsilonToggle,
                                  WidgetStyle(), "%.6f").bCommitted, previewDriver);
    NotifyChange(DrawCheckbox("Fill Depressions", constants.bFillDepressions).bCommitted, previewDriver);
    NotifyChange(DrawCheckbox("Normalize Accumulation", constants.bNormalizeAccumulation).bCommitted,
                 previewDriver);
    DrawSectionEnd();
}

// The Gpu relaxation budgets — v1's single "Iterations" is two numbers here (SCOPE NOTE 2).
void DrawIterationSettings(Proc::FlowAccumulationConstants& constants, FlowTabState& state,
                           Pipeline::PreviewDriver* previewDriver) {
    if (!DrawSectionBegin("Iterations", state.iterationSection)) return;
    NotifyChange(DrawSliderScalarInteger("Fill Iterations Per Side", constants.fillIterationsPerSide,
                                         state.iterationsPerSideRange, state.fillIterationsToggle,
                                         WidgetStyle(), "%d").bCommitted, previewDriver);
    NotifyChange(DrawSliderScalarInteger("Accumulation Iterations Per Side",
                                         constants.accumulationIterationsPerSide,
                                         state.iterationsPerSideRange, state.accumulationIterationsToggle,
                                         WidgetStyle(), "%d").bCommitted, previewDriver);
    NotifyChange(DrawSliderScalarInteger("Convergence Check Interval",
                                         constants.gpuConvergenceCheckInterval,
                                         state.convergenceCheckIntervalRange,
                                         state.convergenceCheckIntervalToggle,
                                         WidgetStyle(), "%d").bCommitted, previewDriver);
    DrawSectionEnd();
}

// The per-stage backend override (SCOPE NOTE 3): the tick's state lives here and is pushed.
void DrawBackendOverride(FlowTabState& state, Pipeline::GenerationAssembler& generationAssembler,
                         Pipeline::PreviewDriver* previewDriver) {
    const WidgetChange change = DrawCheckbox("Use GPU (preview)", state.bUseGpuForPreview);
    if (!change.bValueChanged) return;
    generationAssembler.FlowAccumulation().SetDispatchPolicy(
        FlowPreviewDispatchPolicy(state.bUseGpuForPreview));
    NotifyChange(change.bCommitted, previewDriver);
}

} // namespace

void DrawFlowTab(PreviewCompositeSettings& compositeSettings, FlowTabState& state,
                 Pipeline::GenerationAssembler* generationAssembler,
                 Pipeline::PreviewDriver* previewDriver) {
    ImGui::PushID("flowTab");
    PreviewFieldLayer* const layer =
        PreviewFieldLayerOfKind(compositeSettings, PreviewLayerKind::Flow);
    if (layer != nullptr)
        NotifyChange(DrawCheckbox("Show Overlay", layer->bEnabled).bCommitted, previewDriver);
    else ImGui::TextUnformatted("The composite carries no Flow layer to show.");

    if (generationAssembler != nullptr) {
        Proc::FlowAccumulationConstants& constants =
            generationAssembler->FlowAccumulation().Constants();
        DrawRoutingSettings(constants, state, previewDriver);
        DrawIterationSettings(constants, state, previewDriver);
        DrawBackendOverride(state, *generationAssembler, previewDriver);
    } else {
        ImGui::TextUnformatted("No pipeline bound - the flow constants have nothing to edit.");
    }

    Params::GradientRamp* const ramp = layer != nullptr
        ? PreviewRampOfFieldLayer(compositeSettings, *layer) : nullptr;
    if (ramp != nullptr && DrawGradientEditor("Flow Gradient", *ramp, state.gradientEditor))
        NotifyChange(true, previewDriver);
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
