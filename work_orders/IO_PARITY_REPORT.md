# IO Parity Report — v1 / C# / v2 import-export comparison + implementation plan

*Read-only audit. No program code was changed. Compiled from `core/` (v1 C++),
`Sanctuary-Map-Generation-develop/` (C# reference), `src/io/` + `src/params/` + `src/ui/` (v2),
the arch pack, and a full Format-Expert audit of all three. Extends `PARITY_BACKLOG.md` and
`RECIPE_PARITY_BACKLOG.md` with the format-side evidence those two do not carry.*

---

## 0. The one-paragraph answer

v2's importer/exporter is **structurally correct and functionally a stub**: it writes 17 of the
format's ~90 top-level keys and 7 of each stratum's 16, reads **7**, and handles **zero** entity
domains. The v2 split (Recipe / Layers / Rules / Fields) is sound and should be kept — this is a
filling job, not a redesign. But three findings are *not* "unfinished", and two of them ship a
broken file today:

1. **v2 writes two fields with the wrong JSON type.** `maskRemapMin`/`maskRemapMax` are written as
   bare floats where the format declares `Vector4`, and `height` is written as a float into an
   `int` field. The first is a hard type mismatch in every `.sanmap` v2 produces.
2. **v2's importer ignores the format's own fields.** It reads `width`/`height`, then jumps to
   `mapGeneratorData`. A map authored by the game or the C# tool — neither has that block —
   imports as dimensions and nothing else, with only a logged warning.
3. **The coordinate flip is wrong in v1 at all six sites**, and v2 has not yet copied it. Free fix.

**A correction to the received picture:** v1's `mapGeneratorData` block is **write-only**. v1 writes
the whole generator state into the `.sanmap` and reads exactly one key back (`Aliases`). The
"Open Generator File" action is a *separate* `.json` path that is never fed the block. So the
CRITICAL round-trip the spec names has never been implemented in either generation — v2's version
of it is the first working one, and is already healthier than v1's.

---

## 1. Ground truth — which C# is authoritative

The C# repo contains **two** serializers and only one produces the shipped file:

- `ExtraneousMapGen.map.SanMap` + `SanMapConverter` → writes `mapdef.sanmap`
  ([SanMapExporter.cs:58](Sanctuary-Map-Generation-develop/src/map/exporter/SanMapExporter.cs:58))
- `EM.Map.SanMap` built by `MapUtils.GetSanMap` → written as `<mapName>.sanmap`, **and then
  `mapdef.sanmap` is deleted** ([MapGenerator.cs:147](Sanctuary-Map-Generation-develop/src/map/generator/MapGenerator.cs:147))

**`EM.Map.SanMap` (`Sanctuary/SanMap.cs`, `SanMap.Types.cs`, `Types.cs`) is the ground truth.**
Everything in `src/map/*.cs` — `Prop`, `Decal`, `SkyBox`, `CubeMap`, `WaveGenerator`, `AIMarker`,
`Spawn`, `Army`, `MapParameters` — is a legacy internal model that **never reaches the file**.

This is load-bearing: **skyboxes-as-objects, cubemaps, wave generators and AI markers are not in
the format at all.** Every rich field on them is `[JsonIgnore]`. The real format has `skybox` as
`TextureLoader{path}` and one `waterShoreGeneratorBlueprint` **string**. Do not build UI for them.

---

## 2. Volume comparison

| | v1 C++ | v2 |
|---|---|---|
| Import | `core/MapImporter.cpp` 880 L + `SupComImporter.cpp` 197 L | `src/io/MapImporter_*.cpp` **428 L** |
| Export | `core/export/Export_Metadata.cpp` 1193 L + `Export_Textures.cpp` 139 L | `src/io/MapExporter_*.cpp` **487 L** |
| Top-level format keys written | ~85 of ~90 | **17** |
| Top-level keys read | ~120 | **7** + `width`/`height` |
| Stratum fields written (of 16) | 16 **+ 7 illegal extras** | **7** |
| Entity domains handled | 4 of 6 (no `chains`; `props` dead) | **0** |

---

## 3. Field-by-field gaps

### 3.1 Format-level

| Block | Ground truth | v1 | v2 |
|---|---|---|---|
| `fileVersion` / `mapVersion` | ✔ | write ✔ / **read ✘** | write ✔ / read ✘ |
| `name` / `credits` | ✔ | **clobbered / hardcoded** (§4 D-A) | write ✔ (from options) |
| `width` / `length` / `height` / `heightmapResolution` | ✔ | `length` never read; **`height` hardcoded 128** | `length = mapSize`; **`height` written as float into an int** |
| `hasWater` / `waterLevel` / `waterDepth` | ✔ | **`hasWater` hardcoded true** | write ✔ / read ✘ |
| water wind + shore (7) + `waterShoreGeneratorBlueprint` | ✔ | ✔ both ways | **✘ both ways** — `Params::Water` has 4 fields |
| `shader` / `heightTransition` / `fadeDistance` / `fadeStartDistance` | ✔ | hardcoded | hardcoded / read ✘ |
| `stratumLayers[9]` × 16 fields | ✔ | 16 ✔ **+ 7 non-format keys injected** | **7 of 16; texture paths written empty** |
| sun\* / skylight\* / skybox\* / exposure\* (~21) | ✔ | ✔ both ways | **✘ both ways** |
| background\* / heightFog\* / linearFog\* / fog\* (~25) | ✔ | ✔ both ways | **✘ both ways** |
| `windSpeed` / `windDirection` | ✔ | ✔ | **✘ both ways** |
| `areas` | ✔ | ✔ | empty |
| `armies` (recursive groups/units) | ✔ | ✔ import; **wrong defaults + key renaming** on export | empty |
| `markers` | ✔ | position only — **rotation & scale dropped**; **invents `Plasma`/`Hydro`** | empty |
| `chains` | ✔ | **always empty** — silently dropped | empty |
| `decals` | ✔ | opaque-string passthrough; generated decals never export | empty |
| `props` | ✔ | **built then discarded** — dead | empty |
| `Textures/tint_colors.tga`, `tint_geometry.tga` | ✔ | ✔ | **✘ — regression vs v1** |

`STRATUM_COUNT = 9` is a **convention, not an invariant** — the C# writer assigns the biome's array
verbatim with no count enforcement, and the field default is a zero-length array. v2 hardcodes 9 in
both directions; the importer must tolerate ≠9.

### 3.2 The stratum block — the cheapest large win

v2 omits `tileSizeTriplanar`, `tileSizeFarTriplanar`, `normalScale`, `normalScaleFar`,
`normalFarNearBlend`, `heightFarNearBlend`, `farColorRemap` — **all seven already exist in
`Params::StratumAppearance`**. It also writes `albedo`/`normal`/`mask` paths as empty strings while
`StratumAppearance` holds the real ones, so **every map v2 exports is untextured**.

`Params::StratumAppearance` (16 fields) and `Params::StratumSoilPhysics` (6) are populated by the
Stratums tab and read by `Bake_PROC` / `Erosion_PROC`, but are **not members of
`Params::MapRecipe`** — so they are lost on every save/load cycle as well. Pure wiring debt, no new
types needed.

### 3.3 `mapGeneratorData`

v2's block is **the first working round-trip in the project**: key-for-key mirrored, versioned,
total and defaults-preserving on read, with safety caps and invariant repair — and, correctly, it
transmits **rules**, not baked instances. v1 inverted exactly that: it dropped `MarkerRule`s,
`PlacedMarkerLayers`, `Areas` and `ManualPropLayers` entirely, while serializing resolved
`MarkersList` positions and baked unit transforms.

But the two dialects are **incompatible and ungated**. v1 keys: `Fractal`, `Blend`, `UseImage`,
`ImagePath`, `Erosion{…16…}`, `PhysicsTag*`. v2 keys: `NoiseType`, `FractalType`, `BlendMode`,
`Lacunarity`, `WeightedStrength`, `Locked`. `MapGeneratorDataVersion` is written but never read —
[MapImporter_Recipe_IO.cpp](src/io/MapImporter_Recipe_IO.cpp) has no version branch, so a v1 block
degrades **silently** to defaults.

Present in v1's block, absent from v2's: `PresetVersion`, `GamedataPath`, `GlobalEnvironmentPath`,
`Aliases`, `GlobalGravity`, `DetailNormalMapSize`, `SpawnPointCount`, `HydroMultiplier`,
`ReclaimDensity`, `MexDensity`, `SymAlgorithm`, `SymSuperpositionBlend`, `SymmetryBlurRadius`,
`CrossFadeWidth`, `CylinderZScale`, `TorusMajorRadius`, `TorusMinorRadius`,
`SymmetryDetectionTolerance`, `SnapImperfectSymmetry`, `FlowSettingsParams`, `SlopeSettingsParams`,
`MarkersList`, the marker colour/scale/icon globals, `Armies`, `Atmosphere`, the six GPU/preview
toggles, `FlowMapColor`, `FastPreviewMode`.

### 3.4 C#-only concepts

`numTeams`, `hydroCount`, `unexplored`, `blind`, `tournamentStyle`, `mapDetailSize`/`DetailScale`,
and `SymmetrySettings{TerrainSymmetry, TeamSymmetry, SpawnSymmetry}` (three independent symmetry
axes, where SanGen has one global mask). **Product decisions, not parity gaps** — flag, don't
schedule.

---

## 4. Correctness defects — wrong, not merely missing

### Shipping in v2 today

**V-1 · `maskRemapMin`/`maskRemapMax` written as scalars where the format declares `Vector4`.**
[MapExporter_Recipe_IO.cpp:26](src/io/MapExporter_Recipe_IO.cpp:26). The game's reader expects
`{x,y,z,w}` and gets a bare float. Hard type mismatch in every exported map.

**V-2 · `height` written as a float into an `int` field.**
[MapExporter_Recipe_IO.cpp:66](src/io/MapExporter_Recipe_IO.cpp:66). `128.0` survives Newtonsoft
coercion; a fractional `terrainMaxHeight` — a legal PARAMS value — will not.

**V-3 · Stratum `appearance` + `soilPhysics` lost on every round-trip.** §3.2.

**V-4 · `length` is never read; width is assumed to equal length.** Affects v1 and v2. Breaks the
one known non-square official map (1023×1024) and makes the flip divisor wrong by construction.

**V-5 · `ScatterTransform::templateIdentifier` is `char[8]`.**
[ScatterTransform_PARAMS.h:23](src/params/ScatterTransform_PARAMS.h:23). Correct for units (a tpId
is 7 chars). Wrong for props/decals, whose identity is a `blueprintPath` — Pandemonium ships flat
`.sanprop` files named e.g. `CrystCluster_B1` (15 chars, no code form). It also bakes in the
tpId→path synthesis that `SPEC-1` Correction 2 explicitly forbids.

**V-6 · `heightmap.raw` writer is host-endian, reader is explicit little-endian.** Correct on x86;
an asymmetry between a matched pair.

**V-7 · Tint TGAs not written** — `tint_colors.tga` / `tint_geometry.tga`, a v1 regression.

### In v1 — do not port these forward

**D-1 · The coordinate flip is wrong at all six sites.** Ground truth (`MapUtils.cs:208`) is
`world.z = length - z - 1`. v1 uses `MapSize - z`:
export [Export_Metadata.cpp:257,295,385](core/export/Export_Metadata.cpp:257);
import [MapImporter.cpp:295,487,492,595](core/MapImporter.cpp:295).
Two defects — the **missing `-1`**, and **`MapSize` where the format uses `length`**. Symmetric, so
SanGen→SanGen round-trips hide it; every entity is one world unit off against the engine and
against any imported official map.

**D-2 · Props are silently destroyed on export.** `propsArr` is fully built, then overwritten with
an empty array at [Export_Metadata.cpp:427](core/export/Export_Metadata.cpp:427).

**D-3 · `height` hardcoded to 128** on export while the importer reads the real value. Import a
1600-height map, export it flattened 12.5×.

**D-4 · `hasWater` hardcoded `true`** — ground truth derives it as `waterLevel > 0`.

**D-5 · Marker rotation and scale are dropped and replaced with defaults.** Not unimplemented —
actively lossy: read as nothing, written back as identity/one.

**D-6 · Wrong army defaults on the fallback path** — `faction=1, alloys=1000, energy=1000` vs the
ground truth `0/500/500`.

**D-7 · Spawn transform keys renamed `ARMY_n` → `Army_n`.** Shipped maps use `ARMY_n`, and the
importer's own comment warns that renaming keys makes the editor throw — then the exporter renames
them anyway.

**D-8 · Non-format keys injected into the game's `stratumLayers`** (`previewColor`, `maskMode`,
`hardness`, `friction`, `cohesion`, `capacityMult`, `absorptionRate`), duplicating what already
goes into `mapGeneratorData.Stratums`.

