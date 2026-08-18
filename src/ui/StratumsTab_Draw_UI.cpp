// StratumsTab_Draw_UI.cpp — the catalogued-slider draw path and the driver notification.
// Layer: UI. The limits and the printf format come out of the catalogue
// (StratumsTab_Scalars_UI.h) and the RT toggle out of the stratum's own row, so a panel says which
// control it wants and never repeats a bound. The slider itself is the batch-A shared widget;
// nothing is drawn here directly.
#include "StratumsTab_Draw_UI.h"
#include <cstdio>
#include "../pipeline/PreviewDriver_PIPELINE.h"

namespace SanmapGen {
namespace Ui {
namespace {

// The caller-owned range for one control: the state's copy of the catalogue row, so a host may
// retune a limit at run time without editing the table (Constitution §8).
const ScalarSliderRange& RangeOf(StratumsTabScalar scalar, StratumsTabState& state) {
    const int scalarIndex = static_cast<int>(scalar);
    if (scalarIndex < 0 || scalarIndex >= kStratumsTabScalarCount) return state.scalarRanges[0];
    return state.scalarRanges[scalarIndex];
}

// The toggle for one control OF ONE STRATUM — nine sections draw the same control, and each keeps
// its own drag state (StratumsTab_UI.h).
RealtimeToggle& ToggleOf(StratumsTabScalar scalar, StratumRowState& row) {
    const int scalarIndex = static_cast<int>(scalar);
    if (scalarIndex < 0 || scalarIndex >= kStratumsTabScalarCount) return row.scalarToggles[0];
    return row.scalarToggles[scalarIndex];
}

} // namespace

void NotifyStratumsTabChange(bool bCommitted, Pipeline::PreviewDriver* previewDriver) {
    if (bCommitted && previewDriver != nullptr) previewDriver->NotifyParametersChanged();
}

WidgetChange DrawStratumsTabScalar(StratumsTabScalar scalar, float& value, StratumsTabState& state,
                                   StratumRowState& row) {
    const StratumsTabScalarDescription& description = StratumsTabScalarDescriptionOf(scalar);
    return DrawSliderScalar(description.label, value, RangeOf(scalar, state), ToggleOf(scalar, row),
                            WidgetStyle(), description.valueFormat);
}

WidgetChange DrawStratumsTabScalarChannel(StratumsTabScalar scalar, int channel, const char* channelSuffix,
                                          float& value, RealtimeToggle& toggle, StratumsTabState& state) {
    const StratumsTabScalarDescription& description = StratumsTabScalarDescriptionOf(scalar);
    char label[48];
    std::snprintf(label, sizeof(label), "%s %s", description.label, channelSuffix);
    return DrawSliderScalar(label, value, RangeOf(scalar, state), toggle, WidgetStyle(), description.valueFormat);
}

} // namespace Ui
} // namespace SanmapGen
