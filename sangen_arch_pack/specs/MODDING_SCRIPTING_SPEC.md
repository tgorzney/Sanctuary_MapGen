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
- **`Import()` return-value rule — load-bearing, applies to EVERY `Import()`
  consumer, not just scenario scripts (confirmed live 2026-08-20).**
  `Import()` does **not** use a module's `return` statement at all. Per its
  own doc comment (`common/systems/import.lua` ~lines 20-26) it executes the
  target file via `load(fileToImport, "@"..filePath, 'bt', envTable)` inside a
  custom environment table, then returns that env table — capturing **only
  the file's global variables**. A module written as
  `local Scenario = {} ... return Scenario` yields a table with none of
  `Scenario`'s fields — **silently, no error, no warning**. Rule for every
  SanGen-authored (or human-authored) module meant to be consumed via
  `Import()`: expose the module's API as a **global**
  (`Scenario = {}`, not `local Scenario = {}`), and have the caller pull the
  field off the returned env table (`Import(path).Scenario`), not treat the
  `Import()` result itself as the module.

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
  - **⚠️ F1 reliability now in question (2026-08-20, unresolved, flagged not
    resolved here).** During an earlier iteration of the cross-tree-`Import()`
    investigation (superseded finding, see "Scenario-script file split"
    below), the human reported **nothing appeared in the F1 console** despite
    units visibly spawning in-game. That earlier test is now known to have
    been a false positive on an unrelated axis (the import itself never
    succeeded — see below), so this F1 discrepancy's cause is unresolved on
    its own terms too. Do not treat F1 as the sole diagnostic going
    forward — visual/in-game confirmation and, better, an explicit
    `Engine.FileExists()` check (as used in the disproof below) are more
    reliable. Root cause (build/version difference, console state, timing)
    not investigated; a separate, unscoped follow-up.
- **Scripting API seen:** `NewThread(fn, ...)`, `WaitSeconds(n)`, `WaitTicks(n)`;
  `CreateUnit(armyId, tpId, position)`; `Orders.IssueOrder{ order="Move",
  units={u}, targetPosition=... }`; `Armies[armyId]:GetListOfUnits(tagExpr)`;
  `EngineClasses.float3(x,y,z)`; `MapUtils.GetMapName()`; `TestUtils.
  SpawnDebugUnitGroup(army, groupName)`; `table.random(list)`.
- **Army indices:** Chosen=0, Guard=1, EDA=2 (see data spec).

## Scenario-script file split (ratified 2026-08-20, amended 2026-08-20 x3, RETRACTION/correction 2026-08-20; consolidated into `MAP_SCENARIO_SPEC.md` 2026-08-20)

**Current, binding law for this feature — file structure, module API contract,
the three-tier scenario matching system, `alloyMode` semantics, the mandatory-
`spawns` hard requirement, and the execution/timing rules — now lives in
`sangen_arch_pack/specs/MAP_SCENARIO_SPEC.md`.** Read that spec for "what to
build." This section is retained only as the **investigation trail** that
produced that design — the disproven cross-tree-`Import()` hypothesis and the
general `Import()`-mechanics lessons it surfaced (the global-capture rule is
promoted to first-class law in "Lua runtime & sandbox" above) — real, hard-won
evidence about `Import()` itself, useful for any future cross-file linking
question, independent of scenarios specifically. Do not re-derive the scenario
system's file-structure/module-contract law from this section; it has been
removed here to avoid a second, driftable copy.

**⚠️ Read this whole section before touching this feature.** An earlier
amendment in this same file recorded cross-tree `Import()` (script tree →
asset tree) as "✅ Confirmed live." **That confirmation was a false positive
and is retracted below.** The correct, deployed answer is the opposite:
cross-tree `Import()` does not work, and the two files are colocated in the
script tree (`MAP_SCENARIO_SPEC.md` §2). Do not trust anything below this
notice that predates the retraction; read the retraction first.

**❌ RETRACTED — cross-tree `Import()` does not work (disproven live,
2026-08-20). `Sanctuary_Map_System_Rework.md` §12 open question 6 is answered
in the NEGATIVE: a script-tree file cannot `Import()` an asset-tree file.**