**D-9 · Invented marker types.** `Plasma`/`Plasmas`/`Hydro` written with `resource:true`. They exist
in neither the C# writer nor any surveyed shipped map — only `Spawn` and `Alloys` do.

**D-10 · `markerScaleFactor` is an invented, inconsistently applied rule.** Derived from a
PlayableArea/mapSize mismatch, it scales marker X/Z and pre-scales prop/decal X/Z — but the flip
then subtracts the *un-scaled* `MapSize`, and marker Y is never scaled. Nothing in the format
authorizes it.

**D-11 · SupCom import applies no coordinate flip at all**, so SupCom markers are Z-mirrored
relative to `.sanmap`-imported ones. Plus zero input validation (unbounded `stof`, no size cap) —
Constitution §6 violation.

**D-12 · Decals never exist as data** — held as an opaque JSON string, so scale, symmetry, preview
and editing all skip them, and *generated* decals can never export (dead `if (!empty())` on a
just-emptied array).

**D-13 · Army colours are index-assigned via a function-local `static int` that never resets** —
colours drift across successive loads in one session.

### Blocking prerequisite

`SPEC-1`, `SPEC-2` and `SPEC-3` are all marked *evidence complete, corrections **NOT** applied*.
An importer written against the current `UNIT_PROP_MARKER_DATA_SPEC` / `GAMEDATA_LAYOUT_SPEC`
**will 404 on 41+ shipped props**. And per `SPEC-1` Correction 3: **one unresolvable
`blueprintPath` aborts the remainder of map load**, silently taking the `markers` block with it —
the symptom is "my alloy points disappeared". Constitution §6 therefore requires the exporter to
resolve every blueprintPath against the real pack before writing. That is the single
highest-value validation in the IO layer, and it does not exist.

