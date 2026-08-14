// ScatterRule_PARAMS.h — prop and decal scatter rules.
// Layer: PARAMS. Settings only (PLACEMENT_SCATTER_SPEC). Scatter math lives in PROC.
#pragma once

namespace SanmapGen {
namespace Params {

struct PropRule {
    float density   = 0.5f;
    float minSlope  = 0.0f;
    float maxSlope  = 89.9f;
    float minHeight = 0.0f;
    float maxHeight = 1.0f;
    bool  bAvoidWater = false;
    bool  bNearCliffs = false;
};

struct DecalRule {
    float density   = 0.5f;
    float minSlope  = 0.0f;
    float maxSlope  = 89.9f;
    float minHeight = 0.0f;
    float maxHeight = 1.0f;
};

} // namespace Params
} // namespace SanmapGen
