# ATMOSPHERE_PARAMS_SPEC — `Params::Atmosphere` (sun / sky / fog / wind recipe settings)

Source of truth: `SANMAP_FORMAT_SPEC` "Top-level map fields" (Lighting / Background-fog /
Global-wind entries) and the legacy exporter (`core/export/Export_Metadata.cpp:149-221`),
cross-checked against the already-field-complete UI-only struct that named every one of
these values in full, `src/ui/AtmosphereSettings_UI.h`. That file's own header comment
flags the promotion as a known, deliberately deferred gap:

> v2 has NO atmosphere PARAMS type and no stage that consumes one, so these are
> caller-owned PRESENTATION settings... promoting the whole struct to `Params::Atmosphere`
> later is a mechanical move. That promotion needs its own work-order; this file does not
> make one up.

This spec is that work-order's ARCH ruling (ARCH §10 references this file).

## Scope — a field-level promotion, not a new schema section
Every one of the 49 fields below is a **format-native, camelCase, already-live**
`.sanmap` top-level key (`sunRA`, `sunIntensity`, `skylightIntensity`, `exposure`,
`skyboxRotation`, `fogAttenuationDistance`, `backgroundFogIntensity`, `heightFogIntensity`,
`linearFogIntensity`, `windSpeed`, `windDirection`, …) — not a new SanGen-owned PascalCase
schema-v3 section. Consequences:
- **ARCH §1.6 does not apply** (that rule governs which top-level keys are SanGen-owned
  `PascalCase` sections; nothing here is one).
- **No `IO_MIGRATION_SPEC` version gate is needed.** A migration gate exists to carry a
  *shape change* across a `SanGenVersion` bump; this ratification changes no on-disk shape
  — the keys already round-trip today (`Export_Metadata.cpp`, `core/MapImporter.cpp`). It
  only gives the values a `Params::` home so a PROC/PIPELINE/UI consumer can eventually
  read them without going through `core/`.
- `SANMAP_FORMAT_SPEC`'s "Verified deletions" list already confirmed
  `mapGeneratorData.Atmosphere` (a v1 SanGen-owned *duplicate* of these same format fields)
  as dead and deleted — this ratification does not resurrect that duplicate; it promotes
  the format's own fields directly.

## Shape — one aggregator, eight composed sub-structs
Per ARCH §7.1 ("composition is allowed; rival top-level types are not" — the same ruling
that produced `Stratum` / `StratumAppearance` / `StratumSoilPhysics`), `Params::Atmosphere`
is one aggregator of 8 named, independently-toggled rendering subsystems:

| Sub-struct | Field | Fields | File |
| --- | --- | --- | --- |
| Sun | `sun` | 11 | `AtmosphereSun_PARAMS.h` |
| Skylight | `skylight` | 3 | `AtmosphereSkylight_PARAMS.h` |
| Exposure & skybox | `exposureSkybox` | 8 | `AtmosphereExposureSkybox_PARAMS.h` |
| Legacy fog | `legacyFog` | 5 | `AtmosphereLegacyFog_PARAMS.h` |
| Background fog | `backgroundFog` | 8 | `AtmosphereBackgroundFog_PARAMS.h` |
| Height fog | `heightFog` | 5 | `AtmosphereHeightFog_PARAMS.h` |
| Linear fog | `linearFog` | 7 | `AtmosphereLinearFog_PARAMS.h` |
| Global wind | `globalWind` | 2 | `AtmosphereGlobalWind_PARAMS.h` |

Total: 49 fields, matching `AtmosphereSettings_UI.h` exactly (verified by direct count).

## The file-split call (ARCH §1.5) — RULED: keep all eight, do not merge the small ones
`AtmosphereSkylight` (3 fields) and `AtmosphereGlobalWind` (2 fields) are small enough that
merging them into `Atmosphere_PARAMS.h` itself was a live option (offered by the work
order as either being defensible). **Ruling: keep all eight as separate files.**

Reasoning:
1. **`Water_PARAMS.h` is already-standing precedent for a smaller-than-this struct getting
   its own undivided file** (`Params::Water` is 4 fields, 17 lines total, its own file, not
   folded into anything). A 2-3 field group is not exceptional in this codebase.
2. **Uniform beats asymmetric.** An aggregator whose members are "sometimes their own file,
   sometimes inlined" forces a reader (human or coder) to check per-member instead of
   knowing the rule on sight. All eight following the same one-struct-one-file pattern is a
   stronger, more AI-legible invariant than optimizing two of eight for line count.
