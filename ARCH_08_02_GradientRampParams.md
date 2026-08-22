[← ARCH index](ARCH.md) · [§8 ARCH_08_M4Resolutions](ARCH_08_M4Resolutions.md) · SanGen ARCH §8.2. **Only the ARCH Expert writes this file.**

### 8.2 `GradientRamp_PARAMS` — the missing v2 settings type
A color ramp is an **adjustable setting**, so it is PARAMS, and `src/` may never include a
`core/` header. The type therefore has to exist in the v2 tree before M4-2 can compile.

**Ruling: `src/params/GradientRamp_PARAMS.h`, type `Params::GradientRamp` (with its member
`Params::GradientStop`).** Naming follows §1.1/§7.1 precedent (`Params::Water`,
`Params::Stratum`): the namespace already says "settings", so the type states the
**quantity** — `GradientRamp` — and the legacy role-word name `GradientSettings` is
**not** carried over. M4-2's signature becomes
`BakeGradientLut(const Params::GradientRamp&, int resolution)`.

Binding shape decisions (they are ARCH rulings, not coder preference):
- `stops` is a `std::vector<GradientStop>`; `GradientStop` holds `location` + `color[4]`.
- **`location` is normalized 0..1** along the ramp. The legacy field was "0.0 to 100.0
  (or mapped to degrees 0–90)" — a domain-dependent scale baked into the settings type,
  which is exactly the "name/quantity ambiguity" §1.1 forbids. Domain mapping (slope
  degrees, flow range, height range) belongs to the **consumer**, which normalizes its own
  domain before sampling the LUT. Verified safe: gradients are **not** part of any
  SanGen-owned schema v3 section (no `Gradient` key anywhere in `SANMAP_FORMAT_SPEC`), so
  no round-trip breaks.
- `bSmoothInterpolation` keeps the `b` prefix (§1.1) and selects smoothstep vs linear.
- One ramp **per colorized field** — slope, flow, accumulation, height, water each own
  theirs. The legacy "accumulation reuses the flow gradient" aliasing is retired
  (`PREVIEW_COMPOSITING_SPEC`).
- Resolution is a tweakable (Constitution §8), defaulted to 256, not hardcoded at the
  call site.

