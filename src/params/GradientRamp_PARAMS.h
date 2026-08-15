// GradientRamp_PARAMS.h — one adjustable color ramp (the preview colorization setting).
// Layer: PARAMS. Settings only — no logic, no GL, no DATA (ARCH §3.3). Shape is the
// ARCH §8.2 ruling. Baking the ramp to a LUT is GradientLut_UI (ARCH §8.1); sampling
// and uploading it is PreviewComposite_UI. One ramp per colorized field (slope, flow,
// accumulation, height, water) — the legacy flow/accumulation aliasing is retired.
#pragma once
#include <string>
#include <vector>

namespace SanmapGen {
namespace Params {

// A single color key along the ramp.
struct GradientStop {
    // NORMALIZED 0..1 along the ramp. Domain mapping (slope degrees, flow range,
    // height range) is the consumer's job, not this settings type's (ARCH §8.2).
    float location = 0.0f;

    // Linear RGBA.
    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};

    // Sorting convenience only — the one member function PARAMS permits (ARCH §8.2).
    bool operator<(const GradientStop& other) const { return location < other.location; }
};

struct GradientRamp {
    std::string               name = "New Ramp";
    std::vector<GradientStop> stops;

    // Smoothstep between stops when true, linear when false.
    bool bSmoothInterpolation = true;

    // Entry count of the baked lookup table. Tweakable (Constitution §8) — never
    // hardcoded at the downstream call site.
    int lookupResolution = 256;
};

} // namespace Params
} // namespace SanmapGen
