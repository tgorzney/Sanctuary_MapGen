// MarkersTab_ManualLayerHelpers_UI.h — five small, pure, standalone helpers relocated out of
// MarkersTab_ManualLayers_UI.h (STEP125 file-size remediation, ARCH_19_22_ManualLayersHeaderSplit.md's
// FINAL combined ruling — delivered alongside the separately-ratified RowBody split,
// MarkersTab_ManualLayerRowBody_UI.h; neither split alone cleared the ARCH_01_05 150-line hard
// ceiling once this ticket's own additions to the parent header were counted), plus this ticket's own
// new IsMarkerInstanceLayerRowSuppressed (ARCH_19_15(c)). Share no state with ManualMarkerLayersState's
// own definition or the entry-point declarations that stay in the parent header — the same kind of
// "pure logic, own file" content RigidTransformPivot_MATH.h/MarkerLayerId_UI.h already isolate
// elsewhere in this codebase.
//
// `SelectedManualMarkerLayer` is NOT included here, per ARCH_19_22's explicit carve-out: it is dead
// code with zero call sites and stays in MarkersTab_ManualLayers_UI.h untouched, not silently
// relocated by this ticket.
#pragma once
#include <cmath>
#include <string>
#include <vector>
#include "MarkersTab_ManualLayers_UI.h"
#include "UniqueNameList_UI.h"
#include "../params/MarkerInstance_PARAMS.h"

namespace SanmapGen {
namespace Ui {

// True when `layerIndex` names a layer with bLocked set. Out-of-range (Constitution §6) resolves
// to false — an invalid layerIndex must never itself become a reason to refuse an edit; that is a
// distinct failure mode (see the existing layerIndex clamp-on-import, STEP60 §4) this gate does not
// participate in.
inline bool IsMarkerInstanceLayerLocked(const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                        int layerIndex) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(markerLayers.size())) return false;
    return markerLayers[static_cast<std::size_t>(layerIndex)].bLocked;
}

// STEP125, ARCH_19_15(c): "composing two independent boolean filters with `||` is ordinary predicate
// usage, not a contract violation. Signed off as legal." A layer bundled under a Group (Item 3's own
// Bundle-tree membership) or belonging to a DIFFERENT Type-section than the one currently drawing is
// suppressed from this "Ungrouped Manual Marker Layers" list — the same shape
// IsMarkerRuleLayerRowSuppressed uses one tier over (MarkersTab_RuleLayers_UI.h).
inline bool IsMarkerInstanceLayerRowSuppressed(const Params::MarkerInstanceLayer& layer,
                                               const std::string& markerTypeNameFilter) {
    return layer.parentBundleIdentifier != -1 || layer.markerTypeName != markerTypeNameFilter;
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

// The raw, already-valid mask/count for `layerIndex` — `layer.symmetry.bSymmetryUseGlobal` selects
// between the layer's own fields and the two global ones, per STEP68's own two-line ternary
// (`SymmetryOrbitQuery_PIPELINE.h`'s wrapper deliberately does not resolve this itself). An
// out-of-range `layerIndex` (Constitution §6) falls back to the global pair. ARCH §19.24: a
// `bSymmetryEnabled == false` layer forces the EFFECTIVE mask to `Params::SymmetryAxis::None`
// (radial repeat count 0) WITHOUT touching `layer.symmetry`'s own configured fields — the gate
// applies only here, at read time. Relocated from MarkerDragGesture_UI.h (ARCH §21.3) once that file
// shrank to just `MarkerDragTraits` — this is its natural sibling among the other pure per-layer
// helpers in this file, unchanged in every other respect.
inline void ResolveEffectiveMarkerSymmetry(const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                           int layerIndex, int globalSymmetryMask,
                                           int globalRadialRepeatCount, int& outMask,
                                           int& outRadialRepeatCount) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(markerLayers.size())) {
        outMask = globalSymmetryMask; outRadialRepeatCount = globalRadialRepeatCount; return;
    }
    const Params::MarkerInstanceLayer& layer = markerLayers[static_cast<std::size_t>(layerIndex)];
    if (!layer.bSymmetryEnabled) {
        outMask = Params::SymmetryAxis::None; outRadialRepeatCount = 0; return;
    }
    outMask = layer.symmetry.bSymmetryUseGlobal ? globalSymmetryMask : layer.symmetry.symmetryMask;
    outRadialRepeatCount = layer.symmetry.bSymmetryUseGlobal ? globalRadialRepeatCount
                                                              : layer.symmetry.radialSymmetryRepeatCount;
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

} // namespace Ui
} // namespace SanmapGen
