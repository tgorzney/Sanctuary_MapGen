// MarkersTab_ManualLayers_UI.h — the Manual Marker Layers block of the Markers tab (STEP81 part
// (a)). Layer: UI. Accuracy class: Visual. `PropsTab_Manual_UI.h`/`.cpp` (ARCH_12_
// ManualPropDecalLayers.md §12) is the precedent this block mirrors; every divergence from it is
// enumerated in STEP81_MarkersTabManualLayers_UI.md, not re-derived here.
//
// Edits `recipe.markerLayers` (`Params::MarkerInstanceLayer`, the layer metadata array) and
// repairs `recipe.markers` (`Params::MarkerInstanceGroup::transforms[].layerIndex`) when a layer
// is deleted or reordered — the same genuinely-different-data-source split PropsTab_Manual_UI.h
// documents for props/decals. The repair functions themselves live in the sibling header
// MarkerLayerIndexRepair_UI.h (ARCH_01_05_FileSizeCeilings.md §1.5 — this file's own divergence
// from the Props precedent, which keeps the repairs inline in its own header).
//
// SCOPE NOTES (ARCH §8.4 — reported, not invented):
//  1. No `Data::PlacementInstances` parameter and no read-only transform-list block: unlike Props,
//     the Markers tab already previews the resolved buffer via `DrawPlacedMarkerList`
//     (MarkersTab_Placed_UI.h). Porting Props' `DrawTransformList` here would be a rival second
//     view of the same buffer (STEP81 divergence 1).
//  2. `MarkerInstanceLayer` carries no `bEnabled`/`bHidden` — the shared DraggableList's
//     visibility/lock affordances stay inert here, the same posture props already have (STEP81
//     divergence 5). Not invented here; reported as a field-request candidate.
//  3. Never notifies `Pipeline::PreviewDriver`: `recipe.markers`/`recipe.markerLayers` feed no
//     PROC stage (STEP60 confirmed zero `MarkerInstanceGroup` references under `src/proc/`), the
//     same silent posture STEP49 already adopts for the manual roster.
#pragma once
#include "ColorSwatch_UI.h"
#include "Section_UI.h"
#include "SliderScalar_UI.h"
#include "UniqueNameList_UI.h"
#include "../params/MarkerInstance_PARAMS.h"

namespace SanmapGen {
namespace Ui {

struct ManualMarkerLayersState {
    SectionState       section;
    ColorSwatchOptions previewColorOptions;                     // picker only, no RGBA fields
    ScalarSliderRange  iconScaleRange{ 0.1f, 10.0f, 0.0f };     // same bounds as props (§8)

    bool           bUseGroupColor = false;                      // one tint for every layer
    float          groupColor[kColorSwatchChannelCount] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float          layerIconScale = 1.0f;
    RealtimeToggle groupColorToggle;
    RealtimeToggle layerIconScaleToggle;

    // ONE shared toggle set for the SELECTED row's own color/scale — `Params::MarkerInstanceLayer`
    // is a pure round-tripping type and cannot carry a `RealtimeToggle` member, exactly as
    // `PropInstanceLayer` cannot (`PropsTab_Manual_UI.h:55-60`).
    RealtimeToggle selectedLayerColorToggle;
    RealtimeToggle selectedLayerIconScaleToggle;

    SectionState   symmetrySection;                             // NEW vs. props — layer-level symmetry
    int            selectedLayerIndex = -1;
};

// The layer the per-row controls edit, or null when the selection points at nothing
// (Constitution §6 — an index is validated, never trusted).
inline Params::MarkerInstanceLayer* SelectedManualMarkerLayer(
        std::vector<Params::MarkerInstanceLayer>& markerLayers, int selectedLayerIndex) {
    if (selectedLayerIndex < 0 || selectedLayerIndex >= static_cast<int>(markerLayers.size())) return nullptr;
    return &markerLayers[static_cast<std::size_t>(selectedLayerIndex)];
}

// The color a layer actually draws with: its own, unless the block is set to one shared tint.
inline const float* EffectiveManualMarkerLayerColor(const ManualMarkerLayersState& state,
                                                     const Params::MarkerInstanceLayer& layer) {
    return state.bUseGroupColor ? state.groupColor : layer.color;
}

// The label a layer row shows — never empty (Constitution §6). Reused by part (b)'s Layer picker
// so an unnamed layer never renders as a blank, unpickable row.
inline const char* ManualMarkerLayerRowLabel(const Params::MarkerInstanceLayer& layer) {
    return layer.name.empty() ? "Marker Layer" : layer.name.c_str();
}

// The name "Add Marker Layer" seeds a fresh row with, before the shared uniqueness repair runs.
inline std::string NextMarkerLayerName(int layerCount) { return NextUniqueLabel("Marker Layer", layerCount); }

// `markers` is `recipe.markers`, repaired here when a layer is deleted or reordered.
// No `Data::PlacementInstances*` parameter — see SCOPE NOTE 1.
// No `Pipeline::PreviewDriver*` parameter — see SCOPE NOTE 3.
void DrawManualMarkerLayers(ManualMarkerLayersState& state,
                            std::vector<Params::MarkerInstanceLayer>& markerLayers,
                            std::vector<Params::MarkerInstanceGroup>& markers);

} // namespace Ui
} // namespace SanmapGen
