// AtmosphereBackgroundFog_PARAMS.h — the distant sky/horizon fog blend settings. Layer: PARAMS.
// Verbatim from ATMOSPHERE_PARAMS_SPEC.md's "The types" section.
#pragma once

namespace SanmapGen {
namespace Params {

struct AtmosphereBackgroundFog {
    float backgroundFogIntensity      = 1.0f;
    float backgroundFogRange          = 1024.0f;
    float backgroundFogMinimum        = 0.1f;
    float backgroundSkyColorIntensity = 1.0f;
    float backgroundColor[4]          = { 0.0f, 0.0f, 0.0f, 1.0f };
    float backgroundColorIntensity    = 0.0f;
    float backgroundColorFadeoutRange = 150000.0f;
    float backgroundColorFadeoutPower = 0.3f;
};

} // namespace Params
} // namespace SanmapGen
