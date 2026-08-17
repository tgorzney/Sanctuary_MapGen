# PARAMS_PIPELINE_SPEC — the data model & generation pipeline

Historical source: `core/Parameters.h` (`GenerationParams`, the v1 god object) +
`core/params/Params_{Enums,Gradients,Environment,ErosionFlow,Geometry}.h`
(namespace `SanmapGen`). **This god object is retired.** ARCH §5 (god-object
dismemberment) is ratified law: `GenerationParams` is dismembered into typed
`Params::*` modules under `src/params/`, aggregated by `Params::MapRecipe`. The
dead `core/data/*` + `GenParams_*` family (duplicate `StratumSettings`,
`LayerType`, empty stubs) is deleted per ARCH §5.5.

This spec now records two things: (a) how the v1 god-object's logical groups
map onto the live `Params::*` types, and (b) how those types map onto the
`.sanmap` schema v3 top-level sections that own their round-trip
(`SANMAP_FORMAT_SPEC`, ratified by work-order `SPEC-4`). It is a **refresh**
pass (`SPEC-4`), not a new design — the module boundaries themselves were
already settled in `ARCH.md` §5.

## Ownership map — v1 god object → v2 `Params::*` → `.sanmap` v3 section

| v1 `GenerationParams` group | v2 owner (`src/params/`) | `.sanmap` v3 section |
| --- | --- | --- |
| Seed, MapSize, TerrainMin/MaxHeight, ScaleFeaturesToMapSize, GlobalGravity | `Geometry_PARAMS` fields on `MapRecipe` | `GeneralMapSettings` — `MapSize`/global `TerrainMaxHeight` are the format's own `width`/`length`/`height`, **not** duplicated (verified deletion, `SANMAP_FORMAT_SPEC`) |
| GamedataPath, GlobalEnvironmentPath | (none — app-local, not a recipe field) | **removed from `.sanmap` entirely** — future global app-settings file, not designed yet |
| GeoLayers (GeoLayerDef→NoiseLayer[]) | `Layers_PARAMS` (`GeoLayer`/`Layer`/`LayerStack`, `LAYER_SYSTEM_SPEC`) | `HeightmapStack` |
| Post-process mask stacks (DetailNormal/Smoothness/Tint/Hole) | `Layers_PARAMS` (4× `LayerStack`; still a PARAMS-promotion gap, `IO_PARITY_REPORT.md` §5.A) | `DetailNormal` reserves one field only; the other three stacks have no `.sanmap` v3 section yet — out of scope for `SPEC-4` |
| GlobalSymmetryMask, SymAlgorithm, blend, blur radius, cross-fade, cylinder/torus radii, detection tolerance | `Symmetry_PARAMS` | `Symmetry` |
| Per-stratum slope gate fields | `Params::Stratum` (ARCH §7.1, the sole per-stratum type) | ground truth stays per-stratum; `SlopeDefaults` supplies shared defaults only (`MASKING_SPEC` §1.7) |
| SpawnPointCount, HydroMultiplier, ReclaimDensity, MexDensity (v1: global scalars, never exposed in UI) | `Params::MarkerRule` (NEW per-layer fields) | `MarkersStack` — cardinality change: global → per-layer |
| Marker color/scale/icon globals (Alloy/Plasma/Spawn) | `MarkerRules_PARAMS` | `MarkersStack.GlobalMarkerSettings` |
| Armies (map→Army), UnitDefinitions | format-native `armies` dict (+ `armyColor`/`alias`) + `Params::UnitRule` (NEW type) | `armies[key]` (format-native, lowerCamelCase additions) / `UnitsStack` |
| MarkersList (baked positions) | (deleted) | **deleted** — superseded by `MarkersStack` rules |
| Water, Atmosphere, Stratums (9) "tab data" | `Water_PARAMS`, `Atmosphere_PARAMS`, `Stratum_PARAMS[]` | the format's **own** native top-level blocks — never `mapGeneratorData`-style duplicates (verified deletion: `.Atmosphere`, `.Stratums`, `.Water` sub-block) |
| ProceduralMarkerLayers, PlacedMarkerLayers, Areas | `MarkerRules_PARAMS` / (Areas has no `Params::MapArea` yet, `IO_PARITY_REPORT.md` §5.A) | `MarkersStack`; Areas not yet promoted, out of scope here |
| UseGPUTerrain, UseGPUFlowMap, UseGPUMarkers, WYSIWYGBaking, GPUPreviewIterations, FastPreviewMode | `DispatchPolicy` (ARCH §4) | **removed from `.sanmap` entirely** — global app-settings only, read once at startup to seed `Sys::DispatchPolicy`, never per-map |
| PreviewLayers, Show* toggles | UI-local state (`PREVIEW_COMPOSITING_SPEC`) | not a `.sanmap` concept |
| GeneratedSpawns/Mexes/Hydros/Trees | resolved `Data::` instance arrays (computed output, not settings) | not serialized as settings; instances round-trip through the format's own `markers`/`props`/`decals`/`armies` dicts, never a SanGen section |