Prior text in this section claimed the opposite ("✅ Confirmed live... `Import()`
CAN cross from `LJ/lua` into `Sanctuary_Data/Maps/<MapName>/`"). That claim was
wrong. Evidence chain, in order:

1. **The original "2 BigBots spawned" test that produced the false ✅ was a
   false positive.** It gated an import-path spawn behind
   `pcall(Import, ...)` succeeding, alongside an unconditional direct spawn.
   Both units actually observed spawning came from paths that did **not**
   require a successful cross-tree import; the import-gated spawn never
   actually ran, despite both units appearing (the direct/control spawn plus
   something else — the import-path branch's own gate condition was never
   satisfied). Additionally, the test module used
   `local Scenario = {} ... return Scenario` — a pattern that is
   fundamentally incompatible with `Import()` regardless of path resolution
   (see the global-capture rule above): even if the cross-tree path had
   resolved, `ScenarioTest.SpawnTestBigBot` would have been `nil` and the
   `pcall` branch would never have fired. Two independent reasons the earlier
   "confirmation" could never have been real; a future reader must not
   re-trust that earlier entry.
2. **Direct disproof, live in-game (the load-bearing evidence for this
   retraction).** A diagnostic called `Engine.FileExists(libPath.."/"..path)`
   — the EXACT string `common/systems/import.lua` builds internally at its
   line ~137 (`local ModdedFilePath = libPath.."/"..filePath`) — with
   `path = "../../Sanctuary_Data/Maps/Pandemonium Isthmus/Pandemonium
   Isthmus_Scenarios_Script.lua"`. It returned **false**. Encoded as a
   countable marker-row signal in-game (1 marker = true, 3 markers = false);
   3 markers appeared. The `../../` traversal out of the `LJ/lua` root does
   not resolve.
3. **Consequence:** `Import()` reaches its own
   `Error("Import: Error loading up module: '"..filePath.."'. File doesn't
   exist.", 2)` (import.lua ~line 151) and **throws**, aborting the entire
   enclosing chunk. This is why, across ~13 diagnostic iterations, every
   checkpoint placed BEFORE the `Import` call fired and every checkpoint
   AFTER it never did — regardless of content, position, or file size. (This
   also retroactively explains item 1 above: the "successful" earlier test
   never actually reached its import-gated branch under a working import —
   it degenerated to the control spawn only, and the second unit's origin
   was misattributed.)
4. **Independent second finding, from reading `common/systems/import.lua`
   directly (worth recording as its own rule — it cost hours; promoted to
   first-class law in "Lua runtime & sandbox" above):** `Import()` does not
   use a module's `return` statement at all; it captures only the executed
   file's **global** variables via a custom env table. `local Scenario = {}
   ... return Scenario` silently yields a table with none of `Scenario`'s
   fields — no error. This is why the naive `Scenario.lua` module pattern is
   wrong for anything meant to be `Import()`-consumed, independent of the
   cross-tree question.

**The now-correct, deployed arrangement** is the colocation rule ratified in
`MAP_SCENARIO_SPEC.md` §2: both files live in `LJ/lua/maps/<MapName>/`, linked
by `Import("maps/<MapName>/<MapName>_Scenarios_Script.lua").Scenario`. This is
live and deployed now. The map's asset folder contains no `.lua` files. The
human's original goal — scenario file in the map (asset) folder so only one
file needs moving if `_data.lua` later relocates — is not achievable via
`Import()` and has been abandoned. If `_data.lua` ever relocates, both files
move together.

⚠️ See the F1-reliability side-note under "Map scripting (events)" above —
its cause remains unresolved and is now doubly uncertain given the false
positive it was originally attached to; do not rely on F1 alone for future
`Import()`/lifecycle diagnostics. Prefer explicit `Engine.FileExists()` /
marker-signal-style checks as used in the disproof above.

**⚠️ Unverified observation, NOT a confirmed cause — flag only, do not
over-claim (2026-08-20).** While a 543-line scenario file sat at
`Sanctuary_Data/Maps/<MapName>/<MapName>_Scenarios_Script.lua` (during the
now-retracted cross-tree experiment), the game **hung on window close and
required a force-quit**. Replacing it with a 9-line stub in the same asset-tree
location stopped the hang. At the time, both the 543-line and 9-line files
were failing to load via `Import()` regardless of size (per the disproof
above), so file size/content cannot be the load-bearing variable through the
`Import()` path — the mechanism is **unexplained**. Possible but unconfirmed
candidate: some asset-folder scanner/watcher reacting to a large `.lua`
sitting in the map asset tree, independent of whether anything ever
`Import()`s it. Worth knowing before anyone puts a large `.lua` file in a map
asset folder (`Sanctuary_Data/Maps/<MapName>/`) again, even now that the
design no longer calls for it — a human map author could still do this by
hand. Not investigated further; a separate, unscoped follow-up if it recurs.

**Known tension — now moot, kept for audit trail.** An earlier amendment
flagged tension between the human's stated expectation that `_data.lua` would
eventually relocate to the asset folder, and `Sanctuary_Map_System_Rework.md`
§6.3/§10.2 step 7, which keep `_data.lua` at `LJ/lua/maps/<N>/` unchanged with
no relocation proposed. That tension is now **moot**: the colocation rule
(`MAP_SCENARIO_SPEC.md` §2) ties both files together regardless of where
`_data.lua` ends up, so no design decision hinges on resolving it. If
`_data.lua` relocation ever becomes live work independent of this feature, the
original ❓ (raised to the human/Format Expert) still applies: nothing in the
source material besides the human's own expectation and a stray decoy copy in
`Sanctuary_Data/Maps/Pandemonium Isthmus/` (which the engine does **not**
load) supports `_data.lua` actually moving; `Sanctuary_Map_System_Rework.md`'s
explicit design keeps it in `LJ/lua/maps/`.

**Confirmed, not "not yet confirmed": sibling same-tree `Import()` is the
standard, load-bearing mechanism here** — `<MapName>_data.lua` importing
`<MapName>_Scenarios_Script.lua` from the *same* `LJ/lua/maps/<MapName>/`
folder is exactly the ordinary, root-relative `Import()` pattern already used
throughout the engine (`Import("common/gameUtils.lua")`,
`Import("host/testUtils.lua")`, etc.) — nothing exotic, and no longer resting
on the retracted cross-tree finding. Genuinely still open: whether
`Import()`'s environment-capture (`__index = _G`, `import.lua:107-109`) or
the `Mods_Active` reverse-scan (`replace/<path>` / `append/<path>`, §3.2 of
the Rework doc) interacts with this same-tree sibling call in any way that
matters (e.g. a mod replacing one file but not the other) — the source
material describes both mechanisms but never traces this specific pairing
end-to-end. Lower-risk than the retracted cross-tree question since it's the
engine's own established pattern, but flagged, not verified.

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
