# MAP_UNIT_SPAWNING_SPEC — how to spawn units from a per-map Lua script

**Status:** mechanism established from engine source + live tests, 2026-08-28. Every claim below
cites either a source line or a specific in-game observation. Anything unverified is marked ⚠️.

This exists because the same mechanism was worked out once before, recorded only in
`session_findings_2026-08-17_unit_spawning.md`, never propagated, and then re-derived over three
days. Do not let that happen again — see §8.

---

## 1. The load/execution chain (host)

`script.lua` `init(libPath, isClient)` sets **`IsHost` XOR `IsClient`** — one per Lua state
(`script.lua:7-12`). Neither is ever reset to false.

`InitLobby(lobbyData, argString)`:

| Step | Source | What it does |
|---|---|---|
| 1 | `script.lua:156` | `mapUtils.LoadMapData(mapPath)` — loads `.sanmap`, then `Import`s `<map>_data.lua`; that chunk's top-level code runs **synchronously here** |
| 2 | `script.lua:160` | `gameUtils.CreateArmies()` — **`Armies` is fully populated at the end of this step** |
| 3 | end of `CreateArmies` | `SpawnInitialUnits()` — one commander per non-empty army, `CreateUnit(armyIndex, Factions.FactionsData[army.faction].initialUnit, startingPos)` |
| 4 | `script.lua:186` | `hostMain.Start()` → tick 0 → `RunMapSetup(true)` (props/decals/alloys) → **then** dispatches queued `NewThread` callbacks |

**Consequence:** `Armies` is EMPTY while `_data.lua`'s top-level code runs. Unit spawning must be
deferred into a `NewThread` callback, which fires after step 2.

The client state runs its own `LoadMapData` at `script.lua:189`. It loads map data for local
needs; it is not authoritative for simulation. **⚠️ Note (2026-08-29):** this per-state
`LoadMapData` call is also what makes a shared per-map `NewThread` body run once per Lua state in a
solo/listen-server match — a distinct phenomenon from §2's within-a-state cache-miss hazard below,
not a restatement of it. Full derivation: `NAVMAP_MODIFIER_BLOCKER_SPEC.md` §5.

## 2. ⚠️ The chunk executes TWICE per host state — design for it

`Import` caches in `package.loaded` keyed by the **literal string** passed in
(`common/systems/import.lua:128`). Two host-side callers spell the same file differently:

```
common/mapUtils.lua:47   LoadMapData()          ->  Import("maps/X/X_data.lua")
host/testUtils.lua:364   LoadDebugUnitGroups()  ->  Import("/maps/X/X_data.lua")
                         (reached from host/hostMain.lua:23)
```

Different keys ⇒ cache miss ⇒ **the entire chunk re-executes in the same Lua state**. Both callers
are host-side, so `IsHost` is true both times.

**Confirmed live 2026-08-28:** 300 fighters and 40 ships against the 150 and 20 configured.
A run-counter probe returned 8 markers all stamped "run 2", proving one state, two executions.
Commanders are never doubled because `CreateArmies` is not in this chunk — which is exactly what
made the symptom look selective and cost days.

**Required — scope side effects to the caller that owns them.** Not a run-once counter: the
debug loader wants `MapData.groups` for a UI, it is not starting a match. `import.lua:177` records
the path each file was loaded under, so the file can tell:

```lua
local loadPath = ImportedFileInfo and ImportedFileInfo.FileName or ""
if loadPath ~= string.format("maps/%s/%s_data.lua",
        GameInfo.MapInfo.dataName, GameInfo.MapInfo.dataName) then
    return {}
end
```

The debug loader still imports successfully and still reads any data globals; it simply does not
trigger match setup. This stays correct if the engine ever normalizes its cache key.

⚠️ **Do not conflate this hazard with the separate per-Lua-state execution nuance** (§1's note
above; `NAVMAP_MODIFIER_BLOCKER_SPEC.md` §5) — this section is about one **state** re-running the
chunk twice via an `Import`-cache-key miss; that section is about a solo/listen-server match running
the chunk once per **state** (host and client), a different axis entirely. Both are real and can
co-occur. Confirmed misdiagnosed as the same bug, live, twice, before being correctly separated.

## 2a. Why match logic lives in a data file at all

**There is no per-map script hook in this engine build.** `<map>_script.lua` — named by
`Pandemonium Isthmus_SpawnArmies.lua.bak` as its own caller — no longer exists, and the only
surviving per-map script (`maps/showcase_script.lua`) is invoked by hand from a debug command
(`host/testUtils.lua:544`), never automatically. `<map>_data.lua` is the **only** automatic per-map
Lua entry point left.

This is very likely why the double-execution is a new problem: when this worked, logic lived in a
real script file loaded once. The hook was deprecated, the logic moved into `_data.lua`, and
`_data.lua` has two loaders.

