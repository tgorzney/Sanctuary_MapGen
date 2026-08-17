# MODDING_SCRIPTING_SPEC — map scripting, AI, modding

Source: `engine/LJ/lua/{maps,AI,documentation,common,host}`. **Scope note: only
partially read so far** — the map lifecycle (now including the confirmed
`script.lua`→`hostMain.lua Start()` sequencing, see below), sandbox, and
validators are covered; `AI/` (huge), the rest of `host/`, `client/`,
`systems/`, `examples/`, and `mods/` still need a deep pass (see end).

## Lua runtime & sandbox
- LuaJIT / Lua 5.1 (`_VERSION = "Lua 5.1"`). Modules loaded via `Import("path")`.
- **Sandboxed:** available functions are restricted through `.luarc.json`;
  `load`/`loadstring` are forced to text mode and **precompiled bytecode is
  rejected** (security). `builtInDocumentation.lua` documents the allowed stdlib.

## Map scripting (events)
- Each map folder under `lua/maps/<MapName>/` holds `<MapName>_data.lua` (and a
  `_debug` variant); templates: `defaultMap_script.lua`, `showcase_script.lua`.
- **Dual-path trap (confirmed live):** a map's *script* folder
  (`LJ/lua/maps/<name>/` — the path `LoadMapData()` actually builds from
  `libPath`) is not necessarily where its *asset* folder lives
  (`Sanctuary_Data/Maps/<name>/`, sitting next to the `.sanmap`/Props/Textures).
  A whole feature was live-tested against the wrong copy for several matches
  with zero error. Verify which `_data.lua` the engine actually loaded via the
  F1 console (below) before trusting an edit will take effect.
- **Lifecycle (confirmed, traced through `script.lua`, `common/mapUtils.lua`,
  `common/gameUtils.lua`, `host/hostMain.lua`) — `MapPopulate()`/`MapStart()`
  are dead, no caller found anywhere.** The real call chain, host side, before
  any simulation tick unless noted:
  1. `script.lua`'s `init()` → `InitLobby(lobbyData, argString)`.
  2. `mapUtils.LoadMapData(lobbyData.mapPath)` — loads the `.sanmap`,
     `Import()`s the per-map `<mapName>_data.lua` (its top-level code runs
     synchronously here; any `NewThread()` it queues is deferred, not run
     yet).
  3. `gameUtils.CreateArmies()` — creates every army from
     `GameInfo.MapData.armies`; **armies fully exist by the end of this step,
     still before any tick.** Also calls `SpawnInitialUnits()`: for every
     non-empty army with a `Spawn` marker, spawns exactly one hardcoded
     commander (`CreateUnit(armyIndex,
     Factions.FactionsData[army.faction].initialUnit, startingPos)`) — not
     data-driven.
  4. `host/hostMain.lua`'s `Start()` → simulation tick 0 →
     `mapUtils.RunMapSetup(true)` (props/decals/alloy resource spots) → THEN
     any `NewThread()` coroutine queued in step 2 fires.
- **A per-map `_data.lua` can spawn units itself — no shared-engine edit
  needed:** because its `NewThread` (queued in step 2) fires after
  `CreateArmies()` has already run (step 3), its callback already has
  `Armies`, `CreateUnit`, `GetMarker`/`MarkerToPosition`,
  `SpawnGroup`/`SpawnGroupUnit` available with no `Import()` — all are
  native/global by that point. **Constraint confirmed live (2p/4p/8p): only
  ONE `NewThread()` call per script is honored** — a second, separate call
  silently never runs. A script with more than one host-deferred job must
  merge them into a single `NewThread` callback.
- **Worked pattern (confirmed working live 2026-08-17):** "spawn tpId X for
  every army, at their own spawn point" from a single `NewThread` — loop
  `Armies`, skip empty slots, read each army's spawn position via
  `GameInfo.MapData.markers.Spawn.transforms[army.name]` (proven safer than
  `GetMarker`/`MarkerToPosition`, whose reachability from arbitrary file scope
  was never formally verified), call `CreateUnit(armyIndex, tpId,
  spawnMarker.position)` inside its own `pcall` per army so one army's error
  doesn't abort the rest, log a placed/skipped summary.
- **Debugging: use the F1 in-game console, not `game_logs/*.txt`.** The disk
  logs are confirmed unreliable — stayed empty across a full session even
  while confirmed-running code's `Log()` calls were firing. F1 opens a
  scrolling console showing live `[Host]`/`[Client]` `[DEBUG]`/`[ERROR]`
  lines, every `Import:` file load, and the script's own `Log()`/`Warn()`
  output — point future debugging first here.
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
`AI/*` internals; the rest of `host/` and all of `client/` (authoritative vs
presentation split — `host/hostMain.lua`'s `Start()` is now traced and
confirmed for map-lifecycle sequencing, see "Map scripting" above; the broader
split is still open); `systems/`; `examples/`; `mods/` (how mods are
declared/loaded); the full `builtInDocumentation.lua` engine API surface.
Needed for: custom map AI, event scripting depth, and shipping SanGen-authored
maps/mods.
