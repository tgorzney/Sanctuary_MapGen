// PropsTab_ManualLayerHelpers_UI.h — the Props-domain pure helpers ARCH §21.3's PropDragTraits
// needs, mirroring MarkersTab_ManualLayerHelpers_UI.h's own SelectedMarkerGroup/SelectedMarkerInstance/
// QuantizeMarkerPositionToLayerGrid/ResolveEffectiveMarkerSymmetry field-for-field. New file rather
// than added to PropsTab_Manual_UI.h (already 196 lines, over ARCH_01_05_FileSizeCeilings.md §1.5's
// 150-line hard ceiling under its own existing documented exception) — landing more content there
// would be a further silent ratchet; this is the same "pure logic, own file" move that header's own
// IsPropInstanceLayerLocked/IsPropInstanceLayerRowSuppressed already live behind, just one file over.
// Layer: UI. Pure, imgui-free, header-only, no state of its own.
#pragma once
#include <cmath>
#include <vector>
#include "CoordinateSpace_UI.h"
#include "../params/Geometry_PARAMS.h"
#include "../params/PropInstance_PARAMS.h"
#include "../params/ScatterInstanceLayer_PARAMS.h"
#include "../params/Symmetry_PARAMS.h"

namespace SanmapGen {
namespace Ui {

// Mirrors SelectedMarkerGroup/SelectedMarkerInstance exactly (MarkersTab_Manual_UI.h) — an
// out-of-range index (Constitution §6) resolves to null, never a trusted raw index.
inline Params::PropInstanceGroup* SelectedPropGroup(std::vector<Params::PropInstanceGroup>& props,
                                                     int selectedGroupIndex) {
    if (selectedGroupIndex < 0 || selectedGroupIndex >= static_cast<int>(props.size())) return nullptr;
    return &props[static_cast<std::size_t>(selectedGroupIndex)];
}
inline Params::PropTransform* SelectedPropInstance(std::vector<Params::PropTransform>& transforms,
                                                    int selectedInstanceIndex) {
    if (selectedInstanceIndex < 0 || selectedInstanceIndex >= static_cast<int>(transforms.size())) return nullptr;
    return &transforms[static_cast<std::size_t>(selectedInstanceIndex)];
}

// Mirrors QuantizeMarkerPositionToLayerGrid exactly (MarkersTab_ManualLayerHelpers_UI.h) — `geometry`
// scales the whole-number `gridSnapSizeCellMultiplier` into a world-unit cell size (Part 3), and the
// snap itself is CoordinateSpace_UI's WorldToGrid+GridToWorld, always landing on a cell center.
inline void QuantizePropPositionToLayerGrid(const std::vector<Params::PropInstanceLayer>& propLayers,
                                            int layerIndex, const Params::Geometry& geometry,
                                            float& worldX, float& worldZ) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(propLayers.size())) return;
    const Params::PropInstanceLayer& layer = propLayers[static_cast<std::size_t>(layerIndex)];
    if (!layer.bGridSnapEnabled || layer.gridSnapSizeCellMultiplier < 1) return;
    const float cellSizeWorldUnits = static_cast<float>(layer.gridSnapSizeCellMultiplier) * geometry.worldUnitsPerGenerationCell;
    const WorldPoint snapped =
        GridToWorld(cellSizeWorldUnits, WorldToGrid(cellSizeWorldUnits, WorldPoint{worldX, worldZ}));
    worldX = snapped.worldX;
    worldZ = snapped.worldZ;
}

// Mirrors ResolveEffectiveMarkerSymmetry exactly (MarkersTab_ManualLayerHelpers_UI.h), including
// ARCH §19.24's bSymmetryEnabled == false gate, extended to Props by ScatterInstanceLayer_PARAMS.h.
inline void ResolveEffectivePropSymmetry(const std::vector<Params::PropInstanceLayer>& propLayers,
                                         int layerIndex, int globalSymmetryMask,
                                         int globalRadialRepeatCount, int& outMask,
                                         int& outRadialRepeatCount) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(propLayers.size())) {
        outMask = globalSymmetryMask; outRadialRepeatCount = globalRadialRepeatCount; return;
    }
    const Params::PropInstanceLayer& layer = propLayers[static_cast<std::size_t>(layerIndex)];
    if (!layer.bSymmetryEnabled) {
        outMask = Params::SymmetryAxis::None; outRadialRepeatCount = 0; return;
    }
    outMask = layer.symmetry.bSymmetryUseGlobal ? globalSymmetryMask : layer.symmetry.symmetryMask;
    outRadialRepeatCount = layer.symmetry.bSymmetryUseGlobal ? globalRadialRepeatCount
                                                              : layer.symmetry.radialSymmetryRepeatCount;
}

} // namespace Ui
} // namespace SanmapGen