3. **Blast radius (§1.5's own rationale: file size = per-edit token/mismatch cost).** Editing
   `globalWind`'s two fields should not force re-reading/re-writing a merged file that also
   holds `skylight`'s three unrelated fields. Splitting, not merging, is what §1.5 argues for
   even at small field counts.
4. **None of the eight is a trivial appendage of the aggregator** (unlike, say, a single
   validity flag would be) — sun, skylight, exposure/skybox, four fog kinds, and wind are
   eight genuinely separate rendering subsystems the legacy UI itself grouped as separate
   topics (`AtmosphereSettings_UI.h`'s own `// ---` section comments).

## The one retype — `skyboxIntensityModeIndex` → `skyboxIntensityMode : SkyboxIntensityMode`
The UI struct's `int skyboxIntensityModeIndex` (a raw dropdown-row index, 0/1/2) becomes a
typed enum on promotion:

```
enum class SkyboxIntensityMode { Exposure, Lux, Multiplier };   // GenerationEnums_PARAMS.h
```

- **Precedent:** ARCH §1.8's "retype a format-style category int" rule — the same move
  already made for `Army::faction` (`enum class Faction`) and `MarkerRule::category`
  (`enum class MarkerCategory`). The format's own `skyboxIntensityMode` JSON value is
  already a string enum (`"Exposure"`/`"Lux"`/`"Multiplier"`,
  `Export_Metadata.cpp:180-182`) — the raw-int UI field was always a lossy encoding of an
  already-enum-shaped value.
- **The `Index` suffix is dropped on retype**, matching how `faction` and `category` are
  named (not `factionIndex`/`categoryIndex`) — once typed, the field holds a category value,
  not an index into anything.
- **Every other field name is copied VERBATIM from `AtmosphereSettings_UI.h`** — zero
  renaming, per the work order. This is the only retype and the only rename in the whole
  promotion.

## Not promoted — UI-only presentation plumbing stays in UI
`AtmosphereSettings_UI.h` carries widget-table addressing helpers that operate on
`Ui::AtmosphereSettings` specifically, not on the settings values themselves:
- `skyboxIntensityModeLabels` / `kSkyboxIntensityModeCount` — dropdown row label strings.
- `kSunTintSlot`/`kSkylightTintSlot`/`kBackgroundColorSlot` and `AtmosphereColorAt`,
  `AtmosphereVectorAt`, `AtmosphereVectorComponentCount`, `AtmosphereTextAt`, and their slot
  enums — member-pointer-can't-address-arrays-or-strings control-table plumbing.

None of this is a *setting*; it is `UI`-layer widget wiring (Constitution §1: "UI ... sets
params, trips dirty flags"). It stays in `Ui::AtmosphereSettings` and is not ported. If a
future generic-widget-table work needs equivalent accessors against `Params::Atmosphere`
directly, that is new UI work, not decided here.

## The types
Shape only — these are documentation of the ratified C++ shape for the coder work-order to
build; this spec does not create the files (ARCH Expert is read-only against program code).

```cpp
// Atmosphere_PARAMS.h — the aggregator (ARCH §7.1 composition).
struct Atmosphere {
    AtmosphereSun            sun;
    AtmosphereSkylight       skylight;
    AtmosphereExposureSkybox exposureSkybox;
    AtmosphereLegacyFog      legacyFog;
    AtmosphereBackgroundFog  backgroundFog;
    AtmosphereHeightFog      heightFog;
    AtmosphereLinearFog      linearFog;
    AtmosphereGlobalWind     globalWind;
};
```

```cpp
// AtmosphereSun_PARAMS.h — 11 fields, verbatim from Ui::AtmosphereSettings.
struct AtmosphereSun {
    float sunRightAscension         = 0.0f;
    float sunDeclination            = 0.0f;
    float sunIntensity              = 15000.0f;
    float sunTint[4]                = { 1.0f, 1.0f, 1.0f, 1.0f };
    float sunTemperature            = 6300.0f;
    float sunAngularDiameter        = 0.5f;
    float sunVolumetricMultiplier   = 6.7f;
    float sunVolumetricShadowDimmer = 0.5f;
    float sunPosition[3]            = { 512.0f, 10.0f, 256.0f };
    std::string sunCookiePath;
    float sunCookieSize[2]          = { 1024.0f, 1024.0f };
};
```

```cpp
// AtmosphereSkylight_PARAMS.h — 3 fields.
struct AtmosphereSkylight {
    float skylightIntensity   = 0.0f;
    float skylightTint[4]     = { 1.0f, 1.0f, 1.0f, 1.0f };
    float skylightTemperature = 9000.0f;
};
```

```cpp
// AtmosphereExposureSkybox_PARAMS.h — 8 fields. Depends on GenerationEnums_PARAMS.h
// for SkyboxIntensityMode.
struct AtmosphereExposureSkybox {
    float exposure                 = 12.0f;
    float exposureCompensation     = 2.5f;
    std::string skyboxPath;
    float skyboxRotation           = 0.0f;
    SkyboxIntensityMode skyboxIntensityMode = SkyboxIntensityMode::Exposure;  // retyped, see above
    float skyboxExposure           = 12.0f;
    float skyboxMultiplier         = 1.0f;
    float skyboxLuxValue           = 10000.0f;
};
```

```cpp
// AtmosphereLegacyFog_PARAMS.h — 5 fields. "legacyFog" (not "fog") because three more
// fog systems (background/height/linear) coexist with it.
struct AtmosphereLegacyFog {
    float legacyFogAttenuationDistance = 200.0f;
    float legacyFogBaseHeight          = 15.0f;
    float legacyFogMaximumHeight       = 100.0f;
    float legacyFogMaximumDistance     = 1500.0f;
    float legacyFogAnisotropy          = 0.5f;
};
```

```cpp
// AtmosphereBackgroundFog_PARAMS.h — 8 fields.
struct AtmosphereBackgroundFog {
    float backgroundFogIntensity      = 1.0f;
    float backgroundFogRange          = 1024.0f;
    float backgroundFogMinimum        = 0.1f;
    float backgroundSkyColorIntensity = 1.0f;
    float backgroundColor[4]          = { 0.0f, 0.0f, 0.0f, 1.0f };
    float backgroundColorIntensity    = 0.0f;
    float backgroundColorFadeoutRange = 150000.0f;
    float backgroundColorFadeoutPower = 0.3f;
};
```

```cpp
// AtmosphereHeightFog_PARAMS.h — 5 fields.
struct AtmosphereHeightFog {
    float heightFogIntensity = 1.0f;
    float heightFogRange[2]  = { -10.0f, 100.0f };
    float heightFogStart     = -10.0f;
    float heightFogEnd       = 500.0f;
    float heightFogPower     = 6.0f;
};
```

```cpp
// AtmosphereLinearFog_PARAMS.h — 7 fields.
struct AtmosphereLinearFog {
    float linearFogIntensity       = 0.24f;
    float linearFogStart           = 100.0f;
    float linearFogEnd             = 5000.0f;
    float linearFogPower           = 1.0f;
    float linearFogCameraIntensity = 0.0f;
    float linearFogCameraStart     = 500.0f;
    float linearFogCameraEnd       = 5000.0f;
};
```

```cpp
// AtmosphereGlobalWind_PARAMS.h — 2 fields.
struct AtmosphereGlobalWind {
    float globalWindSpeed     = 0.25f;
    float globalWindDirection = 160.0f;
};
```

All nine files (`Atmosphere_PARAMS.h` + 8 members) carry no logic beyond the shape above
(Constitution §1) — settings only, no GL, no computed data (ARCH §3.2), matching every
other `_PARAMS` file's discipline.

## Where these land (for the coder work-order — not built here)
- `MapRecipe_PARAMS.h` gains `Atmosphere atmosphere;` as a flat sibling of `water`.
- `GenerationEnums_PARAMS.h` gains `enum class SkyboxIntensityMode { Exposure, Lux, Multiplier };`.
- `AtmosphereSettings_UI.h`'s promotion scope note (its header comment, quoted above) is
  retired/updated to point at this spec once the promotion lands.
- The matching `IO` read/write of these fields against `mapdef["sun*"]` /
  `mapdef["skylight*"]` / `mapdef["skybox*"]` / `mapdef["fog*"]` /
  `mapdef["background*"]` / `mapdef["heightFog*"]` / `mapdef["linearFog*"]` /
  `mapdef["windSpeed"/"windDirection"]` (already-live keys, `Export_Metadata.cpp:149-221`,
  `core/MapImporter.cpp`) is a separate coder work-order — this ratification fixes only the
  `Params::` shape, per ARCH §8.4.
