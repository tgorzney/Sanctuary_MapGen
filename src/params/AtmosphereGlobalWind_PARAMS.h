// AtmosphereGlobalWind_PARAMS.h — the ambient wind vector driving foliage/particle sway.
// Layer: PARAMS. Verbatim from ATMOSPHERE_PARAMS_SPEC.md's "The types" section. Own file rather
// than folded into the aggregator, per the spec's file-split ruling (same reasoning as
// AtmosphereSkylight_PARAMS.h).
#pragma once

namespace SanmapGen {
namespace Params {

struct AtmosphereGlobalWind {
    float globalWindSpeed     = 0.25f;
    float globalWindDirection = 160.0f;
};

} // namespace Params
} // namespace SanmapGen
