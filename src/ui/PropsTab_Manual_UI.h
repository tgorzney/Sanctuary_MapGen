// PropsTab_Manual_UI.h — the manual prop layers block of the Props tab.
// Layer: UI. Accuracy class: Visual. TAB_REBUILD_PLAN "§ Props · Manual prop layers".
//
// SCOPE NOTES (ARCH §8.4 — a coder never invents a missing type; reported, not invented):
//  1. v1's manual prop GROUPS (imported, hand-placed props kept as named groups) have no
//     `_PARAMS` home in the tree: `MapRecipe` carries scatter RULES only, and there is no
//     `Params::ManualPropGroup`. The group list below is therefore CALLER-OWNED UI presentation
//     state that the app shell (WO E) fills after an import — the same standing HeightmapTab_UI's
//     global gravity has. It is NOT serialized, and it does NOT notify Pipeline::PreviewDriver:
//     no stage hashes a preview tint, and asking for a regeneration one cannot affect is the
//     "cheap tweak triggers a full regen" defect. A durable home is its own work-order.
//  2. The per-group "transforms" list is READ-ONLY and reads the Placement stage's resolved prop
//     buffer (`Data::PlacementInstances`), because every DATA field has exactly one writing stage
//     (Constitution §1) and a tab that edited it would be the UI simulating.
#pragma once
#include <string>
#include <vector>
#include "ColorSwatch_UI.h"
#include "Section_UI.h"
#include "SliderScalar_UI.h"

namespace SanmapGen {
namespace Data { class PlacementInstances; }
namespace Ui {

struct ManualPropGroup {
    std::string    name;
    float          previewColor[kColorSwatchChannelCount] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float          iconScale = 1.0f;
    RealtimeToggle previewColorToggle;
    RealtimeToggle iconScaleToggle;
};

struct ManualPropLayersState {
    SectionState       section;
    SectionState       transformListSection;
    ColorSwatchOptions previewColorOptions;                       // picker only, no RGBA fields
    ScalarSliderRange  iconScaleRange{ 0.1f, 10.0f, 0.0f };

    bool           bUseGroupColor = false;                        // one tint for every group
    float          groupColor[kColorSwatchChannelCount] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float          layerIconScale = 1.0f;
    RealtimeToggle groupColorToggle;
    RealtimeToggle layerIconScaleToggle;

    std::vector<ManualPropGroup> groups;
    int   selectedGroupIndex  = -1;
    float transformRowHeight  = 20.0f;    // the TRUE row height: the clipper scrolls with it
    float transformListHeight = 160.0f;
};

// The group the per-group controls edit, or null when the selection points at nothing
// (Constitution §6 — an index is validated, never trusted).
inline ManualPropGroup* SelectedManualPropGroup(ManualPropLayersState& state) {
    if (state.selectedGroupIndex < 0
        || state.selectedGroupIndex >= static_cast<int>(state.groups.size())) return nullptr;
    return &state.groups[static_cast<std::size_t>(state.selectedGroupIndex)];
}

// The color a group actually draws with: its own, unless the block is set to one shared tint.
inline const float* EffectiveManualPropGroupColor(const ManualPropLayersState& state,
                                                  const ManualPropGroup& group) {
    return state.bUseGroupColor ? state.groupColor : group.previewColor;
}

// The label a group row shows — never empty (Constitution §6).
inline const char* ManualPropGroupRowLabel(const ManualPropGroup& group) {
    return group.name.empty() ? "Prop Group" : group.name.c_str();
}

// `placedProps` is nullable: before the first generation there is no resolved buffer.
void DrawManualPropLayers(ManualPropLayersState& state, const Data::PlacementInstances* placedProps);

} // namespace Ui
} // namespace SanmapGen
