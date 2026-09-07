[← ARCH index](ARCH.md) · [§15 ARCH_15_MapScenarioSystem](ARCH_15_MapScenarioSystem.md) · SanGen ARCH §15.13. **Only the ARCH Expert writes this file.**

### 15.13 `SanGenScenarioRuntime.lua`'s `ApplyScenario` rewrite — the two-step `spawnId` resolution loop (ratifies `work_orders/ARCH_AMENDMENT_DRAFT_ScenarioSpawnId.md`, revision 3; resolves against the shape ratified at §15.12)

**Category-2 (`_Scenarios_Runtime.lua`) content per `ARCH_15_04`'s file categories** — ratified with
the same weight as any other Runtime.lua algorithm change, not a Data.lua-only or PARAMS-only change.
The immovable engine constraint is unchanged by this ruling: the final applied write always lands
keyed by `ARMY_XX` (`GameInfo.MapData.markers.Spawn.transforms[army.name]`,
`common/gameUtils.lua`'s hardcoded read path). `spawnId` is additive identity layered on top of
`armyName`, never a replacement for it, at every leg.

`ScenarioData.PATTERN_SCENARIOS`/etc.'s capture block (`SanGenScenarioRuntime.lua:40-43`) gains a
fifth capture:

```lua
local SCENARIO_SPAWN_POINTS = ScenarioData.SCENARIO_SPAWN_POINTS
```

`ApplyScenario`'s spawn-application block (currently `SanGenScenarioRuntime.lua:169-181`, the old
`for armyName, pos in pairs(scenario.spawns) do` loop) is **replaced**:

```lua
-- Two-step resolution (§15.12): the custom pool first, then a literal ARMY_XX fallback read
-- straight off the live .sanmap default -- never a second stored copy of that default. Pool
-- spawnIds are RULED to never be ARMY_XX-shaped (§15.12 Validator), so these two steps can never
-- collide on one string. Returns nil (and Warn()s) if spawnId matches neither.
local function ResolveSpawnId(spawnId, spawnTransforms)
    for _, point in ipairs(SCENARIO_SPAWN_POINTS) do
        if point.spawnId == spawnId then
            return point.armyName, point.x, point.y, point.z
        end
    end
    local defaultTransform = spawnTransforms and spawnTransforms[spawnId]
    if defaultTransform then
        return spawnId, defaultTransform.position.x, defaultTransform.position.y, defaultTransform.position.z
    end
    Warn("SANGEN: scenario spawnId '"..tostring(spawnId).."' matched neither the custom spawn-point "..
         "pool nor a live ARMY_XX Spawn marker -- skipped, no transform written.")
    return nil
end

local function ApplyScenario(scenario, total, slotPattern)
    if scenario.spawnIds then
        local spawnTransforms = GameInfo.MapData.markers and GameInfo.MapData.markers.Spawn
            and GameInfo.MapData.markers.Spawn.transforms
        if spawnTransforms then
            for _, spawnId in ipairs(scenario.spawnIds) do
                local armyName, x, y, z = ResolveSpawnId(spawnId, spawnTransforms)
                if armyName and spawnTransforms[armyName] then    -- UNCHANGED guard: never creates a
                    spawnTransforms[armyName].position.x = x       -- missing transform, same as today
                    spawnTransforms[armyName].position.y = y
                    spawnTransforms[armyName].position.z = z
                end
            end
        end
    end
    -- ... alloy handling below is UNCHANGED by this amendment ...
```

**Binding details:**
- **Same final write target as today, different resolution path feeding it** — `spawnTransforms[armyName]`
  is still the only thing ever mutated.
- **The literal-`ARMY_XX`-fallback branch writes back the exact value it just read** — a harmless,
  deliberate no-op (`armyName == spawnId` in that branch). Not special-cased/skipped: one uniform code
  path for both branches is more legible than a conditional that avoids a no-op write with no
  measurable cost — this runs once per map load, not per-frame.
- **Performance basis (rough-estimate, not benchmarked):** `ResolveSpawnId`'s pool scan is O(N) over
  `SCENARIO_SPAWN_POINTS`, N bounded by how many custom spawn points a human hand-authors (tens, not
  thousands), run once per scenario match at map load — not a hot path, no index/hash structure
  justified. If pool sizes ever grow large enough to matter, a `spawnId -> row` lookup table built
  once at file-load time is a straightforward future optimization, not required by this ruling.
- **`Scenario.SpawnMatchedScenarioUnits`/`Scenario.SpawnUnits`/the alloy-application block below are
  UNCHANGED** — this ruling touches only the spawn-application block quoted above.

**Scope for implementation (pointer only, not a work order):** `Scenario_PARAMS.h`
(§15.12's shape), `MapExporter_Scenarios_IO.cpp`/`MapImporter_ScenarioRecord_IO.cpp` (§15.12's wire
legs and legacy-shape WARN-ONLY detection), `ScenarioScript_DataLua_IO.cpp` (the `SCENARIO_SPAWN_POINTS`
render and the `spawnIds` flat-string render), and `resources/lua/SanGenScenarioRuntime.lua` (this
section, in full — not left to coder discretion the way an ordinary implementation detail would be).
`ScenariosTab_Detail_UI.cpp`'s `DrawScenarioSpawnsList` needs a real redesign to editors over
`ScenarioBody::spawnIds` (string references) and `Scenarios::spawnPoints` (the pool itself) — UI
Expert's call, not specified further here.
