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
#include "MarkersTab_ManualInstanceSelection_UI.h"
#include "MarkersTab_ManualLayers_UI.h"
#include "../params/Geometry_PARAMS.h"
#include "../params/MarkerInstance_PARAMS.h"
#include "../params/Symmetry_PARAMS.h"

namespace SanmapGen {
namespace Ui {

// One instance row's own body: Selectable + click (Ctrl toggle/Shift range/plain, STEP141) -> both
// the multi-select set AND the single "primary" selection (the tab-local write, canvas-selection
// callback) update together, plus a drag SOURCE so the row can be dropped onto a Layer elsewhere
// (MarkersTab_ManualInstanceSelection_UI.h's own DrawManualLayerInstanceDropTarget). Declared here
// (not file-local/anonymous) so MarkersTab_UI.cpp's base-section instance list (STEP138 — the one
// "no Layer at all" case the current data model can represent) draws an IDENTICAL row rather than a
// near-duplicate copy.
void DrawManualInstanceRow(std::vector<Params::MarkerInstanceGroup>& markers,
                           const std::pair<int, int>& groupTransformIndex,
                           ManualInstanceRowInteractionContext_UI& interaction);

// STEP142/STEP144 — the layer header's own condensed small-button cluster (human's own instruction:
// no more checkboxes, small buttons instead), right-aligned as a group so the LAST one ("X") always
// lands flush against the row's true right edge regardless of any individual width's own drift
// (mirrors MarkersTab_UI.cpp's own SmallButtonWidth/cluster-width pattern) — left to right for a
// Manual leaf: "SYM" toggle, "COL" toggle + its swatch, "V/I" visibility toggle, "X" delete (a
// Procedural leaf's own "E/D"+"V/I"+"X" cluster, MarkersTab_BundleHeaderExtras_UI.cpp, is narrower
// and simply has some unused margin within this SAME shared reserved zone — the tree passes one
// width for every row kind, Group nodes included). Eyeballed against a live frame, like every other
// constant in this file.
inline constexpr float kMarkerLayerSymmetryButtonWidthPixels      = 34.0f;
inline constexpr float kMarkerLayerColorOverrideButtonWidthPixels = 34.0f;
inline constexpr float kMarkerLayerColorOverrideSwatchWidthPixels = 20.0f;
inline constexpr float kMarkerLayerVisibilityButtonWidthPixels    = 30.0f;
inline constexpr float kMarkerLayerHeaderExtraDeleteButtonWidthPixels = 26.0f;
inline constexpr float kMarkerLayerHeaderExtraCombinedWidthPixels =
    kMarkerLayerSymmetryButtonWidthPixels + kMarkerLayerColorOverrideButtonWidthPixels
    + kMarkerLayerColorOverrideSwatchWidthPixels + kMarkerLayerVisibilityButtonWidthPixels
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
// STEP141: `selectedManualInstanceIdentifiers`/`anchorIdentifier` are the Ctrl/Shift multi-select
// set (MarkersTabState's own new fields) — threaded through so this row's own click can update both
// the multi-select AND the pre-existing single "primary" selection together
// (MarkersTab_ManualInstanceSelection_UI.h).
bool DrawLayerRowBody(Params::MarkerInstanceLayer& layer, int layerIndex,
                      const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                      std::vector<Params::MarkerInstanceGroup>& markers, const Params::Geometry& geometry,
                      int globalSymmetryMask, int globalRadialRepeatCount,
                      Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings, ManualMarkerLayersState& state,
                      const ManualInstanceLayerIndex_UI& instanceIndex, int& selectedManualInstanceIdentifier,
                      std::vector<int>& selectedManualInstanceIdentifiers, int& anchorIdentifier,
                      const std::function<void(int)>& selectManualMarkerInstanceCallback = {});

// STEP123/STEP142: the row header's own compact Color Override control — a "COL" SmallButton toggle
// (was a checkbox, human's own instruction) + its swatch, drawn on EVERY row's header line via
// DraggableList's/TreeListWidget's header-extra slot, not gated on row-expand state. STEP130: this
// is now the ONLY place Color Override draws — the body copy formerly here for bundled layers
// (which had no other way to reach the control) is deleted, since the Bundle tree's own
// `drawLeafHeaderExtra` slot (ARCH §19.23) now reaches this same function for bundled rows too.
void DrawManualMarkerLayerColorOverrideHeaderControl(Params::MarkerInstanceLayer& layer,
                                                      ManualMarkerLayersState& state, bool& bAnyCommitted);

// STEP130/STEP142: the row header's own Symmetry-toggle control — a "SYM" SmallButton (was a plain
// checkbox, human's own instruction), highlighted while on. Mirrors
// DrawManualMarkerLayerColorOverrideHeaderControl's own shape (hover tooltip). Drawn LEFT of the
// Color Override control at every call site (`[SYM][COL][swatch]`).
void DrawMarkerLayerSymmetryToggleHeaderControl(Params::MarkerInstanceLayer& layer, bool& bAnyCommitted);

// STEP142 — double-click-the-header rename for a Layer row (mirrors the Group's own STEP140
// mechanism, human's own instruction), positioned OVER the header's own name text rather than in the
// far-right header-extra zone (human's own correction). Must be called FIRST in the header-extra
// callback, immediately after the row's own CollapsingHeader/TreeNodeEx (the "last item" this reads
// via GetItemRectMin — TreeListWidget_RowLayout_UI.h/DraggableListWidget_RowLayout_UI.h's own shared
// contract). Returns true while a rename is in progress THIS frame — the caller should then skip
// drawing its own SYM/COL/X controls (the name box already claims the rest of the row) and return.
// A SCRATCH buffer, not `layer.name` directly: DraggableList's Collapsible row computes its own
// CollapsingHeader id FROM `row.label` (== the layer's name) every frame, so live-editing the real
// field churns that id every keystroke and collapses the row out from under whoever is typing — the
// bug the human reported, root-caused to exactly this (DraggableList has no caller-owned expand-state
// override the way TreeListWidget's node rows already do via SetNextItemOpen).
bool DrawLayerHeaderNameOverlay(int layerIndex, Params::MarkerInstanceLayer& layer,
                                ManualMarkerLayersState& state, bool& bAnyCommitted);

} // namespace Ui
} // namespace SanmapGen
