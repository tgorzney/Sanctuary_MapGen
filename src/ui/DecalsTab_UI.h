// DecalsTab_UI.h — the Decals tab: the manual decal layers and the procedural decal rule stack.
// Layer: UI. Accuracy class: Visual. ARCH §20 — a real, standalone top-level tab, split out of the
// Props tab (which hosted both domains in one body until this ticket; see
// ARCH_20_DecalsTopLevelTab.md for the ruling and PropsTab_UI.h for what stayed behind). It edits
// two recipe slices — `recipe.decalLayers`/`recipe.decals` (`DecalsTab_Manual_UI.h`) and
// `recipe.decalRules` (`DecalsTab_Rules_UI.h`) — composed here exactly as `PropsTab_UI.h` composed
// them before the split; neither sibling file's own logic changed.
#pragma once
#include "DecalsTab_Manual_UI.h"
#include "DecalsTab_Rules_UI.h"

namespace SanmapGen {
namespace Data { class PlacementInstances; }
namespace Params { struct MapRecipe; }
namespace Pipeline { class PreviewDriver; }
namespace Ui {

struct DecalsTabState {
    ManualDecalLayersState manualDecalLayers;
    DecalRuleStackState    decalStack;
};

// `iconManifest` and `placedDecals` are both nullable, mirroring every other placement tab: with no
// resident atlas the picker degrades to the typed tpId, and before the first generation the
// read-only transform list simply says so.
void DrawDecalsTab(Params::MapRecipe& recipe, DecalsTabState& state,
                   Pipeline::PreviewDriver* previewDriver,
                   const IconAtlasManifest* iconManifest = nullptr,
                   const Data::PlacementInstances* placedDecals = nullptr);

} // namespace Ui
} // namespace SanmapGen
