// MarkersTab_ManualLayerHelpers_UI.h — small, pure, standalone Manual-Marker-Layer helpers
// (relocated out of MarkersTab_ManualLayers_UI.h, STEP125/ARCH_19_22). `SelectedManualMarkerLayer`
// stays out per ARCH_19_22's own carve-out (dead code). STEP241's newer §19.31 resolvers (Name,
// bHidden, iconScale, grid-snap, symmetry) live in the sibling MarkersTab_MarkerLinkResolvers_UI.h;
// `EffectiveManualMarkerLayerColorOverrideEnabled`/`Color` (STEP239) stay below, unmoved.
//
// STEP246, ARCH §19.33/§21.9: Quantize/ResolveEffectiveMarkerSymmetry WIDENED in place (transform +
// links, not a bare layerIndex; also closes a latent gap — neither ever consulted `links` before).
// `EffectiveManualMarkerInstanceLocked`/`IsMarkerInstanceLocked` (governed field #6, bLocked) land
// here, not the new sibling file, per §21.9 — this file already owns their 2-parameter sibling.
#pragma once
#include <cmath>
#include <string>
#include <vector>
#include "MarkersTab_ManualLayers_UI.h"
#include "MarkersTab_MarkerLinkResolvers_UI.h"
#include "UniqueNameList_UI.h"
#include "../params/MarkerInstance_PARAMS.h"
#include "../params/MarkerLink_PARAMS.h"

namespace SanmapGen {
namespace Ui {

// True when `layerIndex` names a layer with bLocked set. Out-of-range (Constitution §6) resolves
// to false — an invalid layerIndex must never itself become a reason to refuse an edit.
inline bool IsMarkerInstanceLayerLocked(const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                        int layerIndex) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(markerLayers.size())) return false;
    return markerLayers[static_cast<std::size_t>(layerIndex)].bLocked;
}

// STEP125, ARCH_19_15(c): a layer bundled under a Group, or belonging to a DIFFERENT Type-section
// than the one currently drawing, is suppressed from this "Ungrouped Manual Marker Layers" list.
inline bool IsMarkerInstanceLayerRowSuppressed(const Params::MarkerInstanceLayer& layer,
                                               const std::string& markerTypeNameFilter) {
    return layer.parentBundleIdentifier != -1 || layer.markerTypeName != markerTypeNameFilter;
}

// The world position `(worldX, worldZ)` quantized to `transform`'s own EFFECTIVE grid setting;
// unchanged if that resolves to snap-off, `transform.layerIndex` is out of range (Constitution §6),
// or the resolved cell size is non-positive (treated as snap-off, not a divide-by-zero hazard).
// ARCH §19.33/§21.9 order: (1) `transform.linkIdentifier` resolves -> that Link's grid-snap pair;
// (2) else the Layer-tier resolvers just above (already Link-aware) -> their result; a dangling
// identifier at either tier soft-degrades to the next step, never a refusal.
inline void QuantizeMarkerPositionToLayerGrid(const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                              const Params::MarkerTransform& transform,
                                              const std::vector<Params::MarkerLink>& links,
                                              float& worldX, float& worldZ) {
    const int layerIndex = transform.layerIndex;
    if (layerIndex < 0 || layerIndex >= static_cast<int>(markerLayers.size())) return;
    const Params::MarkerInstanceLayer& layer = markerLayers[static_cast<std::size_t>(layerIndex)];
    bool bEnabled = EffectiveManualMarkerLayerGridSnapEnabled(layer, links);
    float cellSize = EffectiveManualMarkerLayerGridSnapSizeWorldUnits(layer, links);
    if (transform.linkIdentifier >= 0)
        for (const Params::MarkerLink& link : links)
            if (link.identifier == transform.linkIdentifier) {
                bEnabled = link.bGridSnapEnabled; cellSize = link.gridSnapSizeWorldUnits; break;
            }
    if (!bEnabled || cellSize <= 0.0f) return;
    worldX = std::round(worldX / cellSize) * cellSize;
    worldZ = std::round(worldZ / cellSize) * cellSize;
}

