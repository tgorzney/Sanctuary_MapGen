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

## Entity collections (the distinct domains)
- `areas`: Dictionary<string, Area{ x, y, width, height }>
- `armies`: Dictionary<string, Army{ faction 0/1/2, alloys, energy, groups }>
  - `UnitGroup { units: Dict<string,UnitTransform>, groups: nested UnitGroup }`
    — recursive grouping
  - `UnitTransform : InstancedTransform { type, tpid }`
- `markers`: Dictionary<string, MarkerType{ resource:bool, transforms:
  Dict<string,MarkerTransform> }>
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

## mapGeneratorData — SanGen's generator-state round-trip (CRITICAL)
An extra top-level key present ONLY in SanGen-made maps (e.g. Pandemonium), not
in shipped maps. It is the **entire generator state serialized into the
.sanmap** — i.e. the on-disk form of the code's `GenerationParams`. Import/Export
must round-trip this. Top-level fields observed:
- Terrain/shape: MapSize, Seed, TerrainMinHeight, TerrainMaxHeight,
  ScaleFeaturesToMapSize, CylinderZScale, TorusMajorRadius, TorusMinorRadius,
  GlobalGravity, HydroMultiplier.
- Layers/materials: GeoLayers[], Stratums, DetailNormalMapSize, CrossFadeWidth.
- Symmetry: SymAlgorithm, GlobalSymmetryMask, SymSuperpositionBlend,
  SymmetryBlurRadius, SymmetryDetectionTolerance, SnapImperfectSymmetry.
- Erosion/flow: FlowSettingsParams { FlowMomentum, FlowVolumeMultiplier,
  Iterations, Precipitation, SlopeAdherence, StochasticVariance },
  SlopeSettingsParams.
- Markers/resource: MarkersList, MexDensity, ReclaimDensity, SpawnPointCount,
  marker color/scale/icon globals for Alloy/Plasma/Spawn.
- GPU toggles (match the code survey): UseGPUFlowMap, UseGPUMarkers,
  UseGPUTerrain, WYSIWYGBaking, GPUPreviewIterations, FastPreviewMode.
- Also: Armies (16 entries, 5 keys each), Atmosphere (full fog/sun/sky block),
  Water, GamedataPath / GlobalEnvironmentPath, PresetVersion, Aliases.
- **Implication:** this block is the DATA-layer params in disk form; it belongs
  to a future PARAMS spec and is the Import/Export round-trip contract. Confirms
  the GPU-dispatch toggles the ARCH must unify.
