// SlopeTab_UI.cpp — the imgui composition of the Slope overlay tab. Layer: UI.
// Show-overlay tick, the two degree bounds, and the shared GradientEditor over the layer's ramp.
#include "SlopeTab_UI.h"
#include "Checkbox_UI.h"
#include "../pipeline/PreviewDriver_PIPELINE.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

void NotifyChange(bool bCommitted, Pipeline::PreviewDriver* previewDriver) {
    if (bCommitted && previewDriver != nullptr) previewDriver->NotifyParametersChanged();
}

void DrawSlopeDomain(PreviewFieldLayer& layer, SlopeTabState& state,
                     Pipeline::PreviewDriver* previewDriver) {
    if (!state.minimumDegreesToggle.IsCommitDeferred() && !state.maximumDegreesToggle.IsCommitDeferred())
        LoadSlopeTabValues(layer, state);
    WidgetChange change = DrawSliderScalar("Flat Angle", state.minimumDegrees,
                                           state.slopeDegreeRange, state.minimumDegreesToggle,
                                           WidgetStyle(), "%.1f deg");
    if (change.bValueChanged) StoreSlopeTabValues(state, layer);
    NotifyChange(change.bCommitted, previewDriver);

    change = DrawSliderScalar("Steep Angle", state.maximumDegrees, state.slopeDegreeRange,
                              state.maximumDegreesToggle, WidgetStyle(), "%.1f deg");
    if (change.bValueChanged) StoreSlopeTabValues(state, layer);
    NotifyChange(change.bCommitted, previewDriver);

    NotifyChange(DrawCheckbox("Auto Domain From Field", layer.bAutoDomainFromField).bCommitted,
                 previewDriver);
}

} // namespace

void DrawSlopeTab(PreviewCompositeSettings& compositeSettings, SlopeTabState& state,
                  Pipeline::PreviewDriver* previewDriver) {
    ImGui::PushID("slopeTab");
    PreviewFieldLayer* const layer =
        PreviewFieldLayerOfKind(compositeSettings, PreviewLayerKind::Slope);
    if (layer == nullptr) {
        ImGui::TextUnformatted("The composite carries no Slope layer to show.");
        ImGui::PopID();
        return;
    }
    NotifyChange(DrawCheckbox("Show Overlay", layer->bEnabled).bCommitted, previewDriver);
    DrawSlopeDomain(*layer, state, previewDriver);

    Params::GradientRamp* const ramp = PreviewRampOfFieldLayer(compositeSettings, *layer);
    if (ramp == nullptr) ImGui::TextUnformatted("This layer names no gradient ramp.");
    else if (DrawGradientEditor("Slope Gradient", *ramp, state.gradientEditor))
        NotifyChange(true, previewDriver);
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