// The EFFECTIVE mask/count for `transform` — `symmetry->bSymmetryUseGlobal` selects between the
// resolved setting's own fields and the two global ones (STEP68). Out-of-range `transform.layerIndex`
// (Constitution §6) falls back to the global pair. ARCH §19.24: a resolved `bSymmetryEnabled ==
// false` forces the mask to `Params::SymmetryAxis::None` (count 0) without touching the resolved
// setting's own fields. Same ARCH §19.33/§21.9 resolution order as Quantize just above.
inline void ResolveEffectiveMarkerSymmetry(const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                           const Params::MarkerTransform& transform,
                                           const std::vector<Params::MarkerLink>& links,
                                           int globalSymmetryMask, int globalRadialRepeatCount,
                                           int& outMask, int& outRadialRepeatCount) {
    const int layerIndex = transform.layerIndex;
    if (layerIndex < 0 || layerIndex >= static_cast<int>(markerLayers.size())) {
        outMask = globalSymmetryMask; outRadialRepeatCount = globalRadialRepeatCount; return;
    }
    const Params::MarkerInstanceLayer& layer = markerLayers[static_cast<std::size_t>(layerIndex)];
    bool bEnabled = EffectiveManualMarkerLayerSymmetryEnabled(layer, links);
    const Params::SymmetrySetting* symmetry = &EffectiveManualMarkerLayerSymmetry(layer, links);
    if (transform.linkIdentifier >= 0)
        for (const Params::MarkerLink& link : links)
            if (link.identifier == transform.linkIdentifier) {
                bEnabled = link.bSymmetryEnabled; symmetry = &link.symmetry; break;
            }
    if (!bEnabled) { outMask = Params::SymmetryAxis::None; outRadialRepeatCount = 0; return; }
    outMask = symmetry->bSymmetryUseGlobal ? globalSymmetryMask : symmetry->symmetryMask;
    outRadialRepeatCount = symmetry->bSymmetryUseGlobal ? globalRadialRepeatCount
                                                         : symmetry->radialSymmetryRepeatCount;
}

// The sixth §19.33 governed-field resolver — same shape/order as the two widened functions above.
inline bool EffectiveManualMarkerInstanceLocked(const Params::MarkerTransform& transform,
                                                const Params::MarkerInstanceLayer& layer,
                                                const std::vector<Params::MarkerLink>& links) {
    if (transform.linkIdentifier >= 0)
        for (const Params::MarkerLink& link : links)
            if (link.identifier == transform.linkIdentifier) return link.bLocked;
    return EffectiveManualMarkerLayerLocked(layer, links);
}

// Out-of-range-safe wrapper mirroring IsMarkerInstanceLayerLocked's own convention — what a per-
// instance lock gate calls. `IsMarkerInstanceLayerLocked` above stays UNCHANGED, still correct for
// any layerIndex-only site (a Layer-row header lock toggle).
inline bool IsMarkerInstanceLocked(const Params::MarkerTransform& transform,
                                   const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                   const std::vector<Params::MarkerLink>& links) {
    const int layerIndex = transform.layerIndex;
    if (layerIndex < 0 || layerIndex >= static_cast<int>(markerLayers.size())) return false;
    return EffectiveManualMarkerInstanceLocked(transform, markerLayers[static_cast<std::size_t>(layerIndex)], links);
}

// The color a layer actually draws with: its own, unless the block is set to one shared tint.
inline const float* EffectiveManualMarkerLayerColor(const ManualMarkerLayersState& state,
                                                     const Params::MarkerInstanceLayer& layer) {
    return state.bUseGroupColor ? state.groupColor : layer.color;
}

// STEP239, ARCH §19.31 Mechanism A (read-and-resolve, never write-through): a Link-bound Layer's
// color-override enabled/color mirror the LINK's own fields; unbound/dangling falls back to its own
// field. NOT a rename of the `(state, layer)` overload above — that stays for `bUseGroupColor`.
inline bool EffectiveManualMarkerLayerColorOverrideEnabled(const Params::MarkerInstanceLayer& layer,
                                                           const std::vector<Params::MarkerLink>& links) {
    if (layer.linkIdentifier >= 0)
        for (const Params::MarkerLink& link : links)
            if (link.identifier == layer.linkIdentifier) return link.bColorOverrideEnabled;
    return layer.bColorOverrideEnabled;
}

inline const float* EffectiveManualMarkerLayerColor(const Params::MarkerInstanceLayer& layer,
                                                     const std::vector<Params::MarkerLink>& links) {
    if (layer.linkIdentifier >= 0)
        for (const Params::MarkerLink& link : links)
            if (link.identifier == layer.linkIdentifier) return link.color;
    return layer.color;
}

// The label a layer row shows — never empty (Constitution §6). Reused by part (b)'s Layer picker
// so an unnamed layer never renders as a blank, unpickable row.
inline const char* ManualMarkerLayerRowLabel(const Params::MarkerInstanceLayer& layer) {
    return layer.name.empty() ? "Marker Layer" : layer.name.c_str();
}

// The name "Add Marker Layer" seeds a fresh row with, before the shared uniqueness repair runs.
inline std::string NextMarkerLayerName(int layerCount) { return NextUniqueLabel("Marker Layer", layerCount); }

} // namespace Ui
} // namespace SanmapGen
