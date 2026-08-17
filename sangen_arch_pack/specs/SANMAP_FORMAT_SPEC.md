# SANMAP_FORMAT_SPEC — the .sanmap file format

Source of truth: the `EM.Map` C# format definition (`SanMap.cs`,
`SanMap.Types.cs`, `Types.cs`, `MapUtils.cs`, `Colors.cs`). The IO / BRIDGE
layer's import/export must target this exactly.

## Container
- JSON (Newtonsoft.Json). Extension `.sanmap`. Current `FORMAT_VERSION = 3`.
- Fixed `STRATUM_COUNT = 9` texture layers.
- Version fields: `fileVersion`, `mapVersion`.

## Top-level map fields
- **Base:** name, credits, width, length, height.
- **Shape:** heightmapResolution.
- **Water:** hasWater, waterLevel, waterDepth, wind (speed / direction /
  shoreWavesRemap), shore depth+distance offsets/strengths,
  waterShoreGeneratorBlueprint.
- **Texturing:** shader (default `RTS/TerrainLit`), heightTransition,
  fadeDistance, fadeStartDistance, `stratumLayers[9]` (Stratum).
- **Background / fog:** background*, heightFog*, linearFog* blocks.
- **Lighting:** sun* (RA, DA, intensity, tint, temperature, angularDiameter,
  volumetrics, position), skylight*, skybox* (TextureLoader + intensity mode),
  fog* volumetrics, exposure.
- **Global wind:** windSpeed, windDirection.
- **`Params::Atmosphere` (`ATMOSPHERE_PARAMS_SPEC`)** is the C++ recipe home
  for the whole Lighting / Background-fog / Global-wind field set above
  (sun/skylight/exposure+skybox/legacy fog/background fog/height fog/linear
  fog/global wind) — a field-level promotion, not a new schema section.

## Entity collections (the distinct domains)
- `areas`: Dictionary<string, Area{ x, y, width, height }>
- `armies`: Dictionary<string, Army{ faction 0/1/2, alloys, energy, groups,
  armyColor, alias }>
  - `UnitGroup { units: Dict<string,UnitTransform>, groups: nested UnitGroup }`
    — recursive grouping
  - `UnitTransform : InstancedTransform { type, tpid }`
  - **`armyColor` and `alias` are SanGen-added fields (schema v3, Correction 11
    below)** — lowerCamelCase because they merge into this existing
    format-native dictionary rather than forming a new SanGen-owned section
    (ARCH §1.6).
- `markers`: Dictionary<string, MarkerType{ resource:bool, transforms:
  Dict<string,MarkerTransform{ ..., alias }> }>
  - **`alias` is a SanGen-added field (Correction 11)** — same lowerCamelCase
    rule as `armyColor` above.
- `chains`: Dictionary<string, MarkerChain.Marker[]{ type, name }>
- `decals`: DecalType[]{ blueprintPath, transforms: DecalTransform[] }
- `props`: PropType[]{ blueprintPath, transforms: PropTransform[] }

## Shared transform
`InstancedTransform { Vector3 position, Quaternion rotation, Vector3 scale }`.
Vector2/3/4, Quaternion, Color are plain float structs (`Types.cs`).

## Stratum (one texture layer, ×9)
name; albedo / normal / mask TextureLoader{ path }; tileSize / tileSizeFar
(Vector2); triplanar tile sizes; normalScale (+Far); normal/height
farNearBlend; diffuseRemap / farColorRemap (Color); maskRemapMin / Max (Vector4).

## Notes for the ARCH
- **Reclaim is not a format concept** — `resource` is a flag on `MarkerType`
  (resource markers). Confirms the code survey.
- **Distinct top-level domains confirmed:** Areas, Armies/Units, Markers,
  MarkerChains, Decals, Props — each is a candidate for its own spec.
- Units use `type` + `tpid` (template id); Props/Decals use `blueprintPath`;
  markers/units are keyed by string name in dictionaries.
