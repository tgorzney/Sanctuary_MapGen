// DecalsTab_Manual_UI.h — the manual decal layers block, part of the standalone Decals tab
// (ARCH §20; STEP22's original "no Decals tab" comment cited a ruling that was never actually
// ratified into the ARCH pack — see ARCH_20_DecalsTopLevelTab.md). Layer: UI. Accuracy class:
// Visual/Exact. STEP22 exact mirror of PropsTab_Manual_UI.h for
// `Params::DecalInstanceLayer`/`recipe.decalLayers`/`recipe.decals`/`placedDecals`, now drawn by
// `DecalsTab_UI.h`'s `DrawDecalsTab` rather than joining the Props tab as a sub-block.
//
// It edits `recipe.decalLayers` (`Params::DecalInstanceLayer`, the layer metadata array) and
// repairs `recipe.decals` (`Params::DecalInstanceGroup::transforms[].layerIndex`, the hand-placed/
// imported pass-through tree) when a layer is deleted or reordered — a genuinely different data
// source from the read-only transform list below, which still previews the PROCEDURAL
// `Data::PlacementInstances` buffer (SCOPE NOTE 2, mirrors PropsTab_Manual_UI.h).
//
// SCOPE NOTES (ARCH §8.4 — a coder never invents a missing type; reported, not invented):
//  1. Same posture as `PropsTab_Manual_UI.h` SCOPE NOTE 1: `Params::DecalInstanceLayer` is real
//     recipe content now, but this block does NOT notify `Pipeline::PreviewDriver` — no stage
//     hashes a layer's name, color or icon scale, and `recipe.decals` (where `layerIndex` lives)
//     feeds no pipeline stage either.
//  2. Same posture as `PropsTab_Manual_UI.h` SCOPE NOTE 2: the "transforms" list below is
//     READ-ONLY and reads the Placement stage's resolved decal buffer (`Data::PlacementInstances`)
//     — unfiltered, unrelated to manual-layer membership, because no `layerIndex`-equivalent field
//     exists on it to filter by.
#pragma once
#include <algorithm>
#include <string>
#include <vector>
#include "ColorSwatch_UI.h"
#include "DecalsTab_Bundles_UI.h"
#include "Section_UI.h"
#include "SliderScalar_UI.h"
#include "UniqueNameList_UI.h"
#include "../params/PropInstance_PARAMS.h"

namespace SanmapGen {
namespace Data { class PlacementInstances; }
namespace Ui {

struct ManualDecalLayersState {
    SectionState       section;
    SectionState       transformListSection;
    ColorSwatchOptions previewColorOptions;                       // picker only, no RGBA fields
    ScalarSliderRange  iconScaleRange{ 0.1f, 10.0f, 0.0f };
    // ARCH §20 — grid-snap size range, sibling of iconScaleRange; mirrors ManualPropLayersState.
    ScalarSliderRange  gridSnapSizeRange{ 0.1f, 50.0f, 0.0f };

    bool           bUseGroupColor = false;                        // one tint for every layer
    float          groupColor[kColorSwatchChannelCount] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float          layerIconScale = 1.0f;
    RealtimeToggle groupColorToggle;
    RealtimeToggle layerIconScaleToggle;

    // ONE shared toggle set for the SELECTED/expanded row's own color/scale/grid-snap —
    // `Params::DecalInstanceLayer` is a pure round-tripping type and cannot carry a
    // `RealtimeToggle` member of its own; mirrors `ManualPropLayersState`.
    RealtimeToggle selectedLayerColorToggle;
    RealtimeToggle selectedLayerIconScaleToggle;
    RealtimeToggle selectedLayerGridSnapToggle;   // ARCH §20

    int   selectedLayerIndex  = -1;
    float transformRowHeight  = 20.0f;    // the TRUE row height: the clipper scrolls with it
    float transformListHeight = 160.0f;

    // ARCH §20 — the Group/Bundle tree's own state. Decals has exactly one implicit Type Section
    // (no type-tag field, see ScatterInstanceLayer_PARAMS.h's header note), so there is only ever
    // one tree, unlike Props' per-type array.
    DecalLayerBundlesState bundles;
};

// The layer the per-row controls edit, or null when the selection points at nothing
// (Constitution §6 — an index is validated, never trusted).
inline Params::DecalInstanceLayer* SelectedManualDecalLayer(std::vector<Params::DecalInstanceLayer>& decalLayers,
                                                             int selectedLayerIndex) {
    if (selectedLayerIndex < 0 || selectedLayerIndex >= static_cast<int>(decalLayers.size())) return nullptr;
    return &decalLayers[static_cast<std::size_t>(selectedLayerIndex)];
}

// Decal-typed mirror of IsPropInstanceLayerLocked.
inline bool IsDecalInstanceLayerLocked(const std::vector<Params::DecalInstanceLayer>& decalLayers,
                                       int layerIndex) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(decalLayers.size())) return false;
    return decalLayers[static_cast<std::size_t>(layerIndex)].bLocked;
}

