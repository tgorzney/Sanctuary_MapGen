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
// ARCH_19_22's FINAL combined ruling (STEP125): this file's own row-drawing half lives in the
// sibling MarkersTab_ManualLayerRowBody_UI.h (DrawLayerRowBody / DrawManualMarkerLayerColorOverride-
// HeaderControl + their two width constants), and its five pure helpers live in
// MarkersTab_ManualLayerHelpers_UI.h — both split out so this file clears ARCH_01_05's 150-line hard
// ceiling. The Type-section outer loop that now drives this block per Type (STEP125,
// ARCH §19.14/§19.15) lives one level up, MarkersTab_TypeSections_UI.h.
//
// SCOPE NOTES (ARCH §8.4 — reported, not invented):
//  1. No `Data::PlacementInstances` parameter and no read-only transform-list block: unlike Props,
//     the Markers tab already previews the resolved buffer via `DrawPlacedMarkerList`
//     (MarkersTab_Placed_UI.h) — a ported `DrawTransformList` would be a rival second view (STEP81 divergence 1).
//  2. STEP144: `MarkerInstanceLayer::bHidden` now backs the shared DraggableList's built-in
//     visibility icon (no `bEnabled` — a hand-placed Manual layer has no "generation enabled"
//     concept, unlike Procedural's coupled E/D+V/I). Lock stays inert, same as props (STEP81
//     divergence 5).
//  3. Never notifies `Pipeline::PreviewDriver`: `recipe.markers`/`recipe.markerLayers` feed no
//     PROC stage (STEP60), same silent posture STEP49 already adopts for the manual roster.
#pragma once
#include <functional>
#include <string>
#include "ColorSwatch_UI.h"
#include "DraggableListWidget_UI.h"
#include "ManualInstanceLayerIndex_UI.h"
#include "MarkerSymmetryFixCommand_UI.h"
#include "Section_UI.h"
#include "SliderScalar_UI.h"
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
    RealtimeToggle groupColorToggle{true};

    // ONE shared `RealtimeToggle` instance reused across every row's own color/scale control,
    // header or body — `Params::MarkerInstanceLayer` cannot carry a `RealtimeToggle` member (a pure
    // round-tripping type). Pre-existing limitation (already true for multiple simultaneously
    // expanded row bodies since STEP110); STEP123 widens how often it's exercised (the header
    // control runs it every row, every frame), not the limitation itself.
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

    // STEP142 — double-click-the-header rename (mirrors MarkerLayerBundlesState's own STEP140/142
    // fields one tier up). -1 = no rename. A SCRATCH buffer, not `layer.name` directly — see
    // DrawLayerHeaderNameOverlay's own header comment (MarkersTab_ManualLayerRowBody_UI.h) for why.
    int         renamingLayerIndex = -1;
    std::string renameScratchText;
    // Human's own bug report — see MarkerLayerBundlesState::bRenameFocusPending's own comment
    // (MarkersTab_Bundles_UI.h) for the full "why"; this is the same mechanism, one tier down.
    bool        bRenameFocusPending = false;
};

// The layer the per-row controls edit, or null when the selection points at nothing
// (Constitution §6 — an index is validated, never trusted). Dead code (zero call sites) — kept here
// untouched per ARCH_19_22's explicit carve-out, not relocated to MarkersTab_ManualLayerHelpers_UI.h.
inline Params::MarkerInstanceLayer* SelectedManualMarkerLayer(
        std::vector<Params::MarkerInstanceLayer>& markerLayers, int selectedLayerIndex) {
    if (selectedLayerIndex < 0 || selectedLayerIndex >= static_cast<int>(markerLayers.size())) return nullptr;
    return &markerLayers[static_cast<std::size_t>(selectedLayerIndex)];
}

// MarkersTab_ManualLayers_UI.cpp:

// The block-wide settings: one shared tint for every layer, and the icon scale the whole layer
// stack draws at. STEP125: promoted out of the anonymous namespace (was DrawLayerSettings) and drawn
// exactly ONCE, before the Type-section loop, by DrawMarkerTypeSections — these are map-wide UI-only
// preview preferences with no Type/Bundle scope of their own.
void DrawManualMarkerLayerBlockSettings(ManualMarkerLayersState& state);

