// AtmosphereHeightFog_PARAMS.h — the altitude-banded fog settings. Layer: PARAMS.
// Verbatim from ATMOSPHERE_PARAMS_SPEC.md's "The types" section.
#pragma once

namespace SanmapGen {
namespace Params {

struct AtmosphereHeightFog {
    float heightFogIntensity = 1.0f;
    float heightFogRange[2]  = { -10.0f, 100.0f };
    float heightFogStart     = -10.0f;
    float heightFogEnd       = 500.0f;
    float heightFogPower     = 6.0f;
};

} // namespace Params
} // namespace SanmapGen
