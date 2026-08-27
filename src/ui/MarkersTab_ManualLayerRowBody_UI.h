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
#include <functional>
#include <utility>
#include <vector>
#include "ManualInstanceLayerIndex_UI.h"
#include "MarkersTab_ManualLayers_UI.h"
#include "../params/Geometry_PARAMS.h"
#include "../params/MarkerInstance_PARAMS.h"
#include "../params/Symmetry_PARAMS.h"

namespace SanmapGen {
namespace Ui {

// One instance row's own body (Selectable + click -> select, both the tab-local write and the
// canvas-selection callback). Declared here (not file-local/anonymous) so MarkersTab_UI.cpp's
// base-section instance list (STEP138 — the one "in a Group/Section with no Layer" case the current
// data model can represent, since layerIndex has no true unassigned sentinel) draws an IDENTICAL
// row rather than a near-duplicate copy.
void DrawManualInstanceRow(std::vector<Params::MarkerInstanceGroup>& markers,
                           const std::pair<int, int>& groupTransformIndex,
                           int& selectedManualInstanceIdentifier,
                           const std::function<void(int)>& selectManualMarkerInstanceCallback);

// STEP123 — reserved width for the header's Color Override checkbox + compact swatch
// (DrawManualMarkerLayerColorOverrideHeaderControl, below), left of DraggableList's own strip.
// Eyeballed against a live frame (Checkbox_UI.cpp/ColorSwatch_UI.cpp's own "verified by eye, never
// by test" posture). Shrunk from 90 (checkbox + swatch + RT button) once the swatch's own RT
// button was removed (color edits are always realtime now, human's own instruction) — the control
// is narrower by roughly one RT button's own width (WidgetStyle().realtimeButtonWidth, 30px) plus
// its SameLine gap.
inline constexpr float kMarkerLayerColorOverrideHeaderWidthPixels = 55.0f;
inline constexpr float kMarkerLayerColorOverrideSwatchWidthPixels = 24.0f;

// STEP130 (ARCH §19.24) — reserved width for the header's Symmetry-toggle checkbox
// (DrawMarkerLayerSymmetryToggleHeaderControl, below), placed LEFT of the Color Override control,
// so the header's own reservation is the sum of both. Eyeballed the same way as the constant above:
// a plain, no-label checkbox is narrower than the Color Override pair (no swatch), plus its own
// `ImGui::SameLine()` gap before Color Override starts.
inline constexpr float kMarkerLayerSymmetryToggleWidthPixels = 30.0f;

// STEP140 — reserved width for the header's own "X" delete button (Manual AND Procedural leaves,
// and the Bundle tree's Group nodes reuse this SAME combined width — MarkersTab_Bundles_UI.cpp's
// `Render` call takes one shared width for every row kind), placed RIGHTMOST of everything else on
// the row. Eyeballed the same way as the constants above.
inline constexpr float kMarkerLayerHeaderExtraDeleteButtonWidthPixels = 26.0f;
inline constexpr float kMarkerLayerHeaderExtraCombinedWidthPixels =
    kMarkerLayerSymmetryToggleWidthPixels + kMarkerLayerColorOverrideHeaderWidthPixels
    + kMarkerLayerHeaderExtraDeleteButtonWidthPixels;

// The row's own name, tint, icon scale, grid snap, symmetry setting, and (STEP126, Open Q7) its own
// per-Layer instance list — STEP110: drawn inline in THIS row's own expanded body, not "selected"-
// gated. Tint hides under the block's shared-color mode (ARCH §4 rival-control rule). Layer-level
// symmetry is the deliberate, separately-ratified exception manual markers get over Props/Decals
// (ARCH_14_13_OpenItems.md §14.13 Ruling 3) — returns whether the name committed, so the caller can
// re-run the uniqueness repair. STEP120: lives in its own translation unit (not
// MarkersTab_ManualLayers_UI.cpp's anonymous namespace) so MarkersTab_Bundles_UI.cpp can reuse it
// UNCHANGED as the tree's Manual leaf-body callback (ARCH_19_07's "good news" finding).
// ARCH §19.25, item 5 — `selectManualMarkerInstanceCallback` (Application's own shell-mediated
// closure, empty default so every existing call site compiles unchanged) is called BY the instance-
// list Selectable click below, IN ADDITION TO the existing `selectedManualInstanceIdentifier`
// tab-local write, not instead of it: the tab-local write keeps the list's own highlight in sync
// with itself, and the callback additionally drives the canvas's REAL selection (the actual fix —
// see MapCanvas_UI.h's SelectManualMarkerByInstanceIdentifier).
bool DrawLayerRowBody(Params::MarkerInstanceLayer& layer, int layerIndex,
                      const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                      std::vector<Params::MarkerInstanceGroup>& markers, const Params::Geometry& geometry,
                      int globalSymmetryMask, int globalRadialRepeatCount,
                      Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings, ManualMarkerLayersState& state,
                      const ManualInstanceLayerIndex_UI& instanceIndex, int& selectedManualInstanceIdentifier,
                      const std::function<void(int)>& selectManualMarkerInstanceCallback = {});

// STEP123: the row header's own compact Color Override control (checkbox + swatch), drawn on EVERY
// row's header line via DraggableList's/TreeListWidget's header-extra slot, not gated on row-expand
// state. STEP130: this is now the ONLY place Color Override draws — the body copy formerly here for
// bundled layers (which had no other way to reach the control) is deleted, since the Bundle tree's
// own `drawLeafHeaderExtra` slot (ARCH §19.23) now reaches this same function for bundled rows too.
void DrawManualMarkerLayerColorOverrideHeaderControl(Params::MarkerInstanceLayer& layer,
                                                      ManualMarkerLayersState& state, bool& bAnyCommitted);

// STEP130 (ARCH §19.24): the row header's own Symmetry-toggle control — a plain checkbox bound to
// `layer.bSymmetryEnabled`, no swatch. Mirrors DrawManualMarkerLayerColorOverrideHeaderControl's
// shape exactly (empty label, hover tooltip). Drawn LEFT of the Color Override control at every
// call site (`[Symmetry toggle][Color Override]`).
void DrawMarkerLayerSymmetryToggleHeaderControl(Params::MarkerInstanceLayer& layer, bool& bAnyCommitted);

} // namespace Ui
} // namespace SanmapGen
