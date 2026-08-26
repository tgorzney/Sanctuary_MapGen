// MarkersTab_UI.h — the marker tab: the global section, the procedural rule stack, the
// hand-authored manual roster, and the resolved placed-marker list. Layer: UI. Accuracy class:
// Visual. Edits `recipe.markerRuleLayers` (the procedural rules, STEP66/STEP80's two-level
// MarkerRuleLayer/MarkerRule shape) and `recipe.markers` (the manual roster, STEP49,
// MarkersTab_Manual_UI.h); reads `recipe.armies` for the manual roster's Spawn army picker.
// TAB_REBUILD_PLAN "§ Markers"; extended by tab-rebuild WO C4.
//
// The three shared list widgets each do the job they exist for: the procedural rules are a
// DraggableList (an ORDERED stack of tens of rows, every row a drop target), the placed markers
// are a VirtualList (tens of thousands of rows, O(visible) per frame), and the icons are an
// IconGrid. The tier of a committed edit is derived by Pipeline::PreviewDriver from the stage
// parameter hashes — this tab contains no flag mapping.
//
// ICON PICKER SCOPE (ARCH §8.4): the atlas manifest is OWNED BY THE APP SHELL (M5-7) — it holds
// the Io::AssetAtlasCache / Sys::AtlasResidency pair — and is passed in, nullable; a tab never
// invents an atlas source. The manifest carries `iconId` only, and nothing in the tree maps an
// icon id back to a game `tpId`, so the grid reports the SELECTION and the tpId itself is typed;
// wiring the two together needs the manifest to carry the tpId, which is a work-order this one
// does not own. This still describes `DrawPlacementTemplatePicker`'s rule/prop template pickers
// correctly (untouched by STEP121) — the Global section is the one exception, resolved via
// `IconAtlasPairingLookup` (STEP121, MarkersTab_Globals_UI.h).
//
// FURTHER SCOPE NOTES (ARCH §8.4 — reported, not invented):
//  1. `Params::MarkerRule` has NO `name` and NO `baseColor`, so the plan's per-rule Name TextInput
//     and Base Color ColorSwatch are not drawn; a rule row is labelled by its category and count.
//     Both fields need a PARAMS work-order.
//  2. Editable MANUAL markers are drawn by `MarkersTab_Manual_UI.h`'s `DrawManualMarkers` (STEP49)
//     — see that header, not this one, for the hand-authored roster's shape.
//  3. The global section's Scale/Color/Icon controls edit `recipe.globalMarkerSettings` directly
//     (STEP121); the gamedata root and the icon-scan request remain caller-owned UI state — see
//     `MarkersTab_Globals_UI.h`.
#pragma once
#include <functional>
#include "ConfirmDialog_UI.h"
#include "IconGridWidget_UI.h"
#include "LabelledDialWidget_UI.h"
#include "MarkersTab_Bundles_UI.h"
#include "MarkersTab_Globals_UI.h"
#include "MarkersTab_ManualLayers_UI.h"
#include "MarkersTab_Manual_UI.h"
#include "MarkersTab_Placed_UI.h"
#include "MarkersTab_RuleLayers_UI.h"
#include "MarkersTab_Rules_UI.h"
#include "MarkersTab_TypeSections_UI.h"
#include "RangeSliderWidget_UI.h"
#include "../params/MarkerRule_PARAMS.h"

namespace SanmapGen {
namespace Data { class PlacementInstances; }
namespace Params { struct MapRecipe; }
namespace Pipeline { class PreviewDriver; }
namespace Ui {

struct MarkersTabState {
    // Every limit and metric is a setting, never a literal at a use site (Constitution §8).
    float ruleRowHeight   = 22.0f;
    float ruleListHeight  = 140.0f;
    float iconGridHeight  = 180.0f;
    RangeSliderBounds slopeBounds{ 0.0f, 89.9f, 0.1f };
    RangeSliderBounds heightBounds{ 0.0f, 1.0f, 0.001f };
    DialRange densityRange{ 0.0f, 1.0f, 0.0f, 400.0f };
    // TAB_REBUILD_PLAN "§ Markers": Count 1-1000, Clearance Spacing 0-500. The minimum stays at
    // zero on both because a rule driven by density asks for no fixed count, and a rule with no
    // clearance is a legal rule; the plan's limits are the CEILINGS the tab must be able to reach.
    DialRange countRange{ 0.0f, 1000.0f, 1.0f, 400.0f };
    DialRange clearanceSpacingRange{ 0.0f, 500.0f, 0.0f, 600.0f };
    DialRange obstacleDistanceRange{ 0.0f, 128.0f, 0.0f, 600.0f };

    RealtimeToggle slopeToggle{true};
    RealtimeToggle heightToggle{true};
    RealtimeToggle densityToggle{true};
    RealtimeToggle countToggle{true};
    RealtimeToggle clearanceSpacingToggle{true};
    RealtimeToggle obstacleDistanceToggle{true};

    IconGridState iconGridState;
    // STEP80: the two-level selection — which MarkerRuleLayer, then which MarkerRule inside it.
    // `selectedRuleIndex` is the SAME field the pre-STEP80 flat stack used; it now means "within
    // the selected layer" rather than "within the flat vector."
    int   selectedRuleLayerIndex = 0;
    int   selectedRuleIndex      = 0;
    // The non-empty-layer Delete confirm's pending target (STEP80 §4): -1 when no delete is
    // pending. Re-validated against the vector's current size before it is ever applied.
    int   pendingDeleteRuleLayerIndex = -1;
    ConfirmDialogState deleteRuleLayerConfirmState;
    float countValue        = 4.0f;        // int mirror
    RangeSliderValues slopeValues{ 0.0f, 89.9f };
    RangeSliderValues heightValues{ 0.0f, 1.0f };

