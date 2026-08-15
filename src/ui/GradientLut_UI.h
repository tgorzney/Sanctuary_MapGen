// GradientLut_UI.h — bake one Params::GradientRamp into a sampled linear-RGBA lookup table.
// Layer: UI (ARCH §8.1, binding: the color-ramp LUT bake is presentation, not a PROC stage;
// `Gradient_PROC` is retired and never existed). Pure CPU, no GL — uploading and sampling the
// table is PreviewComposite_UI (M4-3). Reads PARAMS only, which is downward and legal (§3.1);
// it reads no DATA field and re-derives no simulated quantity, so §3.2 is untouched.
#pragma once
#include <vector>
#include "../params/GradientRamp_PARAMS.h"

namespace SanmapGen {
namespace Ui {

// Pass this as `resolution` to bake at the ramp's own Params::GradientRamp::lookupResolution
// (the tweakable, Constitution §8 — never a hardcoded count at the call site).
enum : int { kUseRampLookupResolution = -1 };

// Validation bounds on the requested entry count (Constitution §6): they only fence off
// nonsense input, they are not the setting itself.
enum : int { kMinimumLookupResolution = 1, kMaximumLookupResolution = 65536 };

// Channel count of one lookup entry (linear RGBA).
enum : int { kLookupChannelCount = 4 };

// Bakes `ramp` into `resolution * kLookupChannelCount` linear-RGBA floats.
//
// Entry `i` samples the ramp at position `i / (resolution - 1)` — endpoint-INCLUSIVE, so the
// first and last entries land exactly on the ramp ends (a sampler therefore indexes with
// `(resolution - 1) * normalizedValue`, not with a texel-center offset).
//
// `GradientStop::location` is normalized 0..1 (ARCH §8.2). Mapping a domain onto that 0..1 —
// slope degrees, flow range, accumulation range, height range — is the CALLER's job, not this
// function's. Each colorized field owns its own ramp; the legacy "accumulation reuses the flow
// gradient" aliasing is retired.
//
// `resolution < 0` means "use `ramp.lookupResolution`"; the resolved count is clamped into
// [kMinimumLookupResolution, kMaximumLookupResolution].
//
// `ramp.bSmoothInterpolation` selects smoothstep vs linear blending between adjacent stops.
//
// The input ramp is NEVER modified: stops are clamped and sorted into a local copy
// (Constitution §6). Degenerate input is safe, not a crash — an empty stop list bakes a
// constant table of the default Params::GradientStop color, and a single stop bakes a constant
// table of that stop's color.
std::vector<float> BakeGradientLut(const Params::GradientRamp& ramp,
                                   int resolution = kUseRampLookupResolution);

} // namespace Ui
} // namespace SanmapGen
