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
  - **The dictionary key itself is `ARMY_XX`, machine-owned by SanGen as of
    Correction 18 below** — the human-authored label lives in the new
    `displayName` field, not the key.
- `markers`: Dictionary<string, MarkerType{ resource:bool, transforms:
  Dict<string,MarkerTransform{ ..., alias, layerIndex, symmetryGroupIdentifier,
  iconNameOverride }> }>
  - **`alias` is a SanGen-added field (Correction 11)** — same lowerCamelCase
    rule as `armyColor` above.
  - **`layerIndex`/`symmetryGroupIdentifier`/`iconNameOverride` are SanGen-added
    fields (schema v3, Correction 15/16 below)** — same lowerCamelCase merge
    rule, applied to a dictionary-value object. Companion metadata lives in the
    new top-level `MarkerGroups` section (Correction 16); `MarkerLayerBundles`
    (Correction 19) is a further, separate top-level array — it does NOT merge
    a field onto `MarkerTransform` itself, only onto `MarkerRuleLayer`/
    `MarkerInstanceLayer` (see Correction 19).
- `chains`: Dictionary<string, MarkerChain.Marker[]{ type, name }>
- `decals`: DecalType[]{ blueprintPath, transforms: DecalTransform[]{ ...,
  layerIndex } }
- `props`: PropType[]{ blueprintPath, transforms: PropTransform[]{ ...,
  layerIndex } }
  - **`layerIndex` (both domains) is a SanGen-added field (schema v3,
    Correction 14 below)** — same lowerCamelCase merge rule as `armyColor`/
    `alias` above, applied to an array-element object instead of a
    dictionary-value object (`props`/`decals` are arrays, not dictionaries).
    Companion metadata lives in the new top-level `PropGroups`/`DecalGroups`
    sections (Correction 14) — a genuinely new SanGen-owned array, not a
    merge, so it is PascalCase per the casing law below.
  - **Direct field injection into `props[].transforms[]`/`decals[].transforms[]`
    is now empirically confirmed safe by a live game load (2026-08-24), not
    just architectural precedent.** See Correction 14's "Empirical
    confirmation" note below for the test that closed this gap.

## Shared transform
`InstancedTransform { Vector3 position, Quaternion rotation, Vector3 scale }`.
Vector2/3/4, Quaternion, Color are plain float structs (`Types.cs`).

## Stratum (one texture layer, ×9)
name; albedo / normal / mask TextureLoader{ path }; tileSize / tileSizeFar
(Vector2); triplanar tile sizes; normalScale (+Far); normal/height
farNearBlend; diffuseRemap / farColorRemap (Color); maskRemapMin / Max (Vector4).

Confirmed field-for-field against `SanMap.Types.cs::Stratum` (ground truth):
```
string name;
TextureLoader albedo;            // { path }
NormalTextureLoader normal;      // { path }
MaskTextureLoader mask;          // { path }
Vector2 tileSize;                // default (1,1)
Vector2 tileSizeFar;             // default (1,1)
float   tileSizeTriplanar;       // default 12 — a SCALAR, not a Vector2
float   tileSizeFarTriplanar;    // default 36 — a SCALAR
float   normalScale;             // default 1
float   normalScaleFar;          // default 1
float   normalFarNearBlend;      // default 0.5
float   heightFarNearBlend;      // default 0.5
Color   diffuseRemap;            // default gray (0.5,0.5,0.5,1)
Color   farColorRemap;           // default (1,1,1,0)
Vector4 maskRemapMin;            // default (0,0,0,0) — {x,y,z,w}
Vector4 maskRemapMax;            // default (1,1,1,1) — {x,y,z,w}
```

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
  (2) **props export is disabled** — commented out because "many prop formats are
  outdated, causing maps to fail loading." **Root cause identified (in-game, 2026-08):
  a single unresolvable `blueprintPath` aborts the remainder of map load.** Props
  parsed before the bad entry still render; everything parsed *after* props — the
  `markers` block above all — silently never spawns. The observable symptom is
  "my alloy points disappeared", not "a prop is missing", which is why this was
  mis-attributed to prop formats being outdated.

  **Therefore (Constitution §6): the exporter MUST verify every `blueprintPath`
  resolves to a real file before writing a `.sanmap`.** An unverified path is a
  map-breaking defect, not a cosmetic one. This is the single highest-value
  validation in the IO layer.
- **Per-instance field injection tested independently of export (2026-08-24).**
  Props export being disabled (above) meant no SanGen-*exported* `.sanmap` had
  ever put an extra field on a prop transform in front of the real game — the
  only evidence for that shape's safety was architectural precedent
  (`armyColor`/`alias`/`displayName` on dictionary-value objects) plus
  structural inference, not a directly-confirmed production case. That
  question has now been answered directly: see Correction 14's "Empirical
  confirmation" note below. This is orthogonal to the `blueprintPath` defect
  above — it confirms the *transform schema* tolerates an extra key, not that
  export produces valid `blueprintPath`s.
- **⚠️ Standing recorded defect, confirmed still live — the manual marker
  exporter never writes `layerIndex` back out.** `BuildMarkerTransformJson`
  (`src/io/MapExporter_Markers_IO.cpp:17-39`) has no `json["layerIndex"] = ...`
  line at all — confirmed by direct read, unlike `PropTransform`/
  `DecalTransform`, both of which do (`MapExporter_Props_IO.cpp:35`,
  `MapExporter_Decals_IO.cpp:32`). Every exported marker's layer membership
  currently round-trips as "always absent → clamps to 0" on reimport (the
  importer, `MapImporter_Markers_IO.cpp:69`, reads it correctly — this is a
  write-side-only gap). Flagged by `DESIGN_MarkerGroupLayerRestructure_R1.md`
  §0/§7 item 12 as a real, separate bug; not fixed by this correction pass
  (ARCH does not write code) — routed as its own small coder work-order.
  Whoever picks it up must also confirm the sibling `parentBundleIdentifier`
  field the new `MarkerLayerBundles` feature adds to `MarkersStack`/
  `MarkerGroups` entries (Correction 19) does not ship with the same omission.
- **Faction / army colors (Colors.cs):** EDA = dark green (0.078,0.329,0.196),
  Chosen = dark red (0.412,0.008,0.008), Guard = golden amber
  (0.690,0.549,0.188); plus a 32-entry `ArmyColors[]` palette + named
  `ColorLookup`. Used for army/marker coloring in the UI.

### Sampling terrain height for entity placement (empirically confirmed)
`Textures/heightmap.raw` is headerless little-endian `uint16`, `N×N` where
`N = heightmapResolution`. World-space Y for an entity at world `(x, z)`:

```
row = z
col = (N - 1) - x            // the documented flip, applied to the index
y   = bilinear(heightmap[row][col]) * height / 65536.0
```

