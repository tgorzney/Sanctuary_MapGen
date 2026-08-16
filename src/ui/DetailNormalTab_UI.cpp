// DetailNormalTab_UI.cpp — the Detail Normal tab: the size dropdown, then the shared mask-layer
// composition. Layer: UI. TAB_REBUILD_PLAN "7 · Detail Normal": "Show overlay Checkbox · Detail
// Normal Size Combo {256..4096} · layer stack (DetailNormalLayers)".
#include "DetailNormalTab_UI.h"
#include "Combo_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {

void DrawDetailNormalTab(Params::LayerStack& detailNormalLayers, DetailNormalTabState& state,
                         Pipeline::GenerationAssembler* generationAssembler,
                         Pipeline::PreviewDriver* previewDriver) {
    ImGui::PushID("detailNormalTab");
    ComboOptions sizeOptions;
    sizeOptions.labels     = detailNormalSizeLabels;
    sizeOptions.count      = kDetailNormalSizeCount;
    sizeOptions.emptyLabel = "<custom>";
    // The size drives a preview-side texture the pipeline does not hash today (SCOPE NOTE 2), so
    // the pick is not notified: it would ask for a regeneration nothing would consume.
    DrawCombo("Detail Normal Size", state.detailNormalSizeIndex, sizeOptions);
    ImGui::PopID();

    DrawMaskLayerTab("detailNormalTab", "Show Detail Normal Overlay", "Detail Normal Layers",
                     detailNormalLayers, state.maskLayerTab, generationAssembler, previewDriver);
}

} // namespace Ui
} // namespace SanmapGen