### Two reserved sections with no v1 predecessor
`Flow` and `Accumulation` (`SANMAP_FORMAT_SPEC` — a two-simulation
velocity→accumulation model) have no v1 `GenerationParams` field to trace to —
they are reserved keys with TBD field lists, distinct from both the per-layer
`ErosionLayerSettings` and the current `FlowAccumulation` (drainage/routing)
stage. See `SANMAP_FORMAT_SPEC` for the full disambiguation.

### GPU/GL state (ARCH §3.2/§5.1, evicted — unchanged by this refresh)
`UnitAtlasTexture`, `UnitAtlasUVs`, `IconCache`, `EntityIDBuffer`, per-Stratum
`preview*Tex` — none of these are PARAMS and none are `.sanmap` fields. They
live in `SYS`/`DATA` (`GpuResource_SYS`, `EntityIdBuffer_DATA`).

### DOP patterns (PRESERVE, unchanged by this refresh)
- `StaticPropsList`-equivalent → `Data::Props_DATA` (real SoA, `PLACEMENT_SCATTER_SPEC`).
- `MarkerSpatialGrid` (32×32) → `Data::SpatialGrid_DATA` (ARCH §8.3).

## The generation pipeline (stage order superseded)
The v1 dirty-hash chain quoted here previously —
`GetHash = GetPlacementHash(GetFlowHash(GetErosionHash(GetBlendHash())))`, i.e.
`Blend → Erosion → Flow → Placement` — is **superseded**. The binding v2 stage
order is ARCH §7.4 / `MASKING_SPEC` §1.4:
```
NoiseBlend → Erosion → Thermal → FlowAccumulation → Mask → Placement → Bake
```
Mask was inserted after every sim (not before them, as the old order implied)
so the slope/visibility gate evaluates against the *final* terrain, and
Thermal + Bake were named as their own stages. `Generation_PIPELINE` (ARCH
§3.3) owns the dirty-hash DAG and per-stage backend policy; no PARAMS type
knows the pipeline shape (ARCH §3.2).

## Key supporting types (current names, `src/params/`)
- **`Layers_PARAMS`** (`GeoLayer`/`Layer`/`LayerStack`) — noise (type/fractal/
  freq/octaves/gain), density shaping, levels, blend; per-layer erosion moved
  out into the Simulation-layer type (`LAYER_SYSTEM_SPEC`).
- **`ErosionLayerSettings`** (`Erosion_Settings_PROC.h`) — droplet count,
  lifetime, gravity, evaporation, rain noise, thermal iterations/rate;
  per-layer, lives inside `HeightmapStack`'s Simulation-layer entries.
  Distinct from the reserved `Flow`/`Accumulation` sections above.
- **`Params::Stratum`** (`Stratum_PARAMS.h`) — the sole per-stratum settings
  type (ARCH §7.1): appearance, soil physics, mask slope gate + the new
  `bSlopeUseGlobal` flag, stored-mask merge mode, remap.
- **`Params::MarkerRule` / `PropRule` / `DecalRule` / `UnitRule`**
  (`MarkerRule_PARAMS.h`, `ScatterRule_PARAMS.h`) — placement filters; see
  `PLACEMENT_SCATTER_SPEC` for the full field list and the `MarkersStack`/
  `PropsStack`/`DecalsStack`/`UnitsStack` IO wrapping (`UnitRule` is new).
- **`Army` / `UnitTransform` / `UnitGroup`** — format-native, not `Params::*`;
  see `SANMAP_FORMAT_SPEC` Entity collections (`armyColor`, `alias` additions).

## Enums (`Params::Enums_PARAMS` or equivalent)
`SymmetryFlags` (Point/X/Z/XY/Radial bitmask); `SymmetryAlgorithm`
(Fold/Blur/CrossFade/Cylinder3D/Torus3D/NativeHash/Superposition);
`MarkerPriority`; `MarkerGradientType`; `SkyIntensityMode`; `BlendMode`;
`NoiseType`; `FractalType`; `ImportedMaskMode`; `LayerType` (Material/
Simulation, `LAYER_SYSTEM_SPEC` — supersedes v1's Terrain/Prop/Decal/Manual/
Fixed).

## Implications for the ARCH (status)
- ~~Adopt `params/Params_*` as the single data model; delete `data/*`~~ —
  **done**, ARCH §5.1/§5.5.
- ~~Split `GenerationParams` into per-domain structs; evict GPU/GL state~~ —
  **done**, ARCH §5.1.
- Pipeline stage order — **superseded**, see above; ARCH §7.4 is authoritative,
  not the chain quoted in earlier drafts of this page.
- Slope/passability is **Exact**-class (Constitution §4; engine-parity 30°
  threshold) — unchanged.
- `.sanmap` v3 top-level section ownership (this refresh's new content) — see
  `SANMAP_FORMAT_SPEC` for the ratified schema; this page tracks only the
  `Params::*` ↔ section mapping, never the JSON shape itself.
- **Open, not settled by this refresh:** Areas (`Params::MapArea`), the three
  non-`DetailNormal` mask `LayerStack`s (Smoothness/Tint/Hole), independent
  `length` support, the `Flow`/`Accumulation` two-simulation model, the
  recursive `GeoLayer` grouping, and the shared Group/Layer/LayerType
  hierarchy for the four scatter Stacks — all named and reserved, none
  designed here (`IO_PARITY_REPORT.md` §5.A / Step 8, `SANMAP_FORMAT_SPEC`
  Out-of-scope).
