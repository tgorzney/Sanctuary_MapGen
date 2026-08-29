# MAP_SCENARIO_SPEC — the SanGen Map Scenario system

**Status:** re-derived from the live, in-game-confirmed implementation on **2026-08-28**. Every
non-obvious claim below cites a source file and line. Anything not read in a file is marked ⚠️.

**Companion spec — read first, not duplicated here:** `MAP_UNIT_SPAWNING_SPEC.md` owns the
mechanics of spawning units from a per-map Lua script (the load/execution chain, the
double-execution hazard, `Import()` semantics, the one-`NewThread` rule, the `CreateUnit` call,
position validation, diagnostics, known-good `tpId`s). This spec cites it and does not restate it.

### Ground truth (external to this repo — the game install, not SanGen source)

| Alias | Path | Read |
|---|---|---|
| `SCEN` | `LJ/lua/maps/Pandemonium Isthmus/Pandemonium Isthmus_Scenarios_Script.lua` | 2026-08-28, 755 lines |
| `DATA` | `LJ/lua/maps/Pandemonium Isthmus/Pandemonium Isthmus_data.lua` | 2026-08-28, 516 lines |
| `SANMAP` | `Sanctuary_Data/Maps/Pandemonium Isthmus/Pandemonium Isthmus.sanmap` | 2026-08-28 |

Engine sources cited by path + line are all under `LJ/lua/`. A future reader without game-install
access must treat this spec as the frozen extraction of that set.

---

## 1. What the system is

Playable area, per-army spawn position, per-army alloy/mex marker visibility, and (optionally)
extra unit spawns are all functions of **lobby composition** — how many players, how many human
vs. AI, and separately *which* army slots are filled. The system resolves all of that **once,
deterministically, synchronously, at map load**, from an ordered rule table authored per map.

Two live files today, three under ratified ARCH law (§9). Runtime link:
`Import("maps/<MapName>/<MapName>_Scenarios_Script.lua").Scenario` (`DATA:91`).

## 2. ⚠️ AS-BUILT vs. RATIFIED-TARGET — read before trusting any other section

The live implementation and the ratified ARCH have diverged. **Where they differ, this spec
documents the live code as the description of what runs, and names the ARCH ruling as the target.**
Do not read the ARCH sections as descriptions of running code.

| Concern | Live today (`SCEN`/`DATA`) | Ratified target | Status |
|---|---|---|---|
| File shape | 2 files: `_data.lua` + `_Scenarios_Script.lua` | 3 files: `_data.lua` + `_Scenarios_Runtime.lua` + `_Scenarios_Data.lua` | `ARCH_15_04` — not migrated |
| `ResolveAndApply` 4th arg | `slotPattern` string (`SCEN:391`, `DATA:499`) | `playersInformation` array | `ARCH_15_10` §1 — not migrated |
| `BuildSlotPattern` owner | the orchestrator, `DATA:399-410` | the runtime file | `ARCH_15_10` §1 — not migrated |
| Slot-count bound | hardcoded `for i = 1, 16` (`DATA:400`) | rendered `MAX_ARMY_SLOT_COUNT` | `ARCH_15_10` §2 — not built |
| Occupancy loop bound | hardcoded `for armyId = 1, 4` (`SCEN:344`) | `for armyId, armyName in pairs(ARMY_ID_TO_NAME)` | `ARCH_15_10` §3 — not applied |
| Unit-spawn opt-in field | `spawnsUnits` (`SCEN:141`, read at `SCEN:393`) | `navy` + `ScenarioNavalFleet` | **ARCH is stale** — see §11.1 |
| Deferred spawn entry point | `Scenario.SpawnMatchedScenarioUnits(area)` (`SCEN:748`) | `Scenario.SpawnNavalFleets(area)` | **ARCH is stale** — see §11.1 |

## 3. Execution and timing law

The whole design exists to land in one window: **after** the `.sanmap` is parsed into
`GameInfo.MapData`, and **before** anything reads those marker tables.

| # | Where | What runs |
|---|---|---|
| 1 | `host/hostMain.lua:64-66` | tick 0 → `InitLobby(...)` |
| 2 | `script.lua:156` | `LoadMapData(mapPath)` — parses `.sanmap` into `GameInfo.MapData` (`common/mapUtils.lua:10-42`) |
| 3 | `common/mapUtils.lua:46-51` | builds `maps/<dataName>/<dataName>_data.lua` and `Import`s it — **`DATA`'s whole body runs synchronously here, including `Scenario.ResolveAndApply`** |
| 4 | `script.lua:160` | `CreateArmies()` (`common/gameUtils.lua:209`) — ends at `:334` with `SpawnInitialUnits()` |
| 5 | `common/gameUtils.lua:356-371` | `SpawnInitialUnits` reads each army's `Spawn` marker and creates its commander |
| 6 | `script.lua:186` → `host/hostMain.lua:67` | `RunMapSetup(true)` — creates a resource-spot entity per surviving `Alloys` transform |
| 7 | `host/hostMain.lua:105` | `ThreadsDispatcherUpdate()` — the `NewThread` callbacks finally run, same tick 0 |

**Hard consequences:**

- `ResolveAndApply` mutating `GameInfo.MapData` at step 3 is *guaranteed* to be what steps 5 and 6
  see. There is no later hook that can fix a wrong spawn position — the commander is already placed.
- `Armies` is empty during step 3. Anything touching `Armies`/`CreateUnit` must be deferred to
  step 7 (`MAP_UNIT_SPAWNING_SPEC` §1).
- **The orchestrator's entire body MUST be `pcall`-wrapped** (`DATA:89`, `DATA:508-510`). Nothing up
  the chain protects `LoadMapData`; `Import`'s own `xpcall` re-raises after logging. An uncaught
  throw aborts `LoadMapData`, so steps 4-6 never run at all — no armies, no units, no props, no
  resource spots. Confirmed by a live crash 2026-08-16.
