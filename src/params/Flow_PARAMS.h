// Flow_PARAMS.h — the reserved home for the future two-simulation velocity->accumulation model's
// settings (SANMAP_FORMAT_SPEC Correction 6). Layer: PARAMS. Deliberately minimal: the one field
// the spec actually names, `FlowMapColor` (a preview tint), and nothing else — the real
// velocity/flow simulation is out of scope for this ticket (no PROC consumer yet).
//
// Confirmed distinct from `Params::Stratum`'s `ErosionLayerSettings` and from the existing
// `FlowAccumulation` drainage/routing stage (`FlowAccumulation_PROC.*`, `FlowAccumulationConstants`)
// — neither of those moves or is touched by this reservation.
#pragma once

namespace SanmapGen {
namespace Params {

struct Flow {
    // A sane placeholder default (a blue-ish tint), not prescribed by the spec — the spec names
    // only the field, not a default.
    float flowMapColor[4] = { 0.2f, 0.4f, 1.0f, 1.0f };
};

} // namespace Params
} // namespace SanmapGen