### Rotation — what to inherit, and what not to

The C# repo *does* contain a working Euler→quaternion writer (`src/util/Vector3.cs`), used only by
the discarded `mapdef.sanmap`. Inherit its semantics: **Y-yaw, with a decal-only +90° X
pre-rotation** (props opt out explicitly). Do **not** inherit its bug — it passes the Y angle as
radians with `* DegToRad` commented out.

---

## 5. UI coverage — "every variable in the file has an input"

The definitive checklist is **~90 authorable top-level fields + 16 per stratum × 9 ≈ 234**, plus
per-entity fields. v1 exposes most of the top-level set; **v2 exports 17**.

The v2 tab rebuild did its job — the input surface is nearly complete. The failure is one layer
down: settings land in `ApplicationHostedSettings` or tab-local state and never reach
`Params::MapRecipe`, so they never reach the file.

**A. Has a UI input, no recipe home → does not serialize**
- Atmosphere — all 49 values ([Application_HostedSettings_UI.h](src/ui/Application_HostedSettings_UI.h))
- Detail-Normal / Tint / Holes / Smoothness stacks — 4 × `Params::LayerStack`
- `SymmetryDetection` {tolerance, snap}
- Areas (no `Params::MapArea`), Armies (no `Params::Army`)
- Stratum **appearance** (16) and **soil physics** (6) — types exist, not in `MapRecipe`
- Water level *minimum*; overlay toggles; environment-pack path; detail-normal size
- Accumulation per-stratum settings

