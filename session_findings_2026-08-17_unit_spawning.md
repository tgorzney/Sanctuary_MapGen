# Session findings — 2026-08-17 — map-script unit spawning

Source of truth for a Claude Code update to `sangen_arch_pack` /
`sanunit_strategist_pack` / `ARCH.md`. Everything below was confirmed by direct
file reads and/or a live in-game test this session, not guessed.

## 1. CRITICAL: Pandemonium Isthmus has two `_data.lua` copies — only one is real

- `Sanctuary_Data\Maps\Pandemonium Isthmus\_data.lua` — sits next to the .sanmap,
  Props, Textures. **The engine never loads this file.** An entire feature was
  built and live-tested here across several matches; it silently never ran, no
  error either.
- `LJ\lua\maps\Pandemonium Isthmus\_data.lua` — the real one. Confirmed via the
  in-game F1 debug console: `LoadMapData: Loading data from Lua file:
  ...\engine\LJ\lua\maps\Pandemonium Isthmus\Pandemonium Isthmus_data.lua`.
- Implication for MODDING_SCRIPTING_SPEC: `LoadMapData()` builds the per-map
  script path from `libPath`, which points at `LJ/lua`, NOT wherever the map's
  own asset folder (`Sanctuary_Data/Maps/<name>/`) happens to live. Any map
  whose script-authoring folder was assumed to be next to its .sanmap should be
  double-checked in-game (F1 console) before trusting it's the loaded copy.

## 2. Corrects MODDING_SCRIPTING_SPEC's lifecycle-hooks claim

The spec currently says: *"Lifecycle hooks: `MapPopulate()`, `MapStart()`,
`Start()`."* **`MapPopulate()`/`MapStart()` are confirmed dead** — no caller
found anywhere in a full trace. The real, confirmed call chain (traced through
`engine/LJ/lua/script.lua`, `common/mapUtils.lua`, `common/gameUtils.lua`):

1. `script.lua`'s `init()` → `InitLobby(lobbyData, argString)`, on host, before
   any simulation tick:
2. `Import("common/mapUtils.lua").LoadMapData(lobbyData.mapPath)` — loads the
   `.sanmap`, `Import()`s the per-map `<mapName>_data.lua` (its top-level code
   runs synchronously here; any `NewThread()` it queues is deferred, not run
   yet).
3. `Import("common/gameUtils.lua").CreateArmies()` — creates every army from
   `GameInfo.MapData.armies`. **Armies fully exist by the end of this step,
   still before any tick.**
4. `SpawnInitialUnits()` (called at the end of `CreateArmies()`) — for every
   non-empty army with a `Spawn` marker, spawns exactly one hardcoded unit:
   `CreateUnit(armyIndex, Factions.FactionsData[army.faction].initialUnit,
   startingPos)`. This is the commander. Not data-driven.
5. `Import("host/hostMain.lua").Start()` → simulation tick 0 →
   `mapUtils.RunMapSetup(true)` (props/decals/alloy resource spots) → THEN
   dispatches any `NewThread()` coroutines queued in step 2.

## 3. A per-map `_data.lua` can spawn units itself — no shared/core-engine edit needed

Because `NewThread()` queued during `LoadMapData()` (step 2) fires after
`CreateArmies()` (step 3) has already run, a per-map `_data.lua`'s own
`NewThread` callback can safely call, with no `Import()` needed (all already
global by that point):

- `Armies` — global table, native engine, not defined in any `.lua` file.
- `GetMarker(name, category, noError)` / `MarkerToPosition(name, category)` —
  plain Lua globals declared in `common/gameUtils.lua` (no `local`), already
  defined once that file has executed (which it necessarily has by step 3).
  **Caveat**: reachability from an arbitrary file's scope was never formally
  verified — a more defensive alternative that IS proven working (see §5) is
  to read `GameInfo.MapData.markers.Spawn.transforms[army.name]` directly.
- `CreateUnit(armyIndex, tpId, position)` — native engine global, never defined
  in any `.lua` file.
- `SpawnGroup(armyIndex, groupName)` / `SpawnGroupUnit(armyIndex, unitName)` —
  real (not debug-only) exports of `common/gameUtils.lua`, for a data-driven
  (map-`groups`-table-based) alternative to a hardcoded tpId.

