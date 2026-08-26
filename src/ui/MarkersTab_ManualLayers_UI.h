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
//     (MarkersTab_Placed_UI.h) — a ported `DrawTransformList` would be a rival second view (STEP81 divergence 1).
//  2. `MarkerInstanceLayer` carries no `bEnabled`/`bHidden` — the shared DraggableList's
//     visibility/lock affordances stay inert here, same as props (STEP81 divergence 5).
//  3. Never notifies `Pipeline::PreviewDriver`: `recipe.markers`/`recipe.markerLayers` feed no
//     PROC stage (STEP60), same silent posture STEP49 already adopts for the manual roster.
#pragma once
#include <cmath>
#include "ColorSwatch_UI.h"
#include "MarkerSymmetryFixCommand_UI.h"
#include "Section_UI.h"
#include "SliderScalar_UI.h"
#include "UniqueNameList_UI.h"
#include "../params/Geometry_PARAMS.h"
#include "../params/MarkerInstance_PARAMS.h"
#include "../params/Symmetry_PARAMS.h"

namespace SanmapGen {
namespace Ui {

struct ManualMarkerLayersState {
    SectionState       section;
    ColorSwatchOptions previewColorOptions;                     // picker only, no RGBA fields
    ScalarSliderRange  iconScaleRange{ 0.1f, 10.0f, 0.0f };     // same bounds as props (§8)
    ScalarSliderRange  gridSnapSizeRange{ 0.1f, 100.0f, 0.0f };  // Constitution §8 — a setting, not a
                                                                  // literal at the DrawSliderScalar call

    bool           bUseGroupColor = false;                      // one tint for every layer
    float          groupColor[kColorSwatchChannelCount] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float          layerIconScale = 1.0f;
    RealtimeToggle groupColorToggle{true};
    RealtimeToggle layerIconScaleToggle{true};

    // ONE shared toggle set for the SELECTED row's own color/scale — `Params::MarkerInstanceLayer`
    // is a pure round-tripping type and cannot carry a `RealtimeToggle` member, exactly as
    // `PropInstanceLayer` cannot (`PropsTab_Manual_UI.h:55-60`).
    RealtimeToggle selectedLayerColorToggle{true};
    RealtimeToggle selectedLayerIconScaleToggle{true};
    RealtimeToggle selectedLayerGridSnapToggle{true};

    SectionState   symmetrySection;                             // NEW vs. props — layer-level symmetry
    int            selectedLayerIndex = -1;

    // STEP107 — the "Fix Symmetry" command's own controls. ONE shared instance for the whole block
    // (same constraint `selectedLayerColorToggle`/`selectedLayerIconScaleToggle` already accept
    // above); `fixSymmetryToleranceRange`/`fixSymmetryToleranceToggle` back the recipe-level
    // `Params::MarkerSymmetryFixSettings::distanceTolerance` slider — recipe-level, not per-layer.
    ScalarSliderRange         fixSymmetryToleranceRange{ 0.01f, 10.0f, 0.0f };
    RealtimeToggle            fixSymmetryToleranceToggle{true};
    bool                      bFixSymmetryOverwrite   = false;
    bool                      bHasFixSymmetryResult   = false;
    Ui::MarkerSymmetryFixResult lastFixSymmetryResult;
};

// The layer the per-row controls edit, or null when the selection points at nothing
// (Constitution §6 — an index is validated, never trusted).
inline Params::MarkerInstanceLayer* SelectedManualMarkerLayer(
        std::vector<Params::MarkerInstanceLayer>& markerLayers, int selectedLayerIndex) {
    if (selectedLayerIndex < 0 || selectedLayerIndex >= static_cast<int>(markerLayers.size())) return nullptr;
    return &markerLayers[static_cast<std::size_t>(selectedLayerIndex)];
}

// True when `layerIndex` names a layer with bLocked set. Out-of-range (Constitution §6) resolves
// to false — an invalid layerIndex must never itself become a reason to refuse an edit; that is a
// distinct failure mode (see the existing layerIndex clamp-on-import, STEP60 §4) this gate does not
// participate in.
inline bool IsMarkerInstanceLayerLocked(const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                        int layerIndex) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(markerLayers.size())) return false;
    return markerLayers[static_cast<std::size_t>(layerIndex)].bLocked;
}

// The world position `(worldX, worldZ)` quantized to `layerIndex`'s own grid setting, or
// unchanged if that layer has grid snap off, is out of range (Constitution §6 — resolves to
// unchanged, the same posture as IsMarkerInstanceLayerLocked's out-of-range-safe default), or its
// own `gridSnapSizeWorldUnits` is non-positive (a non-positive cell size cannot quantize; treated
// as snap-off rather than a divide-by-zero/no-op hazard).
inline void QuantizeMarkerPositionToLayerGrid(const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                              int layerIndex, float& worldX, float& worldZ) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(markerLayers.size())) return;
    const Params::MarkerInstanceLayer& layer = markerLayers[static_cast<std::size_t>(layerIndex)];
    if (!layer.bGridSnapEnabled || layer.gridSnapSizeWorldUnits <= 0.0f) return;
    const float cellSize = layer.gridSnapSizeWorldUnits;
    worldX = std::round(worldX / cellSize) * cellSize;
    worldZ = std::round(worldZ / cellSize) * cellSize;
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

// MarkersTab_ManualLayerRowBody_UI.cpp — ARCH §1.5 aspect split (Coder-flagged): one row's name/
// tint/icon scale/grid snap/symmetry, drawn inline in its own expanded body (STEP110). Its own
// translation unit (not MarkersTab_ManualLayers_UI.cpp's anonymous namespace) so
// MarkersTab_Bundles_UI.cpp can reuse it UNCHANGED as the tree's Manual leaf-body callback
// (ARCH_19_07) — this file's own call site (DrawLayerList) is unaffected.
bool DrawLayerRowBody(Params::MarkerInstanceLayer& layer, int layerIndex,
                      const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                      std::vector<Params::MarkerInstanceGroup>& markers, const Params::Geometry& geometry,
                      int globalSymmetryMask, int globalRadialRepeatCount,
                      Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings, ManualMarkerLayersState& state);

// MarkersTab_ManualLayers_UI.cpp:

// The Add Marker Layer button. STEP120: gains an optional Bundle-scoped parent so a Bundle node's
// own "add a Layer here" (MarkersTab_Bundles_UI.cpp) can reuse it; moved out of the anonymous
// namespace. `parentBundleIdentifierForNewLayer < 0` (default) is root scope — this file's own
// existing call site passes -1, unchanged behavior.
bool DrawLayerListButtons(std::vector<Params::MarkerInstanceLayer>& markerLayers, ManualMarkerLayersState& state,
                          int parentBundleIdentifierForNewLayer = -1);

// `markers` is `recipe.markers`, repaired here when a layer is deleted or reordered.
// No `Data::PlacementInstances*` parameter — see SCOPE NOTE 1.
// No `Pipeline::PreviewDriver*` parameter — see SCOPE NOTE 3.
// STEP107: `geometry`/`globalSymmetryMask`/`globalRadialRepeatCount`/`markerSymmetryFixSettings` are
// the "Fix Symmetry" command's own required inputs, unavailable to this function before that ticket.
void DrawManualMarkerLayers(ManualMarkerLayersState& state,
                            std::vector<Params::MarkerInstanceLayer>& markerLayers,
                            std::vector<Params::MarkerInstanceGroup>& markers,
                            const Params::Geometry& geometry, int globalSymmetryMask,
                            int globalRadialRepeatCount,
                            Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings);

} // namespace Ui
} // namespace SanmapGen
