# UNIT_PROP_MARKER_DATA_SPEC — game entity definitions

Source of truth: lua at `engine/LJ/lua/common/{units,props,markers}` +
`resourceSpot.lua`, `area.lua`. **The `.sanpack` files are assets only** (dds
icons, editor brushes, models/textures) — NOT the definitions.

## Units
- Templates live in `units/unitsTemplates/<tpId>/` — ~280 templates. `tpId`
  scheme: `u<faction><domain><NNNN>` (factions c/e/g/n…; domain a=air, l=land,
  s=structure/sea, n, w=water). e.g. `ucl3001`.
- `availableUnits.lua` = the catalog; `templateExplainations.lua` = the schema;
  `unitEnums.lua` = small enums (DestroyType).
- **UnitTemplate sections** (LuaLS `---@class`): `general` (displayName, icon
  {shape,symbol,tech}, iconUI, iconUIType land/air/water/amphibious, tpId,
  orders, toggles), `economy` (buildTime, cost/production/maintenance/storage
  EconomyTables), `defence` (armor / health / shields), `movement` (type enum,
  speed, rotationSpeed, collision layers), `intel` (vision/radar/sonar/counter/
  stealth/jamming radii), `weapons[]` (damage, rangeMin/Max, reloadTime,
  aimControllers, projectileTemplate, muzzle groups, beam), `collisionInfo`
  (hitbox), `footprint` (pathfinding), `skirtSize` (structures), `visuals`
  (mesh LODs, effects), `tags`, `audio`.
- Enums the ARCH/UI may need: Layer (Air/Land/WaterSurface/Water/Seabed);
  MovementType (Gunship/Hover/Legs*/Tracks*/Plane/WaterSurface/UnderWater);
  IconShape/IconSymbol/IconTech (strategic icons).
- Projectiles are separate `ProjectileTemplate`s (also `Projectiles.sanpack`).

## Props
- `props/propsTemplates/` holds `.santp` templates (`exe000x/`,
  `defaultWreckage.santp`). Most **environment props live in
  `Environment.sanpack`** (2.3 GB), referenced by maps via `blueprintPath` like
  `Environment/01_Highlands/Props/edbm0149/edbm0149.santp` (codes edbm*, edmm*).
- Map props store only `{ blueprintPath, transforms[] }` (pos/rot/scale) — see
  the format spec.

## Markers
- `markers/markerTemplates/`: `alloyMarker` (mex/resource), `constructionBracket`,
  `formationBracket`, `selectionBracket`, `m002`, `m003`. Templates are `.santp`.
- `resourceSpot.lua` `ResourcesInfo`: `alloys` → strategicIcon `alloy_spot`,
  decal `Environment/Common/Decals/alloy_spot.sandecal`, markerTemplate
  `common/markers/markerTemplates/alloyMarker/alloyMarker.santp`. Energy is
  commented out (no energy decals) — so resource markers = alloys only, today.

## Areas
- Engine `Area` = center `position` + `size` (float2), XZ plane. Map `MapArea`
  = corner `x,y` + `width,height`. `area.lua` `Area.FromMapArea` converts
  (center = x + w/2). SanGen must map between the two forms on import/export.

## What SanGen actually needs (for the ARCH)
- Map placement stores only `UnitTransform{ type, tpid }`; SanGen's UI needs the
  **unit catalog** — tpId + displayName + icon + iconUIType(layer) — to drive
  the 100k-scrollable placement lists. Full weapon/economy detail is game-side,
  not required for placement but available in the templates.
- The heavy environment prop set lives in `Environment.sanpack`; SanGen needs an
  index of prop blueprintPaths + preview icons, not the models.