- Errors inside the `NewThread` callback are **not** propagated — `common/systems/threads.lua:146-152`
  hands the message to `ErrorHandler` and drops the thread. Everything after the throw is silently
  cancelled, and `Log`/`Warn` do not work in this build. **Ordering inside the thread is therefore
  load-bearing** (`MAP_UNIT_SPAWNING_SPEC` §4; `DATA:446-459`).

### 3.1 Ordering inside the single `NewThread`

Only **one** `NewThread` per script is honored (`MAP_UNIT_SPAWNING_SPEC` §4, empirical). Live order
(`DATA:436-494`), each constraint paid for by a real regression on 2026-08-28:

1. `if not chosenArea then return end` (`DATA:444`) — `ResolveAndApply` threw; bail rather than
   raise a second, masking nil-dereference.
2. `if IsHost` → `SetPlayableArea` twice (`DATA:468-469`): a throwaway `{-1000,-1000,1,1}` nudge,
   then the real area. The resource-spot enable/disable pass only runs when the area actually
   *changes*, so the nudge guarantees it runs even when `chosenArea` equals the map's baked default.
3. `if IsHost and spawnsUnitsEnabled` → `pcall(Scenario.SpawnMatchedScenarioUnits, chosenArea)`
   (`DATA:478-483`).
4. Prefab/navmesh work **last**, `pcall`'d (`DATA:489-492`). It once sat ahead of the unit spawn and
   silently killed every unit when it threw.

⚠️ `common/systems/threads.lua:86-105` appends each `NewThread` into a per-tick **list**, so the
single-honored-thread rule is not visible in that source. Treat it as an empirical rule, not a
mechanism this spec can explain.

⚠️ `NewThread` is registered *before* `ResolveAndApply` is called (`DATA:436` vs `DATA:498`).
`chosenArea`/`spawnsUnitsEnabled` are declared at `DATA:420` and closed over as upvalues, assigned
afterwards. This works only because the thread body does not run until step 7.

### 3.2 Required orchestrator preamble — the load-scope gate

`DATA:82-86`, before the `pcall`. `<map>_data.lua` has two callers with different literal path
strings, so `Import`'s cache misses and the chunk executes **twice per host state**
(`MAP_UNIT_SPAWNING_SPEC` §2 — measured: 300 fighters against 150 configured). The gate scopes side
effects to the caller that owns them:

```lua
local sangenLoadPath = ImportedFileInfo and ImportedFileInfo.FileName or ""
if sangenLoadPath ~= string.format("maps/%s/%s_data.lua",
        GameInfo.MapInfo.dataName, GameInfo.MapInfo.dataName) then
    return {}
end
```

This is not a run-once counter. The debug loader still imports successfully and still reads data
globals; it simply does not trigger match setup, and the gate stays correct if the engine ever
normalizes its cache key.

## 4. The `slotPattern` string

| Property | Value | Source |
|---|---|---|
| Length | 16 chars (one per army slot) | `DATA:400` — `for i = 1, 16` |
| Index | = `player.armyID` = army slot number | `DATA:405-407` |
| `h` | human (anything whose `playerType ~= PlayerType.AI`) | `DATA:406` |
| `A` | AI (`playerType == PlayerType.AI`) | `DATA:406` |
| `-` | empty slot | `DATA:401` |

Built verbatim from `Engine.GetLobbyInformation().playersInformation` (`DATA:399-411`):

```lua
local function BuildSlotPattern(players)
    local chars = {}
    for i = 1, 16 do
        chars[i] = "-"
    end
    for _, player in ipairs(players) do
        if player.armyID and player.armyID >= 1 and player.armyID <= 16 then
            chars[player.armyID] = (player.playerType == PlayerType.AI) and "A" or "h"
        end
    end
    return table.concat(chars)
end
```

`armyID` and `playerType` are the same real fields `CreateArmies` itself reads
(`common/gameUtils.lua:246`, `:256`). `playersInformation` is kept as the **official array** — no
parallel transformed structure is ever stored; counts and the pattern are both derived from it on
demand (`DATA:358-363`, `DATA:364-380`).

⚠️ `PlayerType` is `{Player=0, AI=1, Empty=2, Observer=3}`
(`host/generated/lua/enums.lua:118-123`). The live `else` branches treat **`Observer` and `Empty` as
human** in both `humanCount` (`DATA:373-377`) and the pattern (`DATA:406`). Not observed to have
caused a live defect; flagged as a latent authoring trap.

⚠️ 16 is correct for this map — `SANMAP` declares exactly `ARMY_01`…`ARMY_16` (16 armies, 16 `Spawn`
transforms). `script.lua:163-165,178` demotes any player whose `armyID > mappedArmyCount` to
Observer, so the pattern length must not undershoot the map's real army roster. `ARCH_15_10` §2
makes this authored data (`MAX_ARMY_SLOT_COUNT`, default 16); the live file hardcodes it.

## 5. The three-tier matching system

`FindMatchingScenario(total, humanCount, aiCount, slotPattern)` — `SCEN:281-294`, verbatim:

```lua
local function FindMatchingScenario(total, humanCount, aiCount, slotPattern)
    for _, scenario in ipairs(PATTERN_SCENARIOS) do
        if scenario.pattern == slotPattern then
            return scenario
        end
    end
    for _, scenario in ipairs(COUNT_SCENARIOS) do
        local matchOk, matched = pcall(scenario.match, total, humanCount, aiCount, slotPattern)
        if matchOk and matched then
            return scenario
        end
    end
    return DEFAULT_SCENARIO
end
```

