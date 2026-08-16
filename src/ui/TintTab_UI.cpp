// TintTab_UI.cpp — the Tint tab's label vocabulary over the shared mask-layer composition.
// Layer: UI. TAB_REBUILD_PLAN "8 · Tint": "Show overlay Checkbox · layer stack (TintLayers)".
#include "TintTab_UI.h"

namespace SanmapGen {
namespace Ui {

void DrawTintTab(Params::LayerStack& tintLayers, MaskLayerTabState& state,
                 Pipeline::GenerationAssembler* generationAssembler,
                 Pipeline::PreviewDriver* previewDriver) {
    DrawMaskLayerTab("tintTab", "Show Tint Overlay", "Tint Layers", tintLayers, state,
                     generationAssembler, previewDriver);
}

} // namespace Ui
} // namespace SanmapGen