⚠️ **`<map>_data.lua` is engine-writable — it can be overwritten wholesale.**
`host/testUtils.lua:365` sets `DebugUnitGroupsFilePath = libPath.."/maps/X/X_data.lua"`, and
`:354` does `table.save(DebugUnitGroups, DebugUnitGroupsFilePath, "MapData = ")`. So saving debug
unit groups rewrites THIS file as machine-generated `MapData = { ... }`, destroying every line of
hand-written code in it. It does **not** write to a separate `_data_debug.lua`; the
`*_data_debug.lua` files beside the stock survival maps are unrelated shipped files.

**Keep backups of this file.** If a per-map script hook is ever restored, move all logic out of it.
This is the strongest argument that hosting match logic here is a stopgap, not the intended design.

## 3. `Import()` semantics — not `require`

- **`return` is ignored.** `Import` runs the file in a fresh environment table and returns that
  table. To expose an API, assign a **global**: `Scenario = {}` then `Import(path).Scenario`.
- Each file gets its **own env** with `__index = _G`. A global set in file A is **not** visible to
  file B. Two files can only share state through an object both hold (e.g. the `Scenario` table).
- Cached per Lua state, keyed by literal path string (§2).
- ⚠️ **Cross-tree `Import` is impossible** — paths cannot address outside the `LJ/lua` root.
  A map's own asset folder (`Sanctuary_Data/Maps/<name>/`) is unreachable; disproved via
  `Engine.FileExists` returning false.

## 4. Use ONE `NewThread` per script

⚠️ **Field observation, NOT source-derived — corrected 2026-08-28.** A second `NewThread` in the
same file was observed not to run (`session_findings_2026-08-17` §3, and again this session). But
`threads.lua` contains **no mechanism that drops a second registration**: `ProcessThreads` iterates
every entry in `allThreads[Tick]` and should resume both. The nearest real hazard visible in source
is that calling `NewThread` from *inside* a running thread mutates the table being traversed by
`next`, which is undefined in Lua — that does not explain two top-level calls.

Treat "one `NewThread` per script" as a safe convention, not an explained rule. The underlying
cause is unidentified and worth re-testing before anyone relies on it either way.

⚠️ Errors inside a `NewThread` callback are **swallowed** by `threads.lua`'s `ResumeThread`, and
`Log()`/`Warn()` go to the F1 console, **which does not function in this build** — `game_logs/*.txt`
stays empty too. A throw therefore cancels everything after it with no trace anywhere.

**Therefore ordering inside the thread is load-bearing.** Put nothing ahead of work that must
happen, and `pcall` anything that might throw:

```lua
NewThread(function()
    if not chosenArea then return end
    if IsHost then ...SetPlayableArea... end          -- area first
    if IsHost and spawnsUnitsEnabled then             -- units next
        pcall(Scenario.SpawnMatchedScenarioUnits, chosenArea)
    end
    pcall(SpawnAirBlockerNavmapModifiers)             -- prefab work last, isolated
end)
```

Real regressions caused by getting this wrong, both on 2026-08-28: the air blocker placed ahead of
the unit spawn silently killed every unit when it threw; and placed ahead of `SetPlayableArea` it
risked its prefabs being culled by the throwaway 1×1 area.