- Several lighting/fog fields are marked "Random, unknown value" in source —
  confirm real defaults against the official maps.

## Conversion / import-export logic (MapUtils.cs, Colors.cs)
- **Coordinate flip (critical):** internal map data is in texture coords
  (origin top-left); Sanctuary markers/entities use world coords (origin
  bottom-left). Transform: `world = (x, y, length - z - 1)`
  (`TextureToWorldOrigin`). Export applies it; import must invert it.
- **Entity position encoding (confirmed by dev):** entity transform positions
  (props/units/markers) are **absolute world units = game units** — a prop's Y is a
  world-space elevation, stored and used directly (no normalization at the format
  level). The map's `height` (e.g. 128) is the **vertical extent of the terrain**, so
  an entity with **Y > height sits above every terrain point** on the map (a prop at
  Y = 200 on a height-128 map floats above all terrain). The hardcoded `128` in
  `MASKING_SPEC` / `PLACEMENT_SCATTER_SPEC` is the terrain's vertical scale and must
  be **read from the map's max height, not baked**.
  - *Authoring aside:* if a UI exposes a normalized 0–1 height, it converts to world
    units by `fraction × maxHeight` (0.5 × 50 = 25) — but the value stored/used is the
    resulting world-unit Y, not the fraction.
  - *Not coordinate math:* the "1 game unit ≈ 10 meters" ratio is an **arbitrary
    authoring convention** for sizing a unit's *scale* (a unit the team calls ~10 m
    tall gets scale ≈ 1). It never enters position math; the engine only uses game
    units.
- Marker/area names are fixed constants: `Spawn` (resource=false), `Alloys`
  (resource=true), area `PlayableArea`.
- Export defaults: each army written as `faction=0, alloys=500, energy=500`,
  empty `groups`; spawns→`Spawn`, mexes→`Alloys` (with duplicate-id guard).
- **Known gaps in the current exporter (fix targets):** (1) rotations are NOT
  converted — spawns/mexes/decals/props all write an identity quaternion (TODO);
  (2) **props export is disabled** — commented out because "many prop formats
  are outdated, causing maps to fail loading."
- **Faction / army colors (Colors.cs):** EDA = dark green (0.078,0.329,0.196),
  Chosen = dark red (0.412,0.008,0.008), Guard = golden amber
  (0.690,0.549,0.188); plus a 32-entry `ArmyColors[]` palette + named
  `ColorLookup`. Used for army/marker coloring in the UI.

## Validated against an official map (~TEAM-1v1_Tropical_256)
- Confirms: fileVersion 3, mapVersion 1; width=length=256, height=128;
  `heightmapResolution = 257` (N+1); exactly 9 `stratumLayers`.
- A map on disk is a folder: `<name>.sanmap` + `preview.png` + `Textures/`.
- **Terrain data is NOT in the JSON.** The `.sanmap` holds metadata + entities
  only; the heightmap and stratum masks live as files in the `Textures/`
  folder. Import/Export must handle both the JSON and the texture set.
- Markers seen: `Spawn` (resource=false, keyed per army: `ARMY_1`/`ARMY_2`)
  and `Alloys` (resource=true — these are mexes, keyed `Mex N`). Symmetry is
  encoded in the transform name suffix (`Mex 0 sym 0`).
- `armies` keyed `ARMY_1..N`, faction int, alloys/energy floats; `groups` was
  empty here (units come via markers in this map).
- `areas` has a `PlayableArea` spanning the full map; `decals` and `props`
  empty in this map.

## Feature variation across ~23 official maps (schema survey)
- **Survival difficulty variants are structurally identical** (byte-for-byte
  near-identical: same props, markers, armies). Difficulty lives elsewhere, not
  in the map structure — read one per family, not all six.
- **Props** appear heavily in real maps (Forge 63.5k instances / 39 blueprints;
  White_Desert 29k; Two_Step 22.5k; There_Is_Time 4.9k). Blueprints are
  `.santp` paths; each transform is position/rotation/scale.
