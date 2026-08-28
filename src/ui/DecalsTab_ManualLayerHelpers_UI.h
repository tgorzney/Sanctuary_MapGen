// DecalsTab_ManualLayerHelpers_UI.h — the Decals-domain pure helpers ARCH §21.3's DecalDragTraits
// needs, mirroring MarkersTab_ManualLayerHelpers_UI.h's own SelectedMarkerGroup/SelectedMarkerInstance/
// QuantizeMarkerPositionToLayerGrid/ResolveEffectiveMarkerSymmetry field-for-field (Decal-typed
// mirror of PropsTab_ManualLayerHelpers_UI.h, one tier over). New file rather than added to
// DecalsTab_Manual_UI.h (already 163 lines, close to ARCH_01_05_FileSizeCeilings.md §1.5's 150-line
// hard ceiling) for the same reason. Layer: UI. Pure, imgui-free, header-only, no state of its own.
#pragma once
#include <cmath>
#include <vector>
#include "../params/PropInstance_PARAMS.h"
#include "../params/ScatterInstanceLayer_PARAMS.h"
#include "../params/Symmetry_PARAMS.h"

namespace SanmapGen {
namespace Ui {

inline Params::DecalInstanceGroup* SelectedDecalGroup(std::vector<Params::DecalInstanceGroup>& decals,
                                                       int selectedGroupIndex) {
    if (selectedGroupIndex < 0 || selectedGroupIndex >= static_cast<int>(decals.size())) return nullptr;
    return &decals[static_cast<std::size_t>(selectedGroupIndex)];
}
inline Params::DecalTransform* SelectedDecalInstance(std::vector<Params::DecalTransform>& transforms,
                                                      int selectedInstanceIndex) {
    if (selectedInstanceIndex < 0 || selectedInstanceIndex >= static_cast<int>(transforms.size())) return nullptr;
    return &transforms[static_cast<std::size_t>(selectedInstanceIndex)];
}

inline void QuantizeDecalPositionToLayerGrid(const std::vector<Params::DecalInstanceLayer>& decalLayers,
                                             int layerIndex, float& worldX, float& worldZ) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(decalLayers.size())) return;
    const Params::DecalInstanceLayer& layer = decalLayers[static_cast<std::size_t>(layerIndex)];
    if (!layer.bGridSnapEnabled || layer.gridSnapSizeWorldUnits <= 0.0f) return;
    const float cellSize = layer.gridSnapSizeWorldUnits;
    worldX = std::round(worldX / cellSize) * cellSize;
    worldZ = std::round(worldZ / cellSize) * cellSize;
}

inline void ResolveEffectiveDecalSymmetry(const std::vector<Params::DecalInstanceLayer>& decalLayers,
                                          int layerIndex, int globalSymmetryMask,
                                          int globalRadialRepeatCount, int& outMask,
                                          int& outRadialRepeatCount) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(decalLayers.size())) {
        outMask = globalSymmetryMask; outRadialRepeatCount = globalRadialRepeatCount; return;
    }
    const Params::DecalInstanceLayer& layer = decalLayers[static_cast<std::size_t>(layerIndex)];
    if (!layer.bSymmetryEnabled) {
        outMask = Params::SymmetryAxis::None; outRadialRepeatCount = 0; return;
    }
    outMask = layer.symmetry.bSymmetryUseGlobal ? globalSymmetryMask : layer.symmetry.symmetryMask;
    outRadialRepeatCount = layer.symmetry.bSymmetryUseGlobal ? globalRadialRepeatCount
                                                              : layer.symmetry.radialSymmetryRepeatCount;
}

} // namespace Ui
} // namespace SanmapGen