**B. In the file, no UI input anywhere — v1 or v2**
- `HydroMultiplier`, `ReclaimDensity`, `MexDensity` — v1 wrote all three and exposed **none**
  (confirmed: zero references in `gui/`). No v2 home either.
- `SpawnPointCount` — read only by `Widget_MapCanvas.cpp`; no editor control.
- `shader`, `heightTransition`, `fadeDistance`, `fadeStartDistance` — constants at the write site.
- Stratum slope-gate fields — consumed by the Mask stage, reachable from no UI.
- `name`, `credits` — v1 clobbers/hardcodes both.

**C. Had a UI input in v1, none in v2**
- Symmetry algorithm group — `SymAlgorithm`, `SymSuperpositionBlend`, `SymmetryBlurRadius`,
  `CrossFadeWidth`, `CylinderZScale`, `TorusMajorRadius`, `TorusMinorRadius`. No v2 fields **and**
  no heightfield-symmetry stage — whole feature.
- Water wind/shore group + `WaveGeneratorBlueprint`.
- Durable `GlobalGravity`; `DetailNormalMapSize` (both v2 tab-state only).

**D. Derived / engine-internal — must NOT get an input**
`heightmapResolution` (= mapSize+1), `fileVersion`, `width` (from mapSize), `MapGeneratorDataVersion`,
`shader`, marker-type keys (`Spawn`/`Alloys` are a fixed set), `MarkerType.resource` (derived from
type), and every `Data::` field.