- **Decals** in There_Is_Time (4 types, 76) and Two_Step (1). Blueprints are
  `.sandecal`; DecalType = { blueprintPath, transforms }.
- **Chains** only in Two_Step (`FirstChain`, 3 elems, `{type:'Alloys',
  name:'AlloyMarker'}`) — confirms MarkerChain.
- Marker types across all maps are just `Spawn` (res=false) + `Alloys`
  (res=true = mexes). Sizes 256–2048; height up to 1600 (Forge); one non-square
  map (1023×1024). Army keys casing varies: `ARMY_1` vs `Army_1`; faction 0 or 1.
- Armies show `groups` present but no units found by traversal — verify the
  army→group→unit nesting against a map that actually ships placed units.

---

## SanGen-owned sections (schema v3) — supersedes `mapGeneratorData`

Ratified by work-order `SPEC-4` (`work_orders/SPEC-4_SanmapSchemaV3_DOCS.md`;
evidence in `work_orders/IO_PARITY_REPORT.md` §3.3, §6 Step 5). The old
`mapGeneratorData` blob (~40 keys, ~60% pure duplicate of the format's own
fields, two mutually incompatible dialects squatting on the same key, no
working version gate) is **deleted**. In its place, SanGen's generator state is
split into independently versioned, **top-level** JSON keys that sit as
siblings of the format's own fields — never nested under one container key.

**Casing law (ARCH §1.6, Correction 0):** camelCase top-level key = game-native
field (`width`, `armies`, `markers`, …); PascalCase top-level key = SanGen-owned
section (single-token PascalCase, no spaces — `GeneralMapSettings`, never
`"General Map Settings"`). A field merged into an *existing* format-native
collection keeps lowerCamelCase to match its siblings (`armyColor`, not a new
section) — see Correction 11 and the Entity collections note above.

### `SanGenVersion` (int) — Correction 1
Replaces `MapGeneratorDataVersion`/`mapGeneratorDataVersion`
(`MapExporter_IO.h:75`). Bumped to **3** — a breaking dialect change from both
the v1 (`Fractal`/`Blend`/`UseImage`/`ImagePath`/`Erosion{…16…}`/`PhysicsTag*`)
and the current-v2 (`NoiseType`/`FractalType`/`BlendMode`/`Lacunarity`/
`WeightedStrength`/`Locked`) dialects that both wrote `mapGeneratorData`.
**The importer must actually gate on this field** — unlike today, where
`MapGeneratorDataVersion` is written but never read by any importer in either
generation (`IO_PARITY_REPORT.md` §3.3, Step 5). An absent or old version must
produce a **loud, logged fallback** — never a silent default (Constitution §6).

**The mechanism that does this gating is `IO_MIGRATION_SPEC`** — the
per-(domain, version-step) migration files, the one-file `Sanmap_MigrationManifest_IO`
wiring point, and the `Sanmap_MigrationRunner_IO` that resolves a document's
version, walks it forward, and refuses outright on a version newer than this
build understands. This section states *what* changed at v3; `IO_MIGRATION_SPEC`
states *how* an old document is carried forward to it (and how every future bump
repeats the pattern).

### `GeneralMapSettings` — Correction 2
```
Seed                    (confirmed live, MapExporter_Recipe_IO.cpp)
ScaleFeaturesToMapSize  (confirmed live)
GlobalGravity           (NEW — currently tab-local UI state, HeightmapTab_UI.h:70,
                         explicitly unserialized; genuine new PARAMS field, not
                         a relocation)
TerrainMinHeight        (confirmed live, MapExporter_Recipe_IO.cpp:38)
WorldUnitsPerCell       (confirmed live, MapExporter_Recipe_IO.cpp:41)
```
`TerrainMinHeight`/`WorldUnitsPerCell` are fields v2 already round-trips today;
their absence from the first draft of this schema was a documentation gap, not
a design decision. `Seed`/`ScaleFeaturesToMapSize`/`TerrainMinHeight`/
`WorldUnitsPerCell` are pure relocations into this section; `GlobalGravity` is
new work for the coder tier.

