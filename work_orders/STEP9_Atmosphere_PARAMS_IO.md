# Work-Order — Step 9: `Params::Atmosphere` + its `.sanmap` IO round-trip

*Constitution §7. Executor: SanGen Coder. Implements the fully-ratified `sangen_arch_pack/specs/
ATMOSPHERE_PARAMS_SPEC.md` verbatim (exact types/field names given, zero open questions) plus the
matching IO wiring the spec explicitly leaves for "a separate coder work-order" (its "Where these
land" section, last bullet).*

## Root problem
`Params::Atmosphere` doesn't exist. `AtmosphereSettings_UI.h`'s own header comment already names
the gap: "v2 has NO atmosphere PARAMS type... these are caller-owned PRESENTATION settings."
`MapExporter_IO.h` SCOPE NOTE 2 confirms the IO side: "ATMOSPHERE has no `_PARAMS` home... written
from the format's own defaults." Confirmed by grep: no `Atmosphere*_PARAMS.h` file exists, and
`MapExporter_Recipe_IO.cpp` writes none of the 49 sun/skylight/exposure/skybox/fog/wind keys at
all today — they're simply absent from every exported `.sanmap`.

## Target files
New (verbatim from `ATMOSPHERE_PARAMS_SPEC.md`'s "The types" section — copy exactly, no
deviation): `src/params/Atmosphere_PARAMS.h`, `AtmosphereSun_PARAMS.h`,
`AtmosphereSkylight_PARAMS.h`, `AtmosphereExposureSkybox_PARAMS.h`, `AtmosphereLegacyFog_PARAMS.h`,
`AtmosphereBackgroundFog_PARAMS.h`, `AtmosphereHeightFog_PARAMS.h`, `AtmosphereLinearFog_PARAMS.h`,
`AtmosphereGlobalWind_PARAMS.h` — nine files, one aggregator + eight sub-structs, per the spec's
own ratified file-split ruling (keep all eight separate, do not merge the small ones).
`src/io/MapExporter_Atmosphere_IO.cpp` (`BuildAtmosphereJson` — but see "JSON shape" below, this
likely writes directly into the top-level `document`, not a nested sub-object) / `MapImporter_
Atmosphere_IO.cpp` (`ReadAtmosphereJson`), following this session's established per-domain-file
convention.

Modified:
- `src/params/GenerationEnums_PARAMS.h` — add `enum class SkyboxIntensityMode { Exposure, Lux,
  Multiplier };`.
- `src/params/MapRecipe_PARAMS.h` — add `Atmosphere atmosphere;` as a flat sibling of `water`.
- `src/io/MapExporter_Recipe_IO.h`/`.cpp` — declare/call `BuildAtmosphereJson`.
- `src/io/MapImporter_Recipe_IO.h`, `MapImporter_IO.cpp` — declare/call `ReadAtmosphereJson`.
- `src/io/MapExporter_IO.h`/`MapImporter_IO.h` — retire SCOPE NOTE 2 ("Atmosphere has no `_PARAMS`
  home") — no longer true after this ticket.
- `src/ui/AtmosphereSettings_UI.h` — retire its promotion scope-note comment (quoted verbatim in
  the spec) since the promotion has now landed. **Do NOT retype `Ui::AtmosphereSettings` onto
  `Params::Atmosphere` or touch any UI draw code** — same UI-wiring exclusion every prior PARAMS
  promotion in this project has had; that's a separate ticket.

## Layer & accuracy class
PARAMS + IO/BRIDGE. Accuracy class: Exact.

## Backend policy
CPU only — settings I/O, no compute.

## ARCH rules invoked
- `ATMOSPHERE_PARAMS_SPEC.md` in full — binding, zero deviation on names/shapes/defaults.
- ARCH §7.1 (composition, not rival types) — the aggregator-of-eight shape.
- ARCH §1.8 — the one sanctioned retype (`skyboxIntensityModeIndex` → `skyboxIntensityMode`).
- **No `IO_MIGRATION_SPEC` version gate** — the spec explicitly rules this out: these are
  already-live format-native keys, not a new SanGen-owned section; no shape change, no version
  bump needed (same reasoning already applied to Steps 2-4's new-content-in-existing-keys cases).

## JSON shape — confirm against the real C# ground truth before writing IO code
The spec cites `Export_Metadata.cpp:149-221`/`core/MapImporter.cpp` (legacy exporter) as ground
truth for the exact keys, but per this project's established discipline, verify field-for-field
against `SanMap.cs` (`D:\Projects\Sanctuary\Sanmap File Format\SanMap.cs`) directly before writing
`BuildAtmosphereJson`/`ReadAtmosphereJson` — that file was read extensively earlier this session
and already confirmed to contain exactly these fields (`sunRA`, `sunDA`, `sunIntensity`, `sunTint`,
`sunTemperature`, `sunAngularDiameter`, `sunVolumetricsMultiplier`, `sunVolumetricsShadowDimer`,
`sunPosition`, `sunCookie.path`, `sunCookieSize`, `skylightIntensity`, `skylightTint`,
`skylightTemperature`, `exposure`, `exposureCompensation`, `skybox.path`, `skyboxRotation`,
`skyboxIntensityMode`, `skyboxExposure`, `skyboxMultiplier`, `skyboxLuxValue`,
`backgroundFogIntensity`, `backgroundFogRange`, `backgroundFogMinimum`,
`backgroundSkyColorIntensity`, `backgroundColor`, `backgroundColorIntensity`,
`backgroundColorFadeoutRange`, `backgroundColorFadeoutPower`, `heightFogIntensity`,
`heightFogRange`, `heightFogStart`, `heightFogEnd`, `heightFogPower`, `linearFogIntensity`,
`linearFogStart`, `linearFogEnd`, `linearFogPower`, `linearFogCameraIntensity`,
`linearFogCameraStart`, `linearFogCameraEnd`, `fogAttenuationDistance`, `fogBaseHeight`,
`fogMaximumHeight`, `fogMaximumDistance`, `fogAnisotropy`, `windSpeed`, `windDirection`). All are
**top-level, flat `document[...]` keys** (`SanMap.cs`'s fields are all direct members of the
`SanMap` class itself, confirmed — none nested under a sub-object), so `BuildAtmosphereJson`
writes ~49 flat `document["sunRA"] = ...` style entries directly, no wrapper object. Note the C#
field is `sunRA`/`sunDA` (not `sunRightAscension`/`sunDeclination`) and `sunVolumetricsMultiplier`/
`sunVolumetricsShadowDimer` (not `sunVolumetricMultiplier`/`sunVolumetricShadowDimmer`,
sic — the C# has the real typo "Dimer" not "Dimmer") — the JSON key is the format's own spelling
verbatim (ARCH §1.1), which differs slightly from the `Params::` field's own (corrected) spelling;
do not silently "fix" the JSON key to match the PARAMS field name. `sunCookie`/`skybox` are
`TextureLoader{path}` objects in C# (`{"path": "..."}`), not bare strings — `sunCookiePath`/
`skyboxPath` in `Params::` read/write through that one-field wrapper shape, not a bare string key.
`sunTint`/`skylightTint`/`backgroundColor` are `Color{r,g,b,a}` (not `{x,y,z,w}` — reuse the
`{r,g,b,a}` convention already shipped for `armyColor`/`diffuseRemap`). `sunPosition` is
`Vector3{x,y,z}`; `sunCookieSize`/`heightFogRange` are `Vector2{x,y}`.
**Seven more field-name mismatches (Format Expert catch — do not miss these, same category as the
sun-field mismatches above):**

| `Params::` field | JSON key |
| --- | --- |
| `legacyFogAttenuationDistance` | `fogAttenuationDistance` |
| `legacyFogBaseHeight` | `fogBaseHeight` |
| `legacyFogMaximumHeight` | `fogMaximumHeight` |
| `legacyFogMaximumDistance` | `fogMaximumDistance` |
| `legacyFogAnisotropy` | `fogAnisotropy` |
| `globalWindSpeed` | `windSpeed` |
| `globalWindDirection` | `windDirection` |

`skyboxIntensityMode` is written as a JSON **string** (`"Exposure"`/`"Lux"`/`"Multiplier"`, a
`StringEnumConverter` in the real C#, confirmed by the spec) — read/write it as a string, mapping
to/from `SkyboxIntensityMode`, NOT as an integer like every other enum in this codebase
(`MarkerCategory`, `Faction`, etc. all serialize as ints) — this is a genuine, confirmed exception,
not an oversight; get the string spelling exact (`"Exposure"`, `"Lux"`, `"Multiplier"`) and fail
safe (default to `Exposure`) on an unrecognized string.

## Solution
1. Create the nine PARAMS files verbatim from the spec.
2. Add `SkyboxIntensityMode` to `GenerationEnums_PARAMS.h`; add `atmosphere` to `MapRecipe_PARAMS.h`.
3. `MapExporter_Atmosphere_IO.cpp`: `BuildAtmosphereJson(const Params::MapRecipe& recipe,
   nlohmann::ordered_json& document)` (or return a set of top-level insertions — since these are
   ~49 FLAT top-level keys, not one sub-object, decide the cleanest function shape: likely takes
   `document` by reference and writes directly into it, unlike every other `Build*Json` in this
   codebase which returns one self-contained object — flag this shape difference explicitly in
   the function's own header comment so a future reader isn't confused by the inconsistency).
   Write all 49 keys per the "JSON shape" section above.
4. `MapImporter_Atmosphere_IO.cpp`: the inverse, reading each of the 49 top-level keys directly
   from `document` (not a sub-object) using the existing `JsonPrimitives_IO.h` typed accessors.
   Add whatever new primitive is needed for the `skyboxIntensityMode` string↔enum mapping (a
   small, local, string-comparison function — this one-off doesn't need a new shared primitive
   unless a second string-typed enum appears elsewhere; check before adding one to
   `JsonPrimitives_IO.h`).
5. Wire into `BuildSanmapJsonText`/`ParseSanmapJsonText` — top-level, unconditional, same tier as
   `armies`/`areas`/`markers`/`chains`/`props`/`decals` (before the `mapGeneratorData` gate on
   import, per this session's established wiring-order lesson — these are format-native top-level
   keys, not `mapGeneratorData` content).
6. Retire both stale SCOPE NOTE/comment blocks per "Target files" above.

## Explicit out-of-scope
- **UI wiring** — `Ui::AtmosphereSettings` stays exactly as-is; `AtmosphereTab_UI`/whatever draws
  it is not retyped onto `Params::Atmosphere`. Same posture as every prior PARAMS promotion.
- **Any PROC/PIPELINE consumer** of atmosphere settings — none exists today (per the spec, no
  stage reads these values yet); this ticket only gives them a durable, round-tripping home.
- **The widget-table addressing helpers** (`AtmosphereColorAt`, slot enums, etc.) — explicitly
  ruled UI-only, not ported, per the spec's own "Not promoted" section.

## Acceptance test
Extend the round-trip fixture (`MapImporter_IO_Test.cpp`'s `BuildPopulatedRecipe`/
`RunRoundTripTests`) with a populated `Params::Atmosphere` (non-default values across all eight
sub-structs, including a non-`Exposure` `skyboxIntensityMode`), asserting exact survival through
export→import — specifically covering the `sunCookiePath`/`skyboxPath` wrapper-object shape, the
`{r,g,b,a}` color fields, and the string-typed `skyboxIntensityMode` round-tripping correctly
(including an unrecognized string on import falling back to `Exposure` with a logged warning, not
a crash). Full `SanGenV2` build stays clean.