// STEP142/144 — [SYM][COL][swatch], right-aligned as a cluster so it sits flush against
// DraggableList's own built-in [o]/[L]/[X] strip with no dead gap (this ungrouped row draws no
// delete button of its own — that strip's "X##delete" already covers it). Promoted out of the
// anonymous namespace (was file-local) so a headless test can verify it lands flush against, not
// overlapping, the strip it was designed to sit beside — see MarkersTab_ManualLayers_UI_Test.cpp's
// RunUngroupedClusterDoesNotOverlapAffordanceStripCheck for the bug this closes (STEP145): using
// `ImGui::GetContentRegionAvail()` to size the right-align push reached all the way to the row's
// TRUE right edge, PAST the strip's own reserved space, so the push overshot and landed this
// cluster on top of the strip instead of beside it.
void DrawRightAlignedSymmetryColorOverrideCluster(Params::MarkerInstanceLayer& layer,
                                                  ManualMarkerLayersState& state, bool& bAnyCommitted);

// The layer stack. MUTATES NOTHING while drawing: the signal is applied after the list closes.
// STEP110: each row's body, whenever the row's own CollapsingHeader is open (never gated on
// `state.selectedLayerIndex`), draws that row's OWN settings below its header. `bAnyNameCommitted`
// is set true if any expanded row's name committed this frame, feeding the caller's uniqueness
// repair. STEP125: gains `markerTypeNameFilter` (ARCH §19.15(c), composed with the existing Bundle-
// membership suppression via IsMarkerInstanceLayerRowSuppressed).
// ARCH §19.25, item 5: `selectManualMarkerInstanceCallback` threads straight through to
// DrawLayerRowBody's own per-row instance-list click. Empty default — every existing call site
// compiles unchanged.
// STEP141: `selectedManualInstanceIdentifiers`/`anchorIdentifier` are the Ctrl/Shift multi-select
// set (MarkersTabState's own new fields), threaded through to DrawLayerRowBody's own instance rows.
DraggableListSignal DrawLayerList(std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                  std::vector<Params::MarkerInstanceGroup>& markers,
                                  const Params::Geometry& geometry, int globalSymmetryMask, int globalRadialRepeatCount,
                                  Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings,
                                  ManualMarkerLayersState& state, bool& bAnyNameCommitted,
                                  const ManualInstanceLayerIndex_UI& instanceIndex,
                                  int& selectedManualInstanceIdentifier,
                                  std::vector<int>& selectedManualInstanceIdentifiers, int& anchorIdentifier,
                                  const std::string& markerTypeNameFilter,
                                  const std::function<void(int)>& selectManualMarkerInstanceCallback = {});

// The Add Marker Layer button. STEP120: gains an optional Bundle-scoped parent so a Bundle node's
// own "add a Layer here" (MarkersTab_Bundles_UI.cpp) can reuse it; moved out of the anonymous
// namespace. `parentBundleIdentifierForNewLayer < 0` (default) is root scope — this file's own
// existing call site passes -1, unchanged behavior. STEP125: gains `markerTypeNameForNewLayer`
// (extends ARCH §19.15(a)'s Bundle "Add Group" pattern to this list's own "Add Layer" button — see
// STEP125_MarkersTabTypeSections_UI.md §4) so a layer added from inside a Type-section's own
// "Ungrouped Manual Marker Layers" sub-list mints already scoped to that section's type.
bool DrawLayerListButtons(std::vector<Params::MarkerInstanceLayer>& markerLayers, ManualMarkerLayersState& state,
                          int parentBundleIdentifierForNewLayer = -1,
                          const std::string& markerTypeNameForNewLayer = "");

// STEP125: replaces the retired DrawManualMarkerLayers — the per-Type-section-scoped job (list + Add
// button + repair, no Section wrap, no block settings: those two now live one call outside the
// Type-section loop, see MarkersTab_TypeSections_UI.cpp's own header comment). `markers` is
// `recipe.markers`, repaired here when a layer is deleted or reordered. No `Data::
// PlacementInstances*` parameter — see SCOPE NOTE 1. No `Pipeline::PreviewDriver*` parameter — see
// SCOPE NOTE 3. `selectedManualInstanceIdentifier` is STEP126's own single selection target
// (MarkersTab_UI.h), threaded through to DrawLayerList's own per-row instance-list click.
void DrawManualMarkerLayerListBody(ManualMarkerLayersState& state,
                                   std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                   std::vector<Params::MarkerInstanceGroup>& markers,
                                   const Params::Geometry& geometry, int globalSymmetryMask,
                                   int globalRadialRepeatCount,
                                   Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings,
                                   const std::string& markerTypeNameFilter,
                                   int& selectedManualInstanceIdentifier,
                                   std::vector<int>& selectedManualInstanceIdentifiers, int& anchorIdentifier,
                                   const std::function<void(int)>& selectManualMarkerInstanceCallback = {});

} // namespace Ui
} // namespace SanmapGen