### `HeightmapStack` — Correction 3
Replaces `GeoLayers`. Structural model **confirmed against `LAYER_SYSTEM_SPEC`,
not redesigned here**: a flat, ordered `LayerStack` of `GeoLayer` (composition
bands), each a flat, ordered stack of `Layer` (Material or Simulation type).
**Neither level nests or groups.** `SimulationGrouping`
(`Params::LayerStack::simulationGrouping` — the Separate/Unified sim-mode
toggle, `LAYER_SYSTEM_SPEC` "Sim mode") nests **inside** this key instead of
floating as a stray top-level sibling, which is where v2 currently writes it
(`MapExporter_Recipe_IO.cpp:43`).

**Named gap, explicitly deferred:** the real map's per-layer
`MinHeight`/`MaxHeight`/`MinSlope`/`MaxSlope` height-and-slope gates (confirmed
present at `GeoLayers.Layers[]` in the live file) have **no equivalent field on
v2's current `Layer` PARAMS at all** — silently dropped in the v1→v2 port, not
merely carried over. The internal layer redesign is out of scope for this
work-order (see `LAYER_SYSTEM_SPEC`'s companion note); v2's current `Layer`
field set carries through this schema unchanged, and this gap is logged for
that future conversation.

**New field, in scope now:** `GeoLayer` and `Layer` each gain a local
`bSymmetryUseGlobal` + `symmetryMask` override, matching the pattern already
live on every placement rule type — see `Symmetry` below (Correction 4).

### `Symmetry` — Correction 4
Global section:
```
SymAlgorithm            SymSuperpositionBlend   SymmetryBlurRadius
CrossFadeWidth           CylinderZScale          TorusMajorRadius
TorusMinorRadius         SnapImperfectSymmetry   SymmetryDetectionTolerance
GlobalSymmetryMask
```
Per-rule/per-layer local override — `bSymmetryUseGlobal` + `symmetryMask` — is
**confirmed already live and tested** on `MarkerRule`/`PropRule`/`DecalRule`
(`PlacementRules_PARAMS_Test.cpp:16`); those three carry a pure relocation of
existing fields under this section's umbrella, not new work. **New** for
`HeightmapStack`'s `GeoLayer`/`Layer` (Correction 3).

**Ruled, not deferred:** heightmap symmetry is wanted immediately, in full —
both the basic axis mechanism (`Params::SymmetryAxis` mirror/rotate) **and**
the exotic `SymAlgorithm` group (Fold/Blur/CrossFade/Superposition/Cylinder3D/
Torus3D). No v2 code implements a heightfield-symmetry PROC stage today
(`IO_PARITY_REPORT.md` Decision #6). **This work-order reserves and
round-trips the settings and adds the new override field to `GeoLayer`/`Layer`
only.** Designing and building the actual heightfield-symmetry PROC stage is
real, near-term, out-of-scope work for a separate generator-expert/ARCH
work-order.

### `SlopeDefaults` — Correction 5 (load-bearing PARAMS-shape ruling)
```
bSlopeGateEnabled       minimumSlopeDegrees      maximumSlopeDegrees
slopeFeatherDegreesLow  slopeFeatherDegreesHigh
bUseSmoothstep          bInvertSlopeGate         slopeGateStrength
```
**Ruling: per-stratum slope gates remain the ground truth.** This section adds
a global-default layer on top; it does not replace per-stratum with a flat
global. The seven fields above are **shared defaults**, consumed by a new
`bSlopeUseGlobal` flag on `Params::Stratum` (default `true`) — see
`MASKING_SPEC` §1.7 for the full default/override contract and where it
resolves (the Mask stage's existing config-flattening step; no PROC change).

Do not confuse this section with the real map's `SlopeSettingsParams`, an
unrelated single-field physics-parity toggle (`bUseEngineParityMath`) — that
field has no home decided yet and is not part of this correction.

### `Flow` / `Accumulation` — Correction 6 (both reserved, field lists TBD)
Two separate, global, top-level sections — both computed after heightmap layer
calculation finishes, neither per-layer, neither part of Erosion:
- **`Flow`** — a literal flow-velocity map, produced by simulating rain/water
  movement over time given map variables. `FlowMapColor` (confirmed live, a
  preview tint) lands here.
- **`Accumulation`** — consumes `Flow`'s output to simulate where material
  naturally piles, fills crevices, spills over, and re-flows, without runaway
  mound-building.

**Confirmed NOT the same as Erosion's own settings.** `ErosionLayerSettings`
(`src/proc/Erosion_Settings_PROC.h`) is real, current, and already per-layer
(inside `HeightmapStack`'s Simulation-layer entries) — it already owns
`slopeAdherence`, `bAccurateSimultaneousAccumulation`, `spilloverThreshold`,
and the rain-noise droplet-spawning fields. None of that moves; it stays
exactly where it is.

**Confirmed NOT the same as v2's current `FlowAccumulation` stage**, either.
`FlowAccumulationConstants` (`src/proc/FlowAccumulation_Kernel_PROC.h`) is a
real, single, current stage (`cellWeight`, `flowNoiseImpact`,
`depressionFillEpsilon`, `flowMagnitudeScale`, iteration counts,
`bFillDepressions`, `bNormalizeAccumulation`) — drainage/routing for pathing,
not the two-simulation velocity→accumulation model described above, and its
field names don't match v1's `FlowSettingsParams` (`Precipitation`,
`FlowVolumeMultiplier`, `StochasticVariance`) at all.

**This work-order reserves the two top-level keys (`Flow`, `Accumulation`)
with field lists marked TBD.** Designing the actual two-simulation model — and
reviewing whether the existing sim math (erosion included) is even currently
correct, which the human has separately flagged as suspect — is real PROC work
for a future generator-expert/ARCH work-order, explicitly not this one.

### `MarkersStack` / `PropsStack` / `DecalsStack` / `UnitsStack` — Correction 7
Each a Group→Layer(rule) hierarchy, shape **pending** the deferred shared
Group/Layer/LayerType design (see `PLACEMENT_SCATTER_SPEC`'s companion note).
For this work-order, each Stack's layers are, for now, a flat array of the
existing rule type, wrapped under the new top-level key:
```
MarkersStack → Params::MarkerRule (MarkerRule_PARAMS.h) — field-complete for
               count/density/slope/height gates/priority/focus-gradient/symmetry
PropsStack   → Params::PropRule   (ScatterRule_PARAMS.h) — field-complete
DecalsStack  → Params::DecalRule  (ScatterRule_PARAMS.h) — field-complete
UnitsStack   → Params::UnitRule   (NEW — a fourth, fully-wired v2 rule type,
               already exported today, MapExporter_Rules_IO.cpp:91-105,117,122,
               absent from the first draft of this schema; omitting it would
               regress what v2 ships today)
```
**Confirmed cardinality change, new fields required:**
```
HydroMultiplier   ReclaimDensity   MexDensity   SpawnPointCount
```
move from **global scalars (v1)** to **per-layer fields on `Params::MarkerRule`**
— a genuine addition to that type, not a relocation, needed for the coder
work-order.

**`GlobalMarkerSettings`** sub-key inside `MarkersStack`: `GlobalIconAlloy`,
`GlobalIconPlasma`, `GlobalIconSpawn`, `MarkerColorAlloy`/`MarkerColorPlasma`/
`MarkerColorSpawn`, `MarkerScaleAlloy`/`MarkerScalePlasma`/`MarkerScaleSpawn`.
**Ruled: `Plasma` = Energy, a real planned resource type, not the v1 invention
flagged in `IO_PARITY_REPORT.md` Decision #5** — keep all three Plasma-named
fields.

**C++ shape (ARCH §11): `Params::GlobalMarkerSettings`**, a new standalone
`GlobalMarkerSettings_PARAMS.h`, sibling of `MarkerRule_PARAMS.h` (map-wide, not
per-rule — same global-vs-per-rule scope split as `Symmetry`/`SlopeDefaults`
above). Field naming diverges from the raw JSON key spelling above:
`GlobalIconAlloy`/`Plasma`/`Spawn` → `iconNameAlloy`/`iconNamePlasma`/`iconNameSpawn`
(atlas-manifest name keys, `ASSET_LOADING_SPEC` — not file paths, hence not
`icon*Path`); `MarkerColorAlloy`/`Plasma`/`Spawn` → `colorAlloy`/`colorPlasma`/
`colorSpawn` and `MarkerScaleAlloy`/`Plasma`/`Spawn` → `scaleAlloy`/`scalePlasma`/
`scaleSpawn` (the redundant `Marker` prefix is dropped — the type's own name
already scopes them). Full shape and rationale: ARCH §11.

### `DetailNormal` — Correction 8
```
DetailNormalMapSize   (only field)
```
The future layered-heightmap-delta system (a stack of heightmaps producing a
delta normal map) is explicitly deferred — this correction reserves the key
and the one live field.

### Removed from `.sanmap` scope entirely — Correction 9
Not relocated within this file — **deleted from `.sanmap` scope entirely**:
- `GamedataPath`, `GlobalEnvironmentPath` — confirmed present (empty) in the
  real map, confirmed zero v2 code references. Move to a new, separate, global
  app-settings location (a user-chosen "SanGen folder"), which also holds the
  shared icon/thumbnail cache — one copy, not duplicated per map. **Not
  designed in this work-order.**
- **`PerformanceSettings`** (`UseGPUFlowMap`, `UseGPUMarkers`, `UseGPUTerrain`,
  `WYSIWYGBaking`, `GPUPreviewIterations`, `FastPreviewMode`) — **ruled OUT of
  `.sanmap` entirely, and explicitly no per-map override.** These describe the
  generating machine's hardware/backend, not the map; they belong solely in
  the global app-settings location above, read once at startup to seed
  `Sys::DispatchPolicy` (`DISPATCH_INTERFACE_SPEC`). This supersedes an
  earlier "persist as a non-authoritative hint" recommendation — the final
  ruling is no per-map storage of any kind, so no `DISPATCH_INTERFACE_SPEC`
  amendment is needed for this work-order.

### `MarkersList` — deleted (Correction 10)
Baked marker positions duplicating the format's own `markers` dict. Deleted;
superseded entirely by `MarkersStack`'s rules (Correction 7), consistent with
what v2 already does today (rules, never baked instances).

### Merges into existing format-native collections — Correction 11
Stay lowerCamelCase (ARCH §1.6, Correction 0) — these are additions to the
format's own dictionaries, not new SanGen sections. See the Entity collections
note above for the schema locations:
- `armies[key]` gains **`armyColor`** and **`alias`**.
- `markers[key]` gains **`alias`**.
- The old global `Aliases` block (formerly `mapGeneratorData.Aliases`) is
  **deleted** once both land.

### Verified deletions (pure duplicates — delete outright)
Confirmed line-for-line against a real map — no replacement needed, each
mirrors a field the format's own top level already carries:
- `mapGeneratorData.Atmosphere`
- `mapGeneratorData.Stratums` (**both** the v1 dialect and the current-v2
  dialect — `MapExporter_Layers_IO.cpp::BuildStratumJson` — that squatted on
  this same key; the format's own `stratumLayers[9]` is the single source of
  truth)
- `mapGeneratorData.MapSize` (the format's own `width`/`length`)
- the global `mapGeneratorData.TerrainMaxHeight` (the format's own `height`)
- the entire generator `mapGeneratorData.Water` sub-block (the format's own
  Water fields, listed under Top-level map fields above)
