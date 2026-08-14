// PlacementResults_DATA.h — everything the placement stage resolves, in one aggregate.
// Layer: DATA (computed output). Four instance buffers, one per entity family, so the
// gameplay-authoritative sets (markers, units, collidable props) stay separable from the
// decorative one (decals) — the authority split AI_HOSTCLIENT_SPEC / DETERMINISM_SPEC draw.
#pragma once
#include "PlacementInstances_DATA.h"

namespace SanmapGen {
namespace Data {

struct PlacementResults {
    PlacementInstances markers;   // spawns / alloys / expansions / generic areas
    PlacementInstances props;     // scattered props (collidable ones are gameplay)
    PlacementInstances units;     // pre-placed army units
    PlacementInstances decals;    // decorative only

    void Clear() { markers.Clear(); props.Clear(); units.Clear(); decals.Clear(); }

    std::size_t TotalCount() const {
        return markers.Count() + props.Count() + units.Count() + decals.Count();
    }
};

} // namespace Data
} // namespace SanmapGen