| Tier | Table | Test | Notes |
|---|---|---|---|
| 1 | `PATTERN_SCENARIOS` | `scenario.pattern == slotPattern` — exact string equality | Beats **every** TIER 2 rule. Empty in the live file (`SCEN:130-133`), path is real. Field is `pattern`, not `slotPattern`. |
| 2 | `COUNT_SCENARIOS` | `scenario.match(total, humanCount, aiCount, slotPattern)` in **array order, first match wins** | Each `match` is `pcall`'d — a throwing predicate falls through to the next candidate rather than aborting resolution. |
| 3 | `DEFAULT_SCENARIO` | none — always matches | `SCEN:279`. Single record, no `match`/`pattern` field. |

### 5.1 Why TIER 2 order is load-bearing — the real `slots5to8AnyFilled` case

Array order **is** the entire authoring mechanism for match priority (`ARCH_15_06`). A broader
predicate placed above a narrower one silently shadows it, with no diagnostic.

The live table's first entry (`SCEN:136-146`) exists purely to win on slot **identity** before any
aggregate-count rule can win on **counts**:

```lua
{
    name = "slots5to8AnyFilled",
    match = function(t, h, a, pattern) return pattern:sub(5, 8):find("[^-]") ~= nil end,
    area = AREA_FULL,
    spawnsUnits = true,
    alloyMode = "occupancy",
},
```

Two players in slots 5 and 7 give `total == 2`. If `1v1` (`match = t == 2`, `SCEN:149`) were checked
first it would win, and the lobby would get `AREA_356` — a 356×356 box around map centre — while
both players' actual spawn markers sit far outside it (`SANMAP`: `ARMY_05` at x=675 z=525,
`ARMY_06` at x=1373 z=1523, vs. the box spanning 846..1202 on both axes). Everything outside the
active playable area is culled (`MAP_UNIT_SPAWNING_SPEC` §5). Placing the identity rule first is what
makes `AREA_FULL` win instead.

**The general rule this encodes:** any predicate that tests `pattern` (identity) must be authored
**above** every predicate that tests only `t`/`h`/`a` (aggregates) it could overlap with. The live
file follows the same discipline at the other end: its broad fallbacks `2hRestAI` (`SCEN:262`) and
`floor169` (`match = t > 2`, `SCEN:271`) sit after every more specific rule.

⚠️ A TIER 1 pattern entry outranks `slots5to8AnyFilled` unconditionally, because TIER 1 is checked
before the whole of TIER 2. An exact pattern covering slots 5-8 therefore silently disables the
identity rule for that one composition — intended, but easy to do by accident.

### 5.2 The live `COUNT_SCENARIOS` table, in authored order

| # | `name` | `match` | `area` | `alloyMode` | `spawns`? | Unit spawns? |
|---|---|---|---|---|---|---|
| 1 | `slots5to8AnyFilled` | `pattern:sub(5,8):find("[^-]")` | `AREA_FULL` | `occupancy` | no | **yes** (`spawnsUnits = true`) |
| 2 | `1v1` | `t == 2` | `AREA_356` | `explicit` | ARMY_01/02 | no |
| 3 | `2h1ai` | `h==2 and a==1 and t==3` | `AREA_169` | `keepAll` | no | no |
| 4 | `4human` | `t==4 and h==4` | `AREA_169` | `explicit` | ARMY_01-04 | no — ⚠️ `navy = true` is dead, §11.1 |
| 5 | `1h3ai` | `t==4 and h==1 and a==3` | `AREA_169` | `explicit` | ARMY_01-04 | no |
| 6 | `6total` | `t == 6` | `AREA_1024` | `occupancy` | no (in progress) | no |
| 7 | `2hRestAI` | `h==2 and a>=2 and t>=4` | `AREA_FULL` | `occupancy` | no | no |
| 8 | `floor169` | `t > 2` | `AREA_169` | `occupancy` | no | no |
| — | `DEFAULT_SCENARIO` | always | `AREA_356` | `occupancy` | no | no |

Areas (`SCEN:61-64`), world x/z rects matching the `.sanmap` `areas` format:

| Const | Rect | Note |
|---|---|---|
| `AREA_356` | `{846, 846, 356, 356}` | the map's own baked default — `SANMAP.areas.PlayableArea` is exactly this |
| `AREA_169` | `{668.444…, 824, 711.111…, 400}` | 16:9, 400 tall |
| `AREA_1024` | `{537, 472, 974, 1104}` | 6-player, centred |
| `AREA_FULL` | `{0, 0, 2048, 2048}` | whole map (`SANMAP.width/length` = 2048) |

## 6. Scenario record shape — every field

| Field | Type | Tier | Read at | Meaning |
|---|---|---|---|---|
| `name` | string | all | `SCEN:378`, `:399`, `:404` | Log identifier **and** the dispatch key for unit spawning (§11) — not cosmetic. |
| `pattern` | 16-char string | 1 only | `SCEN:283` | Exact `slotPattern` equality. |
| `match` | `function(total, humanCount, aiCount, slotPattern) -> boolean` | 2 only | `SCEN:288` | `pcall`'d. Returning nil/false falls through. |
| `area` | `{x, y, width, height}` | all | `SCEN:393` | World-space rect; `y` is world **z**. Returned to the orchestrator, applied via `Area.FromMapArea` (`DATA:468-469`). |
| `spawnsUnits` | bool | all | `SCEN:393` | The unit-spawn opt-in. Returned as the orchestrator's second value; gates `DATA:478`. Absent ⇒ nil ⇒ falsy. |
| `alloyMode` | `"explicit"`\|`"occupancy"`\|`"keepAll"`\|`"delta"` | all | `SCEN:323,343,353` | §7. An unrecognized value falls through every branch and behaves as `keepAll`. |
| `spawns` | `{ ARMY_XX = {x=, y=, z=} }` | all | `SCEN:306-318` | Per-army spawn override. §8 — **mandatory** for deterministic compositions. |
| `alloys` | `explicit`/`delta`-shaped, below | all | `SCEN:325`, `:359`, `:369` | Per-army alloy-marker overrides. |
| `navy` | bool | — | **nothing** | ⚠️ **DEAD FIELD.** Still present on 5 live entries; no reader exists anywhere in `SCEN`. §11.1. |