`height` is the map's terrain vertical extent (`128` on Pandemonium, `410` on
Two_Step_Shuffle, `1600` on The_Forge) — **read it from the map, never hardcode.**

Validation: **median absolute error 0.0105** world units over 63,538 prop instances
in `The_Forge.sanmap`; mean 0.311. Alternatives ruled out decisively — un-flipped
`(z, x)` gives mean error 28.1, transposed `(x, z)` gives 22.8.

**Open question, deliberately not resolved here:** which axis carries the flip cannot
be determined from shipped maps. `(N-1-z, x)` and `(z, N-1-x)` disagree on only 28 of
22,528 Two_Step_Shuffle instances, and on those the winner is a 53.6% coin flip,
because every official map tested is symmetric. The documented convention
(`world.z = length - z - 1`) stands; do not "fix" it on the strength of a
better-looking residual. A decisive test needs an asymmetric map.

**Distinct convention, not this one — do not conflate (2026-08-29).**
`NAVMAP_MODIFIER_BLOCKER_SPEC.md` §8 reuses this exact `row = z; col = (N-1) - x`
heightmap-sampling convention, applied in the **inverse** direction (pixel → world
instead of world → pixel), for a hand-authored `Textures/` blocker mask — the same
raster-asset family as `heightmap.raw`. That is a genuinely **different** coordinate
concern from the `world = (x, y, length - z - 1)` entity-position flip documented
above in "Conversion / import-export logic" — two different conventions for two
different kinds of coordinates (raster pixel index vs. JSON entity position field).
A reader must not assume one informs or corrects the other.

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
  `.santp` paths — **but `.sanprop` is also a live extension**: the entire
  Pandemonium prop set ships as `.sanprop` containing dialect-A Lua. An
  extension allow-list of `.santp` only will reject valid blueprints; each
  transform is position/rotation/scale.
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
GlobalSymmetryMask       RadialSymmetryRepeatCount
```
Per-rule/per-layer local override — `bSymmetryUseGlobal` + `symmetryMask` — is
**confirmed live** on `MarkerRule`/`PropRule`/`UnitRule`
(`PlacementRules_PARAMS_Test.cpp:16`); those three carry a pure relocation of
existing fields under this section's umbrella, not new work. **New** for
`HeightmapStack`'s `GeoLayer`/`Layer` (Correction 3).

**Correction, superseding this section's earlier claim (ARCH §13):** the
original text of this correction additionally claimed the `bSymmetryUseGlobal`/
`symmetryMask` pair is "already live and tested" on `DecalRule` as well. **That
claim was factually wrong at ARCH §13 ratification time and was withdrawn.** At
that time `src/params/ScatterRule_PARAMS.h`'s `DecalRule` carried no
`bSymmetryUseGlobal`/`symmetryMask` pair at all, and `AppendDecalRules`
(`src/proc/Placement_Rules_PROC.cpp`) never called `ResolveSymmetryMask` for
decals — decals generated with **no** symmetry at all, not even the global
default. This was flagged as Defect 1 in `PLACEMENT_SCATTER_SPEC`'s "Known
issues" section for a future coder work-order.

**Defect 1 is now FIXED (later session).** `DecalRule` now carries the
`bSymmetryUseGlobal`/`symmetryMask` pair alongside `MarkerRule`/`PropRule`/
`UnitRule`, and `AppendDecalRules` now calls `ResolveSymmetryMask` for decals,
mirroring `AppendPropRules` — confirmed by code read; the `.sanmap` IO
round-trip for the pair exists too. `PLACEMENT_SCATTER_SPEC` records this
closure in full.

**Radial N-fold symmetry (ARCH §13, ratified this session):**
- **New bit:** `SymmetryAxis::Radial = 1 << 4` (`Symmetry_PARAMS.h`). Confirmed
  via code read that `BuildSymmetryOrbit` (`Placement_Symmetry_PROC.h`) already
  composes every set bit of the mask independently, so combining `Radial` with
  the existing mirror/rotation bits already works structurally once the bit and
  its own orbit-generation logic exist — no other PROC combination-logic change
  is needed. The N-way rotation generator itself (the `Radial` analog of
  `AppendQuarterTurns`, generalized from a hardcoded 3 turns to a designer
  count) is new PROC work for a future coder work-order, not designed here.
- **Companion count field**, a flat sibling wherever `symmetryMask` already
  lives (not a wrapper struct, matching the existing `bSymmetryUseGlobal`/
  `symmetryMask` flat-sibling convention):
  ```
  int radialSymmetryRepeatCount = 3;
  ```
  Each independently-overridable mask (`MapRecipe::globalSymmetryMask`,
  `MarkerRule::symmetryMask`, `PropRule::symmetryMask`, `UnitRule::symmetryMask`,
  and, now that Defect 1 above is fixed, `DecalRule::symmetryMask`, plus the
  future `HeightmapStack` `GeoLayer`/`Layer` override, Correction 3) needs its
  own `N`: a local override with `bSymmetryUseGlobal = false` but no local count
  would otherwise silently inherit the global `N`, defeating the point of a
  local override.
- **JSON key `RadialSymmetryRepeatCount`, PascalCase** — sibling of
  `GlobalSymmetryMask` in this section's global field list above, and, per
  rule, sibling of the confirmed-live `SymmetryMask` key
  (`MapExporter_Rules_IO.cpp`) on each per-rule Stack entry.
- **Default axis change:** `MapRecipe::globalSymmetryMask`'s default becomes
  `SymmetryAxis::RotateHalfTurn` (was `SymmetryAxis::None`,
  `MapRecipe_PARAMS.h:31`) — reuses the existing "Point" bit; no new bit needed
  for this default.
- **Default blend — forward-attached requirement, not built now.**
  `Params::SymAlgorithm` does not exist anywhere in `src/` yet (confirmed zero
  matches) — it remains this correction's own reserved, deferred field (see
  the global field list above and "Ruled, not deferred" below). Whichever
  future work-order actually defines
  `Params::SymAlgorithm{Fold, Blur, CrossFade, Superposition, Cylinder3D,
  Torus3D, ...}` **must default it to `Superposition`** — recorded here so the
  requirement is not lost between now and that work-order.
- **Known follow-up defect (ARCH §13, not fixed this session):**
  `Params::symmetryOrbitMaximum = 16` (`Symmetry_PARAMS.h`) backs a fixed-size
  stack array (`SymmetryOrbitPoint orbit[16]`,
  `src/proc/Placement_Accept_PROC.cpp:33`) sized for the old maximum
  combination (mirror X × mirror Z × quarter turns). A designer-chosen
  `radialSymmetryRepeatCount` combined with mirrors can now exceed 16 (e.g.
  8-fold × MirrorX × MirrorZ → up to 32), and the buffer **silently drops
  excess clones** rather than erroring — a real correctness gap this
  ratification creates by making a larger orbit reachable from the UI/PARAMS
  layer. Flagged as Defect 2 in `PLACEMENT_SCATTER_SPEC`'s "Known issues"
  section — raising the cap and/or adding a loud validated clamp on the
  designer-facing `N` (Constitution §6) is PROC/buffer-sizing work for a
  future Compute Optimization Expert or Generator Expert work-order, not
  ratified here.

**Ruled, not deferred:** heightmap symmetry is wanted immediately, in full —
both the basic axis mechanism (`Params::SymmetryAxis` mirror/rotate/radial)
**and** the exotic `SymAlgorithm` group (Fold/Blur/CrossFade/Superposition/
Cylinder3D/Torus3D). No v2 code implements a heightfield-symmetry PROC stage
today (`IO_PARITY_REPORT.md` Decision #6). **This work-order reserves and
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
**Where the per-stratum override values for these same seven fields (plus the
per-stratum soil physics) round-trip on disk: Correction 12,
`StratumGenerationSettings`, below.**

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
               already exported today, MapExporter_Rules_IO.cpp:91-105,117,
               122, absent from the first draft of this schema; omitting it
               would regress what v2 ships today)
```
**Not the same "Group" as Correction 14's `PropGroups`/`DecalGroups`.** This
Stack's `Group` is an organizational container for procedural rule *Layers*
(this correction); Correction 14's `PropGroups`/`DecalGroups` are metadata for
hand-placed instance *layers* (a manual authoring concept). The two "Group"/
"Layer" words are reused across genuinely different concepts in this format —
`ENTITY_AUTHORING_PARAMS_SPEC`'s naming for the manual concept was chosen
specifically (`PropGroups`, not `PropLayers`) to avoid colliding the two.
**A THIRD, container-above-Layer concept exists as of Correction 19 below
(`MarkerLayerBundles`) — deliberately named "Bundle," not "Group," specifically
to avoid becoming a fourth collision with the two already documented here**
(ARCH §19.1).

