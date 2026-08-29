# Map Scenarios: one map that reshapes itself for the lobby

**What this is:** a per-map Lua system that reads the lobby the moment the map loads and, before the engine has built anything, rewrites the map to suit that lobby — playable area, per-army spawn positions, which resource (alloy) spots exist at all, and any bonus units you want handed out.

One map file. A 1v1 gets a tight 356x356 arena with two spawns and six mex. The same map with eight players gets the full 2048x2048 and every spot on it. Nothing is duplicated, nothing is a separate map upload.

Everything below is taken from a live, working implementation — `Pandemonium Isthmus`, confirmed in-game 2026-08-28 — and from engine source in `engine/LJ/lua/`. Where something is unverified it is marked ⚠️.

---

## Contents

1. [Minimal working example](#1-minimal-working-example)
2. [Why two files](#2-why-two-files)
3. [The three-tier match](#3-the-three-tier-match)
4. [The slotPattern string](#4-the-slotpattern-string)
5. [Every scenario field](#5-every-scenario-field)
6. [alloyMode in full](#6-alloymode-in-full)
7. [ARMY_XX and why the zero padding matters](#7-army_xx-and-why-the-zero-padding-matters)
8. [Timing: the load order you must not break](#8-timing-the-load-order-you-must-not-break)
9. [Bonus units](#9-bonus-units)
10. [Worked example: adding a scenario end to end](#10-worked-example-adding-a-scenario-end-to-end)
11. [Troubleshooting by symptom](#11-troubleshooting-by-symptom)
12. [Quick reference](#12-quick-reference)

---

## 1. Minimal working example

Two files, both in the **script tree**, side by side:

```
engine/LJ/lua/maps/<MapName>/
    <MapName>_data.lua                 <- orchestrator (hand-authored)
    <MapName>_Scenarios_Script.lua     <- scenario definitions
```

Not the asset folder. `Sanctuary_Data/Maps/<MapName>/` is where your `.sanmap`, `Props/` and `Textures/` live — Lua cannot reach it (see §2).

### `<MapName>_Scenarios_Script.lua`

[CODE]
```lua
-- NOT local. Import() ignores a file's `return` statement entirely; it only
-- captures GLOBAL variables the file writes into its own environment table.
-- `Scenario` must be global for `Import(path).Scenario` to work from _data.lua.
Scenario = {}

local AREA_SMALL = { x = 846, y = 846, width = 356, height = 356 }
local AREA_FULL  = { x = 0,   y = 0,   width = 2048, height = 2048 }

local KNOWN_ALLOY_MARKERS = {
    ARMY_01 = { "AlloyMarker_219", "AlloyMarker_237", "AlloyMarker_97" },
    ARMY_02 = { "AlloyMarker_220", "AlloyMarker_240", "AlloyMarker_99" },
}
local ARMY_ID_TO_NAME = { [1] = "ARMY_01", [2] = "ARMY_02" }

local PATTERN_SCENARIOS = {}

local COUNT_SCENARIOS = {
    {
        name = "1v1",
        match = function(t, h, a, pattern) return t == 2 end,
        area = AREA_SMALL,
        spawnsUnits = false,
        alloyMode = "occupancy",
    },
}

local DEFAULT_SCENARIO = { name = "default", area = AREA_FULL, alloyMode = "occupancy" }

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

local function ApplyScenario(scenario, total, slotPattern)
    local alloyTransforms = GameInfo.MapData.markers
        and GameInfo.MapData.markers.Alloys
        and GameInfo.MapData.markers.Alloys.transforms
    if alloyTransforms and scenario.alloyMode == "occupancy" then
        for armyId = 1, 2 do
            if slotPattern:sub(armyId, armyId) == "-" then
                for _, markerName in ipairs(KNOWN_ALLOY_MARKERS[ARMY_ID_TO_NAME[armyId]]) do
                    alloyTransforms[markerName] = nil
                end
            end
        end
    end
end

function Scenario.ResolveAndApply(total, humanCount, aiCount, slotPattern)
    local matchedScenario = FindMatchingScenario(total, humanCount, aiCount, slotPattern)
    ApplyScenario(matchedScenario, total, slotPattern)
    return matchedScenario.area, matchedScenario.spawnsUnits
end

return Scenario
```
[/CODE]

### `<MapName>_data.lua`

[CODE]
```lua
-- LOAD-SCOPE GATE -- required, do not remove. See §8.
local sangenLoadPath = ImportedFileInfo and ImportedFileInfo.FileName or ""
if sangenLoadPath ~= string.format("maps/%s/%s_data.lua",
        GameInfo.MapInfo.dataName, GameInfo.MapInfo.dataName) then
    return {}
end

local ok, err = pcall(function()

    local Scenario = Import("maps/<MapName>/<MapName>_Scenarios_Script.lua").Scenario

    local function ReadLobby()
        local infoOk, lobbyInfo = pcall(Engine.GetLobbyInformation)
        if not infoOk or not lobbyInfo or not lobbyInfo.playersInformation then
            return nil
        end
        local total, humanCount, aiCount = 0, 0, 0
        for _, player in ipairs(lobbyInfo.playersInformation) do
            total = total + 1
            if player.playerType == PlayerType.AI then
                aiCount = aiCount + 1
            else
                humanCount = humanCount + 1
            end
        end
        return total, humanCount, aiCount, lobbyInfo.playersInformation
    end

    local total, humanCount, aiCount, playersInformation = ReadLobby()
    if not total then return end

    local function BuildSlotPattern(players)
        local chars = {}
        for i = 1, 16 do chars[i] = "-" end
        for _, player in ipairs(players) do
            if player.armyID and player.armyID >= 1 and player.armyID <= 16 then
                chars[player.armyID] = (player.playerType == PlayerType.AI) and "A" or "h"
            end
        end
        return table.concat(chars)
    end
    local slotPattern = BuildSlotPattern(playersInformation)

    -- Declared before NewThread because the thread closes over them; assigned after.
    local chosenArea, spawnsUnitsEnabled

    -- The ONE NewThread this file is allowed. A second one silently never runs.
    NewThread(function()
        if not chosenArea then return end
        if IsHost then
            local Area = Import("common/area.lua").Area
            local PlayableAreaManager = Import("host/managers/playableArea/hostPlayableAreaManager.lua")
            -- Throwaway nudge first: SetPlayableArea only runs the resource-spot
            -- enable/disable pass when the area actually CHANGES.
            PlayableAreaManager.SetPlayableArea(Area.FromMapArea({ x = -1000, y = -1000, width = 1, height = 1 }))
            PlayableAreaManager.SetPlayableArea(Area.FromMapArea(chosenArea))
        end
    end)

    local resolveOk, resolvedArea, resolvedSpawnsUnits =
        pcall(Scenario.ResolveAndApply, total, humanCount, aiCount, slotPattern)
    if resolveOk then
        chosenArea, spawnsUnitsEnabled = resolvedArea, resolvedSpawnsUnits
    else
        Warn("SANGEN: Scenario.ResolveAndApply threw: "..tostring(resolvedArea))
    end

end)

if not ok then
    Warn("SANGEN: <MapName>_data.lua failed, map loads with defaults instead: "..tostring(err))
end

-- No MapData override returned: LoadMapData's merge is upsert-only and cannot hide
-- markers by omission. Marker deletion mutates GameInfo.MapData directly instead.
return {}
```
[/CODE]

That is a complete, working scenario system. Everything from here is detail.

---

## 2. Why two files

`_data.lua` is loaded automatically by stock, unmodified engine code. `common/mapUtils.lua`'s `LoadMapData()` does this after decoding the `.sanmap`:

[CODE]
```lua
local mapName = GameInfo.MapInfo.dataName
local mapDataFile = string.format("maps/%s/%s_data.lua", mapName, mapName)
local mapDataPath = libPath..mapDataFile
if Engine.FileExists(mapDataPath) then
    local mapData = Import(mapDataFile).MapData
    ...
end
```
[/CODE]

No hook to register, nothing to patch. It survives a game update.

**There is no per-map script hook in this build.** `<map>_script.lua` no longer exists; the only surviving per-map script, `maps/showcase_script.lua`, is invoked by hand from a debug command (`host/testUtils.lua:544`), never automatically. `<map>_data.lua` is the **only** automatic per-map Lua entry point left, so match logic has nowhere else to go.

The split is a discipline choice, not an engine requirement:

| File | Job |
|---|---|
| `<MapName>_data.lua` | **Orchestrator.** Read the lobby, derive counts, build the slot pattern, hand them to the scenario module, wire the result back into the map. No scenario content. |
| `<MapName>_Scenarios_Script.lua` | **Definitions + algorithm.** Every scenario, the matcher, the mutator, any unit generators. |

Why bother: the orchestrator is the dangerous file. It runs synchronously inside `LoadMapData` where an uncaught error aborts the entire remaining load — no armies, no units, no props, no markers, no resource spots. You want it short, stable, and rarely touched. Scenario data changes constantly; keep the churn out of the file that can brick the map.

### Two hard constraints on the split

**Both files must be colocated in `LJ/lua/maps/<MapName>/`.** Putting the scenario file in the map's asset folder was tried and proven impossible. `Import()` cannot address anything outside the `LJ/lua` root — a `"../../Sanctuary_Data/..."` path fails `Engine.FileExists()` outright, so `Import()` raises its "File doesn't exist" error and aborts the entire calling chunk. If one file moves, both move.

**`Import()` is not `require`.** It runs the file in a fresh environment table (with `__index = _G`) and returns *that table*. Your `return Scenario` at the bottom is ignored. Only **globals** the file assigned are visible to the caller:

[CODE]
```lua
-- in the scenario file
Scenario = {}          -- global. Correct.
local Scenario = {}    -- local. Import() returns a table with none of your fields,
                       -- and no error. Silent, total failure.
```
[/CODE]

[CODE]
```lua
-- in _data.lua
local Scenario = Import("maps/X/X_Scenarios_Script.lua").Scenario   -- correct
local Scenario = Import("maps/X/X_Scenarios_Script.lua")            -- wrong, silently
```
[/CODE]

Each file gets its own environment, so a global set in file A is **not** visible in file B. The two files share state only through the `Scenario` table both hold.

---

## 3. The three-tier match

Resolution is three tiers, checked most specific first. This is the whole matcher, verbatim from the live file:

[CODE]
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
[/CODE]

| Tier | Table | Test | Notes |
|---|---|---|---|
| 1 | `PATTERN_SCENARIOS` | `scenario.pattern == slotPattern` | Exact string equality on all 16 characters. No wildcards. |
| 2 | `COUNT_SCENARIOS` | `scenario.match(total, human, ai, pattern)` | **Ordered. First match wins.** Array order is the priority order. |
| 3 | `DEFAULT_SCENARIO` | always | Reached only if nothing above matched. Not a table — a single scenario. |

Note the `pcall` around `scenario.match`. A predicate that throws is treated as "did not match" and the loop continues. That is deliberate — a typo in one predicate degrades to the next rule instead of killing map load — but it also means **a broken predicate fails silently**. If a scenario mysteriously never matches, check the predicate for a nil dereference before anything else.

### Ordering: identity beats counts

Tier 2 is an ordered list, and getting the order wrong is the single easiest way to break this system. The live file's first entry exists purely to win a race:

[CODE]
```lua
{
    -- Checked FIRST (before 1v1/2h1ai/4human/1h3ai) so that occupying any of
    -- slots 5-8 always wins on slot IDENTITY, regardless of aggregate
    -- total/human/AI counts -- e.g. 2 players scattered into slots 5 and 7 would
    -- otherwise match "1v1" and get the tiny AREA_356 before ever reaching this rule.
    name = "slots5to8AnyFilled",
    match = function(t, h, a, pattern) return pattern:sub(5, 8):find("[^-]") ~= nil end,
    area = AREA_FULL,
    spawnsUnits = true,
    alloyMode = "occupancy",
},
{
    name = "1v1",
    match = function(t, h, a, pattern) return t == 2 end,
    area = AREA_356,
    ...
},
```
[/CODE]

Two players in slots 5 and 7 satisfy `t == 2`. On `Pandemonium Isthmus` the slot-5-to-8 spawn markers are far outside the 356x356 centre arena, so matching `"1v1"` would put both commanders outside the playable area. Putting the identity rule first makes that impossible.

**Rule of thumb:** the more specific a predicate, the earlier it goes. Slot-identity rules before count rules; narrow count rules before broad ones. The live file ends with a deliberate catch-all:

[CODE]
```lua
{
    name = "floor169",
    match = function(t, h, a, pattern) return t > 2 end,
    area = AREA_169,
    alloyMode = "occupancy",
},
```
[/CODE]

Anything with more than two players that matched nothing above lands here rather than falling through to `DEFAULT_SCENARIO`.

---

## 4. The `slotPattern` string

A 16-character string, one character per army slot, built fresh every match:

| Char | Meaning |
|---|---|
| `h` | human player |
| `A` | AI player |
| `-` | empty slot |

**Index = armyID.** `slotPattern:sub(3,3)` is army slot 3, i.e. `ARMY_03`.

[CODE]
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
[/CODE]

The input is `Engine.GetLobbyInformation().playersInformation` — the official engine array, one entry per **filled** slot. Empty slots are simply absent, which is why the table is pre-filled with `-` first. The two fields used are real and load-bearing:

- `player.armyID` — the same field `common/gameUtils.lua`'s `CreateArmies()` correlates against `mapStartSlotIndex`.
- `player.playerType` — compared against `PlayerType.AI`, the same way `CreateArmies()` decides whether to attach `aiSettings`.

Examples:

```
"hh--------------"   1v1, humans in slots 1 and 2
"hA--------------"   1 human vs 1 AI
"hhhh------------"   4 humans, slots 1-4
"----hhhh--------"   4 humans, slots 5-8      <- identical counts, different geography
"hAAA------------"   1 human, 3 AI
```

16, not 8: the lobby UI currently exposes 8 slots, but the map data itself supports `ARMY_01` through `ARMY_16`, so the pattern covers the full range rather than assuming the UI's present limit is permanent.

**Why this exists at all:** aggregate counts are ambiguous. "1 human + 3 AI" reads identically whether the human is in slot 1 or slot 4. `"hhhh------------"` and `"----hhhh--------"` have identical `total`, `humanCount` and `aiCount` and completely different spawn geography.

Useful predicate idioms:

[CODE]
```lua
-- any of slots 5-8 occupied
match = function(t, h, a, pattern) return pattern:sub(5, 8):find("[^-]") ~= nil end

-- exactly slots 1 and 2, both human, nothing else
match = function(t, h, a, pattern) return pattern == "hh--------------" end

-- slot 1 is a human and slot 5 is an AI
match = function(t, h, a, pattern)
    return pattern:sub(1,1) == "h" and pattern:sub(5,5) == "A"
end

-- top half of the map (slots 1-4) entirely empty
match = function(t, h, a, pattern) return pattern:sub(1, 4) == "----" end
```
[/CODE]

For a full-string equality test, prefer `PATTERN_SCENARIOS` — same effect, cheaper, and it documents intent.

---

## 5. Every scenario field

A scenario is a plain table. Exact shapes:

| Field | Type | Tier | Required | Meaning |
|---|---|---|---|---|
| `name` | string | all | yes | Identifier. Used in logs and by the unit-spawn dispatch. Make it unique. |
| `pattern` | string | tier 1 only | tier 1 only | 16-char slot pattern, exact match. |
| `match` | function | tier 2 only | tier 2 only | `function(total, humanCount, aiCount, slotPattern) -> boolean` |
| `area` | table | all | yes | `{ x, y, width, height }`. Playable rectangle. |
| `spawnsUnits` | boolean | all | no | Gate for bonus units. Falsy = no bonus units. |
| `alloyMode` | string | all | yes | `"explicit"` / `"occupancy"` / `"keepAll"` / `"delta"`. See §6. |
| `spawns` | table | all | no | `ARMY_XX -> { x, y, z }`. Overrides spawn marker positions. |
| `alloys` | table | all | conditional | Shape depends on `alloyMode`. See §6. |

### `area`

[CODE]
```lua
local AREA_356  = { x = 846, y = 846, width = 356, height = 356 }
local AREA_FULL = { x = 0, y = 0, width = 2048, height = 2048 }
```
[/CODE]

`x`/`y` are the **minimum corner**, not the centre. `y` is world **z** — this matches the `.sanmap`'s own `areas` block:

[CODE]
```json
"areas": { "PlayableArea": { "x": 846.0, "y": 846.0, "width": 356.0, "height": 356.0 } }
```
[/CODE]

`common/area.lua`'s `Area.FromMapArea` does the corner-to-centre conversion for you:

[CODE]
```lua
function Area.FromMapArea(mapArea)
    return Area.new(
        EngineClasses.float2(mapArea.x + mapArea.width * 0.5, mapArea.y + mapArea.height * 0.5),
        EngineClasses.float2(mapArea.width, mapArea.height))
end
```
[/CODE]

The area does two things: it drives the navigation barriers units cannot cross, and it enables/disables every resource spot by containment (`hostPlayableAreaManager.lua`'s `CheckResourceSpots`). ⚠️ Units spawned outside the active area are culled — models and strategic icons both.

### `spawnsUnits`

Boolean, read by `ResolveAndApply` and returned to the orchestrator:

[CODE]
```lua
local chosenArea, spawnsUnitsEnabled = matchedScenario.area, matchedScenario.spawnsUnits
```
[/CODE]

The orchestrator gates the deferred spawn call on it. `DEFAULT_SCENARIO` in the live file omits the field entirely, which is `nil`, which is falsy — fine.

> The live file also carries a `navy = true/false` field on some scenarios. **It is dead.** The old `SpawnNavalFleets` machinery that read it was removed on 2026-08-27 and nothing reads it now. Do not copy it into new scenarios; use `spawnsUnits`.

### `spawns`

[CODE]
```lua
spawns = {
    ARMY_01 = { x = 855,  y = 79.12979888916016, z = 920 },
    ARMY_02 = { x = 1193, y = 79.12979888916016, z = 1128 },
    ARMY_03 = { x = 833,  y = 80.794922,         z = 857 },
    ARMY_04 = { x = 1215, y = 80.759766,         z = 1191 },
},
```
[/CODE]

Keyed by army **name**, not index. `y` is world height. Applied like this:

[CODE]
```lua
if scenario.spawns then
    local spawnTransforms = GameInfo.MapData.markers and GameInfo.MapData.markers.Spawn
        and GameInfo.MapData.markers.Spawn.transforms
    if spawnTransforms then
        for armyName, pos in pairs(scenario.spawns) do
            if spawnTransforms[armyName] then
                spawnTransforms[armyName].position.x = pos.x
                spawnTransforms[armyName].position.y = pos.y
                spawnTransforms[armyName].position.z = pos.z
            end
        end
    end
end
```
[/CODE]

⚠️ **`spawns` only edits Spawn markers that already exist in the `.sanmap`.** The guard `if spawnTransforms[armyName] then` means a scenario cannot invent a spawn point for an army the map never authored one for — it is skipped, silently. (Alloys behave differently and *will* create missing entries; see §6.) Author every `ARMY_XX` Spawn marker you intend to use in the map itself.

**Why `spawns` exists.** `SpawnInitialUnits` reads `GameInfo.MapData.markers.Spawn.transforms[army.name]` with no per-composition branching of its own. Before scenarios, `ARMY_01`'s position was one static value in the `.sanmap` shared by every composition — so editing it to test a 6-player layout silently broke 1v1, which read that same value. Scenarios with `alloyMode = "explicit"` and a full `spawns` block are immune to that: each composition carries its own complete geography.

### `alloys`

Shape depends on the mode. In `"explicit"`:

[CODE]
```lua
alloys = {
    ARMY_01 = {
        { name = "AlloyMarker_219", x = 857, y = 78.72360229492188, z = 911 },
        { name = "AlloyMarker_237", x = 869, y = 78.72360229492188, z = 919 },
        { name = "AlloyMarker_97",  x = 873, y = 78.72360229492188, z = 923 },
    },
    ARMY_02 = { ... },
},
```
[/CODE]

In `"delta"`:

[CODE]
```lua
alloys = {
    add    = { ARMY_XX = { { name = "AlloyMarker_301", x = ..., y = ..., z = ... } } },
    remove = { ARMY_XX = { "AlloyMarker_219" } },
},
```
[/CODE]

`"occupancy"` and `"keepAll"` ignore `alloys` entirely.

### Two supporting tables you must maintain

[CODE]
```lua
local KNOWN_ALLOY_MARKERS = {
    ARMY_01 = { "AlloyMarker_219", "AlloyMarker_237", "AlloyMarker_97" },
    ARMY_02 = { "AlloyMarker_220", "AlloyMarker_240", "AlloyMarker_99" },
    ARMY_03 = { "AlloyMarker_282", "AlloyMarker_283", "AlloyMarker_284" },
    ARMY_04 = { "AlloyMarker_285", "AlloyMarker_286", "AlloyMarker_287" },
}
local ARMY_ID_TO_NAME = { [1] = "ARMY_01", [2] = "ARMY_02", [3] = "ARMY_03", [4] = "ARMY_04" }
```
[/CODE]

`KNOWN_ALLOY_MARKERS` is the **scenario system's jurisdiction**. Only markers listed here are ever deleted. `Pandemonium Isthmus` has 288 alloy markers in its `.sanmap`; the other 276 are neutral map resources that no scenario touches. This is a feature — you opt individual mex into per-army control by listing them.

`ARMY_ID_TO_NAME` maps slot index to army name for the `"occupancy"` path. Both tables must be extended together when you add an army to the system.

---

## 6. `alloyMode` in full

This is `ApplyScenario`'s alloy half, verbatim from the working file:

[CODE]
```lua
local alloyTransforms = GameInfo.MapData.markers and GameInfo.MapData.markers.Alloys
    and GameInfo.MapData.markers.Alloys.transforms
if alloyTransforms then
    if scenario.alloyMode == "explicit" then
        local mentionedArmies = {}
        for armyName, points in pairs(scenario.alloys or {}) do
            mentionedArmies[armyName] = true
            for _, p in ipairs(points) do
                if not alloyTransforms[p.name] then
                    alloyTransforms[p.name] = { rotation = IDENTITY_ROTATION, scale = IDENTITY_SCALE, position = {} }
                end
                alloyTransforms[p.name].position.x = p.x
                alloyTransforms[p.name].position.y = p.y
                alloyTransforms[p.name].position.z = p.z
            end
        end
        for armyName, markerNames in pairs(KNOWN_ALLOY_MARKERS) do
            if not mentionedArmies[armyName] then
                for _, markerName in ipairs(markerNames) do
                    alloyTransforms[markerName] = nil
                end
            end
        end
    elseif scenario.alloyMode == "occupancy" then
        for armyId = 1, 4 do
            if slotPattern:sub(armyId, armyId) == "-" then
                local armyName = ARMY_ID_TO_NAME[armyId]
                for _, markerName in ipairs(KNOWN_ALLOY_MARKERS[armyName]) do
                    alloyTransforms[markerName] = nil
                end
                Log("SANGEN: "..armyName.." has no player -- its 3 mex removed before RunMapSetup.")
            end
        end
    elseif scenario.alloyMode == "delta" then
        for _, points in pairs((scenario.alloys and scenario.alloys.add) or {}) do
            for _, p in ipairs(points) do
                if not alloyTransforms[p.name] then
                    alloyTransforms[p.name] = { rotation = IDENTITY_ROTATION, scale = IDENTITY_SCALE, position = {} }
                end
                alloyTransforms[p.name].position.x = p.x
                alloyTransforms[p.name].position.y = p.y
                alloyTransforms[p.name].position.z = p.z
            end
        end
        for _, markerNames in pairs((scenario.alloys and scenario.alloys.remove) or {}) do
            for _, markerName in ipairs(markerNames) do
                alloyTransforms[markerName] = nil
            end
        end
    end
    -- "keepAll": no deletion, every known marker stays as the .sanmap has it.
end
```
[/CODE]

with these constants:

[CODE]
```lua
local IDENTITY_ROTATION = { w = 1.0, x = 0.0, y = 0.0, z = 0.0 }
local IDENTITY_SCALE = { x = 1.0, y = 1.0, z = 1.0 }
```
[/CODE]

### Summary

| Mode | Positions | Deletion rule | Needs `alloys`? | Use when |
|---|---|---|---|---|
| `explicit` | Written from `scenario.alloys` (creates markers that do not exist) | Every army in `KNOWN_ALLOY_MARKERS` **not mentioned** by this scenario has all its markers deleted | Yes, complete | The scenario fully owns the layout and must be immune to other scenarios' data |
| `occupancy` | Untouched — the `.sanmap`'s baked positions are trusted | Armies 1-4 whose `slotPattern` char is `-` have their markers deleted | No | The baked positions are already right; you only need absent players' mex hidden |
| `keepAll` | Untouched | None | No | You *want* the empty slots' mex left on the map |
| `delta` | Written from `alloys.add` | Only `alloys.remove` | Yes, partial | Layering a small change on top of a baseline |

### Picking one

**`explicit`** — the safe default for any composition whose geography you have tuned by hand. Silence *is* a delete instruction: if a scenario does not mention `ARMY_03`, `ARMY_03`'s mex are removed. That is what makes it self-contained. Pair it with a full `spawns` block and the scenario cannot be perturbed by an edit made for a different player count.

**`occupancy`** — the low-maintenance mode. No coordinates in the scenario at all. It reads `slotPattern` directly and deletes only what belongs to unoccupied slots. Note two limits in the live implementation: the loop is hardcoded `for armyId = 1, 4`, and it keys off `slotPattern`, not `Armies`. Extend the loop bound (and both supporting tables) if your map puts more than four armies under scenario control.

**`keepAll`** — deletes nothing. The live `"2h1ai"` scenario uses it deliberately: two humans plus one AI in a four-slot layout means the fourth slot's three mex stay on the map as extra contested resources.

**`delta`** — ⚠️ **wired in but not used by any live scenario, so it is untested in-game.** The code path is real and correct-looking, but nothing has exercised it. Unlike `explicit`, silence is not a delete instruction: only `alloys.add` and `alloys.remove` are touched, everything else keeps whatever the baseline (or an earlier-applied layer) set. Intended for once a real baseline exists to diff against. If you use it, verify in-game before shipping.

### What deletion actually does

Deleting a marker from `GameInfo.MapData.markers.Alloys.transforms` means `RunMapSetup` never creates a resource spot object for it at all. `common/mapUtils.lua`:

[CODE]
```lua
local alloySpotMarker = GameInfo.MapData.markers["Alloys"]
if alloySpotMarker and alloySpotMarker.resource then
    ResourceSpotLoader.CreateResourceSpotPrefab("alloys")
    if shouldInstantiateEntities then
        for markerName, transformData in pairs(alloySpotMarker.transforms) do
            ...
            _G.CreateResourceSpot("alloys", position, scale, rotation)
        end
    end
end
```
[/CODE]

It iterates whatever survives. A deleted marker produces no spot: not hidden, not disabled — never built.

This is a **different mechanism** from the playable area. `SetPlayableArea` enables/disables *already-created* spots by containment. Deletion removes them from existence. Deletion happens before `RunMapSetup`; the area pass happens after. Both are in play, and confusing them is a common source of "the mex is there but I cannot build on it" (that is the area) versus "the mex is gone entirely" (that is deletion).

Also note `RunMapSetup` **writes back** to the marker data — it snaps each surviving spot to the terrain surface and stores the rounded x/z. Your y coordinate in `alloys` is effectively advisory; the engine resamples it.

---

## 7. `ARMY_XX` and why the zero padding matters

Army names in the `.sanmap` are `ARMY_01` through `ARMY_16`. **The zero padding is load-bearing.** From `common/gameUtils.lua`'s `CreateArmies()`:

[CODE]
```lua
local armySetup = {}
for mapStartSlotName,_ in pairs(mapArmies) do
    table.insert(armySetup, mapStartSlotName)
end
table.sort(armySetup)
...
for mapStartSlotIndex, mapStartSlotName in ipairs(armySetup) do
    ...
    for index = 1, table.getn(playerInfo) do
        if playerInfo[index].armyID == mapStartSlotIndex then
            playerEntry = playerInfo[index]
            break
        end
    end
```
[/CODE]

`table.sort` on strings is **alphabetical**, and the resulting `ipairs` index is what lobby `armyID` is matched against. So:

```
padded:     ARMY_01, ARMY_02, ..., ARMY_09, ARMY_10, ARMY_11   -> indices 1..11  correct
unpadded:   ARMY_1, ARMY_10, ARMY_11, ARMY_2, ARMY_3, ...      -> lobby slot 2 becomes ARMY_10
```

Name your armies `ARMY_01`, not `ARMY_1`. There is no code path that repairs this, and the symptom — players spawning at the wrong start positions once you exceed nine slots — looks nothing like a naming problem.

The same names key everything else: `GameInfo.MapData.markers.Spawn.transforms[army.name]`, your `spawns` table, your `KNOWN_ALLOY_MARKERS` table, and `army.name` inside `Armies`.

---

## 8. Timing: the load order you must not break

Host side, from `script.lua` and `host/hostMain.lua`:

| # | Where | What |
|---|---|---|
| 1 | `script.lua:156` | `LoadMapData(mapPath)` — decodes the `.sanmap` into `GameInfo.MapData`, then `Import`s `<map>_data.lua`. **Your chunk's top-level code runs synchronously right here.** |
| 2 | `script.lua:162` | `CreateArmies()` — populates `Armies`; ends by calling `SpawnInitialUnits()`, which reads `markers.Spawn.transforms[army.name]` and places one commander per occupied army. |
| 3 | `script.lua:183` → tick 0 | `RunMapSetup(true)` — instantiates props, **creates a resource spot for every surviving `Alloys` marker**, instantiates decals, then `InitializePlayableArea()`. |
| 4 | `hostMain.lua` tick 0, after `RunMapSetup` | `ThreadsDispatcherUpdate()` — your `NewThread` callback fires here. |

Two consequences drive the entire design.

**Marker mutation must happen in step 1.** `ApplyScenario` writes `GameInfo.MapData.markers` synchronously during `LoadMapData`, before `CreateArmies` reads `Spawn` and before `RunMapSetup` reads `Alloys`. There is no async gap; it is guaranteed to be what both later steps see. Move that work into a `NewThread` and you are too late — commanders are already placed and resource spots already built.

**Playable-area and unit work must happen in step 4.** `Armies` is empty while your top-level code runs, and `InitializePlayableArea` has not been called yet. Both need deferring.

So the orchestrator does both, in this order:

[CODE]
```lua
local chosenArea, spawnsUnitsEnabled     -- declared first, thread closes over them

NewThread(function()                     -- registered second
    if not chosenArea then return end
    ...
end)

local resolveOk, resolvedArea, resolvedSpawnsUnits =    -- called third, still synchronous
    pcall(Scenario.ResolveAndApply, total, humanCount, aiCount, slotPattern)
```
[/CODE]

`ResolveAndApply` is called *after* the `NewThread` registration but is still fully synchronous — it completes long before the thread body runs. The forward declaration is what lets the thread see the result.

### Four rules learned the hard way

**1. Only ONE `NewThread` per script is honored.** A second call in the same file silently never runs. In the live map, two `NewThread` calls existed; the first claimed the slot and the second — which set the playable area *and* spawned every scenario unit — was silently dropped. No error, no log. Put all deferred work in one callback.

**2. Errors inside a `NewThread` callback are swallowed** by `common/systems/threads.lua`'s `ResumeThread`. ⚠️ And `Log()`/`Warn()` go to the F1 console, **which does not function in this build** — `game_logs/*.txt` stays empty too. A throw silently cancels everything after it in the callback, with no trace anywhere. Therefore **ordering inside the thread is load-bearing** and anything that might throw gets its own `pcall`:

[CODE]
```lua
NewThread(function()
    if not chosenArea then return end
    if IsHost then ... SetPlayableArea ... end               -- area first
    if IsHost and spawnsUnitsEnabled then                    -- units next
        local unitsOk, unitsErr = pcall(Scenario.SpawnMatchedScenarioUnits, chosenArea)
        if not unitsOk then Warn("SANGEN: scenario unit spawn failed: "..tostring(unitsErr)) end
    end
    local blockerOk, blockerErr = pcall(SpawnAirBlockerNavmapModifiers)   -- prefab work last
    if not blockerOk then Warn("SANGEN: blocker threw: "..tostring(blockerErr)) end
end)
```
[/CODE]

Both orderings above were bought with real regressions: prefab work placed ahead of the unit spawn killed every unit when it threw, and placed ahead of `SetPlayableArea` it risked its prefabs being culled by the throwaway 1x1 area. **New work goes at the bottom.**

**3. Set the playable area twice.** `SetPlayableArea` short-circuits when the area has not changed:

[CODE]
```lua
if playableArea:Equals(area) then
    return
end
```
[/CODE]

If your scenario's area happens to equal the map's baked `PlayableArea`, the resource-spot enable/disable pass never runs. Nudge to a throwaway area first so the real call is guaranteed to differ:

[CODE]
```lua
PlayableAreaManager.SetPlayableArea(Area.FromMapArea({ x = -1000, y = -1000, width = 1, height = 1 }))
PlayableAreaManager.SetPlayableArea(Area.FromMapArea(chosenArea))
```
[/CODE]

**4. The load-scope gate is required.** `_data.lua` has two callers:

```
common/mapUtils.lua:47   LoadMapData()          ->  Import("maps/X/X_data.lua")
host/testUtils.lua:364   LoadDebugUnitGroups()  ->  Import("/maps/X/X_data.lua")
```

`Import` caches on the **literal string** (`common/systems/import.lua:128`), and `"maps/..."` is not `"/maps/..."`. Different key, cache miss, and **the entire chunk re-executes in the same Lua state**. Both callers are host-side. Measured 2026-08-28: 300 fighters and 40 ships against the 150 and 20 configured, and two `NewThread` registrations. Commanders were never doubled, because `CreateArmies` is not in this chunk — which is exactly what made the symptom look selective.

The fix is not a run-once counter. The debug loader wants `MapData.groups` for a UI; it is not starting a match. `import.lua:177` records which path each file was loaded under, so the file can tell:

[CODE]
```lua
local sangenLoadPath = ImportedFileInfo and ImportedFileInfo.FileName or ""
if sangenLoadPath ~= string.format("maps/%s/%s_data.lua",
        GameInfo.MapInfo.dataName, GameInfo.MapInfo.dataName) then
    return {}
end
```
[/CODE]

The debug loader still imports successfully and still reads any data globals; it simply does not trigger match setup.

> ⚠️ **Keep a backup of `_data.lua`.** `host/testUtils.lua` does `table.save(DebugUnitGroups, DebugUnitGroupsFilePath, "MapData = ")` with `DebugUnitGroupsFilePath = libPath.."/maps/X/X_data.lua"`. Using the debug unit-group UI **overwrites your file wholesale** with machine-generated data, destroying every line of hand-written code in it.

### Client vs host

`script.lua`'s `init()` sets `IsHost` XOR `IsClient` — one per Lua state, never both, and neither is ever reset. But a solo or listen-server match runs **two states in one process**, and `InitLobby` calls `LoadMapData` in both branches (`script.lua:156` under `if IsHost`, `:189` in the `else`). So your chunk runs in both.

Register the `NewThread` unconditionally so it exists in every role, and guard the host-only work *inside* it with `if IsHost then`. Wrapping separate `NewThread` calls per role is exactly what triggers rule 1.

---

## 9. Bonus units

Units are spawned from the deferred thread, after `Armies` is populated. The live implementation splits this into two pieces that do not know about each other:

- **Executor** (`Scenario.SpawnUnits`) — takes a flat, already-resolved array of `{armyIndex, templateIdentifier, x, y, z}` and creates them. Knows nothing about terrain, water, or unit type.
- **Generator** — decides *where*, for one specific scenario. Hand-authored placement.

### The executor

[CODE]
```lua
local UNIT_SPAWN_BATCH_SIZE = 100

function Scenario.SpawnUnits(instructions)
    local sinceYield = 0
    local placed, failed = 0, 0
    for _, instr in ipairs(instructions) do
        local ok, unit = pcall(CreateUnit, instr.armyIndex, instr.templateIdentifier,
            EngineClasses.float3(instr.x, instr.y, instr.z))
        if ok and unit then placed = placed + 1 else failed = failed + 1 end

        sinceYield = sinceYield + 1
        if sinceYield >= UNIT_SPAWN_BATCH_SIZE then
            sinceYield = 0
            WaitTicks(1)
        end
    end
    Log(string.format("SANGEN: SpawnUnits placed %d, failed %d (of %d requested).",
        placed, failed, #instructions))
end
```
[/CODE]

Four things in there are not optional:

- **Position is `EngineClasses.float3(x, y, z)`**, not a plain table.
- **Check `ok` AND `unit`.** `pcall` catches a *thrown* error. `CreateUnit` can return normally with a falsy result (invalid, occupied or unreachable spot) and `pcall` alone reports that as success.
- **`WaitTicks(1)` every ~100 units** so a large spawn does not hitch a tick.
- **Never call `CreateUnit` outside this function.** Every spawn path produces instructions and hands them here.

### The dispatch and a generator

[CODE]
```lua
function Scenario.SpawnMatchedScenarioUnits(area)
    if currentMatchedScenarioName == "slots5to8AnyFilled" then
        Scenario.SpawnUnits(BuildSlots5to8Instructions(area))
    end
end
```
[/CODE]

`currentMatchedScenarioName` is a module-local set inside `ResolveAndApply`, because the orchestrator only gets `area` back and the deferred call needs to know *which* scenario matched:

[CODE]
```lua
local currentMatchedScenarioName = nil   -- declared ahead of both, so they share the upvalue

function Scenario.ResolveAndApply(total, humanCount, aiCount, slotPattern)
    local matchedScenario = FindMatchingScenario(total, humanCount, aiCount, slotPattern)
    local chosenArea, spawnsUnitsEnabled = matchedScenario.area, matchedScenario.spawnsUnits
    currentMatchedScenarioName = matchedScenario.name
    ApplyScenario(matchedScenario, total, slotPattern)
    return chosenArea, spawnsUnitsEnabled
end
```
[/CODE]

Keep the dispatch a plain `if`/`elseif` until you have more than two or three generators.

### Iterating armies correctly

[CODE]
```lua
for armyIndex, army in pairs(Armies) do
    -- nil lobbyOptions => treat as OCCUPIED (an AI slot IS filled). Never assume a
    -- missing options table means an empty slot -- that silently skips real players.
    local bIsEmptySlot = army.lobbyOptions and army.lobbyOptions.isEmptySlot
    if not bIsEmptySlot then
        local slotNumber = tonumber(army.name:match("^ARMY_0?(%d+)$"))
        ...
    end
end
```
[/CODE]

⚠️ **Never hardcode an army index.** `CreateUnit(1, ...)` works in a slots-1-and-2 lobby and silently fails in a slots-5-and-6 lobby, because `ARMY_01` has no player there. Use `pairs(Armies)` keys — they track the real occupied armies.

⚠️ **Guard `army.lobbyOptions`.** It is nil on at least some entries (an AI army is the confirmed case). The unguarded `if not army.lobbyOptions.isEmptySlot then` **threw**, and because the call is inside a `pcall` and `Warn` goes nowhere, nothing spawned with no visible error anywhere.

### Deriving positions

Do not hardcode coordinates. Derive from the army's own Spawn marker and validate against live terrain. Reading the marker table directly is the confirmed-working pattern:

[CODE]
```lua
local spawnMarker = GameInfo.MapData.markers
    and GameInfo.MapData.markers.Spawn
    and GameInfo.MapData.markers.Spawn.transforms
    and GameInfo.MapData.markers.Spawn.transforms[armyName]
local originX = (spawnMarker and spawnMarker.position.x) or fallbackX
local originZ = (spawnMarker and spawnMarker.position.z) or fallbackZ
```
[/CODE]

For air units, sample the ground and add altitude — an absolute Y would spawn them underground on most terrain:

[CODE]
```lua
local sampleOk, groundHeight = Engine.SampleTerrainHeightFromCell(
    EngineClasses.int2(math.floor(fighterX), math.floor(fighterZ)))
local fighterY = (sampleOk == EngineErrorCode.Success and groundHeight or 78)
    + FIGHTER_ALTITUDE_ABOVE_GROUND
```
[/CODE]

For water, search and **return nil on failure**:

[CODE]
```lua
local function FindNearbyWaterSpot(idealX, idealZ, waterLevel)
    for n = 1, WATER_SPIRAL_MAX_TRIES do
        local gx, gz = GetSpiralGridXZ(n)
        local x = idealX + gx * WATER_SPIRAL_STEP
        local z = idealZ + gz * WATER_SPIRAL_STEP
        local errorCode, h = Engine.SampleTerrainHeightFromCell(EngineClasses.int2(math.floor(x), math.floor(z)))
        if errorCode == EngineErrorCode.Success and h < waterLevel then
            return x, z
        end
    end
    -- returns nil, NOT `idealX, idealZ`. The old fallback handed the caller a point it
    -- had just PROVEN was not water; CreateUnit then failed silently and the ship
    -- vanished with no visible trace. Never substitute a known-bad position for failure.
    return nil
end
```
[/CODE]

That single defect hid a battleship fleet for days. A search that finds nothing must return nil and the caller must count the miss.

### Known-good tpIds

| tpId | Unit | Status |
|---|---|---|
| `ucl4004` | Chosen T4 BigBot | spawn confirmed live |
| `ugl4001` | Guard T4 Bot | OK |
| `uel4001` | EDA T4 Railgun Sniper | OK |
| `uga1201` | Guard T1 "Aerofoil" AA Fighter | OK |
| `ucn3001` | Chosen T3 "Dreadnought" Battleship | OK — `availableUnits.lua` gates only the AI and build menu, **not `CreateUnit`** |
| `uga3201` | Guard T3 "Contrail" Fighter | ⚠️ NOT playable in this build (`BONE_MISSMATCH`) |

Highest validated tier is T4, every faction. No T5 exists in this build.

---

---

## Continued in Part 2

This post hit the forum's 50,000-character limit. The rest — a full worked example, a
troubleshooting table keyed by symptom, and a quick reference — is in **Part 2**.

*(link to Part 2 here)*