`alloys` shape for `explicit` — per-army list of named markers (`SCEN:158-169`):

```lua
alloys = {
    ARMY_01 = {
        { name = "AlloyMarker_219", x = 857, y = 78.72360229492188, z = 911 },
        { name = "AlloyMarker_237", x = 869, y = 78.72360229492188, z = 919 },
        { name = "AlloyMarker_97",  x = 873, y = 78.72360229492188, z = 923 },
    },
}
```

`alloys` shape for `delta` — two sub-tables, never a flat per-army list (`SCEN:359-373`):

```lua
alloys = {
    add    = { ARMY_XX = { { name = "AlloyMarker_1", x = 0, y = 0, z = 0 }, } },
    remove = { ARMY_XX = { "AlloyMarker_2", } },
}
```

⚠️ In the `delta` branch both loops are `for _, ... in pairs(...)` — the `ARMY_XX` key is **ignored**
by the code; markers are addressed purely by `name`. The keying is organizational only.

### 6.1 Per-map roster tables (not per-scenario)

| Table | Source | Role |
|---|---|---|
| `KNOWN_ALLOY_MARKERS` | `SCEN:78-83` | `{ ARMY_01..ARMY_04 = {3 marker names each} }` — the **deletion roster**. Only these 12 names are ever deleted by any mode. |
| `ARMY_ID_TO_NAME` | `SCEN:84` | `{[1]="ARMY_01" … [4]="ARMY_04"}` — maps a `slotPattern` index to an army name for `occupancy`. |
| `IDENTITY_ROTATION` / `IDENTITY_SCALE` | `SCEN:68-69` | Reused when a marker is created fresh; matches every marker already in the `.sanmap`. |

