# Work-Order M4-2 — `GradientLut_UI` (color-ramp LUT baking)

*Constitution §7. Milestone M4. **BATCH 1 (parallel with M4-1). Requires M4-0a
(`GradientRamp_PARAMS`) to have landed** — it is a read-only include. Own files.
Executor: SanGen Coder.*

## Title
Bake each color-ramp `Params::GradientRamp` into a sampled lookup table for the composite.

## Root problem
`PREVIEW_COMPOSITING_SPEC`: the preview colorizes slope / flow / accumulation / height /
water through gradient ramps. Baking each ramp to a fixed LUT once (not evaluating stops
per pixel) is the fast path. This is pure CPU LUT construction — the GPU upload is the
composite's job (M4-3).

## Target files
- `src/ui/GradientLut_UI.h` / `.cpp` (+ `_Test.cpp`).

## Layer & accuracy
`UI`. Visual. Pure CPU array construction (sandbox-testable) — no GL here.

**Layer note (ARCH_08_01_GradientLutBakeIsUi.md §8.1, binding):** this is a **UI** helper. ARCH_05_GodObjectDismemberment.md §5.4 originally said
"gradient LUT bake → `Gradient_PROC`"; that token is **corrected** — `Gradient_PROC` is
retired and never existed. A color ramp is not a pipeline stage (§7.4), has no GPU twin or
DAG node (§6.1), and building a presentation resource from PARAMS alone is not simulating
(§3.2). Do not create anything under `src/proc/` for this work-order.

## Solution
`BakeGradientLut(const Params::GradientRamp& ramp, int resolution = -1) -> std::vector<float>`
(RGBA, `resolution * 4` entries; `resolution < 0` means "use `ramp.lookupResolution`").
Interpolate the ramp's sorted `stops` (normalized `location` 0..1 → `color`) across
`resolution` samples; `ramp.bSmoothInterpolation` selects smoothstep vs linear between
adjacent stops. Sort/clamp stops defensively into a local copy (Constitution §6) — never
mutate the PARAMS input. Ramp domain mapping is the **caller's** job: `location` is
normalized 0..1 (ARCH_08_02_GradientRampParams.md §8.2), so the consumer normalizes slope degrees / flow range /
height range before sampling. Each colorized field owns its own ramp — the legacy
"accumulation reuses the flow gradient" aliasing is retired.

## Acceptance
A 2-stop black→white ramp bakes to a linear grey LUT (endpoints exact, midpoint ~0.5);
the smooth flag changes the midpoint as expected; unsorted stops give the same result as
pre-sorted ones; empty / one-stop input yields a safe constant LUT, no crash; the input
`GradientRamp` is unmodified. ALL PASS. Files within §1.5 ceilings.

## Out of scope
Uploading the LUT to the GPU / sampling it (M4-3, the composite). Defining
`Params::GradientRamp` itself (M4-0a — if it is not present, **stop and report**; do not
define it here and do not include any `core/` header, ARCH_08_04_CoderScopeLaw.md §8.4).
