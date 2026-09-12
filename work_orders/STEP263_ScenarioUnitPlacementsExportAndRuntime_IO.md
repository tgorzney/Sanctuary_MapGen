# STEP263 — `UnitPlacements` Lua-render leg + `SanGenScenarioRuntime.lua` spawn algorithm

**Layer:** IO/BRIDGE (category-2 bundled Lua content, `ARCH_15_04`). **Domain:**
`src/io/ScenarioScript_DataLua_IO.cpp`, `resources/lua/SanGenScenarioRuntime.lua`. **Executor:** SanGen
Coder. **Sequence:** depends on `STEP260` (the `Params::ScenarioUnitPlacement` struct must exist).
Independent of `STEP261`/`STEP262`/`STEP264`/`STEP265`. Implements
`ARCH_15_14_ForeignScenarioFullDataImportAndUnitPlacement.md` Part A's "Runtime consumption" section —
**that section's Lua code is binding, not illustrative; transcribe it, do not reinterpret it.**

## 0. Why

`STEP260` gives `ScenarioBody::unitPlacements` a `.sanmap` round-trip but no path to the game: nothing
renders it into `<MapName>_Scenarios_Data.lua`, and nothing in the bundled runtime spawns baked
placements at match time. This ticket closes both legs, plus forwards rotation through the existing
generic `Scenario.SpawnUnits` executor (today 3-argument, `CreateUnit` with no orientation — the exact
gap `ARCH_15_14` documents as fixable inside this pack's own runtime, distinct from and not fixing the
base game's own separate `gameUtils.lua` gap).

## 1. Lua-render leg — `src/io/ScenarioScript_DataLua_IO.cpp`

Per `ARCH_15_14` Part A's binding wire-shape note: render `x`/`y`/`z`/`rotation={x=,y=,z=,w=}`
(lowerCamelCase-family, matching this file's existing convention — distinct from the `.sanmap` JSON
leg's PascalCase from `STEP260`), applying the same `FlipPositionZ(positionZ, mapSize)` transform
already applied to spawns/alloys (`BuildPositionedRowBody`, `:63-70`) — `x`/`y` unflipped, `z` flipped,
**rotation renders completely verbatim, untouched by any flip** (the same established law already
stated in `MapExporter_Armies_IO.cpp`/`_Decals_IO.cpp`/`_Props_IO.cpp`/`_Markers_IO.cpp`).

Add a `BuildUnitPlacementsTableText`-style renderer (name it consistently with this file's existing
row-builder naming, e.g. alongside `BuildPositionedRowBody`), emitting one Lua table literal per
placement:
```lua
{ armyName = "ARMY_03", templateIdentifier = "ucn3001", x = 128.0, y = 0.0, z = 921.0,
  rotation = { x = 0.0, y = 0.0, z = 0.0, w = 1.0 } }
```
Called from wherever `BuildScenarioDataLuaText` (or its per-record equivalent) renders the other
`ScenarioBody` fields, emitting a new `UnitPlacements = { ... }` array key on the same record, always
present (empty array when `unitPlacements` is empty) — matching this file's own "every field always
rendered" convention already used for `SpawnIds`/`Alloys`.

## 2. Runtime — `resources/lua/SanGenScenarioRuntime.lua`

### 2a. Retain the full matched-scenario table

`Scenario.ResolveAndApply` (`:305-318`) currently keeps only `currentMatchedScenarioName` (`:303`,
`:309`). Add a second upvalue, `currentMatchedScenario`, set alongside it:
```lua
local currentMatchedScenarioName = nil
local currentMatchedScenario = nil    -- NEW (STEP263) -- the full matched scenario table, needed by
                                       -- Scenario.SpawnBakedUnitPlacements below (ARCH_15_14 Part A)
```
```lua
function Scenario.ResolveAndApply(total, humanCount, aiCount, playersInformation)
    ...
    currentMatchedScenarioName = matchedScenario.name
    currentMatchedScenario = matchedScenario   -- NEW (STEP263)
    ...
```

### 2b. New `Scenario.SpawnBakedUnitPlacements`

Add immediately before `Scenario.SpawnMatchedScenarioUnits` (`:359`), transcribing
`ARCH_15_14` Part A's binding code verbatim:
```lua
local function ResolveArmyIndexByName(armyName)
    for armyIndex, army in pairs(Armies) do
        if army.name == armyName then return armyIndex end
    end
    return nil
end

function Scenario.SpawnBakedUnitPlacements(scenario)
    if not scenario.unitPlacements then return end
    local instructions = {}
    for _, placement in ipairs(scenario.unitPlacements) do
        local armyIndex = ResolveArmyIndexByName(placement.armyName)
        if armyIndex then
            instructions[#instructions + 1] = { armyIndex = armyIndex,
                templateIdentifier = placement.templateIdentifier,
                x = placement.x, y = placement.y, z = placement.z,
                rotationX = placement.rotationX, rotationY = placement.rotationY,
                rotationZ = placement.rotationZ, rotationW = placement.rotationW }
        else
            Warn("SANGEN: scenario unit placement named unknown army '"..tostring(placement.armyName)..
                 "' -- skipped.")
        end
    end
    Scenario.SpawnUnits(instructions)
end
```

### 2c. Widen `Scenario.SpawnUnits` to forward rotation, nil-safely

Replace `:332-348` with (identical structure, only the `CreateUnit` call and the new `rotation` local
change — `placed`/`failed`/batching/logging all unchanged):
```lua
function Scenario.SpawnUnits(instructions)
    local sinceYield = 0
    local placed, failed = 0, 0
    for _, instr in ipairs(instructions) do
        local rotation = (instr.rotationW ~= nil)
            and { x = instr.rotationX, y = instr.rotationY, z = instr.rotationZ, w = instr.rotationW }
            or IDENTITY_ROTATION   -- this file's own existing constant (:74) -- unchanged default
        local ok, unit = pcall(CreateUnit, instr.armyIndex, instr.templateIdentifier,
            EngineClasses.float3(instr.x, instr.y, instr.z), rotation)
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
**Critical invariant:** existing category-4 hand-authored `GenerateScenarioUnits` generator output has
never produced a `rotationW` field — the `(instr.rotationW ~= nil)` guard means that instruction shape
falls back to `IDENTITY_ROTATION`, exactly today's behavior. This must not regress any existing
category-4 generator's spawn output.

### 2d. Wire `SpawnBakedUnitPlacements` into `Scenario.SpawnMatchedScenarioUnits`

At the top of `Scenario.SpawnMatchedScenarioUnits` (`:359`), before the existing
`if not currentMatchedScenarioName then return end` early-out (which only governs the category-4
`Import()` path below it), add:
```lua
    if currentMatchedScenario then
        Scenario.SpawnBakedUnitPlacements(currentMatchedScenario)
    end
```
Both mechanisms fire independently — a scenario may have `spawnsUnits=true` (category-4 generator),
non-empty `unitPlacements` (baked), both, or neither (`ARCH_15_14` Part A, "Corrections" item 2).

## 3. Tests

1. **Lua-render**: a `ScenarioBody` with 2 `unitPlacements` renders an `UnitPlacements` array with
   exactly those 2 table literals, correct field spelling (`armyName`/`templateIdentifier`/`x`/`y`/`z`/
   `rotation={x=,y=,z=,w=}`), `z` flipped via `FlipPositionZ`, `x`/`y`/rotation unflipped. An empty
   `unitPlacements` still renders `UnitPlacements = {}`.
2. **Runtime — `Scenario.SpawnUnits` rotation forwarding**: an instruction with all four `rotationX/Y/Z/W`
   fields set produces a `CreateUnit` call with a matching 4th `rotation` argument (verify via the
   project's existing Lua test harness — check `ScenarioScript_RuntimeResource_IO_Test.cpp` for the
   pattern). An instruction with `rotationW == nil` (simulating existing category-4 generator output)
   produces a `CreateUnit` call using `IDENTITY_ROTATION` — **this is the regression case; it must pass
   unchanged**.
3. **`Scenario.SpawnBakedUnitPlacements`**: a scenario table with 2 `unitPlacements` referencing a
   resolvable and an unresolvable `armyName` produces exactly 1 `Scenario.SpawnUnits` instruction (the
   resolvable one) and one logged `Warn(...)` for the unresolvable one; a scenario with
   `unitPlacements == nil` (every pre-STEP263 scenario table) is a no-op, no error.
4. **Wiring**: `Scenario.SpawnMatchedScenarioUnits` calls `Scenario.SpawnBakedUnitPlacements` even when
   `spawnsUnits == false` and no category-4 generator file exists (baked placements are independent of
   the generator opt-in flag).

## 4. Out of scope

- Anything in `common/gameUtils.lua` (the base game's own file, outside SanGen's ownership) — `ARCH_15_14`
  explicitly notes this ticket cannot and does not fix that file's own separate "TODO: rotation" gap.
- Any foreign-`.lua` import path — `STEP264`/`STEP265`.
- The UI editor — `STEP261`.

## 5. Files touched

**Modified:** `src/io/ScenarioScript_DataLua_IO.cpp`, `resources/lua/SanGenScenarioRuntime.lua`, plus
additions to their existing test files (`ScenarioScript_DataLua_IO_Test.cpp`,
`ScenarioScript_RuntimeResource_IO_Test.cpp` or wherever runtime-Lua behavior is currently exercised —
confirm exact file before starting).
