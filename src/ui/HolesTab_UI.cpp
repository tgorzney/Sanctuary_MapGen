// HolesTab_UI.cpp — the Holes tab's label vocabulary over the shared mask-layer composition.
// Layer: UI. TAB_REBUILD_PLAN "9 · Holes": "Show overlay Checkbox · layer stack (HoleLayers)".
#include "HolesTab_UI.h"

namespace SanmapGen {
namespace Ui {

void DrawHolesTab(Params::LayerStack& holeLayers, MaskLayerTabState& state,
                  Pipeline::GenerationAssembler* generationAssembler,
                  Pipeline::PreviewDriver* previewDriver) {
    DrawMaskLayerTab("holesTab", "Show Holes Overlay", "Hole Layers", holeLayers, state,
                     generationAssembler, previewDriver);
}

} // namespace Ui
} // namespace SanmapGen
