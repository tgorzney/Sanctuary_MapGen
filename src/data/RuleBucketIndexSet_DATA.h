// RuleBucketIndexSet_DATA.h — one Data::RuleBucketIndex per Data::PlacementResults collection,
// same 4-way split (markers/props/units/decals), so a caller never has to guess which index goes
// with which collection. Layer: DATA (computed output over Data::PlacementResults).
#pragma once
#include "RuleBucketIndex_DATA.h"

namespace SanmapGen {
namespace Data {

struct RuleBucketIndexSet {
    RuleBucketIndex markers, props, units, decals;   // one per PlacementResults collection
    void Clear() { markers.Clear(); props.Clear(); units.Clear(); decals.Clear(); }
};

} // namespace Data
} // namespace SanmapGen