**`MarkersStack` is no longer flat like the other three Stacks — see
Correction 15 below.** `PropsStack`/`DecalsStack`/`UnitsStack` remain exactly
the flat rule arrays described in this correction; `MarkersStack` alone gains
the one-tier Group(`MarkerRuleLayer`)→Rule(`MarkerRule`) wrapper (ARCH §16),
the first concrete slice of this correction's own long-deferred Group/Layer
hierarchy, scoped exactly to what layer-scoped marker symmetry needs — not a
retroactive redesign of the other three Stacks.

**Confirmed cardinality change, new fields required:**
```
HydroMultiplier   ReclaimDensity   MexDensity   SpawnPointCount
```
move from **global scalars (v1)** to **per-layer fields on `Params::MarkerRule`**
— a genuine addition to that type, not a relocation, needed for the coder
work-order.

**`GlobalMarkerSettings`** is its own top-level PascalCase key, a sibling of
`MarkersStack` — **not** nested inside it. (This correction's original text
called it a "sub-key inside `MarkersStack`"; that phrasing is superseded by
ARCH §11's ruling and STEP13's implementation, both of which ship it as a flat
top-level sibling. `MarkersStack` is itself a bare array — Correction 7's own
"flat array … wrapped under the new top-level key" wording above — so it could
never structurally host a nested key regardless.) Fields:
`GlobalIconAlloy`, `GlobalIconPlasma`, `GlobalIconSpawn`, `MarkerColorAlloy`/
`MarkerColorPlasma`/`MarkerColorSpawn`, `MarkerScaleAlloy`/`MarkerScalePlasma`/
`MarkerScaleSpawn`.
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

### `StratumGenerationSettings` — Correction 12 (per-stratum soil physics + slope-gate overrides)
Ratified alongside the ARCH §7.2 item 10 remap-shape amendment and
`MASKING_SPEC` §1.7's `SlopeDefaults`/`bSlopeUseGlobal` amendment, which this
correction gives an IO home to. New top-level key (ARCH §1.6: single-token
PascalCase, no spaces), sibling of `stratumLayers` — **not** nested inside it,
and **not named `StratumSettings`** — that name is claimed by the legacy v1
duplicate type multiple specs already flag for deletion (`MASKING_SPEC` Part 2
"Known issues"; ARCH hit-list #1, `src/params/StratumMask_PARAMS.h`/ARCH §7.1
"Standing violations to clear"); reusing it here would recreate exactly the
two-`StratumSettings` confusion those specs exist to end.

**No new C++ type.** Every field below already is (or, for `SlopeUseGlobal`,
will be once `MASKING_SPEC` §1.7 is implemented) a direct member of the single
`Params::Stratum` — `StratumSoilPhysics soilPhysics` (6 fields, already live)
plus the 7 slope-gate fields already on `Params::Stratum` and the new
`bSlopeUseGlobal` flag. This section is purely a new IO surface serializing
fields that already have a PARAMS home (ARCH §7.1) — it creates no rival
per-stratum settings type and no rival top-level array.

**Shape:** an array of exactly 9 objects, index-aligned with `stratumLayers[9]`
(same convention, same cardinality rule as below — **not** a dictionary):
```
StratumGenerationSettings: [ 9 × {
    Hardware fields (Params::Stratum::soilPhysics, 6 fields — NEW writes):
        Hardness                (float)
        Friction                (float)
        Cohesion                (float)
        CapacityMultiplier      (float)
        AbsorptionRate          (float)
        Erodable                (bool)   // bErodable, "b" dropped per this
                                          // section's own casing convention

    Slope-gate fields (Params::Stratum, 1 NEW + 7 relocated):
        SlopeUseGlobal          (bool)   // NEW — Stratum::bSlopeUseGlobal
        SlopeGateEnabled        (bool)   // relocated, verbatim key
        MinimumSlopeDegrees     (float)  // relocated, verbatim key
        MaximumSlopeDegrees     (float)  // relocated, verbatim key
        SlopeFeatherDegreesLow  (float)  // relocated, verbatim key
        SlopeFeatherDegreesHigh (float)  // relocated, verbatim key
        UseSmoothstep           (bool)   // relocated, verbatim key
        InvertSlopeGate         (bool)   // relocated, verbatim key
        SlopeGateStrength       (float)  // relocated, verbatim key
} ]
```
The 8 slope-gate keys (`SlopeGateEnabled` … `SlopeGateStrength`) are carried
over **verbatim** from the doomed `mapGeneratorData.Stratums` block's
`BuildStratumJson` and its importer counterpart
(`src/io/MapExporter_Layers_IO.cpp:62-81`, `src/io/MapImporter_Recipe_IO.cpp:57-76`)
— a **zero-cost relocation** of an already-working read/write pair; only the
container these keys live in changes (from `mapGeneratorData.Stratums[]` to
this new top-level `StratumGenerationSettings[]`). `SlopeUseGlobal` and the 6
soil-physics keys are genuinely new writes: the fields exist on
`StratumSoilPhysics`/`Stratum` today, but nothing currently serializes them
(soil physics has been write-only-to-nothing since the type was created;
`bSlopeUseGlobal` does not exist in `src/` yet — it is `MASKING_SPEC` §1.7's
field, pending its own coder work-order).

