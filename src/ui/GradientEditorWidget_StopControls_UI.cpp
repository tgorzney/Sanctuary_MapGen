// GradientEditorWidget_StopControls_UI.cpp — the selected-stop sub-panel: the universal color
// swatch (Ui::DrawColorSwatch), the stop-location slider (raw 0..1, or degrees for Slope via
// `GradientEditorOptions::ToDisplayUnits`/`FromDisplayUnits`) and the delete button. Split out of
// GradientEditorWidget_Draw_UI.cpp per ARCH §1.5 (the aspect split keeps both files inside the
// size ceilings); every mutation still goes through the pure edit functions in
// GradientEditorWidget_UI.cpp.
#include "GradientEditorWidget_StopControls_UI.h"
#include "ColorSwatch_UI.h"
#include "imgui.h"
#include <cstddef>

namespace SanmapGen {
namespace Ui {
namespace {

void ClearSelectionAfterDelete(GradientEditorState& state, int deletedStopIndex) {
    if (state.selectedStopIndex == deletedStopIndex) state.selectedStopIndex = -1;
    else if (state.selectedStopIndex > deletedStopIndex) --state.selectedStopIndex;
    state.draggedStopIndex = -1;
}

} // namespace

bool DrawSelectedStopControls(Params::GradientRamp& ramp, GradientEditorState& state,
                              const GradientEditorOptions& options) {
    if (state.selectedStopIndex < 0 ||
        state.selectedStopIndex >= static_cast<int>(ramp.stops.size())) return false;
    const std::size_t stopIndex = static_cast<std::size_t>(state.selectedStopIndex);

    bool bChanged = false;
    float color[kGradientStopChannelCount];
    for (int channel = 0; channel < kGradientStopChannelCount; ++channel)
        color[channel] = ramp.stops[stopIndex].color[channel];
    ColorSwatchOptions colorSwatchOptions;
    colorSwatchOptions.bAlphaEnabled  = true;
    colorSwatchOptions.bAlphaBarShown = true;
    const WidgetChange colorChange = DrawColorSwatch("Stop color", color, colorSwatchOptions,
                                                      state.stopColorRealtimeToggle);
    if (colorChange.bValueChanged)
        bChanged = RecolorGradientStop(ramp, state.selectedStopIndex, color) || bChanged;

    // Slope passes ToDisplayUnits/FromDisplayUnits so this slider reads/writes degrees; every
    // other tab leaves both null and edits the raw 0..1 `location` exactly as before.
    const bool bDisplayConverted = options.ToDisplayUnits != nullptr && options.FromDisplayUnits != nullptr;
    const float storedLocation = ramp.stops[stopIndex].location;
    float displayLocation = bDisplayConverted ? options.ToDisplayUnits(storedLocation) : storedLocation;
    const float displayMinimum = bDisplayConverted ? options.ToDisplayUnits(0.0f) : 0.0f;
    const float displayMaximum = bDisplayConverted ? options.ToDisplayUnits(1.0f) : 1.0f;
    if (ImGui::SliderFloat(options.locationSliderLabel, &displayLocation, displayMinimum, displayMaximum)) {
        const float newLocation = bDisplayConverted ? options.FromDisplayUnits(displayLocation) : displayLocation;
        bChanged = MoveGradientStop(ramp, state.selectedStopIndex, newLocation) || bChanged;
    }

    if (ImGui::Button("Delete stop")) {
        const int deletedStopIndex = state.selectedStopIndex;
        if (DeleteGradientStop(ramp, deletedStopIndex)) {
            ClearSelectionAfterDelete(state, deletedStopIndex);
            bChanged = true;
        }
    }
    return bChanged;
}

} // namespace Ui
} // namespace SanmapGen