**E. Not in the format — do NOT build UI for these**
Skybox sub-fields (horizon/zenith colours, planets, cirrus), cubemaps, wave-generator objects, AI
markers, `NoRushRadius`, `CutOffLOD`, the `DecalType` enum, cartographic colours. All `[JsonIgnore]`
or unreachable legacy (§1).

---

## 6. Plan

Ordered "wrong before incomplete", per Constitution §6. Each step is independently shippable.

### Step 0 — Apply SPEC-1 / SPEC-2 / SPEC-3 *(ARCH Expert, docs only)*
Blocking. Three spec claims are known-false and every prop/asset decision below is written against
them. Zero code. Within it: Correction 3 (map-load abort) → Correction 2 (folder naming).

### Step 1 — The two shipping type bugs *(IO, ~4 lines)*
`maskRemapMin`/`maskRemapMax` → `Vector4`; `height` → int. **V-1, V-2.** Do this first; it is the
only change that fixes files v2 is producing right now.

### Step 2 — `SanmapCoordinates_IO` *(IO)*
One function pair — `world.z = length - z - 1` and its inverse — plus one Euler↔quaternion pair
(Y-yaw, +90° X for decals, **in degrees**). Everything downstream depends on it. v1's six
independent copies are exactly how the `-1` got lost six times. **D-1, D-11.**

### Step 3 — Stratum appearance + soil physics wiring *(PARAMS + IO)*
Add both to `MapRecipe`; serialize into `stratumLayers` (the 7 missing format keys **and** the
three texture paths) and into `mapGeneratorData.Stratums`. No new types. Highest value/effort ratio
in the plan. **V-3**, and it makes exported maps textured for the first time.

### Step 4 — Read the format's own fields *(IO)*
`length`, `heightmapResolution`, `hasWater`, `waterLevel`, `waterDepth`, `shader`,
`heightTransition`, `fade*`, and the full `stratumLayers` block — so a **game-authored map imports
meaningfully**, which it currently does not. Tolerate `stratumLayers.length ≠ 9`.
`mapGeneratorData` keeps priority where both are present.

### Step 5 — `mapGeneratorData` version gate *(IO)* — **needs decision #1**
Read `MapGeneratorDataVersion`; absent ⇒ v1 dialect. Then either translate
(`Fractal`→`FractalType`, `Blend`→`BlendMode`, per-layer `Erosion`→per-stratum
`StratumSoilPhysics`, drop `UseImage`/`ImagePath` with a logged warning), or **reject loudly**.
Note that v1 never read its own block back, so "restore an old map" was never a working feature —
this is a rescue mission, not a regression fix. Silent degradation to defaults, the current
behaviour, is the worst of the three options.

