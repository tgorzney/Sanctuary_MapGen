[← ARCH index](ARCH.md) · SanGen ARCH §10. Part of the ratified v2 architecture; the Constitution (`sangen_arch_pack/CONSTITUTION.md`) and this file's preamble in `ARCH.md` bind alongside it. **Only the ARCH Expert writes this file.**

## 10. `Params::Atmosphere` (ARCH ruling, ratifies `ATMOSPHERE_PARAMS_SPEC`)

Realizes the `Atmosphere_PARAMS` placeholder §5.1 already named in the `GenerationParams`
dismemberment list, by promoting the already-field-complete, 49-field `Ui::AtmosphereSettings`
(`src/ui/AtmosphereSettings_UI.h`) to a real recipe type. Confirmed against
`SANMAP_FORMAT_SPEC` "Top-level map fields" (Lighting / Background-fog / Global-wind) and the
legacy exporter (`Export_Metadata.cpp:149-221`): every field is an existing, live, camelCase,
format-native `.sanmap` key (or its already-established legible expansion, e.g. `sunRA` →
`sunRightAscension`) — **not** a new SanGen-owned schema-v3 section, so §1.6 does not apply and
no `IO_MIGRATION_SPEC` version gate is needed (the keys already round-trip today; this only
gives them a `Params::` home).

- **Shape:** one `Params::Atmosphere` aggregator (`sun`, `skylight`, `exposureSkybox`,
  `legacyFog`, `backgroundFog`, `heightFog`, `linearFog`, `globalWind`) composed of 8 named
  sub-structs — composition per §7.1 ("composition is allowed; rival top-level types are not"),
  the same pattern as `Stratum`/`StratumAppearance`/`StratumSoilPhysics`.
- **File split — RULED: all 8 sub-structs get their own `_PARAMS` header, none merged into
  the aggregator**, even the 2-3 field ones (`AtmosphereSkylight_PARAMS.h`,
  `AtmosphereGlobalWind_PARAMS.h`). `Water_PARAMS.h` (4 fields, its own file) is standing
  precedent that a small struct still gets its own file in this codebase; uniform
  one-struct-per-file beats an asymmetric "some inlined, some not" layout. Full reasoning:
  `ATMOSPHERE_PARAMS_SPEC`.
- **The one retype:** `skyboxIntensityModeIndex` (raw `int`) becomes
  `skyboxIntensityMode : SkyboxIntensityMode`
  (`enum class SkyboxIntensityMode { Exposure, Lux, Multiplier }`, added to
  `GenerationEnums_PARAMS.h`) — a §1.8 "retype a format-style category int" call, same
  precedent as `Army::faction`/`MarkerRule::category`. The `Index` suffix is dropped on
  retype, matching how `faction`/`category` are named (not `factionIndex`/`categoryIndex`).
  Every other field name is copied **verbatim** from `AtmosphereSettings_UI.h` — this is the
  only rename in the whole promotion.
- **Not promoted:** the UI-only dropdown label array (`skyboxIntensityModeLabels`) and the
  slot-addressing helpers (`AtmosphereColorAt`/`AtmosphereVectorAt`/`AtmosphereTextAt` and
  their slot enums) are `Ui::AtmosphereSettings`-specific widget-table plumbing, not settings
  — they stay in `UI` (Constitution §1) and are not ported.
- **Shape only, not wiring.** `MapRecipe_PARAMS.h` gaining `Atmosphere atmosphere;` (flat
  sibling of `water`), the `GenerationEnums_PARAMS.h` edit, the matching `IO` round-trip
  against the already-live `mapdef["sun*"]`/`["skylight*"]`/`["skybox*"]`/`["fog*"]`/
  `["background*"]`/`["heightFog*"]`/`["linearFog*"]`/`["windSpeed"/"windDirection"]` keys,
  and retiring `AtmosphereSettings_UI.h`'s promotion scope note are a separate coder
  work-order (`ATMOSPHERE_PARAMS_SPEC` "Where these land").
