// MarkerInstanceId_UI.h — mirrors MarkerLayerId_UI.h's own single-purpose-file precedent one tier
// down (ARCH §19.16). MarkersTab_Manual_UI.h — the natural host, alongside NextMarkerInstanceName —
// is already 184 lines, over ARCH §1.5's 150-line hard ceiling, unremediated, predating this ticket;
// landing this helper there inline (NextMarkerLayerBundleId's own precedent) would be a further
// silent ratchet. NextMarkerLayerId's dedicated-file precedent is the one that actually applies here:
// that precedent is conditioned on the host having headroom (MarkersTab_Bundles_UI.h had 120 lines
// when STEP120 placed NextMarkerLayerBundleId there inline); MarkersTab_Manual_UI.h does not.
#pragma once
#include <algorithm>
#include <vector>
#include "../params/MarkerInstance_PARAMS.h"

namespace SanmapGen {
namespace Ui {

// ARCH §19.16 — scans max(instanceIdentifier) + 1 across EVERY group's transforms (roster-wide, not
// per-group) — same shape as NextMarkerLayerId, one tier down (two-level walk: group, then transform).
inline int NextMarkerInstanceIdentifier(const std::vector<Params::MarkerInstanceGroup>& markers) {
    int maximumId = -1;
    for (const Params::MarkerInstanceGroup& group : markers)
        for (const Params::MarkerTransform& transform : group.transforms)
            maximumId = std::max(maximumId, transform.instanceIdentifier);
    return maximumId + 1;
}

} // namespace Ui
} // namespace SanmapGen
