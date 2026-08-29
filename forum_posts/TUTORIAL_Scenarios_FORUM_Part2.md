# Map Scenarios, Part 2: worked example, troubleshooting, reference

Continuation of **Map Scenarios: one map that reshapes itself for the lobby**. Read Part 1 first —
it covers the two-file layout, the three-tier match, `slotPattern`, every scenario field, all four
`alloyMode` values, `ARMY_XX` sorting, the load-order timing rules, and bonus units. Everything
here assumes you have that.

*(link to Part 1 here)*

Everything below is taken from a live, working implementation — `Pandemonium Isthmus`, confirmed
in-game 2026-08-28 — and from engine source in `engine/LJ/lua/`. Where something is unverified it
is marked ⚠️.

---

## Contents

1. [Worked example: adding a scenario end to end](#10-worked-example-adding-a-scenario-end-to-end)
2. [Troubleshooting by symptom](#11-troubleshooting-by-symptom)
3. [Quick reference](#12-quick-reference)

---

## 10. Worked example: adding a scenario end to end

Goal: four humans in slots 5-8 get the full map, their own hand-tuned spawn positions and mex, plus one T4 bot each as a bonus. Everything below drops into an existing scenario file.

### Step 1 — author the markers in the map

Before any Lua, the `.sanmap` must contain:

- `markers.Spawn.transforms.ARMY_05` … `ARMY_08` (a `spawns` block cannot create these).
- Alloy markers for those armies. Note their exact names — you need them in step 2.

### Step 2 — register the markers with the scenario system

Extend both supporting tables. Marker names here are placeholders — use your map's real ones.

[CODE]
```lua
local KNOWN_ALLOY_MARKERS = {
    ARMY_01 = { "AlloyMarker_219", "AlloyMarker_237", "AlloyMarker_97" },
    ARMY_02 = { "AlloyMarker_220", "AlloyMarker_240", "AlloyMarker_99" },
    ARMY_03 = { "AlloyMarker_282", "AlloyMarker_283", "AlloyMarker_284" },
    ARMY_04 = { "AlloyMarker_285", "AlloyMarker_286", "AlloyMarker_287" },
    -- NEW
    ARMY_05 = { "AlloyMarker_301", "AlloyMarker_302", "AlloyMarker_303" },
    ARMY_06 = { "AlloyMarker_304", "AlloyMarker_305", "AlloyMarker_306" },
    ARMY_07 = { "AlloyMarker_307", "AlloyMarker_308", "AlloyMarker_309" },
    ARMY_08 = { "AlloyMarker_310", "AlloyMarker_311", "AlloyMarker_312" },
}

local ARMY_ID_TO_NAME = {
    [1] = "ARMY_01", [2] = "ARMY_02", [3] = "ARMY_03", [4] = "ARMY_04",
    [5] = "ARMY_05", [6] = "ARMY_06", [7] = "ARMY_07", [8] = "ARMY_08",
}
```
[/CODE]

⚠️ If any of your scenarios use `alloyMode = "occupancy"`, also widen its loop bound in `ApplyScenario`, or armies 5-8 will never be cleaned up:

[CODE]
```lua
elseif scenario.alloyMode == "occupancy" then
    for armyId = 1, 8 do            -- was 1, 4
```
[/CODE]

### Step 3 — add the scenario

Exact slots, so this is tier 1:

[CODE]
```lua
local PATTERN_SCENARIOS = {
    {
        name = "4human-slots5to8",
        pattern = "----hhhh--------",
        area = AREA_FULL,
        spawnsUnits = true,
        alloyMode = "explicit",
        spawns = {
            ARMY_05 = { x = 400,  y = 78.72360229492188, z = 400 },
            ARMY_06 = { x = 1648, y = 78.72360229492188, z = 400 },
            ARMY_07 = { x = 400,  y = 78.72360229492188, z = 1648 },
            ARMY_08 = { x = 1648, y = 78.72360229492188, z = 1648 },
        },
        alloys = {
            ARMY_05 = {
                { name = "AlloyMarker_301", x = 392,  y = 78.72360229492188, z = 392 },
                { name = "AlloyMarker_302", x = 408,  y = 78.72360229492188, z = 392 },
                { name = "AlloyMarker_303", x = 400,  y = 78.72360229492188, z = 412 },
            },
            ARMY_06 = {
                { name = "AlloyMarker_304", x = 1640, y = 78.72360229492188, z = 392 },
                { name = "AlloyMarker_305", x = 1656, y = 78.72360229492188, z = 392 },
                { name = "AlloyMarker_306", x = 1648, y = 78.72360229492188, z = 412 },
            },
            ARMY_07 = {
                { name = "AlloyMarker_307", x = 392,  y = 78.72360229492188, z = 1640 },
                { name = "AlloyMarker_308", x = 408,  y = 78.72360229492188, z = 1640 },
                { name = "AlloyMarker_309", x = 400,  y = 78.72360229492188, z = 1660 },
            },
            ARMY_08 = {
                { name = "AlloyMarker_310", x = 1640, y = 78.72360229492188, z = 1640 },
                { name = "AlloyMarker_311", x = 1656, y = 78.72360229492188, z = 1640 },
                { name = "AlloyMarker_312", x = 1648, y = 78.72360229492188, z = 1660 },
            },
        },
    },
}
```
[/CODE]

`alloyMode = "explicit"` means armies 1-4 are not mentioned, so their twelve markers are deleted — exactly what you want when nobody is in those slots.

If you would rather this trigger on *any* occupancy of slots 5-8 instead of exactly four humans, make it a tier-2 entry at the **top** of `COUNT_SCENARIOS`:

[CODE]
```lua
{
    name = "4human-slots5to8",
    match = function(t, h, a, pattern) return pattern:sub(5, 8):find("[^-]") ~= nil end,
    area = AREA_FULL,
    spawnsUnits = true,
    alloyMode = "explicit",
    spawns = { ... },   -- same as above
    alloys = { ... },   -- same as above
},
```
[/CODE]

First in the list, so slot identity beats any count rule below it.

### Step 4 — write the unit generator

[CODE]
```lua
local BONUS_BOT_TPID = "ucl4004"   -- Chosen T4 BigBot, spawn confirmed live
local BONUS_BOT_OFFSET = 24        -- world units in front of the spawn marker

local function BuildSlots5to8BonusUnits()
    local instructions = {}
    for armyIndex, army in pairs(Armies) do
        -- nil lobbyOptions => OCCUPIED. Never assume nil means empty.
        local bIsEmptySlot = army.lobbyOptions and army.lobbyOptions.isEmptySlot
        if not bIsEmptySlot then
            local slotNumber = tonumber(army.name:match("^ARMY_0?(%d+)$"))
            if slotNumber and slotNumber >= 5 and slotNumber <= 8 then
                local spawnMarker = GameInfo.MapData.markers
                    and GameInfo.MapData.markers.Spawn
                    and GameInfo.MapData.markers.Spawn.transforms
                    and GameInfo.MapData.markers.Spawn.transforms[army.name]
                if spawnMarker then
                    local x = spawnMarker.position.x + BONUS_BOT_OFFSET
                    local z = spawnMarker.position.z
                    local errorCode, groundHeight = Engine.SampleTerrainHeightFromCell(
                        EngineClasses.int2(math.floor(x), math.floor(z)))
                    if errorCode == EngineErrorCode.Success then
                        instructions[#instructions + 1] = {
                            armyIndex = armyIndex,
                            templateIdentifier = BONUS_BOT_TPID,
                            x = x, y = groundHeight, z = z,
                        }
                    end
                    -- No else branch on purpose: a failed height sample DROPS the unit
                    -- rather than guessing a Y. Never substitute a known-bad position.
                end
            end
        end
    end
    return instructions
end
```
[/CODE]

### Step 5 — wire it into the dispatch

[CODE]
```lua
function Scenario.SpawnMatchedScenarioUnits(area)
    if currentMatchedScenarioName == "slots5to8AnyFilled" then
        Scenario.SpawnUnits(BuildSlots5to8Instructions(area))
    elseif currentMatchedScenarioName == "4human-slots5to8" then
        Scenario.SpawnUnits(BuildSlots5to8BonusUnits())
    end
end
```
[/CODE]

The name string must match `scenario.name` exactly.

### Step 6 — check the orchestrator

Nothing to change if you started from §1: `spawnsUnitsEnabled` already flows from the scenario into the gated call:

[CODE]
```lua
if IsHost and spawnsUnitsEnabled then
    local unitsOk, unitsErr = pcall(Scenario.SpawnMatchedScenarioUnits, chosenArea)
    if not unitsOk then
        Warn("SANGEN: scenario unit spawn failed, rest of map load unaffected: "..tostring(unitsErr))
    end
end
```
[/CODE]

### Step 7 — test

Host a lobby with humans (or AI, adjusting the pattern) in slots 5-8 and check, in order: commanders at the four new spawn positions; twelve mex around them and none in the centre; the full map traversable; one T4 bot beside each commander.

Test each composition you have a scenario for. A `"1v1"` regression caused by a slots-5-to-8 edit is exactly the failure mode `alloyMode = "explicit"` exists to prevent, and it is only visible if you actually load a 1v1.

---

## 11. Troubleshooting by symptom

⚠️ **Read this first: `Log()` and `Warn()` go to the F1 console, which does not function in this build, and `game_logs/*.txt` stays empty.** You cannot debug this from logs. The only signal proven to work is **a unit appearing on the map**. When instrumenting, make the readout *countable* (how many units in a row) rather than *spatial* (x=1000 vs x=1040 on a 2048-wide map is unreadable), wrap the probe in `pcall`, and spawn inside the active playable area or it gets culled.

### "The wrong scenario matched"

1. **A count rule is beating your identity rule.** `COUNT_SCENARIOS` is ordered and first-match-wins. Two players in slots 5 and 7 satisfy `t == 2` and will take `"1v1"` if `"1v1"` is listed first. Move the more specific rule up.
2. **Tier 1 needs all 16 characters.** `pattern = "hhhh"` never matches — `slotPattern` is always exactly 16 chars. Pad it: `"hhhh------------"`.
3. **Your predicate is throwing.** `FindMatchingScenario` `pcall`s each `match` and treats a throw as "no match", silently. A nil dereference or a typo makes the scenario permanently unreachable. Simplify it to `return true` temporarily to confirm the entry is even being reached.
4. **`h` vs `A` confusion.** AI is capital `A`; human is lowercase `h`. `pattern:sub(5,5) == "a"` never matches.
5. **You are counting observers.** `ReadLobby` classifies anything that is not `PlayerType.AI` as human. An observer is not an AI, so it inflates `humanCount` and `total`.

### "The playable area didn't change"

1. **Your area equals the current area.** `SetPlayableArea` returns early on `playableArea:Equals(area)`. Use the throwaway 1x1 nudge before the real call (§8, rule 3). This bites hardest when a scenario's area happens to equal the `.sanmap`'s baked `PlayableArea`.
2. **Your `NewThread` never ran.** Only one `NewThread` per script is honored. If the file has two, the second is silently dropped. Merge them.
3. **Something earlier in the thread threw.** Errors in a thread callback are swallowed. Anything after the throw never runs, with no trace. Move the `SetPlayableArea` block to the very first thing in the callback and `pcall` everything below it.
4. **You are not on the host.** The area call is guarded by `if IsHost`. That is correct — the host is authoritative and broadcasts to clients — but if you restructured the guards, check the branch is actually reached.
5. **Corner vs centre.** `area.x`/`area.y` is the **minimum corner**. If your rectangle looks shifted by half its size, you passed centre coordinates.
6. **`ResolveAndApply` threw**, so `chosenArea` is nil and the thread bails at `if not chosenArea then return end`. Everything downstream — area, units — dies with it.

### "The alloys are wrong"

1. **Too many mex, empty slots still have theirs.** You are in `keepAll` (deletes nothing) or `occupancy` with a loop bound that does not cover the army — the live loop is `for armyId = 1, 4`.
2. **Too few mex, a real player lost theirs.** You are in `explicit` and the scenario does not mention that army. Silence *is* a delete instruction in `explicit`. List every army the layout uses.
3. **`KNOWN_ALLOY_MARKERS` and `ARMY_ID_TO_NAME` are out of sync.** `occupancy` indexes `KNOWN_ALLOY_MARKERS[ARMY_ID_TO_NAME[armyId]]` with no nil guard — a missing entry throws, and because `ResolveAndApply` is `pcall`'d in the orchestrator, that throw takes out the *entire* scenario silently: no area, no units, nothing.
4. **A marker name is misspelled.** In `explicit`, an unknown name is *created* as a brand-new marker rather than moving the one you meant, so you get a spot in the right place *and* the original still sitting where it was. Copy names straight out of the `.sanmap`.
5. **The spot is visible but unusable.** That is the playable area, not deletion — `CheckResourceSpots` disables spots outside the area. Deletion removes them entirely. Two different mechanisms.
6. **Your y coordinate is ignored.** Expected. `RunMapSetup` snaps every surviving spot to the terrain surface via `Engine.SampleTerrainHeight` and writes the rounded x/z back into the marker data.
7. **You edited a marker no scenario owns.** Only names listed in `KNOWN_ALLOY_MARKERS` are subject to deletion. On `Pandemonium Isthmus` that is 12 of 288 — the rest are neutral map resources.

### "My spawn positions are ignored"

1. **The Spawn marker does not exist in the `.sanmap`.** This is the big one. `ApplyScenario` guards with `if spawnTransforms[armyName] then` — it will **never create** a missing Spawn marker, and skips silently. Author `ARMY_05`…`ARMY_08` in the map itself first. (Alloys behave differently: `explicit` and `delta` *do* create missing alloy markers.)
2. **Army name mismatch.** Keys are `ARMY_01`, zero-padded. `ARMY_1` matches nothing.
3. **You mutated markers too late.** `SpawnInitialUnits` runs at the end of `CreateArmies`, which is step 2 — before your `NewThread` body in step 4. `ResolveAndApply` must be called from the synchronous body of `_data.lua`.
4. **The chunk did not run in match scope.** Without the load-scope gate the chunk executes twice, and if you added your own run-once guard it may have latched on the debug-loader pass and skipped the real one. Use the `ImportedFileInfo.FileName` gate from §8.
5. **`ResolveAndApply` threw before reaching the spawn block.** It is `pcall`'d, so a throw is invisible. Comment out `alloys` and retest with `spawns` alone to isolate.
6. **Everything worked but the commander is somewhere odd.** `SpawnInitialUnits` clamps to the terrain bounds:

   ```lua
   startingPos.x = math.clamp(startingPos.x, 0, terrainSize.x - 2)
   startingPos.z = math.clamp(startingPos.z, 0, terrainSize.y - 2)
   ```

   An out-of-bounds coordinate is clamped to the map edge, not rejected.

### "No bonus units spawned"

1. **`spawnsUnits` is not set** on the matched scenario, so the orchestrator never calls the dispatch. It defaults to nil/falsy.
2. **The dispatch name does not match `scenario.name`** exactly, character for character.
3. **`army.lobbyOptions` is nil and you dereferenced it unguarded.** Confirmed nil on AI armies. The throw is swallowed and *nothing* spawns. Use `army.lobbyOptions and army.lobbyOptions.isEmptySlot`, and treat nil as **occupied**.
4. **You hardcoded an army index.** `CreateUnit(1, ...)` works in slots 1-2 and fails silently in slots 5-6, where `ARMY_01` has no player. Iterate `pairs(Armies)`.
5. **The position is invalid.** `CreateUnit` returns falsy without throwing. If you only checked `pcall`'s `ok`, you counted a failure as a success. Check `ok and unit`.
6. **The units are outside the playable area** and were culled — models and icons both.
7. **A second `NewThread`** stole the slot from the one that spawns units.
8. **The tpId is not spawnable.** `uga3201` is confirmed broken in this build. Try `ucl4004` first to prove the path works, then swap in your real unit.

---

## 12. Quick reference

**Files** — both in `engine/LJ/lua/maps/<MapName>/`, never the asset folder:

```
<MapName>_data.lua                 orchestrator: lobby -> pattern -> module -> map
<MapName>_Scenarios_Script.lua     definitions + matcher + mutator + generators
```

**Scenario shape:**

[CODE]
```lua
{
    name        = "unique-string",
    pattern     = "hh--------------",                      -- tier 1 only
    match       = function(t, h, a, pattern) ... end,      -- tier 2 only
    area        = { x = 0, y = 0, width = 2048, height = 2048 },
    spawnsUnits = true,
    alloyMode   = "explicit",                              -- explicit|occupancy|keepAll|delta
    spawns      = { ARMY_01 = { x = 855, y = 79.13, z = 920 } },
    alloys      = { ARMY_01 = { { name = "AlloyMarker_219", x = 857, y = 78.72, z = 911 } } },
}
```
[/CODE]

**Match order:** `PATTERN_SCENARIOS` (exact) → `COUNT_SCENARIOS` (ordered, first wins) → `DEFAULT_SCENARIO`.

**Pattern:** 16 chars, `h`/`A`/`-`, index = armyID.

**Host load order:** `LoadMapData` (your chunk, synchronous — mutate markers **here**) → `CreateArmies` → `SpawnInitialUnits` → `RunMapSetup` → `NewThread` callbacks (area + units **here**).

**Non-negotiables:**

- `Scenario` must be **global** in the scenario file.
- Army names **zero-padded**: `ARMY_01`, not `ARMY_1`.
- Exactly **one** `NewThread` per file.
- The **load-scope gate** in `_data.lua`.
- `SetPlayableArea` **twice** (throwaway nudge, then real).
- `CreateUnit`: check **`ok` AND `unit`**.
- A failed position search returns **nil**, never a known-bad coordinate.
- Guard `army.lobbyOptions`; nil means **occupied**.
- Back up `_data.lua` — the debug unit-group UI overwrites it.

---

### Verification notes

Everything above is drawn from the live `Pandemonium Isthmus` implementation (confirmed working in-game 2026-08-28) and from engine source in `engine/LJ/lua/` — `script.lua`, `common/mapUtils.lua`, `common/gameUtils.lua`, `common/area.lua`, `host/hostMain.lua`, `host/managers/playableArea/hostPlayableAreaManager.lua`.

Explicitly **not** verified in-game:

- **`alloyMode = "delta"`** — the code path exists and is wired in, but no live scenario uses it. Untested.
- **`PATTERN_SCENARIOS` tier-1 matching** — the live `PATTERN_SCENARIOS` table is empty. The matching code is real and trivially correct on inspection, but no shipped scenario has exercised it.
- **The `alloys` and `spawns` coordinates in §10** are illustrative placeholders, not real map data.
- **The `navy` field** appears on some live scenarios and is dead — nothing reads it.

The forward-looking SanGen spec (`MAP_SCENARIO_SPEC.md`) ratifies a future **three**-file split — `_data.lua` plus a generated `_Scenarios_Runtime.lua` and `_Scenarios_Data.lua` — where the runtime algorithm and the per-map tables are generated by the map generator rather than hand-authored. This tutorial documents the **two-file shape that is live and working today**. The scenario data format is the same either way; only which file holds it changes.
