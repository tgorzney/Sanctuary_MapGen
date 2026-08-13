# MODDING_SCRIPTING_SPEC — map scripting, AI, modding

Source: `engine/LJ/lua/{maps,AI,documentation,common}`. **Scope note: only
partially read so far** — the map lifecycle, sandbox, and validators are
covered; `AI/` (huge), `host/`, `client/`, `systems/`, `examples/`, and `mods/`
still need a deep pass (see end).

## Lua runtime & sandbox
- LuaJIT / Lua 5.1 (`_VERSION = "Lua 5.1"`). Modules loaded via `Import("path")`.
- **Sandboxed:** available functions are restricted through `.luarc.json`;
  `load`/`loadstring` are forced to text mode and **precompiled bytecode is
  rejected** (security). `builtInDocumentation.lua` documents the allowed stdlib.

## Map scripting (events)
- Each map folder under `lua/maps/<MapName>/` holds `<MapName>_data.lua` (and a
  `_debug` variant); templates: `defaultMap_script.lua`, `showcase_script.lua`.
- **Lifecycle hooks:** `MapPopulate()`, `MapStart()`, `Start()`.
- **Scripting API seen:** `NewThread(fn, ...)`, `WaitSeconds(n)`, `WaitTicks(n)`;
  `CreateUnit(armyId, tpId, position)`; `Orders.IssueOrder{ order="Move",
  units={u}, targetPosition=... }`; `Armies[armyId]:GetListOfUnits(tagExpr)`;
  `EngineClasses.float3(x,y,z)`; `MapUtils.GetMapName()`; `TestUtils.
  SpawnDebugUnitGroup(army, groupName)`; `table.random(list)`.
- **Army indices:** Chosen=0, Guard=1, EDA=2 (see data spec).

## Tags system
Units carry `tags`; queries use a `Tags` table with **multiplication as set
intersection**, e.g. `Tags.FACTORY * Tags.STRUCTURE`, `Tags.TECH3 * Tags.LAND *
Tags.BUILDABLE_BY_T3_FACTORY`. Tag families include faction (EDA/CHOSEN/GUARD),
layer (LAND/AIR/NAVAL/STRUCTURE), tech (COMMAND/TECH1..4), and role
(DIRECT/INDIRECT/ANTI_AIR/DEFENCE/CONSTRUCTION/ECONOMIC/INTEL/…). The validator's
`CreateUnitID` derives a tpId purely from a unit's tags.

## AI system (scoped; internals still a deep read)
`AI/` is large: `AIFunctions.lua` (233 KB), `AIInit.lua`, `AIStrategyManager`,
`AITargetManager` (96 KB), `AILocationCreator/Manager`, `ProfilerAI`,
`debugUtilities`.
- **Pluggable AI mods** live in `AI/mods/<name>/` — ships with `AI-Sanctuary`,
  `AI-Lukas`, `AI-Uveso`. A mod = `AIPlatoonFunctions.lua` + `formers/` +
  `strategies/`. This is the entry point for authoring a custom AI.
- **`AIMarkerGenerator.lua`** builds the AI's spatial analysis of a map: a
  terrain path map with per-movement-layer pathability (`IsPathable`,
  `CanUnitMoveOnTerrainType`), flood-fill area detection, and derives
  `GetStartPositions`, `GetAlloyPositions`, `GetLandExpansions`,
  `GetNavalExpansions`, connecting markers with pathing.
- **Relevance to SanGen:** a generated map must be *analyzable* by this — valid
  spawn/alloy/expansion marker placement and terrain that is pathable per
  movement layer. SanGen's marker/terrain generation should target these
  invariants.

## Blueprint validators (good-vs-bad data)
`AI/UnitBlueprintValidator.lua` + `ProjectileBlueprintValidator.lua`: per-section
`*Check` functions validate a loaded template and repair missing/invalid fields
with defaults, logging each change. Entry points: `ValidateUnitBlueprints`,
`CheckUnitBlueprint`, `analyzeUnitID`, `CreateUnitID`. Pattern to mirror in
SanGen: validate → default → log, never trust a pre-alpha file blindly.

## Still to read (deep pass for the ARCH Expert)
`AI/*` internals; `host/` and `client/` (authoritative vs presentation split);
`systems/`; `examples/`; `mods/` (how mods are declared/loaded); the full
`builtInDocumentation.lua` engine API surface. Needed for: custom map AI, event
scripting depth, and shipping SanGen-authored maps/mods.
