// MarkerLayerId_UI.h — new, single-purpose, mirrors PropsTab_Manual_UI.h's NextPropLayerId
// (STEP56 §2) one function early, since no MarkersTab_ManualLayers_UI.h host exists yet to hold it
// alongside a NextMarkerLayerName sibling. The future Phase 2 tab includes this header instead of
// duplicating the function — do not re-derive it inline there.
#pragma once
#include <algorithm>
#include <vector>
#include "../params/MarkerInstance_PARAMS.h"

namespace SanmapGen {
namespace Ui {

inline int NextMarkerLayerId(const std::vector<Params::MarkerInstanceLayer>& markerLayers) {
    int maximumId = -1;
    for (const Params::MarkerInstanceLayer& layer : markerLayers) maximumId = std::max(maximumId, layer.layerId);
    return maximumId + 1;
}

} // namespace Ui
} // namespace SanmapGen
