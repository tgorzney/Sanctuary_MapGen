// HolesTab_UI.h — the Holes tab: a show-overlay toggle over the terrain-hole layer stack.
// Layer: UI. Accuracy class: Visual. TAB_REBUILD_PLAN "9 · Holes".
//
// The whole composition is the shared MaskLayerTab (show toggle + hosted Layer Editor); this file
// exists so the host names a TAB rather than passing three label strings at the call site, and so
// the tab's vocabulary lives with the tab. The stack it edits comes from the caller —
// `Params::MapRecipe` has no `holeLayers` field (MaskLayerTab_UI.h SCOPE NOTE 1).
#pragma once
#include "MaskLayerTab_UI.h"

namespace SanmapGen {
namespace Ui {

void DrawHolesTab(Params::LayerStack& holeLayers, MaskLayerTabState& state,
                  Pipeline::GenerationAssembler* generationAssembler,
                  Pipeline::PreviewDriver* previewDriver);

} // namespace Ui
} // namespace SanmapGen
