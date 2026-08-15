# Work-Order M4-0a — `GradientRamp_PARAMS` (the color-ramp settings type)

*Constitution §7. Milestone M4. **BATCH 0 (parallel with M4-0b, M4-1). Must land before
M4-2 starts.** Own file, no dependencies. Executor: SanGen Coder.*

## Title
The adjustable color ramp — the v2 PARAMS type every preview colorization samples.

## Root problem
The v2 tree has **no** color-ramp settings type. The only one that exists is the legacy
`core/params/Params_Gradients.h` (`GradientSettings` / `GradientStop`), and `src/` may
never include a `core/` header (ARCH §2, hit-list #1). M4-2 (`GradientLut_UI`) cannot
compile against a real type until this lands. Ruled in **ARCH §8.2**.

## Target files
- `src/params/GradientRamp_PARAMS.h` (+ `_Test.cpp`).

## Layer & accuracy
`PARAMS`. Settings only — no logic, no behavior, no GL, no DATA (ARCH §3.3).

## Solution (shapes are ARCH §8.2 rulings, not coder choices)
```
namespace SanmapGen { namespace Params {
struct GradientStop {
    float location = 0.0f;                        // NORMALIZED 0..1 along the ramp
    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};    // linear RGBA
};
struct GradientRamp {
    std::string             name = "New Ramp";
    std::vector<GradientStop> stops;
    bool  bSmoothInterpolation = true;            // smoothstep vs linear between stops
    int   lookupResolution     = 256;             // tweakable (Constitution §8), not hardcoded downstream
};
}}
```
- **`location` is normalized 0..1.** The legacy 0..100 / "or degrees 0–90" scale is
  **not** carried over: domain mapping (slope degrees, flow range, height range) is the
  consumer's job. Verified safe — gradients are not serialized in `mapGeneratorData`
  today (no `Gradient` key anywhere in `core/export/`), so no `.sanmap` round-trip breaks.
- The legacy role-word name `GradientSettings` is **not** carried over (ARCH §1.1: the
  namespace already says settings; the type states the quantity).
- One ramp **per colorized field** — slope, flow, accumulation, height, water each own
  their own instance. The legacy "accumulation reuses the flow ramp" aliasing is retired.
- `operator<` on `GradientStop` by `location` may be provided for sorting convenience;
  that is the only member function permitted (PARAMS holds no logic).

## Acceptance
Header compiles standalone; default-constructed ramp is valid (empty `stops`,
`lookupResolution == 256`); a stop list sorts by `location`; no include of any `core/`
header, no GL, no DATA type. Files within §1.5 ceilings.

## Out of scope
Baking the ramp to a LUT (M4-2). Sampling / uploading it (M4-3). Any gradient **editor**
widget (M5). Serializing ramps into `mapGeneratorData` (needs its own IO work-order and a
`SANMAP_FORMAT_SPEC` decision — do not add JSON keys here).
