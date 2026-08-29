# Spawning units from a per-map Lua script

**Build:** Sanctuary: Shattered Sun Demo, verified 2026-08-28 against `engine/LJ/lua/` source and live in-game tests.

This is the complete, working method for spawning units on your own map from Lua. Every claim here is either a line of engine source or a live in-game observation. Line numbers refer to files under `<game>/engine/LJ/lua/`.

There are about six ways to get this wrong, and every one of them fails **silently** — no error, no log, nothing on screen. That is the real difficulty, not the API. Read the whole thing before you start debugging.

---

## Contents

1. [Quick start — minimal working example](#quick-start)
2. [Where map script code actually goes](#where-code-goes)
3. [The load chain, and why you must defer into `NewThread`](#load-chain)
4. [One `NewThread` per script](#one-newthread)
5. [`Import()` is not `require()`](#import)
6. [The big one: your data file runs TWICE](#double-execution)
7. [Calling `CreateUnit` correctly](#createunit)
8. [Never hardcode an army index](#army-index)
9. [Position validation](#positions)
10. [Ordering inside the thread is load-bearing](#ordering)
11. [Diagnostics — units are your only working signal](#diagnostics)
12. [Known-good template IDs](#tpids)
13. [Troubleshooting by symptom](#troubleshooting)

---

<a name="quick-start"></a>
## 1. Quick start — minimal working example

Create `engine/LJ/lua/maps/<MapName>/<MapName>_data.lua`, where `<MapName>` matches your `.sanmap` file name exactly (folder and file both).

This spawns five T4 bots for every occupied army, near that army's own spawn marker. It is complete and runnable — paste it, change the tpId, done.

[CODE]
```lua
-- maps/MyMap/MyMap_data.lua
--
-- Spawns 5 units per occupied army, offset from that army's Spawn marker.

-- ---------------------------------------------------------------------------
-- LOAD-SCOPE GATE. Required. See section 6 — without this, everything below
-- runs twice and you get double the units you asked for.
-- ---------------------------------------------------------------------------
local loadPath = ImportedFileInfo and ImportedFileInfo.FileName or ""
if loadPath ~= string.format("maps/%s/%s_data.lua",
        GameInfo.MapInfo.dataName, GameInfo.MapInfo.dataName) then
    return {}
end

-- ---------------------------------------------------------------------------
-- Everything is wrapped in pcall. An uncaught error in this file aborts
-- LoadMapData, which means CreateArmies and RunMapSetup never run: no armies,
-- no units, no props, no markers, no resource spots. Do not skip this.
-- ---------------------------------------------------------------------------
local ok, err = pcall(function()

    local UNIT_TPID   = "ugl4001"   -- Guard T4 bot
    local UNIT_COUNT  = 5
    local SPACING     = 8
    local FORWARD     = 25          -- world units +Z from the spawn marker

    -- The ONE NewThread this file gets. Registered unconditionally.
    NewThread(function()
        -- Host is authoritative for units; it broadcasts creation to clients.
        if not IsHost then return end

        local markers = GameInfo.MapData
            and GameInfo.MapData.markers
            and GameInfo.MapData.markers.Spawn
            and GameInfo.MapData.markers.Spawn.transforms
        if not markers then return end

        local placed, failed = 0, 0

        for armyIndex, army in pairs(Armies) do
            -- Guarded. nil lobbyOptions means OCCUPIED, not empty.
            local isEmptySlot = army.lobbyOptions and army.lobbyOptions.isEmptySlot

            local marker = markers[army.name]
            if not isEmptySlot and marker then
                local originX = marker.position.x
                local originZ = marker.position.z
                local startX  = originX - (UNIT_COUNT - 1) * SPACING * 0.5

                for i = 0, UNIT_COUNT - 1 do
                    local pos = EngineClasses.float3(
                        startX + i * SPACING,
                        0,                       -- y = 0 => CreateUnit snaps to ground
                        originZ + FORWARD)

                    -- Check BOTH: pcall caught nothing thrown, AND we got a unit.
                    local createdOk, unit = pcall(CreateUnit, armyIndex, UNIT_TPID, pos)
                    if createdOk and unit then placed = placed + 1
                    else failed = failed + 1 end
                end
            end
        end

        Log(string.format("MyMap: placed %d, failed %d", placed, failed))
    end)

end)

if not ok then
    Warn("MyMap_data.lua failed, map loads with defaults instead: " .. tostring(err))
end
```
[/CODE]

That is the whole shape. The rest of this post explains why each piece is there, because if you remove any of them you get a silent failure that is genuinely hard to diagnose.

---

<a name="where-code-goes"></a>
## 2. Where map script code actually goes

**There is no per-map script hook in this build.** `<map>_script.lua` no longer exists as an automatic entry point. The only surviving per-map script, `maps/showcase_script.lua`, is invoked by hand from a debug command (`host/testUtils.lua:544`), never automatically.

`<map>_data.lua` is the **only** automatic per-map Lua entry point left. `common/mapUtils.lua` looks for it next to your map and `Import()`s it if present:

[CODE]
```lua
-- common/mapUtils.lua:46-51 (inside LoadMapData)
local mapName = GameInfo.MapInfo.dataName
local mapDataFile = string.format("maps/%s/%s_data.lua", mapName, mapName)
local mapDataPath = libPath..mapDataFile
if Engine.FileExists(mapDataPath) then
    Log("LoadMapData: Loading data from Lua file: " .. mapDataPath)
    local mapData = Import(mapDataFile).MapData
```
[/CODE]

This is stock, unmodified game code. It is not a custom hook, so it survives a game patch.

### Two hard constraints on file location

- The file lives in **`LJ/lua/maps/<MapName>/`**, *not* in the map's asset folder (`Sanctuary_Data/Maps/<name>/`, where the `.sanmap`, Props and Textures live). `libPath`, which the path above is built from, points at `LJ/lua`.
- **`Import()` cannot address anything outside the `LJ/lua` root.** A `"../../Sanctuary_Data/..."` path fails `Engine.FileExists()` outright, so `Import()` raises "File doesn't exist" and aborts your whole chunk. Tested and disproved in-game 2026-08-20. Any helper file you split out must be colocated in `LJ/lua/maps/<MapName>/`.

### ⚠️ Back this file up

`host/testUtils.lua` can **overwrite `<map>_data.lua` wholesale**:

[CODE]
```lua
-- host/testUtils.lua:365
DebugUnitGroupsFilePath = libPath..mapPath        -- .../maps/X/X_data.lua
-- host/testUtils.lua:354
table.save(DebugUnitGroups, DebugUnitGroupsFilePath, "MapData = ")
```
[/CODE]

If the debug unit-group UI saves, your file is replaced with machine-generated `MapData = { ... }` and every line of hand-written code in it is gone. It does **not** write to a separate debug file. Keep backups.

---

<a name="load-chain"></a>
## 3. The load chain, and why you must defer into `NewThread`

`script.lua`'s `init(libPath, isClient)` sets **`IsHost` or `IsClient`**, one per Lua state (`script.lua:7-12`). Neither is ever reset to false.

> ⚠️ `IsHost` and `IsClient` are **not** mutually exclusive across the process. A solo game or a listen server is host *and* client, in two separate Lua states, in the same process. Your file runs in both. Assuming "if I'm here, I'm the client" is a bug — and a well-documented one; see section 4.

Host-side `InitLobby` (`script.lua:150-186`) runs in this order:

| Step | Source | What happens |
|---|---|---|
| 1 | `script.lua:156` | `mapUtils.LoadMapData(mapPath)` — reads the `.sanmap`, then `Import`s `<map>_data.lua`. **Your file's top-level code runs synchronously, here.** |
| 2 | `script.lua:160` | `gameUtils.CreateArmies()` — **`Armies` is fully populated at the end of this.** Its last act is `SpawnInitialUnits()`, one commander per non-empty army. |
| 3 | `script.lua:186` | `hostMain.Start()` |
| 4 | `host/hostMain.lua:67` | still tick 0 — `RunMapSetup(true)` creates props, decals, alloy resource spots |
| 5 | `host/hostMain.lua:105` | still tick 0 — `ThreadsDispatcherUpdate()` runs your queued `NewThread` callbacks |

**The consequence that matters:**

> While your file's top-level code is running, `Armies` is **empty**. `GameInfo.MapData` markers exist and are readable; armies do not. Nothing you spawn at load time can belong to a real player.

So: **anything that touches `Armies` must be deferred into a `NewThread` callback.** `NewThread` registers the callback into the current tick's bucket (`common/systems/threads.lua:86-106`); the dispatcher at the end of the *same* tick-0 pass runs it — after `CreateArmies` and after `RunMapSetup`. That is exactly the window you want.

The split is:

[CODE]
```lua
-- Top level (synchronous, inside LoadMapData):
--   read the lobby, pick a scenario, mutate GameInfo.MapData markers/areas
--   (these must be mutated BEFORE CreateArmies and RunMapSetup read them)

-- Inside NewThread (deferred, end of tick 0):
--   set the playable area, spawn units, instantiate prefabs
--   (these need Armies and the finished map setup)
```
[/CODE]

Reading the lobby at load time is done directly, since `Armies` is not available yet:

[CODE]
```lua
local lobbyInfo = Engine.GetLobbyInformation()
for _, player in ipairs(lobbyInfo.playersInformation) do
    -- player.armyID   1..16, correlates with ARMY_01..ARMY_16 alphabetically
    -- player.playerType   compare against PlayerType.AI
    -- player.nickname, player.team, player.faction
end
```
[/CODE]

Those are the same fields `common/gameUtils.lua:221-258` uses to build the armies, so they are the authoritative source at load time.

---

<a name="one-newthread"></a>
## 4. One `NewThread` per script

**Only one `NewThread` registration per script file is honoured. A second one silently never runs.** Observed the hard way twice — once in 2026-08-17, and again on 2026-08-28.

The 2026-08-28 case is worth spelling out because it is the exact trap:

A map file had two `NewThread` calls — one guarded "client work", one guarded "host work" — written on the assumption that `IsHost` and `IsClient` are mutually exclusive so only one would ever register. They are not (section 3). In a solo match against AI, both fired. The first claimed the single honoured slot; the second — the one that set the playable area **and spawned every unit** — was silently dropped. No error, no log, nothing on screen. Pure "my units don't spawn", reproducible only in a lobby where the tester is also the host, which is every local test.

**The rule:** register exactly one `NewThread`, unconditionally, and put the role guards *inside* it.

[CODE]
```lua
-- WRONG - two registrations, the second is silently dropped
if IsClient then NewThread(function() ... end) end
if IsHost   then NewThread(function() ... end) end

-- RIGHT - one registration, guards inside
NewThread(function()
    if IsHost then
        -- host work
    end
    -- work both roles do
end)
```
[/CODE]

---

<a name="import"></a>
## 5. `Import()` is not `require()`

`common/systems/import.lua` replaces `require`. Four behaviours will bite you:

**1. `return` is ignored.** `Import` loads the chunk into a fresh environment table and returns *that table*. It never collects the chunk's return value (`import.lua:191-226`). To expose an API, assign a **global**:

[CODE]
```lua
-- maps/MyMap/MyMap_helpers.lua
Scenario = {}                                -- GLOBAL, not local
function Scenario.SpawnFleet(area) ... end
```
[/CODE]

[CODE]
```lua
-- maps/MyMap/MyMap_data.lua
local Scenario = Import("maps/MyMap/MyMap_helpers.lua").Scenario
```
[/CODE]

`local Scenario = Import(path)` alone gives you a table with none of your fields and **no error** — you get nil-index throws later, far from the cause.

**2. Each file gets its own environment**, with `__index = _G` (`import.lua:107-109`, `197`). A global set in file A is *not* visible in file B. Two files can only share mutable state through an object both hold — e.g. the `Scenario` table above.

**3. `_G.Foo = x` writes to the real global table.** Use it deliberately and sparingly. Note that bare identifiers that *look* global resolve through `__index = _G` and quietly come back `nil` if nothing defined them:

> A real bug from this exact cause: `NavmapModifiers.GetNavmapModifierIDs(...)` was called without `local NavmapModifiers = Import("common/navmapModifiers.lua")` above it. `NavmapModifiers` is not an engine global — every real consumer imports it. The bare identifier resolved to nil, indexing it threw, and because it was inside a `NewThread` callback the throw was swallowed. Navmesh blocking had been dead for weeks.

**4. Modules are cached per Lua state, keyed by the literal path string** you pass in (`import.lua:128`). This is the source of the next section, which is the single worst trap in this whole subject.

---

<a name="double-execution"></a>
## 6. The big one: your data file runs TWICE

**Symptom:** exactly double the units you configured. **Cause:** two host-side callers `Import` your data file with two different spellings of the same path, and `Import` caches on the *literal string*.

[CODE]
```lua
-- common/mapUtils.lua:47  (LoadMapData)
Import("maps/X/X_data.lua")

-- host/testUtils.lua:364  (LoadDebugUnitGroups, called unconditionally
--                          from host/hostMain.lua:23)
Import("/maps/X/X_data.lua")     -- note the LEADING SLASH
```
[/CODE]

[CODE]
```lua
-- common/systems/import.lua:128
if loadedModules[filePath] then
    return loadedModules[filePath]
end
```
[/CODE]

`"maps/..."` and `"/maps/..."` are different keys. Cache miss. **The entire chunk re-executes in the same Lua state.** Both callers are host-side, so `IsHost` is true both times and role guards do not help you.

**Confirmed live 2026-08-28:** 300 fighters and 40 ships against the 150 and 20 configured. A run-counter probe returned markers all stamped "run 2", proving one Lua state and two executions.

Why this is so hard to spot: **commanders are never doubled**, because commander spawning lives in `CreateArmies` (`gameUtils.lua:356-371`), not in your file. So the symptom looks *selective* — "some things doubled, some didn't" — which sends you hunting in entirely the wrong place. This cost days.

This is probably a new problem, not an old one: when per-map logic lived in a real `<map>_script.lua`, it was loaded once. That hook was deprecated, logic moved into `_data.lua`, and `_data.lua` has two loaders.

### The fix

**Do not use a run-once counter.** The second caller is not wrong to import you — `LoadDebugUnitGroups` wants your `MapData.groups` table to populate a debug UI. It is not starting a match. The correct fix is for your file to **scope its side effects to the caller that owns them**.

`Import` records the path each file was loaded under, at `import.lua:176`, and exposes it in the file's own environment as `ImportedFileInfo`. Gate on it:

[CODE]
```lua
-- FIRST executable lines of <map>_data.lua.
local loadPath = ImportedFileInfo and ImportedFileInfo.FileName or ""
if loadPath ~= string.format("maps/%s/%s_data.lua",
        GameInfo.MapInfo.dataName, GameInfo.MapInfo.dataName) then
    return {}
end
```
[/CODE]

The debug loader still gets a fully successful import and still reads whatever data globals it wants — it simply does not trigger match setup. That is the correct division of responsibility, and unlike a run counter it **stays correct if the engine ever normalises its cache key**.

Note `GameInfo.MapInfo.dataName` is available by the time your file runs — `LoadMapData` populates `GameInfo.MapInfo` before the `Import` call. `dataName` is the `.sanmap` file name (no spaces), which may differ from the display name.

**Anything below the gate that is a pure data declaration** — your `MapData = { groups = ... }` table, for example — should go *above* the gate if you want the debug loader to see it. Everything that spawns, resizes, or instantiates goes below.

---

<a name="createunit"></a>
## 7. Calling `CreateUnit` correctly

[CODE]
```lua
-- host/units/unitsUtilities.lua:15
function _G.CreateUnit(armyId, tpId, position, orientation, progress)
```
[/CODE]

| Parameter | Notes |
|---|---|
| `armyId` | integer army index. **Never hardcode it** — see section 8 |
| `tpId` | template id string, e.g. `"ugl4001"` |
| `position` | `float3`. **If `position.y == 0`, `CreateUnit` snaps it to the terrain surface for you** (`unitsUtilities.lua:23-25`). Pass `y = 0` for ground units; pass a real `y` for air and for hulls floating at water level |
| `orientation` | optional `quaternion`, defaults to identity |
| `progress` | optional `0.0`–`1.0`, defaults to `1.0` (finished unit) |

The call you want:

[CODE]
```lua
local ok, unit = pcall(CreateUnit, armyIndex, tpId, EngineClasses.float3(x, y, z))
if ok and unit then
    placed = placed + 1
else
    failed = failed + 1
end
```
[/CODE]

**Check `ok` AND `unit`.** `pcall` only catches a *thrown* error. A call can return normally without giving you a usable unit — an invalid, occupied or unreachable spot — and `pcall` alone reports that as success. Both conditions are required, and the failure count is your only visible diagnostic (section 11).

### Batching

`CreateUnit` is synchronous and does real work per unit (prefab instantiation plus a network command per unit). A few hundred in one tick will hitch. Yield periodically:

[CODE]
```lua
local UNIT_SPAWN_BATCH_SIZE = 100

local sinceYield = 0
for _, instr in ipairs(instructions) do
    local ok, unit = pcall(CreateUnit, instr.armyIndex, instr.tpId,
        EngineClasses.float3(instr.x, instr.y, instr.z))
    if ok and unit then placed = placed + 1 else failed = failed + 1 end

    sinceYield = sinceYield + 1
    if sinceYield >= UNIT_SPAWN_BATCH_SIZE then
        sinceYield = 0
        WaitTicks(1)
    end
end
```
[/CODE]

`WaitTicks(1)` only works inside a `NewThread` callback (it is a coroutine yield). That is one more reason all spawning lives there.

### ⚠️ Units outside the active playable area are culled

Models and strategic icons both. If you shrink the playable area, spawn inside the final area — and set the area **before** you spawn (section 10).

---

<a name="army-index"></a>
## 8. Never hardcode an army index

This is the most common way a map script "works on my machine" and does nothing for anyone else.

`CreateArmies` (`gameUtils.lua:208-334`) sorts the map's army slot names alphabetically and walks them in order. Slot names are `ARMY_01` … `ARMY_16` — **zero-padded, and the padding is load-bearing**, because the sort is alphabetical. Occupied slots get the lobby player's data. **Unoccupied slots still get an army created**, with `playerName = "no Player"` and `lobbyOptions.isEmptySlot = true` (`gameUtils.lua:290-301`).

So `Armies[1]` almost always *exists*. That is precisely what makes hardcoding dangerous: `CreateUnit(1, ...)` never errors, it just gives your units to whoever happens to be army 1.

**Confirmed live 2026-08-28:** `CreateUnit(1, ...)` worked in a lobby using slots 1 and 2, and silently produced nothing usable in a lobby using slots 5 and 6 — there, `ARMY_01` is an unowned filler army. No error either time.

**Use the real, populated `Armies` table.** This is also exactly what the engine's own `SpawnInitialUnits` does (`gameUtils.lua:356-371`):

[CODE]
```lua
for armyIndex, army in pairs(Armies) do
    -- Guard the dereference. lobbyOptions is nil on some entries.
    local isEmptySlot = army.lobbyOptions and army.lobbyOptions.isEmptySlot
    if not isEmptySlot then
        -- armyIndex is the value to pass to CreateUnit
        -- army.name is "ARMY_01".."ARMY_16"
        -- army.faction indexes Factions.FactionsData
    end
end
```
[/CODE]

### Guarding `army.lobbyOptions`

Write `army.lobbyOptions and army.lobbyOptions.isEmptySlot`, never `army.lobbyOptions.isEmptySlot`.

**A nil `lobbyOptions` means the slot is OCCUPIED.** An AI army is the confirmed case. Treating nil as "empty" silently skips real players.

> This exact unguarded dereference cost a day on 2026-08-28. A 1v1 with a human in slot 5 and an AI in slot 6 got the correct playable area and correct alloy handling — both of which are resolved elsewhere and never touch `lobbyOptions` — and **zero units**. The dereference threw, the throw was inside a `pcall` inside a `NewThread`, and there was no visible error anywhere.

### Extracting a slot number from the army name

[CODE]
```lua
local slotNumber = tonumber(army.name:match("^ARMY_0?(%d+)$"))
```
[/CODE]

### Getting an army's spawn position

Read the marker transforms directly. This is the pattern confirmed live:

[CODE]
```lua
local transforms = GameInfo.MapData
    and GameInfo.MapData.markers
    and GameInfo.MapData.markers.Spawn
    and GameInfo.MapData.markers.Spawn.transforms
local marker = transforms and transforms[army.name]
if marker then
    local x, z = marker.position.x, marker.position.z
end
```
[/CODE]

`marker.position` can be passed straight through to `CreateUnit` if you want the exact spawn point — no conversion needed.

---

<a name="positions"></a>
## 9. Position validation

### Rule 1: never substitute a known-bad position for a failure

This is the defect that hid a fleet of battleships for days. A spiral water search looked for a wet cell, and when it ran out of budget it did this:

[CODE]
```lua
-- WRONG. Do not do this.
return idealX, idealZ    -- a point the search just PROVED was dry
```
[/CODE]

`CreateUnit` was then handed a dry coordinate for a ship, failed, and the ship vanished with no trace. Because the "failure" path returned a plausible-looking position, nothing anywhere counted a miss.

**A search that finds nothing returns `nil`, and the caller counts the miss:**

[CODE]
```lua
local function FindNearbyWaterSpot(idealX, idealZ, waterLevel)
    for n = 1, WATER_SPIRAL_MAX_TRIES do
        local gx, gz = GetSpiralGridXZ(n)
        local x = idealX + gx * WATER_SPIRAL_STEP
        local z = idealZ + gz * WATER_SPIRAL_STEP
        local errorCode, h = Engine.SampleTerrainHeightFromCell(
            EngineClasses.int2(math.floor(x), math.floor(z)))
        if errorCode == EngineErrorCode.Success and h < waterLevel then
            return x, z
        end
    end
    return nil        -- bare nil. No fallback. Never a known-bad point.
end
```
[/CODE]

And at the call site, a point that cannot be resolved is **dropped and counted**, never appended at its bad coordinate.

### Rule 2: derive positions, don't hardcode them

Derive from the army's own spawn marker, then validate against live terrain. Useful engine calls:

[CODE]
```lua
local _, hasWater   = Engine.HasWater()
local _, waterLevel = Engine.GetWaterLevel()

local errorCode, height = Engine.SampleTerrainHeightFromCell(EngineClasses.int2(cx, cz))
if errorCode == EngineErrorCode.Success then
    -- height is the terrain height at that heightmap cell
end

local _, terrainSize = Engine.GetTerrainHeightmapSize()
```
[/CODE]

Validating live means your placement is robust against any residual error in offline analysis — it is *corrected* at runtime, not merely trusted.

### The spiral search helper

Copied from `host/testUtils.lua`'s own spawn code. Generates grid offsets outward from the origin, so nearer cells are visited first and ties naturally resolve toward your anchor:

[CODE]
```lua
local function GetSpiralGridXZ(n)
    local k = math.ceil((math.sqrt(n) - 1) / 2)
    local t = 2 * k + 1
    local m = t ^ 2
    t = t - 1
    if n >= m - t then return k - (m - n), -k else m = m - t end
    if n >= m - t then return -k, -k + (m - n) else m = m - t end
    if n >= m - t then return -k + (m - n), k else return k, k - (m - n - t) end
end
```
[/CODE]

Two useful variants built on it:

- **First-hit search** — stop at the first valid cell. Cheap, good enough for "somewhere near here that isn't underwater".
- **Best-in-budget search** — scan the whole budget and keep the *best* cell (e.g. deepest water), rejecting anything below a minimum threshold. Costs one terrain sample per probe; at a few hundred probes per army, once per match at load, that is not a hot path.

For water in particular, set a real minimum depth. A first-hit search anchors your fleet in the first puddle it finds, which is often a beach shelf a hull cannot float on.

### Altitude for air units

Sample the terrain and add your altitude — never use an absolute world Y:

[CODE]
```lua
local sampleOk, groundHeight = Engine.SampleTerrainHeightFromCell(
    EngineClasses.int2(math.floor(x), math.floor(z)))
local y = (sampleOk == EngineErrorCode.Success and groundHeight or FALLBACK_HEIGHT)
          + ALTITUDE_ABOVE_GROUND
```
[/CODE]

On a map whose terrain sits around Y 78-83, a bare absolute `y = 60` spawns your fighters underground almost everywhere. Pick a `FALLBACK_HEIGHT` near your map's typical terrain height, not zero, so even a failed sample lands somewhere sane.

---

<a name="ordering"></a>
## 10. Ordering inside the thread is load-bearing

Two facts combine into the nastiest failure mode in this whole subject:

1. **Errors inside a `NewThread` callback are swallowed.** `common/systems/threads.lua`'s `ResumeThread` catches the failed `coResume`, routes it to `ErrorHandler`, and drops the thread (`threads.lua:146-152`).
2. **`Log()` and `Warn()` go to the F1 console, which does not function in this build.** `game_logs/*.txt` stays empty too.

So a throw anywhere in your thread **silently cancels everything after it, with no trace anywhere.**

Two consequences:

**Put nothing ahead of work that must happen.** New work goes at the *bottom* of the callback.

**`pcall` anything that might throw**, so one subsystem's failure cannot take another's down with it.

[CODE]
```lua
NewThread(function()
    -- Bail early on a precondition rather than raising a second, misleading
    -- error on a nil dereference that would mask the real one.
    if not chosenArea then return end

    -- 1. Playable area FIRST. Prefabs created before the final area is set
    --    risk being culled.
    if IsHost then
        local Area = Import("common/area.lua").Area
        local PlayableAreaManager =
            Import("host/managers/playableArea/hostPlayableAreaManager.lua")

        -- SetPlayableArea only runs the resource-spot enable/disable pass when
        -- the area actually CHANGES. Nudge to a throwaway area first so the real
        -- call below is guaranteed to differ and always runs that pass.
        PlayableAreaManager.SetPlayableArea(
            Area.FromMapArea({ x = -1000, y = -1000, width = 1, height = 1 }))
        PlayableAreaManager.SetPlayableArea(Area.FromMapArea(chosenArea))
    end

    -- 2. Units NEXT, isolated in their own pcall.
    if IsHost and spawnsUnitsEnabled then
        local unitsOk, unitsErr = pcall(Scenario.SpawnMatchedScenarioUnits, chosenArea)
        if not unitsOk then
            Warn("scenario unit spawn failed: " .. tostring(unitsErr))
        end
    end

    -- 3. Prefab / navmesh work LAST, isolated. Nothing load-bearing after it.
    local blockerOk, blockerErr = pcall(SpawnAirBlockerNavmapModifiers)
    if not blockerOk then
        Warn("SpawnAirBlockerNavmapModifiers threw: " .. tostring(blockerErr))
    end
end)
```
[/CODE]

Both ordering constraints above are real regressions, both from 2026-08-28:

- The navmesh-blocker pass placed *ahead* of the unit spawn threw (the missing `Import` from section 5) and **silently killed every unit** as collateral damage.
- The same pass placed ahead of `SetPlayableArea` risked its prefabs being culled by the throwaway 1×1 area.

One more subtlety in that snippet: the throwaway 1×1 `SetPlayableArea` call is deliberate. `SetPlayableArea` only runs the resource-spot enable/disable pass when the area actually changes from what is already active — so if your chosen area happens to match the map's baked default, the pass never runs. Nudging first guarantees the real call differs.

### Variables the thread closes over

Declare them **before** `NewThread`, assign them **after**. The callback closes over the locals; it does not run until end of tick, so the assignment lands first:

[CODE]
```lua
local chosenArea, spawnsUnitsEnabled          -- declared before

NewThread(function()
    if not chosenArea then return end
    ...
end)

-- Assigned after. pcall'd separately from the file's outer pcall so a throw
-- here is attributable to THIS call rather than to the whole chunk.
local resolveOk, resolvedArea, resolvedSpawns =
    pcall(Scenario.ResolveAndApply, total, humanCount, aiCount, slotPattern)
if resolveOk then
    chosenArea, spawnsUnitsEnabled = resolvedArea, resolvedSpawns
else
    Warn("ResolveAndApply threw: " .. tostring(resolvedArea))
end
```
[/CODE]

---

<a name="diagnostics"></a>
## 11. Diagnostics — units are your only working signal

`Log()` and `Warn()` are **not** diagnostics in this build. Write them anyway — they cost nothing and will start working when the console is fixed — but do not plan around them.

**The only channel proven to work is a unit appearing on the map.** Instrument by spawning probe units. Four rules, each learned by getting it wrong:

**1. Make the readout countable, not spatial.** Asking a human to distinguish x=1000 from x=1040 on a 2048-wide map does not work — it produced an unusable result. Encode progress as *how many* units appear in one row, or as *which unit type* appears:

[CODE]
```lua
-- Each stage that completes adds one more unit to a row at map centre.
local function Mark(stage, armyIndex)
    pcall(CreateUnit, armyIndex, "ugl4001",
        EngineClasses.float3(1024 + stage * 10, 0, 1024))
end
```
[/CODE]

Then: 1 unit means it got to stage 1 and died; 3 units means stage 3. A human can count three units. A human cannot read a coordinate.

**2. Wrap the whole probe in `pcall`.** A probe that throws kills the thread it is measuring. This happened, and it took the navmesh blocker down with it.

**3. A probe cannot report a failure that prevents the probe from running.** If you want to know whether the thread was registered at all, you cannot ask the thread. Register the thread first, and have it report a **counter the synchronous code advanced**:

[CODE]
```lua
local reached = 0
-- ... top-level code does reached = reached + 1 at each milestone ...

NewThread(function()
    -- reports how far the SYNCHRONOUS code got, not how far this thread got
    for i = 1, reached do Mark(i, someArmyIndex) end
end)
```
[/CODE]

Zero units means the thread never ran at all — which is itself the answer.

**4. Spawn your probes inside the active playable area,** or they get culled and you learn nothing.

A run-counter probe built exactly this way is what proved the double-execution in section 6: eight markers, all stamped "run 2".

---

<a name="tpids"></a>
## 12. Known-good template IDs

Verified 2026-08-28. Highest validated tier is T4, every faction; there is no T5 in this build.

| tpId | Unit | Status |
|---|---|---|
| `ucl4004` | Chosen T4 BigBot | spawn confirmed live |
| `ugl4001` | Guard T4 Bot | OK |
| `uel4001` | EDA T4 Railgun Sniper | OK |
| `uga1201` | Guard T1 AA Fighter | OK |
| `ucn3001` | Chosen T3 Battleship | OK. Not build-menu approved, but `availableUnits.lua` gates only the AI and the build menu — **not `CreateUnit`**. Max weapon range 140, footprint 2.5 × 9.5 |
| `uga3201` | Guard T3 Contrail | ⚠️ **Not playable** — `BONE_MISSMATCH`. Do not use |

The engine's own commander choice, if you want it, is `Factions.FactionsData[army.faction].initialUnit`.

---

<a name="troubleshooting"></a>
## 13. Troubleshooting by symptom

### "No units appear at all"

Work down this list. Each of these produces exactly this symptom with no error message.

1. **Two `NewThread` calls in the file.** Only one is honoured. In a solo/listen-server game both `IsHost` and `IsClient` states exist, so role-guarded registrations that "can't both fire" do both fire. → Section 4.
2. **Unguarded `army.lobbyOptions.isEmptySlot`.** Throws on AI armies, and the throw is swallowed. → Section 8.
3. **Something ahead of your spawn threw.** A prefab pass, a probe, an `Import` you forgot. Move the spawn earlier and `pcall` everything around it. → Section 10.
4. **You spawned at load time instead of inside `NewThread`.** `Armies` is empty during `LoadMapData`. → Section 3.
5. **Missing `Import` for a module you reference by bare name.** Bare identifiers resolve to `nil` through `__index = _G` and throw on index. → Section 5.
6. **Your file never ran at all.** Check the folder and file name match `GameInfo.MapInfo.dataName` (the `.sanmap` file name) exactly, and that the file is under `LJ/lua/maps/`, not the asset folder. → Section 2.
7. **Your outer `pcall` is swallowing a load-time throw.** Keep the `pcall` — but confirm the `if not ok then Warn(...)` branch is not the path you are actually taking, using a unit probe rather than the log. → Section 11.

**To distinguish 1/4 from 2/3:** put a single unconditional probe spawn as the *first* statement inside the thread. If it appears, the thread ran and something later threw. If it does not, the thread never ran.

### "Double the units I asked for"

Your data file executed twice in one Lua state, because `common/mapUtils.lua` imports `"maps/X/X_data.lua"` and `host/testUtils.lua` imports `"/maps/X/X_data.lua"`, and `Import` caches on the literal string. Add the load-scope gate. → Section 6.

Tell-tale: commanders are **not** doubled (they come from `CreateArmies`, not your file), so the doubling looks selective. That selectivity is the confirming signature, not a reason to look elsewhere.

### "Some units missing, or fewer than I asked for"

- **Bad positions.** Ground units on water, ships on land, air units underground. Check that your position search returns `nil` on failure rather than a fallback coordinate, and that the caller counts the miss. → Section 9.
- **Culled by the playable area.** Units outside the active area are removed, models and icons both. Confirm the area is set *before* the spawn and that your positions are inside the final area. → Section 10.
- **Formation corners.** A rectangular grid over irregular terrain will put some cells in bad spots even when the anchor is good. Validate *every* grid point, not just the anchor, and tighten spacing.

Instrument it: count `placed` and `failed` in the spawn loop and spawn one probe unit per N failures somewhere obvious. A countable readout will tell you whether the shortfall is 1 unit or 80.

### "Works in a 1v1, breaks in a bigger lobby" (or vice versa)

You hardcoded an army index. `Armies[1]` exists even when nobody is in slot 1 — it is a `"no Player"` filler army with `isEmptySlot = true`. Iterate `pairs(Armies)` and use the key. → Section 8.

### "It worked yesterday and now my whole script is gone"

The debug unit-group UI overwrote `<map>_data.lua` with machine-generated `MapData = { ... }`. → Section 2. Restore from backup, and make backups.

---

## Summary checklist

- [ ] File is at `LJ/lua/maps/<MapName>/<MapName>_data.lua`, name matching the `.sanmap`
- [ ] Load-scope gate on `ImportedFileInfo.FileName` is the first executable code
- [ ] Whole body wrapped in `pcall`
- [ ] Exactly **one** `NewThread`, registered unconditionally, role guards inside
- [ ] All `Armies` access happens inside the thread
- [ ] `pairs(Armies)` for army indices — nothing hardcoded
- [ ] `army.lobbyOptions and army.lobbyOptions.isEmptySlot` — guarded, nil means occupied
- [ ] `local ok, unit = pcall(CreateUnit, ...)` — both `ok` and `unit` checked
- [ ] Position searches return `nil` on failure; callers count misses
- [ ] Playable area set before prefab/unit work; risky work `pcall`'d and last
- [ ] `WaitTicks(1)` every ~100 units
- [ ] Backup of the data file kept somewhere the engine cannot reach

Questions and corrections welcome — particularly if you find a build where the F1 console works, which would make all of this dramatically easier to debug.
