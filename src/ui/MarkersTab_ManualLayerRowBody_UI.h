// MarkersTab_ManualLayerRowBody_UI.h — DrawLayerRowBody/DrawManualMarkerLayerColorOverrideHeaderControl,
// relocated out of MarkersTab_ManualLayers_UI.h (ARCH_19_22_ManualLayersHeaderSplit.md's FINAL
// combined ruling, delivered STEP125 alongside the Helpers split, MarkersTab_ManualLayerHelpers_UI.h
// — neither split alone cleared the ARCH_01_05 150-line hard ceiling once STEP125's own additions to
// the parent header were counted; both together do, with real margin).
//
// One row's own name/tint/icon scale/grid snap/symmetry/instance-list body (STEP110/STEP123/STEP126)
// and the row header's own compact Color Override control (STEP123) — both defined in the sibling
// MarkersTab_ManualLayerRowBody_UI.cpp, both declared here so MarkersTab_Bundles_UI.cpp can reuse
// DrawLayerRowBody UNCHANGED as the tree's Manual leaf-body callback (ARCH_19_07's "good news"
// finding) without pulling in the rest of MarkersTab_ManualLayers_UI.h's own list-mechanics surface.
#pragma once
#include <vector>
#include "ManualInstanceLayerIndex_UI.h"
#include "MarkersTab_ManualLayers_UI.h"
#include "../params/Geometry_PARAMS.h"
#include "../params/MarkerInstance_PARAMS.h"
#include "../params/Symmetry_PARAMS.h"

namespace SanmapGen {
namespace Ui {

// STEP123 — reserved width for the header's Color Override checkbox + compact swatch
// (DrawManualMarkerLayerColorOverrideHeaderControl, below), left of DraggableList's own strip.
// Eyeballed against a live frame (Checkbox_UI.cpp/ColorSwatch_UI.cpp's own "verified by eye, never
// by test" posture).
inline constexpr float kMarkerLayerColorOverrideHeaderWidthPixels = 90.0f;
inline constexpr float kMarkerLayerColorOverrideSwatchWidthPixels = 24.0f;

// The row's own name, tint, icon scale, grid snap, symmetry setting, and (STEP126, Open Q7) its own
// per-Layer instance list — STEP110: drawn inline in THIS row's own expanded body, not "selected"-
// gated. Tint hides under the block's shared-color mode (ARCH §4 rival-control rule). Layer-level
// symmetry is the deliberate, separately-ratified exception manual markers get over Props/Decals
// (ARCH_14_13_OpenItems.md §14.13 Ruling 3) — returns whether the name committed, so the caller can
// re-run the uniqueness repair. STEP120: lives in its own translation unit (not
// MarkersTab_ManualLayers_UI.cpp's anonymous namespace) so MarkersTab_Bundles_UI.cpp can reuse it
// UNCHANGED as the tree's Manual leaf-body callback (ARCH_19_07's "good news" finding).
bool DrawLayerRowBody(Params::MarkerInstanceLayer& layer, int layerIndex,
                      const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                      std::vector<Params::MarkerInstanceGroup>& markers, const Params::Geometry& geometry,
                      int globalSymmetryMask, int globalRadialRepeatCount,
                      Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings, ManualMarkerLayersState& state,
                      const ManualInstanceLayerIndex_UI& instanceIndex, int& selectedManualInstanceIdentifier);

// STEP123: the row header's own compact Color Override control (checkbox + swatch), drawn on EVERY
// row's header line via DraggableList's header-extra slot, not gated on row-expand state — see
// MarkersTab_ManualLayerRowBody_UI.cpp for why the (unchanged) body copy above is not removed.
void DrawManualMarkerLayerColorOverrideHeaderControl(Params::MarkerInstanceLayer& layer,
                                                      ManualMarkerLayersState& state, bool& bAnyCommitted);

} // namespace Ui
} // namespace SanmapGen
