// AtmosphereLinearFog_PARAMS.h — the camera-distance-banded fog settings. Layer: PARAMS.
// Verbatim from ATMOSPHERE_PARAMS_SPEC.md's "The types" section.
#pragma once

namespace SanmapGen {
namespace Params {

struct AtmosphereLinearFog {
    float linearFogIntensity       = 0.24f;
    float linearFogStart           = 100.0f;
    float linearFogEnd             = 5000.0f;
    float linearFogPower           = 1.0f;
    float linearFogCameraIntensity = 0.0f;
    float linearFogCameraStart     = 500.0f;
    float linearFogCameraEnd       = 5000.0f;
};

} // namespace Params
} // namespace SanmapGen
