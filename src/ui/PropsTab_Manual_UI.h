// PropsTab_Manual_UI.h — the manual prop layers block of the Props tab.
// Layer: UI. Accuracy class: Visual/Exact. TAB_REBUILD_PLAN "§ Props · Manual prop layers";
// retyped onto the real `Params::PropInstanceLayer` by STEP22 (`ENTITY_AUTHORING_PARAMS_SPEC.md`,
// ARCH §12 — confirmed the same "authoring-convenience metadata" concept as the v1 UI-only
// `ManualPropGroup` it replaces, not a coincidental shape match).
//
// It edits `recipe.propLayers` (`Params::PropInstanceLayer`, the layer metadata array) and repairs
// `recipe.props` (`Params::PropInstanceGroup::transforms[].layerIndex`, the hand-placed/imported
// pass-through tree) when a layer is deleted or reordered — a genuinely different data source from
// the read-only transform list below, which still previews the PROCEDURAL `Data::PlacementInstances`
// buffer (SCOPE NOTE 2).
//
// SCOPE NOTES (ARCH §8.4 — a coder never invents a missing type; reported, not invented):
//  1. `Params::PropInstanceLayer` (Step 4/5, ENTITY_AUTHORING_PARAMS_SPEC / ARCH §12) is the durable
//     home the v1 manual GROUPS lacked — this block edits `recipe.propLayers` directly now, real
//     recipe content, not caller-owned UI presentation state (retiring the old "no home" framing,
//     STEP22). It still does NOT notify `Pipeline::PreviewDriver`: no stage hashes a layer's name,
//     color or icon scale, and `recipe.props`/`recipe.decals` — where `layerIndex` actually lives —
//     feed no pipeline stage either (PropInstance_PARAMS.h; unlike `UnitRule::armyIndex`, which the
//     Placement stage hashes), so asking for a regeneration a cosmetic tag cannot affect would be
//     the "cheap tweak triggers a full regen" defect. The repair functions below therefore stay
//     silent too, same posture, not a broadened one.
//  2. STEP22 ruling #2: the "transforms" list below is READ-ONLY and reads the Placement stage's
//     resolved prop buffer (`Data::PlacementInstances`) — a COMPLETELY SEPARATE data source from the
//     layer list above it. There is NO filter or parent-child relationship between the two, today or
//     after this retype: `Data::PlacementInstances` carries no `layerIndex`-equivalent field, so the
//     transform list cannot be scoped to a layer. It stays exactly as it always has — unfiltered,
//     previewing every procedurally-placed prop regardless of manual-layer membership — because a
//     tab that filtered or edited it would be the UI simulating (Constitution §1).
#pragma once
#include <algorithm>
#include <string>
#include <vector>
#include "ColorSwatch_UI.h"
#include "PropsTab_Bundles_UI.h"
#include "Section_UI.h"
#include "SliderScalar_UI.h"
#include "UniqueNameList_UI.h"
#include "../params/PropInstance_PARAMS.h"

namespace SanmapGen {
namespace Data { class PlacementInstances; }
namespace Ui {

// ARCH §20 — Props' two Type Sections. `PropInstanceLayer::propTypeName`/`PropLayerBundle::
// propTypeName` carry one of these two values; "Reclaim" has no other mechanism (not derived from
// `PropInstanceGroup::bReclaimable`, which stays permanently independent — see
// ScatterInstanceLayer_PARAMS.h's header note).
inline constexpr int kPropTypeSectionCount = 2;
inline constexpr const char* const kPropTypeSectionNames[kPropTypeSectionCount] = { "Prop", "Reclaim" };

struct ManualPropLayersState {
    SectionState       section;
    SectionState       transformListSection;
    ColorSwatchOptions previewColorOptions;                       // picker only, no RGBA fields
    ScalarSliderRange  iconScaleRange{ 0.1f, 10.0f, 0.0f };
    // ARCH §20 — grid-snap size range, sibling of iconScaleRange; mirrors
    // ManualMarkerLayersState::gridSnapSizeRange.
    ScalarSliderRange  gridSnapSizeRange{ 0.1f, 50.0f, 0.0f };

