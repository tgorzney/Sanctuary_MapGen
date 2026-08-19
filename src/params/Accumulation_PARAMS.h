// Accumulation_PARAMS.h — the reserved home for the future accumulation half of the two-simulation
// velocity->accumulation model (SANMAP_FORMAT_SPEC Correction 6). Layer: PARAMS. Genuinely empty:
// the spec has no field list at all for this section yet — "TBD" means TBD, nothing is invented
// here. No PROC consumer.
//
// Confirmed distinct from the existing `FlowAccumulation` drainage/routing stage
// (`FlowAccumulation_PROC.*`, `FlowAccumulationConstants`) — that stage is untouched by this
// reservation.
#pragma once

namespace SanmapGen {
namespace Params {

struct Accumulation {
};

} // namespace Params
} // namespace SanmapGen
