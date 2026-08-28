// SpatialGridSet_DATA.h — one Data::SpatialGrid per Data::PlacementResults collection, same 4-way
// split (markers/props/units/decals) as RuleBucketIndexSet_DATA.h, a byte-identical structural
// mirror of it (ARCH §21.6). Layer: DATA (computed output over Data::PlacementResults).
#pragma once
#include "SpatialGrid_DATA.h"

namespace SanmapGen {
namespace Data {

struct SpatialGridSet {
    SpatialGrid markers, props, units, decals;   // one per PlacementResults collection
    void Clear() { markers.Clear(); props.Clear(); units.Clear(); decals.Clear(); }
};

} // namespace Data
} // namespace SanmapGen