    bool           bUseGroupColor = false;                        // one tint for every layer
    float          groupColor[kColorSwatchChannelCount] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float          layerIconScale = 1.0f;
    RealtimeToggle groupColorToggle;
    RealtimeToggle layerIconScaleToggle;

    // ONE shared toggle set for the SELECTED/expanded row's own color/scale/grid-snap —
    // `Params::PropInstanceLayer` is a pure round-tripping type (ENTITY_AUTHORING_PARAMS_SPEC) and
    // cannot carry a `RealtimeToggle` member the way the retired `ManualPropGroup` did; same posture
    // `ArmiesTabState` already uses for `Params::Army`'s own fields (STEP22 ruling — mirrors
    // ArmiesTab_UI.h).
    RealtimeToggle selectedLayerColorToggle;
    RealtimeToggle selectedLayerIconScaleToggle;
    RealtimeToggle selectedLayerGridSnapToggle;   // ARCH §20

    int   selectedLayerIndex  = -1;
    float transformRowHeight  = 20.0f;    // the TRUE row height: the clipper scrolls with it
    float transformListHeight = 160.0f;

    // ARCH §20 — the Group/Bundle tree's own state, one instance per tab (mirrors
    // ManualMarkerLayersState carrying no separate bundle state of its own only because
    // MarkersTabState hosts it directly one tier up; Props has no such extra tier, so it lives here).
    // `selectedLayerIndex`/`bundles.selectedBundleIdentifier` are shared across both Type Sections
    // (matches Markers' own single shared selection, MarkersTabState) — only the collapsible header
    // open/closed state needs one slot PER section, hence the array below.
    PropLayerBundlesState bundles;
    SectionState typeSections[kPropTypeSectionCount];
};

// The layer the per-row controls edit, or null when the selection points at nothing
// (Constitution §6 — an index is validated, never trusted).
inline Params::PropInstanceLayer* SelectedManualPropLayer(std::vector<Params::PropInstanceLayer>& propLayers,
                                                           int selectedLayerIndex) {
    if (selectedLayerIndex < 0 || selectedLayerIndex >= static_cast<int>(propLayers.size())) return nullptr;
    return &propLayers[static_cast<std::size_t>(selectedLayerIndex)];
}

// True when `layerIndex` names a layer with bLocked set. Out-of-range (Constitution §6) resolves
// to false — an invalid layerIndex must never itself become a reason to refuse an edit; same
// out-of-range-safe posture as SelectedManualPropLayer immediately above and
// IsMarkerInstanceLayerLocked (STEP106).
inline bool IsPropInstanceLayerLocked(const std::vector<Params::PropInstanceLayer>& propLayers,
                                      int layerIndex) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(propLayers.size())) return false;
    return propLayers[static_cast<std::size_t>(layerIndex)].bLocked;
}

// ARCH §20 — a layer belonging to a Bundle (shown in the Group/Bundle tree instead) or to a
// DIFFERENT Type Section than the one currently drawing is suppressed from this "Ungrouped" flat
// list. Mirrors IsMarkerInstanceLayerRowSuppressed's exact compound shape (not an XOR — both
// conditions independently suppress).
inline bool IsPropInstanceLayerRowSuppressed(const Params::PropInstanceLayer& layer,
                                             const std::string& propTypeNameFilter) {
    return layer.parentBundleIdentifier != -1 || layer.propTypeName != propTypeNameFilter;
}

// The color a layer actually draws with: its own, unless the block is set to one shared tint.
inline const float* EffectiveManualPropLayerColor(const ManualPropLayersState& state,
                                                   const Params::PropInstanceLayer& layer) {
    return state.bUseGroupColor ? state.groupColor : layer.color;
}

// The label a layer row shows — never empty (Constitution §6).
inline const char* ManualPropLayerRowLabel(const Params::PropInstanceLayer& layer) {
    return layer.name.empty() ? "Prop Layer" : layer.name.c_str();
}

// The name "Add Prop Layer" seeds a fresh row with, before the shared uniqueness repair runs.
// Cosmetic only here (STEP22 ruling #6): `PropGroups` exports as a plain array, not a name-keyed
// dictionary, so duplicate names do not collide on export the way `Army`'s do. Reused for UX
// consistency with the Armies/Areas tabs, not because a collision would corrupt anything.
inline std::string NextPropLayerName(int layerCount) { return NextUniqueLabel("Prop Layer", layerCount); }

