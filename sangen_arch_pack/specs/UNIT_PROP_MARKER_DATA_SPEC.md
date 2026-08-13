# UNIT_PROP_MARKER_DATA_SPEC — game entity definitions

Source of truth: lua at `engine/LJ/lua/common/{units,props,markers}`,
`resourceSpot.lua`, `area.lua`, and `AI/UnitBlueprintValidator.lua`
(authoritative tpId decoder). **The `.sanpack` files are assets only** (dds
icons, editor brushes, models/textures) — NOT the definitions.

## Factions
Three factions: **Chosen, Guard, EDA**. Army indices in map scripts:
Chosen = 0, Guard = 1, EDA = 2.

## tpId scheme (authoritative, from `analyzeUnitID`)
A template id is 7 chars: `<object><faction><type><tech><role><id2>`.
- **char1 object:** `u`=Unit, `p`=Projectile, `e`=Prop.
- **char2 faction:** `c`=Chosen, `g`=Guard, `e`=EDA (`x`=unknown).
- **char3 type:** `s`=Structure, `a`=Mobile Air, `l`=Mobile Land, `n`=Mobile
  Naval, `o`=Mobile Orbital.
- **char4 tech:** `0`=Commander, `1`=T1, `2`=T2, `3`=T3, `4`=T4/Experimental,
  `5`=experimental/unknown.
- **char5 role:** 0 Direct_Fire, 1 Indirect_Fire, 2 Anti_Air, 3 Anti_Naval,
  4 Defence, 5 Construction, 6 Economy, 7 Intel, 8 Special, 9 Civilian.
- **chars6-7:** unique id. Example `ucl3001` = Unit / Chosen / Land / T3 /
  Direct / 01.

## Units
- Templates in `units/unitsTemplates/<tpId>/` (~280). `availableUnits.lua` =
  catalog; `templateExplainations.lua` = the schema; `unitEnums.lua` = enums.
- **UnitTemplate sections:** `general` (displayName, icon{shape,symbol,tech},
  iconUI, iconUIType, tpId, orders, toggles), `economy` (buildTime, cost/
  production/maintenance/storage), `defence` (armor/health/shields), `movement`
  (type enum, speed, collision layers), `intel` (vision/radar/sonar/…),
  `weapons[]` (damage, range, reload, aimControllers, projectileTemplate,
  muzzles, beam), `collisionInfo`, `footprint`, `skirtSize` (structures),
  `visuals` (mesh LODs, effects), `tags`, `audio`.
- Enums the UI needs: Layer (Air/Land/WaterSurface/Water/Seabed); MovementType
  (Gunship/Hover/Legs*/Tracks*/Plane/WaterSurface/UnderWater); IconShape/Symbol/
  Tech (strategic icons).

## Props
- `props/propsTemplates/` holds `.santp` templates. Most environment props live
  in `Environment.sanpack` (2.3 GB), referenced by maps via `blueprintPath` like
  `Environment/01_Highlands/Props/edbm0149/edbm0149.santp` (codes edbm*, edmm*).

## Markers
- `markers/markerTemplates/`: `alloyMarker` (mex), construction/formation/
  selection brackets, `m002`, `m003`. `resourceSpot.lua` maps `alloys` →
  alloyMarker.santp + decal `alloy_spot.sandecal`. Energy is commented out ⇒
  resource markers = alloys only today.

## Areas
- Engine `Area` = center + size; map `MapArea` = corner x,y + width,height.
  `area.lua Area.FromMapArea` converts. SanGen maps between them on import/export.

## The `.san*` proprietary format family
`.sanmap` (map), `.santp` (unit/prop/marker template), `.sandecal` (decal),
`.sanmodel` (proprietary FBX-like mesh — per owner), `.sananimation`, `.sanvfx`,
`.sanmaterial`. SanGen references these by path; it does not need to parse the
mesh/vfx bodies for map generation, only the templates/decals.

## Asset validation — pre-alpha files are unreliable (REQUIRED)
The game ships `AI/UnitBlueprintValidator.lua` (171 KB) and
`ProjectileBlueprintValidator.lua`: they validate every blueprint **data**
section and auto-fill missing/invalid fields with safe defaults, logging each
fix. Mirror that **validate-then-fallback** pattern.
- BUT those validate lua data, NOT texture/icon files. SanGen must add its own
  **asset-safety layer** for loading dds icons/textures into the UI (the
  100k-scroll lists): before loading, cap file size and image dimensions,
  sanity-check the dds/format header, and fall back to a placeholder icon on any
  failure — never load an unverified/corrupt file straight into RAM or the UI.
  This is the concrete form of the project rule "validate all input to avoid
  crashes."

## What SanGen actually needs (for the ARCH)
Placement stores only `UnitTransform{ type, tpid }`; the UI needs a unit catalog
(tpId + displayName + icon + iconUIType) and a prop-blueprint index (path +
preview icon), both loaded through the asset-safety layer above.