⚠️ `SANMAP` holds **288** `Alloys` transforms; the roster names **12** of them. The remaining 276 are
untouched by every `alloyMode`. Likewise, armies 5-16 have `Spawn` markers but **no** roster entry —
their alloys are unmanaged and `occupancy` can never delete them (`SCEN:344`'s `for armyId = 1, 4`).

### 6.2 SanGen-only additive field, no Lua counterpart — `areaName` / `AreaName` (2026-08-28, corrected same day)

Everything in §6 above documents the **live Lua ground truth** (`SCEN`'s own scenario-record shape,
as read by `FindMatchingScenario`/`ApplyScenario`/`ResolveAndApply`). SanGen's own `.sanmap` JSON
persistence of this same record carries one field with **no Lua-side counterpart at all**:
`Params::ScenarioBody::areaName`, wire key `AreaName`, a sibling of `Area`. It exists purely so a
SanGen author can **reference** a named `Params::MapArea` from `recipe.areas` instead of holding a
disconnected private rectangle.

**The STRING never reaches Lua — confirmed correct, unchanged.** `areaName`/`AreaName` itself has no
Lua-side counterpart anywhere: `SanGenScenarioRuntime.lua`'s `ResolveAndApply` returns the matched
scenario's own `area` field verbatim, with no per-scenario name-lookup capability of its own — there
is nothing for a Lua-side `areaName` string to do even if it were rendered.

**The NUMBERS, however, are resolved at TWO independent SanGen-owned export legs, not one — a
correction to this section's own original text (2026-08-28, same day, caught by the Format Expert
drafting `work_orders/STEP209_ScenarioAreaNameReference_PARAMS_IO_UI.md`).** SanGen exports two
separate, never-merged artifacts from the same `Params::Scenarios` data
(`ScenarioScript_Export_IO.h`: "Two independent calls, two independent result types, never
merged"): the `.sanmap` JSON leg (`MapExporter_Scenarios_IO.cpp`'s `BuildScenarioRecordJson`) and
the `<MapName>_Scenarios_Data.lua` leg (`ScenarioScript_DataLua_IO.cpp`'s
`AppendScenarioBodyFields`, invoked by `ScenarioScript_Export_IO.cpp`'s `ExportMapScenario`) — **the
file the game actually loads** (§14). Both builders read `body.area.originX/originZ/width/length`
directly and independently; both must resolve `areaName` against `recipe.areas` before doing so, or
the Lua leg silently ships stale numbers whenever a referenced Area is resized without the scenario
being reselected. Full corrected ruling — the resolution algorithm applied identically at both call
sites, the shared stale-reference validator, and the paired UI ruling (read-only sliders while
referenced, one new Combo sentinel entry): `ARCH_15_05_ParamsScenariosType.md` §15.5's "AMENDED
2026-08-28" note, including its own "CORRECTED 2026-08-28 (same day, second pass)" paragraph, which
records this exact discrepancy as a formal correction rather than a silent patch.

**Note:** the `.sanmap` JSON wire shape's own authoritative home is `SANMAP_FORMAT_SPEC.md`'s
"Correction 17," which — per `sangen_arch_pack/INDEX.md`'s own already-recorded flag — does not
currently exist as written text inside that file (a pre-existing documentation gap, not created or
closed by this note). Whoever eventually writes that Correction must include `AreaName` alongside
`Name`/`Area`/`SpawnsUnits`/etc.

## 7. `alloyMode` — the four values

The consumer is `RunMapSetup` (`common/mapUtils.lua:113-145`): it takes
`GameInfo.MapData.markers["Alloys"]`, and if `.resource` is truthy (`SANMAP`: `true`) it creates one
real resource-spot entity **per surviving `transforms` entry**. Deleting a transform entry before
step 6 is therefore how a mex is removed from the match — there is no later API for it.

| Mode | Exact behaviour (`SCEN`) | Choose when |
|---|---|---|
| `explicit` | `:323-342`. For every army in `scenario.alloys`: create the marker if missing (identity rot/scale), then write x/y/z. Then, for every army in `KNOWN_ALLOY_MARKERS` **not** mentioned, delete all its markers. | The scenario fully owns the layout. **Default choice for any confirmed composition.** Self-contained and immune to other scenarios' data. |
| `occupancy` | `:343-352`. Ignores `scenario.alloys` entirely. For `armyId = 1..4`, if `slotPattern:sub(armyId,armyId) == "-"`, delete that army's roster markers. Trusts the `.sanmap`'s baked positions for the rest. | The `.sanmap`'s baked alloy positions are already right and you only need empty slots cleaned up. The pragmatic default while a composition is still being tuned. |
| `keepAll` | No branch at all — falls out of the if/elseif (`:375`). Nothing is created, moved, or deleted. | You deliberately want an empty slot's mex left on the map (live use: `2h1ai`, so the lone AI gets extra resources — `SCEN:176`). |
| `delta` | `:353-373`. Applies only `alloys.add` (create-if-missing + position) and `alloys.remove` (delete by name). **Silence is NOT a delete instruction** — unlike `explicit`. | Once a real baseline exists to diff against. ⚠️ **Wired but used by zero live scenarios.** Untested in-game. |

**`explicit` vs `delta` is the load-bearing distinction:** under `explicit`, an army you forget to
list gets its mex **deleted**; under `delta`, an army you forget to list is **left alone**.

⚠️ An `alloyMode` typo (or omission) silently behaves as `keepAll` — no branch matches, no error.

## 8. ⚠️ HARD REQUIREMENT — explicit `spawns` for deterministic compositions

**Every scenario that needs deterministic spawn positions MUST declare its own `spawns` table.**

The `.sanmap` stores exactly **one** spawn transform per army name
(`SANMAP.markers.Spawn.transforms`, 16 entries). `SpawnInitialUnits` reads it unconditionally, with
no per-composition branching of its own — `GetMarker(army.name, "Spawn", true)` →
`GameInfo.MapData.markers.Spawn.transforms[army.name]` (`common/gameUtils.lua:50-64`), then
`MarkerToPosition` (`:88-92`), then `CreateUnit` (`:356-371`). That single value is **shared mutable
state across every scenario that does not override it.** Editing it to tune one composition silently
changes spawn behaviour for every other composition lacking its own `spawns`.

Concretely, from the files: `SANMAP`'s baked `ARMY_01` spawn is `{745, 82.259766, 669}`, while the
`1v1` scenario overrides it to `{855, 79.129…, 920}` (`SCEN:155`). Any scenario without `spawns`
gets the 745/669 baseline, whatever the author assumed.

**Live proof (2026-08-20):** a 3-player lobby (armies 1 and 3 human, 2 AI) correctly matched `2h1ai`
and correctly applied `AREA_169` — but spawned players at the 6-player positions, because `2h1ai`
declares `area`/`alloyMode` and no `spawns`. Working exactly as defined; the failure was incomplete
scenario data. The same class of regression had previously broken 1v1, 4-human and 1-human-3-AI
(`SCEN:106-113`).

**How `spawns` is applied** (`SCEN:306-318`) — note the guard:

```lua
for armyName, pos in pairs(scenario.spawns) do
    if spawnTransforms[armyName] then          -- ⚠️ never creates a missing transform
        spawnTransforms[armyName].position.x = pos.x
        spawnTransforms[armyName].position.y = pos.y
        spawnTransforms[armyName].position.z = pos.z
    end
end
```

⚠️ A `spawns` entry for an army with no `Spawn` marker in the `.sanmap` is **silently ignored**.
Position is written in place; rotation/scale are untouched.

**Rule:** omitting `spawns` is acceptable only when the composition is *intentionally* meant to
inherit the baked default (say so in the entry's own comment / `authoringNote`), or is explicitly
`occupancy`/`keepAll` pending a future `spawns` table — as the live `6total` entry does, flagged in
its own comment as in progress (`SCEN:260`). Silent omission is the exact failure mode above.

## 9. `ARMY_XX` zero-padded naming is load-bearing

`CreateArmies` builds the army list from `pairs(GameInfo.MapData.armies)` — an **unordered** Lua
table — then sorts it as **strings**:

```lua
for mapStartSlotName,_ in pairs(mapArmies) do table.insert(armySetup, mapStartSlotName) end
table.sort(armySetup)                                          -- common/gameUtils.lua:219
...
for mapStartSlotIndex, mapStartSlotName in ipairs(armySetup) do -- :240
    ... if playerInfo[index].armyID == mapStartSlotIndex then   -- :246
```

So **lobby slot N is whichever army name sorts Nth alphabetically.** With zero padding,
`ARMY_01 < ARMY_02 < … < ARMY_10 < … < ARMY_16` — index equals slot number. Without it,
`"ARMY_10" < "ARMY_2"` lexicographically, and slot 2 would resolve to army 10: every `spawns` key,
every `KNOWN_ALLOY_MARKERS` key, every `slotPattern` index and every `ARMY_ID_TO_NAME` entry would
point at the wrong army, with no error. `SANMAP` uses `ARMY_01`…`ARMY_16` correctly.

This is the same fact `MAP_UNIT_SPAWNING_SPEC` §5 states for `CreateUnit` army indices.

## 10. Module API contract

`Scenario` **MUST be a global table** — `Scenario = {}` at `SCEN:30`, never `local`. `Import` runs
the file in a fresh environment table and returns that table; a file's `return` is ignored
(`MAP_UNIT_SPAWNING_SPEC` §3). A `local` module silently yields no fields, with no error. `SCEN:754`
does `return Scenario` — harmless, and not what makes the link work.

| Function | Signature | Called from | When |
|---|---|---|---|
| `Scenario.ResolveAndApply` | `(total, humanCount, aiCount, slotPattern) -> chosenArea, spawnsUnitsEnabled` (`SCEN:391`) | `DATA:498-499`, `pcall`'d separately from the outer `pcall` | **Synchronously**, step 3 |
| `Scenario.SpawnMatchedScenarioUnits` | `(area)` (`SCEN:748`) | `DATA:479`, `pcall`'d, gated `IsHost and spawnsUnitsEnabled` | Deferred, step 7 |
| `Scenario.SpawnUnits` | `(instructions)` (`SCEN:442`) | `SCEN:750` | Deferred; the only `CreateUnit` caller |

`ResolveAndApply` body (`SCEN:391-408`): find the match → cache `matchedScenario.name` into the
module-local `currentMatchedScenarioName` (`SCEN:383`, `:399`) → `ApplyScenario(matched, total,
slotPattern)` → log → return `area, spawnsUnits`.

`ApplyScenario(scenario, total, slotPattern)` is **file-local** (`SCEN:305`) — not part of the API.

The orchestrator reads the lobby itself via `Engine.GetLobbyInformation()` (`DATA:365`), not
`GameInfo.MapData.armies` (static per-map data, identical regardless of lobby size), and derives
`total`/`humanCount`/`aiCount` from `playersInformation` on demand (`DATA:370-379`).

## 11. Opting a scenario into unit spawning

**Two independent edits are required. Setting `spawnsUnits = true` alone spawns nothing.**

| Step | Edit | Where |
|---|---|---|
| 1 | `spawnsUnits = true` on the scenario record | `SCEN` `COUNT_SCENARIOS`/`PATTERN_SCENARIOS` |
| 2 | A branch on the scenario's `name` in `Scenario.SpawnMatchedScenarioUnits` | `SCEN:748-752` |

```lua
function Scenario.SpawnMatchedScenarioUnits(area)
    if currentMatchedScenarioName == "slots5to8AnyFilled" then
        Scenario.SpawnUnits(BuildSlots5to8Instructions(area))
    end
end
```

The `area` argument is all the orchestrator hands back, so `currentMatchedScenarioName`
(`SCEN:383`, set at `:399`) is how the deferred path learns *which* scenario matched. The dispatch
stays an `if/elseif` until there are more than ~2-3 cases (`SCEN:744-746`).

The design is a deliberate **executor / generator** split (`SCEN:412-422`):

- **Executor** — `Scenario.SpawnUnits(instructions)` (`SCEN:442-458`). Input: a flat array of
  `{armyIndex, templateIdentifier, x, y, z}`. Knows nothing about water, terrain, or unit type.
  Checks **both** `ok` and `unit`, and `WaitTicks(1)` every `UNIT_SPAWN_BATCH_SIZE = 100`
  (`SCEN:425`). **Never call `CreateUnit` outside this function.**
- **Generator** — e.g. `BuildSlots5to8Instructions(area)` (`SCEN:670-741`). Scenario-specific,
  hand-authored placement; iterates `pairs(Armies)` for real army indices, guards
  `army.lobbyOptions and army.lobbyOptions.isEmptySlot` (`SCEN:691`), and derives every position
  live from that army's own `Spawn` marker.

All spawning mechanics — the `CreateUnit` contract, why army indices are never hardcoded, the
`lobbyOptions` nil guard, position validation returning `nil` rather than a known-bad point, culling
outside the playable area, and known-good `tpId`s — are owned by `MAP_UNIT_SPAWNING_SPEC` §§5-7,9
and are not restated here.

### 11.1 ⚠️ The naval-fleet machinery is GONE — correction to earlier revisions

`SCEN` as read 2026-08-28 contains **no** `Scenario.SpawnNavalFleets`, `NAVAL_FLEET`,
`NAVAL_POND_SIDE_BY_ARMY`, `NAVAL_SIDE_BIAS_DISTANCE`, `NavalFindSpot`, or any `NAVAL_*` tuning
constant. The 2026-08-27 rewrite replaced all of it with the generic `SpawnUnits` /
`SpawnMatchedScenarioUnits` pair above (`SCEN:410-422`).

Consequently:
- `navy` is a **dead field** (§6). `4human` still carries `navy = true` and spawns nothing, because
  it has no `spawnsUnits` and no dispatch branch — stated in its own comment (`SCEN:182-185`).
- **`ARCH_15_05` §15.5's `ScenarioNavalFleet`/`ScenarioNavalPondSide`/`ScenarioNavalPondAssignment`
  types and `ScenarioBody::navy` were shaped on 2026-08-21 from a live read of a `SpawnNavalFleets`
  body that no longer exists.** This spec does not and cannot amend the ARCH. **Action required:
  the ARCH Expert should review §15.5 against the current `spawnsUnits` model.** The parallel
  `Params::Scenarios` field is `navy` + `navalFleet`; the live field is `spawnsUnits` + a
  name-keyed dispatch.

## 12. Complete worked example — a new scenario, end to end

Adds a TIER 1 pattern scenario for a 1v1 played in **slots 3 and 4**, with its own area, spawns and
alloys, plus an opt-in unit spawn. Every coordinate and marker name below is real: taken from
`SCEN`'s `4human` entry (`SCEN:190-217`) and cross-checked against `SANMAP`.

**Step 1 — the record.** Into `PATTERN_SCENARIOS` (`SCEN:130`). Pattern is 16 chars; `h` at indices
3 and 4:

```lua
local PATTERN_SCENARIOS = {
    {
        name = "1v1slots3and4",
        pattern = "--hh------------",
        area = AREA_356,
        spawnsUnits = true,
        alloyMode = "explicit",
        spawns = {
            ARMY_03 = { x = 833,  y = 80.794922, z = 857 },
            ARMY_04 = { x = 1215, y = 80.759766, z = 1191 },
        },
        alloys = {
            ARMY_03 = {
                { name = "AlloyMarker_282", x = 823, y = 80.744141, z = 848 },
                { name = "AlloyMarker_283", x = 833, y = 80.759766, z = 856 },
                { name = "AlloyMarker_284", x = 835, y = 80.681641, z = 860 },
            },
            ARMY_04 = {
                { name = "AlloyMarker_285", x = 1225, y = 80.712891, z = 1200 },
                { name = "AlloyMarker_286", x = 1215, y = 80.771484, z = 1192 },
                { name = "AlloyMarker_287", x = 1213, y = 80.712891, z = 1188 },
            },
        },
    },
}
```

What each choice does:
- `pattern` — matched at `SCEN:283`. TIER 1, so it beats **every** `COUNT_SCENARIOS` entry including
  `slots5to8AnyFilled` and `1v1`. Length must be exactly 16 or it can never equal a built pattern.
- `alloyMode = "explicit"` — `SCEN:323-342`. ARMY_03/04 markers are repositioned; ARMY_01 and
  ARMY_02 are in `KNOWN_ALLOY_MARKERS` (`SCEN:79-80`) and are **not** mentioned, so their 6 markers
  are deleted before `RunMapSetup` creates resource spots.
- `spawns` — mandatory (§8). Without it, both players would spawn at `SANMAP`'s baked ARMY_03/04
  positions (595/595 and 1453/1453), far outside `AREA_356`.
- `spawnsUnits = true` — returned at `SCEN:393`, gates `DATA:478`. Inert without Step 3.

**Step 2 — the generator.** Adapted from `BuildSlots5to8Instructions` (`SCEN:670-741`), reduced to
the parts this scenario needs. Place near the other generators, above the dispatch:

```lua
local ESCORT_TPID  = "ucl4004"   -- Chosen T4 BigBot, spawn confirmed live
local ESCORT_COUNT = 8

local function Build1v1Slots3and4Instructions(area)
    local instructions = {}
    for armyIndex, army in pairs(Armies) do
        -- nil lobbyOptions => treat as OCCUPIED (an AI slot is filled).
        local bIsEmptySlot = army.lobbyOptions and army.lobbyOptions.isEmptySlot
        if not bIsEmptySlot then
            local slotNumber = tonumber(army.name:match("^ARMY_0?(%d+)$"))
            if slotNumber == 3 or slotNumber == 4 then
                local marker = GameInfo.MapData.markers
                    and GameInfo.MapData.markers.Spawn
                    and GameInfo.MapData.markers.Spawn.transforms
                    and GameInfo.MapData.markers.Spawn.transforms[army.name]
                if marker then
                    -- Derived from the army's own Spawn marker -- never hardcoded coordinates.
                    AppendUnitGrid(instructions, armyIndex, ESCORT_TPID,
                        marker.position.x + 30, marker.position.y, marker.position.z,
                        ESCORT_COUNT, 6)
                end
            end
        end
    end
    return instructions
end
```

Notes: `pairs(Armies)` keys are the real army indices (`MAP_UNIT_SPAWNING_SPEC` §5 — never hardcode
one); `army.lobbyOptions` is guarded because it is nil on some entries and the unguarded form threw
and silently killed every spawn (`SCEN:672-691`); `AppendUnitGrid` is the existing helper at
`SCEN:632`, called here with `requiresWater` omitted (land units).

**Step 3 — the dispatch.** `SCEN:748-752`:

```lua
function Scenario.SpawnMatchedScenarioUnits(area)
    if currentMatchedScenarioName == "slots5to8AnyFilled" then
        Scenario.SpawnUnits(BuildSlots5to8Instructions(area))
    elseif currentMatchedScenarioName == "1v1slots3and4" then
        Scenario.SpawnUnits(Build1v1Slots3and4Instructions(area))
    end
end
```

**Step 4 — orchestrator changes: none.** `DATA` is composition-agnostic. It builds the pattern
(`DATA:411`), calls `ResolveAndApply` (`DATA:499`), applies the area (`DATA:468-469`) and gates the
spawn on the returned flag (`DATA:478`). Adding a scenario never requires touching `_data.lua` — and
under `ARCH_15_04` SanGen may never write that file at all.

**Step 5 — what happens at runtime.** Lobby = humans in slots 3 and 4 →
`slotPattern == "--hh------------"` → TIER 1 hit → `ApplyScenario` moves ARMY_03/04 spawn and alloy
markers and deletes ARMY_01/02's 6 alloy markers (all before step 4/6 of §3) → `CreateArmies` places
both commanders at the new positions → `RunMapSetup` creates resource spots for the surviving
markers → tick 0 threads run: playable area set to `AREA_356`, then 8 BigBots per army.

**Verification (per `MAP_UNIT_SPAWNING_SPEC` §7):** `Log`/`Warn` do not work in this build. The only
reliable signal is a unit appearing. Confirm by counting units, not by reading logs — and note that
`AREA_356` spans 846..1202, so the escort grid at ARMY_03's marker (x≈863, z=857) is inside the
playable area and will not be culled.

## 13. Corrections to the previous revision of this spec

| Previous claim | Live code | Correction |
|---|---|---|
| API includes `Scenario.SpawnNavalFleets(area)` | absent from `SCEN` | Replaced 2026-08-27 by `SpawnMatchedScenarioUnits` + `SpawnUnits`. §11.1 |
| §5.1 documented `NAVAL_FLEET` / `NAVAL_POND_SIDE_BY_ARMY` / `NAVAL_SIDE_BIAS_DISTANCE` as live per-map values | all removed | Section deleted. The `ARCH_15_05` PARAMS types shaped from them need ARCH-Expert review. §11.1 |
| Scenario field is `navy` (bool) | `spawnsUnits` is read; `navy` has no reader | `navy` is a dead field left on 5 entries. §6 |
| `ResolveAndApply(total, humanCount, aiCount, playersInformation)`, runtime builds `slotPattern` | `(…, slotPattern)`; orchestrator builds it at `DATA:399-411` | `ARCH_15_10` §1 is ratified but **not migrated**. §2 |
| Three-file split described as the shape | two files; `_Scenarios_Script.lua` still live | `ARCH_15_04` ratified, not migrated. §2, §14 |
| `occupancy` "deletes markers for armies with no player" | loop is `for armyId = 1, 4` | Only roster armies 1-4 are ever considered; armies 5-16 are unmanaged. §6.1, §7 |
| `explicit` deletes markers of unmentioned armies | true — but only the 12 names in `KNOWN_ALLOY_MARKERS` | 276 of the map's 288 alloy markers are untouched by every mode. §6.1 |
| Not previously stated | `spawns` writes only if the transform already exists (`SCEN:311`) | An entry for an army with no `Spawn` marker is silently ignored. §8 |
| Not previously stated | `spawnsUnits = true` alone does nothing | The `name`-keyed dispatch branch is a second required edit. §11 |
| Not previously stated | the load-scope gate (`DATA:82-86`) | Mandatory orchestrator preamble; without it the chunk runs twice. §3.2 |
| §1 framed the optional extra as "a per-army naval fleet" | generic per-scenario unit spawning | §11 |

## 14. SanGen ownership and on-disk shape (ratified law — not yet built)

Per `ARCH_15_03`/`ARCH_15_04`/`ARCH_15_10`. SanGen owns scenario **data**, rendered to Lua on
export; it never parses Lua back (option (c) — no Lua parser in the import direction).

| File | Role | Written by SanGen? |
|---|---|---|
| `<MapName>_data.lua` | Orchestrator — load-scope gate, lobby read, counts, `NewThread`, wiring | **Never, under any code path.** |
| `<MapName>_Scenarios_Runtime.lua` | Generic algorithm — `BuildSlotPattern`, `FindMatchingScenario`, `ApplyScenario`, `ResolveAndApply`, the spawn executor, all tuning constants. Identical across every map. | Yes — bundled resource, copied per export. |
| `<MapName>_Scenarios_Data.lua` | Per-map tables — `PATTERN_SCENARIOS`/`COUNT_SCENARIOS`/`DEFAULT_SCENARIO`/`MAX_ARMY_SLOT_COUNT`, rendered from `Params::Scenarios` (`ARCH_15_05`, `ARCH_15_10` §2). | Yes — fully regenerated per export, never hand-edited, never read back. |

- **All three colocated in `LJ/lua/maps/<MapName>/`.** Cross-tree `Import` is impossible — the map's
  asset folder (`Sanctuary_Data/Maps/<MapName>/`) is unreachable (`MAP_UNIT_SPAWNING_SPEC` §3,
  disproved via `Engine.FileExists`). This is not a preference; the pair cannot be separated.
- The generated data file declares its tables as **globals**, for the same `Import()` global-capture
  reason as `Scenario` (§10).
- **Overwrite safety** (`ARCH_15_04`): (1) filename disjointness — SanGen writes only the two
  `_Scenarios_Runtime`/`_Scenarios_Data` paths, never `_data.lua` nor the legacy
  `_Scenarios_Script.lua`; (2) a machine-checkable generated-file banner token in both; (3) on an
  unrecognized occupant, refuse to write **that file only**, log loudly by path, and continue the
  rest of the export. A write-target safety refusal — distinct from Constitution §6's import-time
  "a version marker is never grounds to refuse the file."
- **`COUNT_SCENARIOS` array order is the authoring action for match priority** (`ARCH_15_06`), so the
  UI surface must be a reorderable list — never a set or an unordered table. §5.1 is why.
- ⚠️ **`<map>_data.lua` is engine-writable.** `host/testUtils.lua` can `table.save` over it as
  machine-generated `MapData = { … }`, destroying every hand-written line
  (`MAP_UNIT_SPAWNING_SPEC` §2a). Keep backups.
- **Migration of the live map is a one-time human action** (`ARCH_15_04`, `ARCH_15_10` §1): author the
  data in SanGen preserving `COUNT_SCENARIOS` order exactly and set `maxArmySlotCount = 16`; export
  once; then hand-edit `_data.lua` to retarget its `Import()`, change the `ResolveAndApply` call to
  pass `playersInformation`, and delete its own `BuildSlotPattern`. SanGen never deletes the orphaned
  legacy file — as forbidden as overwriting one.

## 15. Cross-references

- `MAP_UNIT_SPAWNING_SPEC.md` — **authoritative for all spawning mechanics.** §1 load chain, §2
  double execution, §2a why logic lives in `_data.lua` + the engine-overwrite hazard, §3 `Import`
  semantics, §4 one-`NewThread`/ordering, §5 `CreateUnit`, §6 position validation, §7 diagnostics,
  §9 known-good `tpId`s.
- `MODDING_SCRIPTING_SPEC.md` — the historical investigation trail (the disproven cross-tree
  `Import()` hypothesis) and the F1-console reliability caveat.
- `ARCH_15_MapScenarioSystem.md` §15 and subsections §15.1-§15.10 — the binding law. §15.5 needs
  review against §11.1 of this spec.
- `IO_MIGRATION_SPEC.md` §1 — the per-domain `.sanmap` JSON IO convention the new `Scenarios`
  section may extend; the companion-`.lua` surface explicitly does **not** reuse it.
- `AI_HOSTCLIENT_SPEC.md` — host/client split; background for the `IsHost` gating in §3.1.