// ARCH §20 — a layer belonging to a Bundle (shown in the Group/Bundle tree instead) is suppressed
// from the "Ungrouped" flat list. No type check (unlike IsPropInstanceLayerRowSuppressed): Decals
// has no type-tag field at all — exactly one implicit Type Section.
inline bool IsDecalInstanceLayerRowSuppressed(const Params::DecalInstanceLayer& layer) {
    return layer.parentBundleIdentifier != -1;
}

// The color a layer actually draws with: its own, unless the block is set to one shared tint.
inline const float* EffectiveManualDecalLayerColor(const ManualDecalLayersState& state,
                                                    const Params::DecalInstanceLayer& layer) {
    return state.bUseGroupColor ? state.groupColor : layer.color;
}

// The label a layer row shows — never empty (Constitution §6).
inline const char* ManualDecalLayerRowLabel(const Params::DecalInstanceLayer& layer) {
    return layer.name.empty() ? "Decal Layer" : layer.name.c_str();
}

// The name "Add Decal Layer" seeds a fresh row with, before the shared uniqueness repair runs.
// Cosmetic only here (STEP22 ruling #6): `DecalGroups` exports as a plain array, not a name-keyed
// dictionary, so duplicate names do not collide on export.
inline std::string NextDecalLayerName(int layerCount) { return NextUniqueLabel("Decal Layer", layerCount); }

// The stable id a newly created layer takes: max-plus-one across the current in-memory
// `decalLayers`, or 0 if empty. Decal-typed mirror of `NextPropLayerId`.
inline int NextDecalLayerId(const std::vector<Params::DecalInstanceLayer>& decalLayers) {
    int maximumId = -1;
    for (const Params::DecalInstanceLayer& layer : decalLayers) maximumId = std::max(maximumId, layer.layerId);
    return maximumId + 1;
}

// Repairs `recipe.decals` after a layer row is removed: every transform that named the removed
// layer CLAMPS to layer 0 (STEP22 ruling #5 — never dropped: a decal losing its layer tag is still
// a real, still-rendered decal), exactly the clamp-to-0-on-out-of-range semantic ARCH §12 already
// ratified for import. Every transform above the removed layer shifts down one. Reports whether the
// recipe moved. Decal-typed mirror of `ClampPropLayerIndicesForRemovedLayer`.
inline bool ClampDecalLayerIndicesForRemovedLayer(std::vector<Params::DecalInstanceGroup>& decals,
                                                   int removedLayerIndex) {
    if (removedLayerIndex < 0) return false;
    bool bRecipeMoved = false;
    for (auto& group : decals)
        for (auto& transform : group.transforms) {
            if (transform.layerIndex == removedLayerIndex)      { transform.layerIndex = 0; bRecipeMoved = true; }
            else if (transform.layerIndex > removedLayerIndex)  { --transform.layerIndex;   bRecipeMoved = true; }
        }
    return bRecipeMoved;
}

// Keeps every transform's `layerIndex` correct after `recipe.decalLayers` is reordered from source
// to target. Decal-typed mirror of `RenumberPropLayerIndicesForReorder`.
inline bool RenumberDecalLayerIndicesForReorder(std::vector<Params::DecalInstanceGroup>& decals,
                                                 int sourceLayerIndex, int targetLayerIndex,
                                                 int layerCount) {
    if (sourceLayerIndex < 0 || sourceLayerIndex >= layerCount) return false;
    int clampedTarget = targetLayerIndex;
    if (clampedTarget < 0) clampedTarget = 0;
    if (clampedTarget > layerCount - 1) clampedTarget = layerCount - 1;
    if (clampedTarget == sourceLayerIndex) return false;
    bool bRecipeMoved = false;
    for (auto& group : decals)
        for (auto& transform : group.transforms) {
            if (transform.layerIndex == sourceLayerIndex) { transform.layerIndex = clampedTarget; bRecipeMoved = true; }
            else if (sourceLayerIndex < clampedTarget && transform.layerIndex > sourceLayerIndex
                     && transform.layerIndex <= clampedTarget) { --transform.layerIndex; bRecipeMoved = true; }
            else if (sourceLayerIndex > clampedTarget && transform.layerIndex >= clampedTarget
                     && transform.layerIndex < sourceLayerIndex) { ++transform.layerIndex; bRecipeMoved = true; }
        }
    return bRecipeMoved;
}

// `decals` is `recipe.decals`, repaired here when a layer is deleted/reordered (SCOPE NOTE above).
// `decalLayerBundles` is `recipe.decalLayerBundles` (ARCH §20) — the Group/Bundle tree this block
// now hosts. `placedDecals` is nullable: before the first generation there is no resolved buffer.
void DrawManualDecalLayers(ManualDecalLayersState& state, std::vector<Params::DecalInstanceLayer>& decalLayers,
                           std::vector<Params::DecalInstanceGroup>& decals,
                           std::vector<Params::DecalLayerBundle>& decalLayerBundles,
                           const Data::PlacementInstances* placedDecals);

} // namespace Ui
} // namespace SanmapGen