Field-name casing follows this section's own established convention for
SanGen-owned array entries — PascalCase, `b`-prefix dropped — the same
convention `GeneralMapSettings` (Correction 2) and the doomed block's own
`BuildStratumJson`/`MaskRemapMinimum`/`Enabled`/`TintRed` keys already use.
(ARCH §1.6 governs top-level keys and *format-native* collection members; a
SanGen-owned section's own internal field spelling is unconstrained by that
rule and follows established sibling-section precedent instead.)

**Cardinality rule:** always write exactly 9 entries, padding past
`recipe.strata.size()` with `Params::Stratum()` defaults — the same pattern
`BuildStratumLayersJson` already uses for `stratumLayers`
(`src/io/MapExporter_Recipe_IO.cpp:14-16`). A length mismatch between
`stratumLayers` and `StratumGenerationSettings` on import is a **loud, logged
warning** (Constitution §6) — never silent truncation, and never a hard
refusal (this is generator recipe state, not a version gate).

**Cross-reference:** `MASKING_SPEC` §1.7 states the PARAMS-side
default/override *mechanism* (`bSlopeUseGlobal`, config-flattening step); this
correction states where those fields, plus soil physics, actually round-trip
on disk.

### `stratumLayers` appearance wiring — Correction 13 (closes "appearance is write-only")
Confirmed: every `StratumAppearance_PARAMS.h` field maps 1:1 onto a real,
already-format-native `stratumLayers[9]` key (the "Stratum" section above,
confirmed field-for-field against `SanMap.Types.cs::Stratum`) — this is a
**bug-fix / completion of existing wiring, not a new schema section.**

**Confirmed gap: v2 has no importer for `stratumLayers` at all today.**
Grepping `src/io/MapImporter_Recipe_IO.cpp` (and all of `src/io/`) for
`stratumLayers` finds zero matches — appearance never round-trips on load,
on top of being written mostly blank on export.

**Export fixes** (`BuildStratumLayersJson`, `src/io/MapExporter_Recipe_IO.cpp:12-31`):
```
layer["albedo"]["path"]       <- stratum.appearance.albedoTexturePath      (BUG: currently always "")
layer["normal"]["path"]       <- stratum.appearance.normalTexturePath     (BUG: currently always "")
layer["mask"]["path"]         <- stratum.appearance.compositeTexturePath  (BUG: currently always "")
layer["tileSize"]             = { x: stratum.tileCount, y: stratum.tileCount }         (unchanged — correct)
layer["tileSizeFar"]          <- appearance.farTileCount                  (BUG: currently reuses
                                                                             stratum.tileCount, the
                                                                             NEAR tile size)
layer["tileSizeTriplanar"]    <- appearance.triplanarTileCount            (currently never written)
layer["tileSizeFarTriplanar"] <- appearance.farTriplanarTileCount         (currently never written)
layer["normalScale"]          <- appearance.normalScale                   (currently never written)
layer["normalScaleFar"]       <- appearance.farNormalScale                (currently never written)
layer["normalFarNearBlend"]   <- appearance.normalFarNearBlend            (currently never written)
layer["heightFarNearBlend"]   <- appearance.heightFarNearBlend            (currently never written)
layer["diffuseRemap"]         = { r: tintRed, g: tintGreen, b: tintBlue, a: 1.0 }  (unchanged — correct;
                                                                             see the dead-field note below
                                                                             for why this is NOT
                                                                             appearance.diffuseRemapColor)
layer["farColorRemap"]        <- appearance.farColorRemapColor[4]         (currently never written; this
                                                                             field has no scalar collapse
                                                                             elsewhere on Stratum, so it is
                                                                             the correct, sole consumer of
                                                                             the format's farColorRemap key)
layer["maskRemapMin"/"Max"]   <- stratum.maskRemapMinimum/Maximum[4]      (once ARCH §7.2 item 10 lands,
                                                                             writes the full 4-component
                                                                             object, {x,y,z,w} — flagged
                                                                             here only so the two
                                                                             corrections are not
                                                                             implemented out of order
                                                                             against the same field)
```

**Import — a NEW reader is needed** (a `stratumLayers` reader added to
`src/io/MapImporter_Recipe_IO.cpp`, the mirror of `BuildStratumLayersJson`),
populating, per index `0..8`:
```
Params::Stratum::appearance.albedoTexturePath     <- layer["albedo"]["path"]
Params::Stratum::appearance.normalTexturePath     <- layer["normal"]["path"]
Params::Stratum::appearance.compositeTexturePath  <- layer["mask"]["path"]
Params::Stratum::tileCount                        <- layer["tileSize"]["x"]   (y ignored — Params::Stratum
                                                                                 has no anisotropic tile field)
Params::Stratum::appearance.farTileCount          <- layer["tileSizeFar"]["x"]
Params::Stratum::appearance.triplanarTileCount    <- layer["tileSizeTriplanar"]
Params::Stratum::appearance.farTriplanarTileCount <- layer["tileSizeFarTriplanar"]
Params::Stratum::appearance.normalScale           <- layer["normalScale"]
Params::Stratum::appearance.farNormalScale        <- layer["normalScaleFar"]
Params::Stratum::appearance.normalFarNearBlend    <- layer["normalFarNearBlend"]
Params::Stratum::appearance.heightFarNearBlend    <- layer["heightFarNearBlend"]
Params::Stratum::tintRed/tintGreen/tintBlue       <- layer["diffuseRemap"]["r"/"g"/"b"] (alpha dropped —
                                                                                            Stratum has no
                                                                                            tint-alpha field)
Params::Stratum::appearance.farColorRemapColor[4] <- layer["farColorRemap"]["r"/"g"/"b"/"a"]
Params::Stratum::maskRemapMinimum/Maximum[4]      <- layer["maskRemapMin"/"Max"]["x"/"y"/"z"/"w"]
                                                       (per ARCH §7.2 item 10's widened shape)
```
Same cardinality/mismatch handling as Correction 12: `stratumLayers` shorter
or longer than 9 is a loud, logged warning (Constitution §6) —
`stratumLayers[9]` is otherwise already a fixed-size format invariant
(confirmed `STRATUM_COUNT = 9` above).

