# PARAMS_PIPELINE_SPEC — the data model & generation pipeline

Source of truth: `core/Parameters.h` (`GenerationParams`) + `core/params/
Params_{Enums,Gradients,Environment,ErosionFlow,Geometry}.h` (namespace
`SanmapGen`). **This is the LIVE model.** `core/data/*` and `GenParams_*` are the
DEAD family — ignore/retire them (they diverge: duplicate `StratumSettings`,
`LayerType`, empty stubs).

## GenerationParams (the god object — dismember per hit-list)
One struct holding everything. Logical groups:
- **General:** PresetVersion, paths (GlobalEnvironmentPath, MapFolderPath,
  GamedataPath), Seed, MapSize, TerrainMin/MaxHeight, ScaleFeaturesToMapSize,
  GlobalGravity.
- **Markers/gamedata:** per-type (Alloy/Plasma/Spawn) scale/color/icon,
  `MarkersList` (map→MarkerTransform), KnownMarkerTypes.
- **Armies/units:** Armies (map→Army), UnitDefinitions (from `.santp`),
  placement-tool state.
- **Layers:** `GeoLayers` (GeoLayerDef→NoiseLayer[]) for heightmap; post-process
  mask stacks (DetailNormal/Smoothness/Tint/Hole); `GetFlatLayers()` flattens.
- **Symmetry:** GlobalSymmetryMask (bitmask), SymAlgorithm, blend, blur radius,
  cross-fade, cylinder/torus radii, detection tolerance.
- **Gameplay:** SpawnPointCount, HydroMultiplier, ReclaimDensity, MexDensity.
- **Tab data:** Water, Atmosphere, Stratums (9), ProceduralMarkerLayers,
  PlacedMarkerLayers, Areas.
- **Perf/accuracy toggles:** UseGPUTerrain, UseGPUFlowMap, UseGPUMarkers,
  WYSIWYGBaking, GPUPreviewIterations, FastPreviewMode. (These are the CPU/GPU
  dispatch flags the ARCH must unify — §9 of the Constitution.)
- **Preview:** PreviewLayers (Z-order compositing + blend modes), Show* toggles.
- **Generated output:** GeneratedSpawns/Mexes/Hydros/Trees (Point2D[]).

### GPU/GL state living in the data model (EVICT — hit-list #2)
`UnitAtlasTexture` (GLuint), `UnitAtlasUVs`, `IconCache` (name→GLuint),
`EntityIDBuffer` (mutable selection buffer), and per-Stratum `preview*Tex`. These
belong in a GPU/UI layer, NOT in DATA.

### DOP patterns already present (PRESERVE)
- `StaticPropsList` = flat SoA `PropInstance{X,Y,Z,TintColor,IconScale,indices}`
  — props segregated from interactive markers so UI loops don't touch 100k+ items.
- `MarkerSpatialGrid` (32×32 `MarkerChunk`) for O(1) click hit-testing.

## The generation pipeline (from the dirty-hash chain)
`GetHash = GetPlacementHash(GetFlowHash(GetErosionHash(GetBlendHash())))` — a
strict dependency chain = the pipeline order:
1. **Blend** — noise layers (GeoLayers) → heightmap; depends on per-layer
   density shaping, levels, blend, noise hash (seed+symmetry).
2. **Erosion** — hydraulic droplet + thermal (per-layer ErosionSettings);
   depends on the blended heightmap.
3. **Flow** — flow + accumulation (FlowSettingsParams); depends on eroded map.
4. **Placement** — marker rules + explicit markers; depends on flow.
Each stage re-runs only when its hash changes (dirty flags). This chain is the
backbone the ARCH's module/layer split should follow.

## Key supporting structs
- **NoiseLayer** (Params_Geometry): noise (type/fractal/freq/octaves/gain) +
  image/bake caching + density shaping (Land/Plateau/Mountain/Ramp) + Photoshop
  levels + per-layer Erosion + prop/decal fields. `GetNoiseHash` caches raw noise.
- **ErosionSettings** (Params_ErosionFlow): DropletCount (default 1,000,000),
  lifetime, gravity, evaporation, rain noise, orographic wind, deposition,
  thermal iterations/rate, plus scientific flow vars.
- **FlowSettings:** Precipitation, Iterations, `UseGPU`, stochastic flow vars,
  accumulation. (Note the rival GPU flag vs `UseGPUFlowMap` — unify.)
- **StratumSettings:** 9× texture set (albedo/normal/mask) + tiling + remaps +
  **soil physics** (hardness/friction/cohesion/capacityMult/absorptionRate) used
  by erosion + `previewColor` + imported mask.
- **MarkerRule:** placement filter (min/max slope+height, clearance, edge
  padding, priority, focus gradient, density/count, symmetry).
- **SlopeSettings:** `bUseEngineParityMath` + gradient stops at **29°/30°** =
  the gameplay-passability threshold ⇒ slope is an **Exact** class calc (§9).
- **Army / UnitTransform / UnitGroup / MarkerTransform** (Params_Environment):
  MarkerTransform is rich (symmetry id/mask, alias, type, custom JSON name, icon
  override, IsManual/IsValid/IsHidden).

## Enums (Params_Enums)
SymmetryFlags (Point/X/Z/XY/Radial bitmask); SymmetryAlgorithm (Fold/Blur/
CrossFade/Cylinder3D/Torus3D/NativeHash/Superposition); MarkerPriority;
MarkerGradientType; SkyIntensityMode; BlendMode; NoiseType; FractalType;
ImportedMaskMode; LayerType (Terrain/Prop/Decal/Manual/Fixed).

## Implications for the ARCH
- Adopt `params/Params_*` (SanmapGen) as the single data model; delete `data/*`.
- Split GenerationParams into per-domain DATA structs; evict GPU/GL state to a
  GPU/UI layer; keep the SoA/spatial-grid DOP patterns.
- The pipeline (Blend→Erosion→Flow→Placement) defines the PROC module boundaries;
  the dirty-hash chain is the recompute contract.
- Slope/passability is Exact-class (engine-parity 30° threshold).
