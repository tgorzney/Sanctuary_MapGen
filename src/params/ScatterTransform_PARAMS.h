// ScatterTransform_PARAMS.h — the per-rule instance transform ranges shared by every
// scatter rule (marker / prop / unit / decal).
// Layer: PARAMS. Settings only. PLACEMENT_SCATTER_SPEC "Missing capability": no scale-range,
// no rotation-range and no align-to-normal existed — they are first-class tweakables here
// (Constitution §8). The scatter samples inside these ranges with the position hash, never
// with rand(), so the transform is a pure function of (seed, rule, position).
#pragma once

namespace SanmapGen {
namespace Params {

struct ScatterTransform {
    float scaleMinimum = 1.0f;
    float scaleMaximum = 1.0f;

    float rotationMinimumDegrees = 0.0f;     // yaw around the world up axis
    float rotationMaximumDegrees = 360.0f;

    bool  bAlignToTerrainNormal = false;     // tilt the instance onto the local slope
    bool  bCollidable           = false;     // collidable => gameplay-relevant (pathing/reclaim)

    // Game template id (`tpId`, UNIT_PROP_MARKER_DATA_SPEC) — a naming-law verbatim exception.
    char templateIdentifier[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
};

} // namespace Params
} // namespace SanmapGen
