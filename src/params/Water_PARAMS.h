// Water_PARAMS.h — water surface + depth settings.
// Layer: PARAMS. Settings only.
#pragma once

namespace SanmapGen {
namespace Params {

struct Water {
    bool  bEnabled              = false;
    float waterLevelMaximum     = 0.0f;   // surface height (game units)
    float deepWaterDepthMinimum = 0.0f;   // depth at which "deep water" shading begins
    float deepWaterDepthMaximum = 0.0f;   // depth at which it saturates
};

} // namespace Params
} // namespace SanmapGen
