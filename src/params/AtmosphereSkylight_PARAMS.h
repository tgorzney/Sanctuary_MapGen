// AtmosphereSkylight_PARAMS.h — the ambient skylight's own settings. Layer: PARAMS.
// Verbatim from ATMOSPHERE_PARAMS_SPEC.md's "The types" section. Own file rather than folded into
// the aggregator, per the spec's file-split ruling (uniform one-struct-one-file beats optimizing
// small structs for line count — Water_PARAMS.h is the standing precedent).
#pragma once

namespace SanmapGen {
namespace Params {

struct AtmosphereSkylight {
    float skylightIntensity   = 0.0f;
    float skylightTint[4]     = { 1.0f, 1.0f, 1.0f, 1.0f };
    float skylightTemperature = 9000.0f;
};

} // namespace Params
} // namespace SanmapGen
