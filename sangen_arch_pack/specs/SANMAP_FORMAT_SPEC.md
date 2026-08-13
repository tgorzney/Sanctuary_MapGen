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
- **Still to read:** `MapUtils.cs` (save/load logic) and `Colors.cs` (palette)
  — pending, needed for the full import/export spec.

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
