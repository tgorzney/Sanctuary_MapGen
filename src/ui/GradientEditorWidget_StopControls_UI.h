// GradientEditorWidget_StopControls_UI.h — the selected-stop sub-panel (color + location +
// delete), split out of GradientEditorWidget_Draw_UI.cpp to keep both files inside the ARCH §1.5
// ceilings. Internal to the widget: only GradientEditorWidget_*_UI.cpp include it.
#pragma once
#include "GradientEditorWidget_UI.h"

namespace SanmapGen {
namespace Ui {

// Draws the universal color swatch (Ui::DrawColorSwatch) and the stop-location slider (converted
// through `options.ToDisplayUnits`/`FromDisplayUnits` when set) and the delete button for
// `state.selectedStopIndex`. Returns false with nothing drawn when no stop is selected. Every
// mutation goes through the pure edit functions (GradientEditorWidget_UI.h) — this TU adds no
// edit rule of its own.
bool DrawSelectedStopControls(Params::GradientRamp& ramp, GradientEditorState& state,
                              const GradientEditorOptions& options);

} // namespace Ui
} // namespace SanmapGen
