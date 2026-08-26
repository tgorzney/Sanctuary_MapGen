// ManualInstanceLayerIndex_UI.h — new, single-purpose (mirrors MarkerLayerId_UI.h's own "no host has
// room" shape): a per-frame index from layerIndex -> every (groupIndex, transformIndex) pair among
// recipe.markers whose own transform.layerIndex matches, consumed by DrawLayerRowBody's new per-Layer
// instance list (Open Q7, DESIGN_MarkerTypeSectionsAndInstanceSelection_R1.md — no dedicated
// ARCH_19_2x ruling exists for this UI shape; followed directly from the design doc's own text).
// Mirrors MarkerLayerBundleLeafIndex_UI/BuildMarkerLayerBundleLeafIndex (MarkersTab_Bundles_UI.h)
// one tier down — a (groupIndex, transformIndex) pair instead of a single leaf key.
#pragma once
#include <unordered_map>
#include <utility>
#include <vector>
#include "../params/MarkerInstance_PARAMS.h"

namespace SanmapGen {
namespace Ui {

struct ManualInstanceLayerIndex_UI {
    // (groupIndex, transformIndex) — into recipe.markers[groupIndex].transforms[transformIndex].
    std::unordered_map<int, std::vector<std::pair<int, int>>> instancesByLayerIndex;
};

inline ManualInstanceLayerIndex_UI BuildManualInstanceLayerIndex(
        const std::vector<Params::MarkerInstanceGroup>& markers) {
    ManualInstanceLayerIndex_UI index;
    for (int groupIndex = 0; groupIndex < static_cast<int>(markers.size()); ++groupIndex) {
        const std::vector<Params::MarkerTransform>& transforms =
            markers[static_cast<std::size_t>(groupIndex)].transforms;
        for (int transformIndex = 0; transformIndex < static_cast<int>(transforms.size()); ++transformIndex)
            index.instancesByLayerIndex[transforms[static_cast<std::size_t>(transformIndex)].layerIndex]
                .push_back({groupIndex, transformIndex});
    }
    return index;
}

} // namespace Ui
} // namespace SanmapGen
