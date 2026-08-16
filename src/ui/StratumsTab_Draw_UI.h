// StratumsTab_Draw_UI.h — the composition helpers the Stratums tab's three panel translation units
// share. Layer: UI. The tab's draw path is split across StratumsTab_UI / _Material_UI /
// _Appearance_UI / _Soil_UI to stay inside the ARCH §1.5 ceilings; these are the pieces all of them
// need, written once rather than four times.
//
// Every helper composes the batch-A shared widgets — no panel here draws a control of its own and
// none calls ImGui::SliderFloat/Checkbox/Combo directly (UI_FRAMEWORK_SPEC "Universal widget
// library").
#pragma once
#include "Checkbox_UI.h"
#include "StratumsTab_UI.h"

namespace SanmapGen {
namespace Ui {

// The ONE thing a panel does with a commit. WHICH tier it becomes is the driver's derivation from
// the stage parameter hashes, never this call site's decision.
void NotifyStratumsTabChange(bool bCommitted, Pipeline::PreviewDriver* previewDriver);

// One catalogued slider: the label, limits, format and RT toggle all come from the enumerator and
// the stratum's own row, so a panel names a control and nothing else.
WidgetChange DrawStratumsTabScalar(StratumsTabScalar scalar, float& value,
                                   StratumsTabState& state, StratumRowState& row);

// The same, with the commit already routed to the driver — the shape almost every row wants.
inline void DrawStratumsTabScalarRow(StratumsTabScalar scalar, float& value, StratumsTabState& state,
                                     StratumRowState& row, Pipeline::PreviewDriver* previewDriver) {
    NotifyStratumsTabChange(DrawStratumsTabScalar(scalar, value, state, row).bCommitted, previewDriver);
}

// The three per-stratum panels, one translation unit each. `generationAssembler` may be null.
void DrawStratumMaterialPanel(Params::Stratum& stratum, StratumsTabState& state, StratumRowState& row,
                              Pipeline::PreviewDriver* previewDriver);
void DrawStratumAppearancePanel(Params::Stratum& stratum, StratumsTabState& state, StratumRowState& row,
                                Pipeline::PreviewDriver* previewDriver);
void DrawStratumSoilPanel(Params::Stratum& stratum, int stratumIndex, StratumsTabState& state,
                          StratumRowState& row, Pipeline::GenerationAssembler* generationAssembler,
                          Pipeline::PreviewDriver* previewDriver);

} // namespace Ui
} // namespace SanmapGen