**Constraint discovered the hard way: only ONE `NewThread()` call per script is
honored.** A second, separate `NewThread(function() ... end)` call added later
in the same file silently never ran, in every live test (2p/4p/8p), while a
first one (pre-existing, unrelated feature) worked every time. Leading
explanation: the engine only queues one thread per script and drops
subsequent calls — unconfirmed against engine source (native, closed), but it
fully explains the symptom and the fix (merging both jobs into one `NewThread`
callback) resolved it. **Any per-map script doing more than one host-deferred
job must do all of it from a single `NewThread` callback.**

## 4. Confirmed: no T5-tier unit exists anywhere in this build, any faction

Checked exhaustively, zero `T5`/`5xxx` found in:
- `common/units/availableUnits.lua` — the authoritative safety-flag list.
- `common/units/unitsTemplates/` (loose, one folder per tpId) — every
  faction/domain (Chosen `ucl`/`uca`, Guard `ugl`/`uga`, EDA `uel`/`uea`, plus
  structures `ucs`/`ugs`/`ues`) tops out at a `4xxx` tpId. No `5xxx` directory
  exists for any faction.
- `Gamedata/UnitsTemplates.sanpack` — genuinely empty zip (0 central-directory
  entries). Confirms templates aren't hidden in a separate pack; the loose
  folder above is the sole source.
- `Gamedata/Gameplay.sanpack` (592 entries, fully inspected) — zero `t5`
  string matches; strategic-icon tiers only go up to `_t3_`.
- Not checked: `Gamedata/Units.sanpack` (1.35GB, likely models/animations, not
  Lua templates — the dedicated template pack above being empty makes this
  very unlikely to matter, but it's the one gap for 100% certainty).

**Highest validated tier is T4, every faction.** One safe (`true`/OK) T4 land
unit per faction, useful as known-good spawn-test tpIds: Chosen `ucl4004`
ChosenT4BotBig, Guard `ugl4001` GuardT4Bot, EDA `uel4001` EDAT4RailgunSniper.

## 5. Worked example: bonus-unit spawn now live in Pandemonium's real `_data.lua`

Added `SpawnBonusUnits()`: loops `Armies`, skips empty slots, reads each
army's `Spawn` marker directly via
`GameInfo.MapData.markers.Spawn.transforms[army.name]` (not
`GetMarker`/`MarkerToPosition` — this pattern was already proven live by an
existing naval-fleet feature in the same file, specifically to avoid an
unverified-reachability dependency), calls
`CreateUnit(armyIndex, "ucl4004", spawnMarker.position)` per army inside its
own `pcall` (one army's error doesn't abort the rest — mirrors a fix for a
prior "6p test, only 2 of 6 armies got fleets" incident), logs a placed/skipped
summary via `Log()`. Runs unconditionally (every player count, any faction)
from the same single `NewThread` as the file's other host-deferred work.
**Confirmed working live 2026-08-17** (2 BigBots spawned in a 2-army test).

Worth turning into a reusable pattern/snippet in the spec: this is now a
proven, minimal template for "spawn tpId X for every army, at their own spawn
point, safely" from any per-map `_data.lua`.

## 6. F1 in-game debug console — a real diagnostic tool, use it

Pressing F1 in a live match opens a scrolling debug console showing
`[Host]`/`[Client]` `[DEBUG]`/`[ERROR]` lines — every `Import:` file load, and
critically, our own `Log()`/`Warn()` output. No copy/paste or export
available. **This is far more useful than `game_logs/*.txt` on disk, which
stayed empty across this entire session even while confirmed-running code's
`Log()` calls were firing.** Any future modding-spec guidance on debugging
should point here first, not at the log files.

## 7. Separate, unconfirmed: reported severe slowdown in one test

User reported the map "ran insanely slow" in one test session (unclear which
player count). The real `Pandemonium Isthmus_data.lua` already has a
naval-fleet feature (200 Frigates + 50 Battleships per army, 6p/8p tiers only)
whose own comments document a prior, similar "massively laggy" 6p incident —
this is the leading suspect, unrelated to anything built this session, but
NOT independently confirmed as the cause of this particular report.
