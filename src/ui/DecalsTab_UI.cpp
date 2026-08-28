// DecalsTab_UI.cpp — the imgui composition of the Decals tab. Layer: UI.
// Body moved verbatim from PropsTab_UI.cpp's own decal draw calls (ARCH §20); neither sibling
// function's own logic changed.
#include "DecalsTab_UI.h"
#include "../params/MapRecipe_PARAMS.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {

void DrawDecalsTab(Params::MapRecipe& recipe, DecalsTabState& state,
                   Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest,
                   const Data::PlacementInstances* placedDecals) {
    ImGui::PushID("decalsTab");
    DrawManualDecalLayers(state.manualDecalLayers, recipe.decalLayers, recipe.decals, placedDecals);
    DrawDecalRuleStack(recipe.decalRules, state.decalStack, previewDriver, iconManifest);
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