**Not part of this correction, noted only:** `layer["name"]` currently writes
a generated placeholder (`"Stratum " + index`) rather than
`stratum.appearance.name`. Real, but not in the ratified scope of this
correction — flagged for a future pass, not fixed here.

**Flagged, not fixed — a real defect for the coder to resolve when
implementing:** `StratumAppearance::diffuseRemapColor`
(`src/params/StratumAppearance_PARAMS.h:40`) is dead and self-contradictory.
The file's own header comment states `diffuseRemap` is deliberately **not**
duplicated here because `tintRed/Green/Blue` is the source of truth — and the
export mapping above (unchanged, already correct) bears that out:
`diffuseRemap` writes from `tintRGB`, never from `diffuseRemapColor`. Yet
`diffuseRemapColor` still exists as a live field, is wired into the Stratum tab
as its own "Diffuse Remap" swatch (`StratumsTab_Appearance_UI.cpp:53`,
distinct from the separate "Preview Base Color" swatch that edits `tintRGB`),
and would round-trip **nothing** even after this correction's import lands,
since `diffuseRemap` maps to `tintRGB`, not to it. The coder implementing this
correction should **delete `diffuseRemapColor` and its UI row** rather than
wire it — wiring it would recreate a second, competing color source for the
same shader key. (`farColorRemapColor` is not this defect — it has no
competing scalar field and is the correct, sole consumer of `farColorRemap`.)

**Flagged, not resolved — two more fields with no ratified format home:**
`importedMaskMode` and `bEnabled` (`Params::Stratum`) have no `stratumLayers`
equivalent; today they are exported only into the doomed
`mapGeneratorData.Stratums` blob (`BuildStratumJson`'s `"ImportedMaskMode"` and
`"Enabled"` keys, `src/io/MapExporter_Layers_IO.cpp:72,75`) and lose their only
home once that blob is deleted (per "Verified deletions" below). This is an
**open follow-up**, out of this correction's scope — do not invent a home for
them here; a future correction (possibly `StratumGenerationSettings` itself,
if that turns out to be the right container) must rule on it explicitly.

### `PropGroups` / `DecalGroups` — Correction 14 (manual-layer metadata, ARCH §12)
New top-level SanGen-owned keys (ARCH §1.6: single-token PascalCase, no
spaces), siblings of `props`/`decals`. Companion metadata array for the
`layerIndex` field this correction also adds to `props[].transforms[]`/
`decals[].transforms[]` (see the Entity collections note above) — the
`Params::PropInstanceLayer`/`DecalInstanceLayer` shape lives in
`ENTITY_AUTHORING_PARAMS_SPEC`; this correction states only the wire-format
placement and casing.

**No new C++ type beyond what `ENTITY_AUTHORING_PARAMS_SPEC` already
introduces.** `PropGroups`/`DecalGroups` each serialize a
`std::vector<Params::PropInstanceLayer>`/`std::vector<Params::DecalInstanceLayer>`
— array order is the layer's identity, and `PropTransform`/
`DecalTransform::layerIndex` is a plain index into that array (see
`ENTITY_AUTHORING_PARAMS_SPEC`'s "`layerIndex`" section for the full
direct-field-injection-vs-index-range reasoning and the general principle it
establishes).

**Shape (mirrors `StratumGenerationSettings`'s array-of-objects convention,
Correction 12, but with no fixed cardinality — this is designer-authored
count, not a fixed format invariant like `STRATUM_COUNT`):**
```
PropGroups / DecalGroups: [ N × {
    Name        (string)
    Color       ({r,g,b,a})
    IconScale   (float)
} ]
```
Field-name casing follows this section's own established SanGen-owned-array
convention (PascalCase, `b`-prefix dropped where relevant) — same convention
as `StratumGenerationSettings`/`GeneralMapSettings`.

**Import validation:** a `props[].transforms[].layerIndex`/
`decals[].transforms[].layerIndex` at or beyond the corresponding
`PropGroups`/`DecalGroups` array length is a **loud, logged clamp to `0`**
(Constitution §6) — per-instance, not per-file, since different instances in
the same file can carry different out-of-range values. Never a hard refusal —
this is authoring-convenience metadata, not gameplay-authoritative data, and a
missing/foreign file simply degrades every instance to layer `0`.

