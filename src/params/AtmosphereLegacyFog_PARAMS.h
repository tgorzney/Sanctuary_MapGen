// AtmosphereLegacyFog_PARAMS.h — the original distance-fog system. "legacyFog" (not "fog") because
// three more fog systems (background/height/linear) coexist with it. Layer: PARAMS.
// Verbatim from ATMOSPHERE_PARAMS_SPEC.md's "The types" section.
#pragma once

namespace SanmapGen {
namespace Params {

struct AtmosphereLegacyFog {
    float legacyFogAttenuationDistance = 200.0f;
    float legacyFogBaseHeight          = 15.0f;
    float legacyFogMaximumHeight       = 100.0f;
    float legacyFogMaximumDistance     = 1500.0f;
    float legacyFogAnisotropy          = 0.5f;
};

} // namespace Params
} // namespace SanmapGen
