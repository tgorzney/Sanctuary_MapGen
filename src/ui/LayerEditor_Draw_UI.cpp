// LayerEditor_Draw_UI.cpp — the catalogued-slider draw path and the driver notification.
// Layer: UI. The limits, the printf format and the integer/float choice all come out of the
// catalogue (LayerEditor_Scalars_UI.h), so a panel says which control it wants and never repeats
// a bound. The slider itself is the batch-A shared widget; nothing is drawn here directly.
#include "LayerEditor_Draw_UI.h"
#include "../pipeline/PreviewDriver_PIPELINE.h"

namespace SanmapGen {
namespace Ui {
namespace {

// The caller-owned range for one control: the state's copy of the catalogue row, so a host may
// retune a limit at run time without editing the table (Constitution §8).
const ScalarSliderRange& RangeOf(LayerEditorScalar scalar, LayerEditorState& state) {
    const int scalarIndex = static_cast<int>(scalar);
    if (scalarIndex < 0 || scalarIndex >= kLayerEditorScalarCount) return state.scalarRanges[0];
    return state.scalarRanges[scalarIndex];
}

RealtimeToggle& ToggleOf(LayerEditorScalar scalar, LayerEditorState& state) {
    const int scalarIndex = static_cast<int>(scalar);
    if (scalarIndex < 0 || scalarIndex >= kLayerEditorScalarCount) return state.scalarToggles[0];
    return state.scalarToggles[scalarIndex];
}

} // namespace

void NotifyLayerEditorChange(bool bCommitted, Pipeline::PreviewDriver* previewDriver) {
    if (bCommitted && previewDriver != nullptr) previewDriver->NotifyParametersChanged();
}

WidgetChange DrawLayerEditorScalar(LayerEditorScalar scalar, float& value, LayerEditorState& state) {
    const LayerEditorScalarDescription& description = LayerEditorScalarDescriptionOf(scalar);
    return DrawSliderScalar(description.label, value, RangeOf(scalar, state),
                            ToggleOf(scalar, state), WidgetStyle(), description.valueFormat);
}

WidgetChange DrawLayerEditorScalarInteger(LayerEditorScalar scalar, int& value,
                                          LayerEditorState& state) {
    const LayerEditorScalarDescription& description = LayerEditorScalarDescriptionOf(scalar);
    return DrawSliderScalarInteger(description.label, value, RangeOf(scalar, state),
                                   ToggleOf(scalar, state), WidgetStyle(), description.valueFormat);
}

} // namespace Ui
} // namespace SanmapGen