**Empirical confirmation (2026-08-24) — per-instance field injection on
`props[].transforms[]` is now production-proven, not merely inferred.** Until
this date, confidence in `layerIndex`'s direct-field-injection shape rested on
architectural precedent (`armyColor`/`alias`/`displayName` shipping safely
into `armies[key]`/`markers[key]` *dictionary-value* objects, Corrections
11/18) plus structural inference — no shipped `.sanmap` had ever tested an
*array-element* injection specifically, because SanGen's own props export has
been disabled the whole time (see the "props export is disabled" note in
"Conversion / import-export logic" above), so no SanGen-authored file with an
extra prop-transform field had ever reached a real game load. That gap is now
closed by a direct test, not an inference: the human hand-added an
`"InstanceId"` integer field to **all 1,180 prop transforms across all 17 prop
groups** in a real shipped map (`Pandemonium Isthmus.sanmap`, *Sanctuary:
Shattered Sun* Demo), and the map **loaded successfully in-game**
(human-verified, 1-player test, spawn placed correctly). This is a second,
independently-confirmed production case for direct field injection — this
time on an **array-element** object (`props[].transforms[]`), not just the
dictionary-value objects Correction 11/18 already proved. `layerIndex`'s shape
(and any future per-instance scalar following the identical pattern — e.g. a
stable per-instance id for a cross-layer "Assembly" grouping feature) is
**production-proven for props specifically**, not just analogized from the
armies/markers case. Decals (`decals[].transforms[]`) share the exact same
`PropTransform`/`DecalTransform` wrapper shape (see "Why props/decals now need
a wrapper transform type" in `ENTITY_AUTHORING_PARAMS_SPEC`) and are covered
by the same structural argument, though decals themselves were not separately
live-tested.

### `MarkersStack` — Correction 15 (layer-scoped symmetry, ARCH §16) — amends Correction 7 for `MarkersStack` only
`MarkersStack` upgrades from Correction 7's flat `Params::MarkerRule` array to a one-tier
Group→Rule wrapper — the first concrete slice of Correction 7's long-deferred Group/Layer/LayerType
hierarchy, scoped exactly to what ARCH §16's layer-scoped marker symmetry needs. `PropsStack`/
`DecalsStack`/`UnitsStack` are UNCHANGED — still flat rule arrays per Correction 7.

```
MarkersStack → [ N × {
    Name                       (string)  // Params::MarkerRuleLayer::name
    Enabled                    (bool)    // bEnabled, "b" dropped per this array's established
                                          // casing convention (Correction 12/14 precedent)
    Hidden                     (bool)    // bHidden, same
    SymmetryUseGlobal          (bool)    // Params::SymmetrySetting::bSymmetryUseGlobal
    SymmetryMask               (int)     // Params::SymmetrySetting::symmetryMask
    RadialSymmetryRepeatCount  (int)     // Params::SymmetrySetting::radialSymmetryRepeatCount
    ParentBundleIdentifier     (int)     // NEW, Correction 19 (ARCH §19.4) — Params::MarkerRuleLayer::
                                          // parentBundleIdentifier, -1/absent = root (ungrouped)
    Rules                      ([...])   // Params::MarkerRuleLayer::rules — NEW nested array, ruled below
} ]
```

**`Rules` — the nested-array key spelling, ruled.** `Rules`, PascalCase, bare plural of the contained
type's role ("rule"), no qualifier prefix. Based on this codebase's only existing nested-array
precedent inside a Stack-shaped/Group-shaped container: Correction 3's `HeightmapStack`, whose real,
live file shape is confirmed (`GeoLayers.Layers[]`) to name its own nested array the bare plural of
the contained type ("Layer" → `Layers`), not a qualified form like `GeoLayerLayers` or
`CompositionLayers`. Applying the same pattern here (contained type "MarkerRule" → bare plural
`Rules`, not `MarkerRules`) keeps the convention uniform across the format's two nested-array sites
rather than inventing a second spelling rule. `MarkerRuleLayer::rules` (the PARAMS field, fixed by
ARCH §16.1) is the C++ home; only the JSON key was open, and it is `Rules`.

**`SymmetrySetting` flattens to sibling keys, not a nested `"Symmetry"` sub-object** — same
established convention `StratumGenerationSettings` (Correction 12) already applies to
`StratumSoilPhysics` (a C++ wrapper struct flattened to sibling wire keys, no nested JSON object).
`SymmetryUseGlobal`/`SymmetryMask`/`RadialSymmetryRepeatCount` are the same three keys already
confirmed live at the per-rule tier (Correction 4) — reused verbatim at this new per-layer tier, not
renamed, so a reader recognizes the triplet immediately regardless of which tier it's reading.

**Consequence for `MarkerRule` itself — the triplet is REMOVED from the per-rule object.** ARCH §16.1
moves `bSymmetryUseGlobal`/`symmetryMask`/`radialSymmetryRepeatCount` up one tier, off `MarkerRule`
and onto the new `MarkerRuleLayer` wrapper. `PLACEMENT_SCATTER_SPEC`'s "Rules — MarkerRule" section
("Symmetry: `SymmetryUseGlobal/SymmetryMask`") is now stale for `MarkerRule` specifically — see the
follow-up edit below. `PropRule`/`DecalRule`/`UnitRule` keep the triplet exactly where it is; only
`MarkerRule` loses it. **This is a genuine breaking `.sanmap` schema change on an already-shipped
field family — the actual migration mechanics are the IO Architecture Expert's domain (ARCH §16.6),
not this correction's concern; this correction states only the new/target shape.**

### `MarkerGroups` — Correction 16 (manual-layer metadata, ARCH §16, extends Correction 14)
New top-level SanGen-owned key (ARCH §1.6: single-token PascalCase, no spaces), sibling of
`PropGroups`/`DecalGroups`/`markers`. Companion metadata array for `MarkerTransform::layerIndex`
(new, see the `markers[type].transforms[name]` merge below), serializing
`std::vector<Params::MarkerInstanceLayer>` — array order is the layer's identity, same convention as
`PropGroups`/`DecalGroups` (Correction 14).

**Field list corrected (2026-08-25) — the shape below was stale relative to the live exporter/importer
before this pass.** `MapExporter_Markers_IO.cpp:77-80`/`MapImporter_Markers_IO.cpp:138-141` have
shipped `Locked`/`GridSnapEnabled`/`GridSnapSizeWorldUnits`/`ColorOverrideEnabled` (STEP106/STEP116)
since before this correction last documented this array's shape — confirmed genuinely missing from
this section's text by direct grep, not merely uncommitted-and-pending. Added below, no shape change
to the actual wire format (documentation-only fix):

```
MarkerGroups: [ N × {
    Name                       (string)   // Params::MarkerInstanceLayer::name
    Color                      ({r,g,b,a})// Params::MarkerInstanceLayer::color[4]
    IconScale                  (float)    // Params::MarkerInstanceLayer::iconScale
    Id                         (int)      // Params::MarkerInstanceLayer::layerId — stable id,
                                           // legacy-backfill by array index on import when absent.
                                           // See the naming note below — this key is a confirmed,
                                           // now-ruled-on naming defect (ARCH §1.9), not fixed by
                                           // this documentation-only pass.
    SymmetryUseGlobal          (bool)     // Params::MarkerInstanceLayer::symmetry.bSymmetryUseGlobal
    SymmetryMask               (int)      // Params::MarkerInstanceLayer::symmetry.symmetryMask
    RadialSymmetryRepeatCount  (int)      // Params::MarkerInstanceLayer::symmetry.radialSymmetryRepeatCount
    Locked                     (bool)     // Params::MarkerInstanceLayer::bLocked (STEP106)
    GridSnapEnabled            (bool)     // Params::MarkerInstanceLayer::bGridSnapEnabled (STEP106)
    GridSnapSizeWorldUnits     (float)    // Params::MarkerInstanceLayer::gridSnapSizeWorldUnits (STEP106)
    ColorOverrideEnabled       (bool)     // Params::MarkerInstanceLayer::bColorOverrideEnabled (STEP116)
    ParentBundleIdentifier     (int)      // NEW, Correction 19 (ARCH §19.4) — Params::MarkerInstanceLayer::
                                           // parentBundleIdentifier, -1/absent = root (ungrouped)
} ]
```

Confirms the shape parallels `PropGroups`/`DecalGroups`' `Name`/`Color`/`IconScale` exactly (Correction
14), PLUS several things Props/Decals' manual layers don't (yet) carry: the stable `Id` key (already
specified for markers specifically, `MarkerInstanceLayer::layerId`/STEP60 §3 `BuildMarkerGroupsJson`,
since markers introduce `layerId` from day one rather than retrofitting it as Props/Decals did via
STEP56), the `SymmetrySetting` triplet (ARCH §16.1), and the per-layer `Locked`/grid-snap/color-override
fields (STEP106/STEP116) — flattened to sibling keys using the exact same spelling already ruled for
`MarkersStack` above, so a reader/importer can share one flattening helper across both new arrays if
convenient (not mandated, just enabled by the consistent spelling).

**Import validation** for the symmetry triplet: no range to validate (unlike `layerIndex`) —
`SymmetryMask`/`RadialSymmetryRepeatCount` are free integers, tolerated the same way `Symmetry`
(Correction 4)'s existing per-rule fields are — no new validation rule introduced here.