    // WO C4 additions: the sections the rest of one MarkerRule is drawn in, and the two blocks
    // shared with every other placement tab (PlacementRuleSections_UI.h).
    MarkersTabGlobals       globals;
    MarkerRuleDetailState   ruleDetail;
    PlacementGateState      gate;
    PlacementTransformState transform;
    MarkersPlacedListState  placedList;
    SectionState            ruleStackSection;

    // STEP49: the hand-authored roster editor, a sibling block to the procedural stack and the
    // read-only placed list above (MarkersTab_Manual_UI.h).
    ManualMarkersState      manual;

    // STEP81: the Manual Marker Layers block, authored before STEP49's manual roster in
    // `DrawMarkersTab` so a layer added this frame is pickable by part (b)'s Layer combo on the
    // same frame (MarkersTab_ManualLayers_UI.h).
    ManualMarkerLayersState manualLayers;

    // STEP120: the Group/Bundle tree state (expand/select), shared across every Type-section's own
    // filtered Render call (MarkersTab_Bundles_UI.h) — a bundle's own expand-state and "is this the
    // selected bundle" bit are tab-wide concepts, not per-section ones.
    MarkerLayerBundlesState bundles;

    // STEP125: the dynamic Type-section outer loop's own per-type collapse state, keyed by
    // markerTypeName (MarkersTab_TypeSections_UI.h). `bundles` above still holds the ONE shared
    // Bundle-tree state (expand/select), reused across every Type-section's own filtered Render call.
    MarkerTypeSectionsState typeSections;

    // STEP126 — the SINGLE selection target for the per-Layer instance-list click (Open Q7) and the
    // MapCanvas static highlight (ARCH §19.19/§19.20, manual-only). Lives at the TOP level, not
    // inside ManualMarkerLayersState/ManualMarkersState, because it must stay visible regardless of
    // which Layer's own row body happens to be expanded when the click occurs — same reasoning
    // MarkerLayerBundlesState::selectedBundleIdentifier already applies one tier up its own struct.
    // -1 = no selection (Constitution §6 sentinel convention).
    int selectedManualInstanceIdentifier = -1;
};

// rule -> widget mirrors (the paired min/max fields the range sliders edit, and the int count).
inline void LoadMarkerRuleValues(const Params::MarkerRule& rule, MarkersTabState& state) {
    state.countValue = static_cast<float>(rule.count);
    state.slopeValues.minimumValue  = rule.minSlope;
    state.slopeValues.maximumValue  = rule.maxSlope;
    state.heightValues.minimumValue = rule.minHeight;
    state.heightValues.maximumValue = rule.maxHeight;
}

// widget mirrors -> rule. Reports whether the recipe actually moved.
inline bool StoreMarkerRuleValues(const MarkersTabState& state, Params::MarkerRule& rule) {
    const int count = static_cast<int>(ClampDialValue(state.countValue, state.countRange) + 0.5f);
    const bool bMoved = count != rule.count
                     || state.slopeValues.minimumValue  != rule.minSlope
                     || state.slopeValues.maximumValue  != rule.maxSlope
                     || state.heightValues.minimumValue != rule.minHeight
                     || state.heightValues.maximumValue != rule.maxHeight;
    rule.count     = count;
    rule.minSlope  = state.slopeValues.minimumValue;
    rule.maxSlope  = state.slopeValues.maximumValue;
    rule.minHeight = state.heightValues.minimumValue;
    rule.maxHeight = state.heightValues.maximumValue;
    return bMoved;
}

// The rule the detail controls edit, or null when either index misses: an out-of-range layer, or
// an in-range layer whose rule index misses (STEP80's two-index walk, mirroring `SelectedLayer`,
// LayersTab_UI.cpp:120-127).
Params::MarkerRule* SelectedMarkerRule(std::vector<Params::MarkerRuleLayer>& markerRuleLayers,
                                       const MarkersTabState& state);

// `iconManifest`/`pairingLookup`/`placedMarkers` are all nullable: with no resident atlas the
// picker degrades to the typed tpId (or, for the Global section, a disabled placeholder button),
// and before the first generation the placed list simply says so. ARCH §19.25, item 5:
// `selectManualMarkerInstanceCallback` is Application's own shell-mediated closure (empty default —
// every existing test/call site compiles unchanged), threaded straight through to
// DrawMarkerTypeSections, riding the SAME chain previewDriver/iconManifest already ride down.
// STEP132 (ARCH §19.27): `selectProceduralMarkerInstanceCallback` is that closure's procedural
// sibling, riding the exact same chain one parameter further along.
void DrawMarkersTab(Params::MapRecipe& recipe, MarkersTabState& state,
                    Pipeline::PreviewDriver* previewDriver,
                    const IconAtlasManifest* iconManifest = nullptr,
                    const IconAtlasPairingLookup* pairingLookup = nullptr,
                    const Data::PlacementInstances* placedMarkers = nullptr,
                    const std::function<void(int)>& selectManualMarkerInstanceCallback = {},
                    const std::function<void(int)>& selectProceduralMarkerInstanceCallback = {});

} // namespace Ui
} // namespace SanmapGen