### Step 6 — `Params::Atmosphere` + extended `Params::Water` *(ARCH Expert → IO)*
Promote `Ui::AtmosphereSettings` verbatim (its header already states the move is mechanical) and
add the 8 missing water fields. Then `MapExporter/Importer_Atmosphere_IO` and `_Water_IO`,
serializing to the format's **own** lowercase blocks, not into `mapGeneratorData`. Closes ~53 keys
and unblocks the UI-coverage audit for §5.A. Note `skyboxIntensityMode` is a **string** enum.

### Step 7 — Entity IO *(ARCH Expert → IO)* — the largest structural hole
The v2 recipe is **rules-only**; the format is **instances-only**. A PARAMS home for manually
authored entities is a genuine gap in the law — route it to the ARCH Expert, do not let IO invent
it. Prerequisites: Steps 0 and 2, and **V-5** (`blueprintPath` string on Prop/DecalRule).
- Entity Y is absolute world units; sample terrain per `SPEC-1`'s validated formula
  (`row = z`, `col = (N-1) - x`, `y = bilinear × height / 65536`).
- **Blueprint-path validation before write is mandatory.** Refuse the export and name the path.
- Domains in ascending risk: `areas` → `armies` → `markers` → `chains` → `decals` → `props`.
- Fixes **D-2, D-5, D-12** and closes both of `SANMAP_FORMAT_SPEC`'s named fix-targets.

### Step 8 — Remaining PARAMS promotions
`SymmetryDetection`; the four mask `LayerStack`s; `Params::MapArea`; `Params::Army`; independent
`length` (**V-4**, needs decision #2).

### Step 9 — UI inputs for §5.B and §5.C
Controls for the four texturing constants and the stratum slope-gate fields (data already exists —
cheapest wins in the plan). Decide `HydroMultiplier` / `ReclaimDensity` / `MexDensity` /
`SpawnPointCount`: promote with controls, or drop from the format. They have been
written-but-unreachable for the whole life of v1 — do not port that state forward.

### Step 10 — `MapExporter_Tints_IO` (**V-7**), then SupCom import as a validated v2 module
The Files tab already drives an **unbound** SupCom seam. Small, self-contained, low priority.

### Acceptance gate for every step
Extend `MapExporter_IO_Test` / `FilesTab_Roundtrip_UI_Test` with a **field-count assertion**:
export a fully-populated recipe, re-import, assert every promoted field survives. Today
`TestDocumentCarriesTheFormatsOwnFields` checks the document *has* the keys; it never checks that
importing gives the recipe back. That asymmetry is why the gaps went unnoticed.

---

## 7. Decisions needed from you

1. **Must v1-saved maps load in v2?** Gates Step 5 and the size of Steps 6–8.
2. **Non-square maps** — support `length ≠ width`, or fix square and simplify? (One shipped
   official map is 1023×1024.)
3. **Stratum mask image size** — v1 and v2 both write `mapSize²`; the C# reference writes
   `(detailSize+1)²`. The engine decides this one and there is no `.sanmap` in this repo to
   measure against. **Needs a real map or an engine answer.**
4. **`HydroMultiplier` / `ReclaimDensity` / `MexDensity` / `SpawnPointCount`** — promote with UI,
   or drop from the format?
5. **`Plasma` / `Plasmas` / `Hydro` marker types** — a real forward-looking engine feature, or a v1
   invention to delete? (`resourceSpot.lua` has energy/hydro commented out.)
6. **Symmetry algorithm group** — is Fold/Blur/CrossFade/Superposition/Cylinder3D/Torus3D a real v2
   feature? It needs a whole heightfield-symmetry stage, not a field.
7. **`mapVersion`** — user-bumpable, or a constant?
8. **C#-only concepts** (`numTeams`, `hydroCount`, `unexplored`, `blind`, `tournamentStyle`,
   three-axis symmetry, `mapDetailSize`) — in scope for SanGen, or out?