⚠️ **Note (2026-08-29): the snippet above is illustrative, not this map's current shape.**
`NAVMAP_MODIFIER_BLOCKER_SPEC.md` §6 records a session where this exact pattern (ordering, each
call `pcall`'d) was individually correct at every step and still did not fix a real bug — the
actual cause was an `Import`-omission elsewhere, not an ordering defect. That spec's §6.1 states
the corollary this section's own "`pcall` anything that might throw" line already implies but does
not spell out: ordering only reduces the *chance* an earlier failure cancels a later call; `pcall`
on the call itself is what removes the *consequence* if that call throws for any reason, ordering
or not. `NAVMAP_MODIFIER_BLOCKER_SPEC.md` §6 also records that Pandemonium Isthmus's own live
`_data.lua` today places blocker-prefab work **before**, not after, the scenario unit spawn shown
above — this section's snippet remains valid as an illustration of the general rule, not as a claim
about that file's current call order.

## 5. The `CreateUnit` call

```lua
local ok, unit = pcall(CreateUnit, armyIndex, templateIdentifier, EngineClasses.float3(x, y, z))
if ok and unit then --[[ placed ]] end
```

- **`pcall` it.** On engine failure `CreateUnit` calls `Error()` (`host/units/unitsUtilities.lua:32`),
  which throws — so `pcall` is required. ⚠️ The additional `unit` check is cheap insurance but its
  stated rationale was wrong: reading `unitsUtilities.lua`, there is **no path that returns a falsy
  value** — the function either throws or returns a unit object. Keep the check; do not rely on the
  old claim that a silent falsy return is a known failure mode.
- **`position.y == 0` snaps to terrain.** `CreateUnit` calls `SnapToGround(position)` when y is
  exactly 0 (`unitsUtilities.lua:23-25`). Pass 0 to place on the ground; pass a real y to override.
- **Never hardcode an army index.** ⚠️ Mechanism corrected 2026-08-28 — the earlier explanation in
  this spec was wrong. `CreateArmies` builds an army for **every map slot**, sorted, not just the
  filled ones (`common/gameUtils.lua`, `armySetup` from `GameInfo.MapData.armies`). So `Armies[1]`
  **does exist** in a slots-5-and-6 lobby. A hardcoded `CreateUnit(1, ...)` therefore does not fail —
  it hands the units to an **unowned, empty-slot army**, which is why nothing usable appeared.
  Observed live: hardcoded army 1 produced visible units in a slots-1-and-2 lobby and nothing
  usable in a slots-5-and-6 lobby. Use `pairs(Armies)` keys and skip empty slots.
- **Guard `army.lobbyOptions`** — nil on some entries; `army.lobbyOptions and army.lobbyOptions.isEmptySlot`.
  nil ⇒ treat as OCCUPIED (an AI slot is filled).
- Army names are `ARMY_01`…`ARMY_16`, zero-padded — the engine assigns lobby slots by
  **alphabetical army-name sort**, so the padding is load-bearing.
- Marker positions from `GameInfo.MapData.markers.Spawn.transforms[army.name]`; pass
  `.position` straight through (confirmed live 2026-08-17).
- `WaitTicks(1)` every ~100 units to avoid hitching a tick.
- ⚠️ Units outside the active playable area are culled — models and strategic icons.

## 6. Position validation

**Never substitute a known-bad position for a failure.** The defect that hid battleships for days:
a spiral water search whose fallback was `return idealX, idealZ` — a point it had just proven was
dry. `CreateUnit` then failed silently.

A search that finds nothing must `return nil`, and the caller must count the miss. The shipped
`NavalFindSpot` (in `_data.lua.backup-2026-08-20-presplit`) got this right — a bare `return nil`.

Do not hardcode coordinates. Derive from the army's own spawn marker and validate against live
terrain (`Engine.SampleTerrainHeightFromCell`, `Engine.GetWaterLevel`, `Engine.HasWater`).

## 7. Diagnostics — units are the only working signal

`Log()`/`Warn()` are **not** diagnostics in this build. The only channel proven to work is a unit
appearing on the map. When instrumenting:

- **Make the readout countable, not spatial.** Asking a human to distinguish x=1000 from x=1040 on
  a 2048-wide map does not work; it produced an unusable result. Encode progress as *how many* units
  appear in one row, or as *which unit type*.
- **Wrap the whole probe in `pcall`.** A probe that throws kills the thread it is measuring — this
  happened and took the air blocker down with it.
- **A probe cannot report a failure that prevents the probe from running.** To measure whether a
  thread is registered at all, register the thread first and have it report a counter the
  synchronous code advanced.
- Spawn inside the active playable area or it will be culled.

## 8. Propagation requirement

This spec supersedes the scattered notes. When any of it changes, update **here first**, then:
`.claude/agents/sangen-coder.md`, `sangen-format-expert.md`, `sangen-generator-expert.md`.
`session_findings_2026-08-17_unit_spawning.md` was written to be folded into the charters and never
was — that omission is why this was re-derived from scratch. Do not repeat it.

## 9. Known-good tpIds

| tpId | Unit | Note |
|---|---|---|
| `ucl4004` | Chosen T4 BigBot | spawn confirmed live |
| `ugl4001` | Guard T4 Bot | `true`/OK |
| `uel4001` | EDA T4 Railgun Sniper | `true`/OK |
| `uga1201` | Guard T1 AA Fighter | `true`/OK |
| `ucn3001` | Chosen T3 Battleship | `false`/OK_PENDING_APPROVAL — but `availableUnits.lua` gates only AI + build menu, **not `CreateUnit`**; max weapon range 140, footprint 2.5×9.5 |
| `uga3201` | Guard T3 Contrail | ⚠️ NOT playable (`BONE_MISSMATCH`) |

Highest validated tier is T4, every faction. No T5 exists in this build.

## 10. Related law

- `NAVMAP_MODIFIER_BLOCKER_SPEC.md` §5/§6/§6.1 — the per-Lua-state execution nuance (distinct from
  §2 above), the ordering law extension for navmap-modifier blocker work sharing this same
  `NewThread`, and the pcall-vs-ordering correction recorded live on Pandemonium Isthmus.