// The stable id a newly created layer takes: max-plus-one across the current in-memory
// `propLayers`, or 0 if empty. Ruling 1 (ARCH_14_13_OpenItems.md §14.13 item 3): derived, never a
// persisted counter — self-healing across manual JSON edits, ids already present in a loaded file
// are never renumbered.
inline int NextPropLayerId(const std::vector<Params::PropInstanceLayer>& propLayers) {
    int maximumId = -1;
    for (const Params::PropInstanceLayer& layer : propLayers) maximumId = std::max(maximumId, layer.layerId);
    return maximumId + 1;
}

// Repairs `recipe.props` after a layer row is removed: every transform that named the removed layer
// CLAMPS to layer 0 (STEP22 ruling #5 — DELIBERATE DIVERGENCE from `DropUnitRulesForRemovedArmy`: a
// prop losing its layer tag is still a real, still-rendered prop, so the instance is never dropped),
// exactly the clamp-to-0-on-out-of-range semantic ARCH §12 already ratified for import. Every
// transform above the removed layer shifts down one. Reports whether the recipe moved. Exact shape
// UI-Expert-provided.
inline bool ClampPropLayerIndicesForRemovedLayer(std::vector<Params::PropInstanceGroup>& props,
                                                  int removedLayerIndex) {
    if (removedLayerIndex < 0) return false;
    bool bRecipeMoved = false;
    for (auto& group : props)
        for (auto& transform : group.transforms) {
            if (transform.layerIndex == removedLayerIndex)      { transform.layerIndex = 0; bRecipeMoved = true; }
            else if (transform.layerIndex > removedLayerIndex)  { --transform.layerIndex;   bRecipeMoved = true; }
        }
    return bRecipeMoved;
}

// Keeps every transform's `layerIndex` correct after `recipe.propLayers` is reordered from source to
// target (the exact same erase-then-insert move `ApplyDraggableListSignal` performs on the layers
// vector itself) — the Reorder-signal counterpart to `ClampPropLayerIndicesForRemovedLayer`'s
// Delete-signal repair. Identical shape/math to `RenumberUnitRuleArmyIndicesForReorder` (Step 20),
// applied to `layerIndex`.
inline bool RenumberPropLayerIndicesForReorder(std::vector<Params::PropInstanceGroup>& props,
                                                int sourceLayerIndex, int targetLayerIndex,
                                                int layerCount) {
    if (sourceLayerIndex < 0 || sourceLayerIndex >= layerCount) return false;
    int clampedTarget = targetLayerIndex;
    if (clampedTarget < 0) clampedTarget = 0;
    if (clampedTarget > layerCount - 1) clampedTarget = layerCount - 1;
    if (clampedTarget == sourceLayerIndex) return false;
    bool bRecipeMoved = false;
    for (auto& group : props)
        for (auto& transform : group.transforms) {
            if (transform.layerIndex == sourceLayerIndex) { transform.layerIndex = clampedTarget; bRecipeMoved = true; }
            else if (sourceLayerIndex < clampedTarget && transform.layerIndex > sourceLayerIndex
                     && transform.layerIndex <= clampedTarget) { --transform.layerIndex; bRecipeMoved = true; }
            else if (sourceLayerIndex > clampedTarget && transform.layerIndex >= clampedTarget
                     && transform.layerIndex < sourceLayerIndex) { ++transform.layerIndex; bRecipeMoved = true; }
        }
    return bRecipeMoved;
}

// `props` is `recipe.props`, repaired here when a layer is deleted/reordered (SCOPE NOTE above).
// `propLayerBundles` is `recipe.propLayerBundles` (ARCH §20) — the Group/Bundle tree each Type
// Section hosts. `placedProps` is nullable: before the first generation there is no resolved buffer.
void DrawManualPropLayers(ManualPropLayersState& state, std::vector<Params::PropInstanceLayer>& propLayers,
                          std::vector<Params::PropInstanceGroup>& props,
                          std::vector<Params::PropLayerBundle>& propLayerBundles,
                          const Data::PlacementInstances* placedProps);

} // namespace Ui
} // namespace SanmapGen
