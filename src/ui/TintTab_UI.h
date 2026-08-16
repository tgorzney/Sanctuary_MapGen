// TintTab_UI.h — the Tint tab: a show-overlay toggle over the procedural tint layer stack.
// Layer: UI. Accuracy class: Visual. TAB_REBUILD_PLAN "8 · Tint".
//
// The whole composition is the shared MaskLayerTab (show toggle + hosted Layer Editor); this file
// exists so the host names a TAB rather than passing three label strings at the call site, and so
// the tab's vocabulary lives with the tab. The stack it edits comes from the caller —
// `Params::MapRecipe` has no `tintLayers` field (MaskLayerTab_UI.h SCOPE NOTE 1).
#pragma once
#include "MaskLayerTab_UI.h"

namespace SanmapGen {
namespace Ui {

void DrawTintTab(Params::LayerStack& tintLayers, MaskLayerTabState& state,
                 Pipeline::GenerationAssembler* generationAssembler,
                 Pipeline::PreviewDriver* previewDriver);

} // namespace Ui
} // namespace SanmapGen
