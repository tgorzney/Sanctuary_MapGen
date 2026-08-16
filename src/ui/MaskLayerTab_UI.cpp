// MaskLayerTab_UI.cpp — the imgui composition the four mask-layer tabs share. Layer: UI.
// One shared checkbox, one collapsing section, one hosted Layer Editor. The only raw imgui here is
// the id scope; every control is a batch-A/B shared unit.
#include "MaskLayerTab_UI.h"
#include "Checkbox_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {

void DrawMaskLayerTab(const char* identifier, const char* overlayLabel, const char* sectionLabel,
                      Params::LayerStack& layerStack, MaskLayerTabState& state,
                      Pipeline::GenerationAssembler* generationAssembler,
                      Pipeline::PreviewDriver* previewDriver) {
    ImGui::PushID(identifier);
    // Presentation only (MaskLayerTab_UI.h SCOPE NOTE 2): it trips no pipeline refresh, so the
    // driver is not notified of it.
    DrawCheckbox(overlayLabel, state.bShowOverlay);
    if (DrawSectionBegin(sectionLabel, state.layerSection)) {
        DrawLayerEditor(layerStack, state.layerEditor, generationAssembler, previewDriver);
        DrawSectionEnd();
    }
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