**Naming note — RULED, not merely flagged (ARCH §1.9, supersedes this paragraph's prior text).**
`MarkerInstanceLayer::layerId` is spelled with the "Id" abbreviation ARCH §16.5 ruled out for the
sibling field `symmetryGroupIdentifier`. The prior text of this note assumed "STEP60/STEP56 are still
undispatched work-orders, so no shipped code needs migrating if ARCH acts before either lands" — **that
assumption is now confirmed false**: direct read of `src/params/PropInstance_PARAMS.h` and
`src/params/MarkerInstance_PARAMS.h` (2026-08-25) confirms STEP56/STEP60/STEP111/STEP116 have all
shipped, `layerId` is live on all three of `PropInstanceLayer`/`DecalInstanceLayer`/
`MarkerInstanceLayer`, and the wire key `"Id"` is live in both the exporter and importer above. This is
therefore a real, standing, already-shipped naming-law violation — ARCH §1.9 rules the exact fix
(`layerId` → `layerIdentifier`, wire `"Id"` → `"Identifier"` with a legacy-`"Id"`-fallback import path)
and routes the migration mechanics to the IO Architecture Expert as a standing recorded defect, not
blocking on any ticket. **The new `MarkerLayerBundles` array (Correction 19) does not repeat this
defect** — its own stable-id field is spelled `Identifier` from day one.

### `markers[type].transforms[name]` — three new merged fields (extends Correction 11's `alias`/ARCH §12's `layerIndex` precedent)
All new, all **direct field injection** (ARCH §16.5/§1.8) into the existing format-native
`MarkerTransform` object, all **lowerCamelCase**, merged the same way `alias` (Correction 11) and
`armyColor` are — this is a format-native dictionary-value object, not a new SanGen-owned array, so
ARCH §1.6's camelCase merge rule applies, not the PascalCase rule governing the new top-level
`MarkerGroups`/`MarkersStack`/`MarkerLayerBundles` sections:

- **`layerIndex`** (`int`, default `0`) — indexes `recipe.markerLayers` (`MarkerGroups` above).
  Spelled **identically** to the already-live `PropTransform`/`DecalTransform::layerIndex` wire key
  (`ENTITY_AUTHORING_PARAMS_SPEC`, ARCH §12) — same name, same casing, same default, same
  direct-injection placement; no marker-specific divergence. Import validation: out-of-range against
  `MarkerGroups.size()` is a loud, logged clamp to `0` (Constitution §6), identical rule to
  Props/Decals. **⚠️ Confirmed still-live export gap, see "Conversion / import-export logic" above:
  the exporter never actually writes this key today** (`BuildMarkerTransformJson`) — the importer reads
  it correctly; this is a write-side-only bug, not a format-shape defect, and is not fixed by this
  documentation pass.
- **`symmetryGroupIdentifier`** (`int`, default `0`, `0` = ungrouped) — NEW. Spelled in full per ARCH
  §16.5's naming amendment (no abbreviation); the wire key matches the C++ field name verbatim,
  lowerCamelCase, same merge rule as `layerIndex` above. No range to validate on import — `0` is
  always legal (ungrouped), any positive value is accepted as-is; no clamp logic is needed.
- **`iconNameOverride`** (`string`, default empty) — NEW (STEP114), previously undocumented in this
  section, confirmed genuinely absent from the field list by direct grep. Empty = use the owning
  `MarkerInstanceGroup`'s type-default icon; any non-empty value is an atlas-manifest NAME key
  (`ASSET_LOADING_SPEC`), never a numeric atlas index. No range to validate on import — any string is
  legal, same tolerance as `alias`.

All three fields are genuinely novel scalars with no format-native competing home, so all three use
direct field injection rather than a side table — the same §1.8/§12 rule already governing
`armyColor`/`alias`/`layerIndex`, applied consistently (ARCH §16.5's own framing, restated here as
format truth).

### STEP49 export-time-warning ticket scope — ruled (ARCH §16.8/§16.10 routing)
**Ruling: yes — the future export-time "warn on missing Spawn content" ticket (STEP49's own
"Explicit out-of-scope" bullet, same class as the existing `blueprintPath` warn-dialog) should be
scoped per-`Army`, not per-group, and that formulation already subsumes the missing-group case — it
is not two separate checks.**

Concretely, whenever that ticket is built: for every `Army` in `recipe.armies`, check whether at least
one `markers["Spawn"].transforms[...]` entry's `name` (the STEP49-established army-picker convention,
`transform.name` set from the chosen army's name/alias) matches that `Army`'s identity; if none does,
warn. This single per-Army scan already covers the "no `Spawn` group at all" case for free (every
`Army` fails the same check when the group is absent or empty) — there is no reason to special-case
"group entirely missing" separately from "group exists but this one Army's marker was orphaned by a
symmetry-group shrink" (ARCH §16.8/`DESIGN_MarkerLayerSymmetry_R2.md` §2); both are the exact same
observable failure ("this army gets no commander at spawn") from the exporter's point of view, and a
per-Army check is the strictly more general, superset formulation of the per-group check STEP49
originally sketched. No new PARAMS field is needed (ARCH §16.8 already confirmed `Army` carries no
back-reference) — the check is a pure export-time scan over already-existing data, same posture as
the existing `blueprintPath` resolution check it's modeled on. **Built, since:
`work_orders/STEP82_ArmySpawnMarkerValidation_IO.md`**, which also corrects (in its own text) an
imprecision in this paragraph and in `ARCH_16_08_SpawnArmyShrink.md`'s original wording: the match key
against `Army::name` is `MarkerTransform::name` (the format's `transforms` dictionary key) only —
never `MarkerTransform::alias`, a SanGen-added field (Correction 11) the engine never reads for this
purpose.

### `armies[key]` gains `displayName` — Correction 18 (army engine-identity/display-name split, ARCH ruling — `work_orders/STEP76_ArmyIdentityNaming_IO.md`)
**Ruling: `Army::name` becomes machine-owned format truth; the human-authored label moves to a new,
merged `displayName` field.** STEP76 ("`ARMY_XX` engine identity vs. `displayName`") establishes that
SanGen — not the human — owns the `armies` dictionary key outright:

- **`Army::name` keeps its existing role as the folded-in `armies[key]` dictionary key** (ARCH §1.8's
  "`Dictionary<string, X>` → `std::vector<X>` with the dictionary key folded in as `name`" rule is
  unchanged) — but it is now always the auto-generated, zero-padded `ARMY_XX` engine identity (`ARMY_`
  + 1-based roster position, at least two digits, so an alphabetical sort of the roster's keys equals
  roster order — the property `common/gameUtils.lua`'s `CreateArmies()` relies on to assign lobby
  slots). It is **never human-settable** — no text box, no import-time override, no escape hatch.
- **`displayName`** (`string`, new) — the human-authored organization label. **Merged directly into
  the existing format-native `armies[<ARMY_XX>]` object, lowerCamelCase, as a sibling of the
  already-shipped `armyColor`/`alias` (Correction 11)** — direct field injection (ARCH §1.8), not a
  new top-level section and not a side table, because it is genuinely novel per-army scalar data with
  no competing format-native home, the same test `armyColor`/`alias`/`layerIndex` already passed:
  ```jsonc
  "armies": {
    "ARMY_01": {
      "faction": 0, "alloys": 500.0, "energy": 500.0, "groups": {},
      "armyColor": { "r": 1.0, "g": 0.2, "b": 0.2, "a": 1.0 },
      "alias": "",
      "displayName": "North Ridge"
    }
  }
  ```
  Free-form, may be empty, **not** required to be unique — two armies may both display as "North";
  they remain `ARMY_01`/`ARMY_02` to the engine regardless. `displayName` is a genuinely new field; it
  does **not** reuse `alias` (already-shipped, already human-editable via the STEP49 marker
  army-picker convention — folding a migrated legacy name into it would silently overwrite authored
  data, the exact discard Constitution §6 forbids).

**Confidence-limited reasoning, stated explicitly rather than silently assumed (STEP76 §2).** Two
distinct consumers read a `.sanmap`, and this ruling is on different footing for each:
- **The Lua engine (`LoadMapData`) — settled, no risk.** It whitelists `armies` at the *top level*
  only and has no per-army-object schema; an extra key inside an army object is simply inert.
- **The C# `EM.Map` deserializer — safe by inference plus production evidence, not by a direct read
  of the deserialization call site.** `EM.Map.Army` (`SanMap.Types.cs`) declares exactly four fields
  and carries neither `[JsonObject(MissingMemberHandling=...)]` nor `[JsonExtensionData]`; Newtonsoft's
  default `MissingMemberHandling` is `Ignore`. The actual `JsonSerializerSettings` at the call site
  could **not** be located in the vendored ground truth (`D:\Projects\Sanctuary\Sanmap File Format\`),
  so this is not proven outright — but `armyColor` and `alias` **already ship** into this exact
  position today (Correction 11) and maps load without issue, which is production evidence that
  whatever the real settings are, they tolerate an extra key here. `displayName` is the third instance
  of an already-settled, in-production pattern, not a new risk class.

**Rejected alternative, recorded so it is not re-proposed:** a top-level SanGen-owned
`ArmyDisplayNames` PascalCase section (`{ "ARMY_01": "North Ridge", ... }`), which would drive the
residual C#-deserializer risk to provably zero (dropped by `LoadMapData`'s whitelist; no member for it
on `SanMap` either). **Not adopted** — it splits one entity's data across two places for a risk
already disproven in production, contradicts §1.8's direct-injection branch, and adds a second
key-synchronization surface (`ArmyDisplayNames` keys would need re-mapping on every `ARMY_XX`
re-mint). Left on the shelf only if the human ever wants zero residual risk regardless of cost.

**No `SanGenVersion` bump.** Purely additive, same precedent as Corrections 12/14/17: a new field
merged into an existing collection, no reshape of anything an existing migration step would need to
touch. **Import tolerance:** an army object with no `displayName` key (any pre-STEP76 export) reads as
an empty string — ordinary Constitution §6 "absent key" tolerance, matching Correction 14's "missing
`PropGroups` → zero manual layers" precedent. **Legacy name recovery on import** (an already-shipped
`.sanmap` whose `armies[key]` was itself a human-authored string like `"Army_0"`, not yet `ARMY_XX`)
is IO/BRIDGE mechanism, not format truth, and is `STEP76`'s own §4 to specify — flagged `⚠️ ASSUMPTION`
in that ticket, not re-litigated here.

### `MarkerLayerBundles` — Correction 19 (the Group-above-Layer container, ARCH §19)
New top-level SanGen-owned key (ARCH §1.6: single-token PascalCase, no spaces), sibling of
`MarkerGroups`/`MarkersStack`/`markers`. Serializes `std::vector<Params::MarkerLayerBundle>`
(`ARCH_19_03_FieldSpellings.md`) — array order is NOT this array's identity (unlike `PropGroups`/
`DecalGroups`/`MarkerGroups`); membership and nesting are addressed by `Identifier`, because a Bundle
forest can be reordered/reparented independently of array position (the same reason `Assemblies`,
`DESIGN_Assembly_R1.md` §5, needs a stable id rather than positional identity).

```
MarkerLayerBundles: [ N × {
    Identifier                 (int)      // Params::MarkerLayerBundle::identifier — stable, survives
                                           // reorder/delete. Spelled in full per ARCH §1.9 — this new
                                           // array does NOT repeat MarkerGroups' pre-§1.9 "Id" defect.
    Name                       (string)
    ParentBundleIdentifier     (int)      // -1/absent = root; enables Bundle-in-Bundle nesting
    MarkerTypeName             (string)   // single-type scope, e.g. "Alloy" — free-form, same string
                                           // space as markers[type]'s own dictionary key, NOT MarkerCategory
    AssemblyIdentifier         (int)      // -1/absent = ungrouped; inert until the separate,
                                           // still-unbuilt Assembly feature exists (ARCH §19.5)
} ]
```

**Per-Layer back-reference, additive, direct field injection (ARCH §1.8), lowerCamelCase, merged into
the existing `MarkersStack`/`MarkerGroups` array-of-objects entries** (see those two sections above for
the exact position each `parentBundleIdentifier` key lands at):
```
MarkersStack[i].ParentBundleIdentifier    // Params::MarkerRuleLayer::parentBundleIdentifier
MarkerGroups[i].ParentBundleIdentifier    // Params::MarkerInstanceLayer::parentBundleIdentifier
```
Both `-1`/absent = root (not inside any Bundle) — no range to validate on import, an out-of-range or
dangling `ParentBundleIdentifier` degrades to root (loud, logged), the same soft-degrade posture ARCH
§19.12 rules for this whole feature.

**Additive, no `SanGenVersion` bump** — same precedent class as every prior top-level-array addition
this file records (Corrections 12/14/16/18): an unrecognized-key-tolerant importer simply never finds
`MarkerLayerBundles` in an older file, and the two merged `ParentBundleIdentifier` keys are absent-key
tolerant on any pre-Bundle export. Not self-ratified — IO Architecture Expert territory for final
migration-mechanics sign-off, per the same split already used at §16.4/ARCH §19.4.

**Scope note:** this correction covers Markers only. `PropGroups`/`DecalGroups` gain no
`ParentBundleIdentifier` field by this correction — the analogous `PropLayerBundles`/
`DecalLayerBundles` arrays are a separate, later, independently-ticketed addition (ARCH §19.2), not
implied or reserved by this entry.

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
