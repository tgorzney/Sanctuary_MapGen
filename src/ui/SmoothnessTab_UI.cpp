// SmoothnessTab_UI.cpp — the Smoothness tab's label vocabulary over the shared mask-layer
// composition. Layer: UI.
// TAB_REBUILD_PLAN "10 · Smoothness": "Show overlay Checkbox · layer stack (SmoothnessLayers)".
#include "SmoothnessTab_UI.h"

namespace SanmapGen {
namespace Ui {

void DrawSmoothnessTab(Params::LayerStack& smoothnessLayers, MaskLayerTabState& state,
                       Pipeline::GenerationAssembler* generationAssembler,
                       Pipeline::PreviewDriver* previewDriver) {
    DrawMaskLayerTab("smoothnessTab", "Show Smoothness Overlay", "Smoothness Layers",
                     smoothnessLayers, state, generationAssembler, previewDriver);
}

} // namespace Ui
} // namespace SanmapGen
