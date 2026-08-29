# REFERENCE — unit-spawning recipe (Sanctuary engine, per-map Lua)

> # 🕮 HISTORICAL — SUPERSEDED 2026-08-28 by `sangen_arch_pack/specs/MAP_UNIT_SPAWNING_SPEC.md`
>
> **Do not use this file as the recipe. Use `MAP_UNIT_SPAWNING_SPEC.md`.** That spec is the
> authoritative, source-cited mechanism (load/execution chain, the double-execution hazard,
> `Import()` semantics, the one-`NewThread` rule, the `CreateUnit` contract, position validation,
> diagnostics, known-good `tpId`s), and `MAP_SCENARIO_SPEC.md` §15 names it as such.
>
> **This file is kept as the investigation record** — the evidence-tiering discipline below
> ([OBSERVED]/[CLAIMED]/[UNVERIFIED]) is the part still worth reading, and §7's diff table records
> what the `.bak` did that current code does not. It is **not** being edited to track the spec;
> where the two disagree, the spec wins.
>
> **Known contradictions with the spec, so nobody acts on the stale version:**
> - **§2 / §2a / §7's "army index from `pairs(Armies)` is the prime suspect" is resolved and the
>   stated mechanism was wrong.** `MAP_UNIT_SPAWNING_SPEC.md` §5: `CreateArmies` builds an army for
>   **every** map slot, not only filled ones — so `Armies[1]` *does* exist in a slots-5-and-6
>   lobby, and a hardcoded `CreateUnit(1, ...)` does not fail; it hands the units to an **unowned,
>   empty-slot army**. The rule (never hardcode an index; use `pairs(Armies)` keys and skip empty
>   slots) survives; §2a's explanation of *why* does not, and §7's "⚠️ unproven — prime suspect"
>   row is retracted.
> - **§4/§7's "only ONE `NewThread` per script is honored" is downgraded.** Spec §4: no mechanism
>   in `threads.lua` drops a second registration; treat it as a safe convention with an
>   unidentified cause, not an explained rule.
> - **§2a item 4's unexplained doubled BigBots is explained.** Spec §2: `<map>_data.lua` executes
>   **twice per host state** (two callers spell the `Import` path differently, so the cache misses)
>   — confirmed live with a run-counter probe.
> - **§1's `ok AND unit` check stands, but its rationale is corrected.** Spec §5: there is no path
>   in `unitsUtilities.lua` that returns a falsy value — the function throws or returns a unit.
>   Keep the check; drop the "silent falsy return is a known failure mode" claim.
> - **The naval vocabulary throughout (§3, §4, §6, §7) is historical.**
>   `Scenario.SpawnNavalFleets` and every `NAVAL_*` constant were deleted by the 2026-08-27
>   rewrite; `ARCH_15_05_ParamsScenariosType.md` retires the matching PARAMS family. Where this
>   file discusses naval-vs-land placement it is describing the `.bak`'s algorithm, not any
>   current code path.

> ## ⚠️ READ THIS FIRST — CORRECTED 2026-08-28. The evidence base is much weaker than the first
> ## draft of this file claimed.
>
> The first draft labelled everything from `Pandemonium Isthmus_SpawnArmies.lua.bak` as
> **"VERIFIED"** and called it *"the spawner that provably placed units in-game."*
> **That was wrong and has been retracted.** The basis was two *comments*: the live scenario file
> describing the `.bak` as a "confirmed-working reference," and the `.bak`'s own header describing
> its own fixes. **A comment is an author's claim, not a result.** Nobody in this session ever ran
> that file, and a `.bak` is by definition the version that was replaced. **We do not know whether
> it worked.**
>
> This is the same failure mode as the earlier "2 BigBots confirm cross-tree `Import()`" false
> positive: an assertion treated as evidence.
>
> **Everything below is therefore reclassified into three honest tiers:**
> - **[OBSERVED]** — someone watched it happen in a live match this session. Very little qualifies.
> - **[CLAIMED]** — asserted in a comment or by a prior session, never independently confirmed.
>   Useful as a lead. Not a foundation.
> - **[UNVERIFIED]** — current code that has never been proven to work.
>
> ⚠️ **The one thing genuinely [OBSERVED] cuts AGAINST this file's original conclusion.** BigBots
> did appear on the map at least once this session, and that code obtained its army index from
> `pairs(Armies)` — the exact thing the first draft named as the prime suspect. So "the army index
> from `pairs(Armies)` is wrong" is **not** supported by our own evidence. It remains possible, but
> it is a hypothesis, not a finding.
>
> ⚠️ **Also unresolved:** that test had two spawn paths targeting one army and produced **two**
> BigBots, but the import-gated path could not have worked (the `local Scenario` capture bug). The
> arithmetic does not close. Do not build on that observation without re-deriving it.

---

## 1. The minimum call shape (consistent across every spawner seen, incl. the one [OBSERVED] BigBot spawn)

```lua
local ARMY = 1                                   -- ⚠️ a HARDCODED LITERAL, see §2
local pos = EngineClasses.float3(x, h, z)
local ok, unit = pcall(CreateUnit, ARMY, tpId, pos)
if ok and unit then
    -- placed
end
```

**[CLAIMED] properties — from the `.bak`, never independently run:**
- `CreateUnit(armyIndex, tpId, float3)` — three args, position is `EngineClasses.float3`.
- **Both `ok` AND `unit` must be checked.** The working file's own note: *"CreateUnit's actual
  return value is now checked (not just whether pcall raised an error), so silent failures are
  counted instead of assumed to have worked."* `pcall` only catches a **thrown** error — `CreateUnit`
  can return normally with a falsy result (invalid/occupied/unreachable spot) and `pcall` reports
  that as success.
- Structures additionally pass through `RoundIntoGrid(pos)` before the call.

## 2a. RESOLVED 2026-08-28 — a hardcoded army index is fatal in slots 5-8 [OBSERVED]

**Evidence.** A diagnostic probe using `CreateUnit(1, ...)` was run twice by the human:
- lobby with players in **slots 1 and 2** -> BigBots appeared.
- lobby with players in **slots 5 and 6** -> **nothing appeared at all.**

Same code, same build, same map, back to back. The only difference is which slots were filled.
With players in slots 5-8, **ARMY_01 has no player, so army 1 does not exist**, and every
`CreateUnit(1, ...)` fails silently.

**Consequences, in order of importance:**
1. `SpawnArmies.lua.bak`'s `local ARMY = 1` **cannot work for a slots-5-to-8 lobby.** Whatever it
   did historically, it was only ever exercised on lobbies that fill slot 1. This is now a concrete
   reason not to treat that file as a working reference.
2. It is a general rule, not a quirk of one file: **never hardcode an army index.** A literal that
   works is only evidence about the lobby it was tested in.
3. It cuts the other way for `pairs(Armies)` keys — those track the real, occupied armies and are
   the correct source. §2 below treated the literal as the safer of the two. That was backwards.
4. ⚠️ Unexplained, do not build on it: the slots-1-and-2 run produced **two** BigBots from a probe
   that calls `CreateUnit` exactly **once**. Either the map script executes twice (host and client
   both reaching it) or something else is also spawning. The 2026-08-17 "2 BigBots in a 2-army
   test" result was read as one-per-army; that reading is no longer safe either, since a
   single-call probe produced the same count. Re-derive before relying on either.

## 2. THE ARMY INDEX — a hypothesis, NOT a finding

**[CLAIMED]:** the `.bak` spawner used `local ARMY = 1` — a hardcoded literal. It **never iterated
`Armies`** and **never read `lobbyOptions`**. Every unit went to army 1.

**UNVERIFIED:** the current scenario spawner does:
```lua
for armyIndex, army in pairs(Armies) do
    ...
    pcall(CreateUnit, armyIndex, tpId, pos)   -- armyIndex is the TABLE KEY of Armies
```
⚠️ **Correction:** an earlier draft called this "the single most likely cause." That overstated it.
`pairs(Armies)` keys have NOT been proven correct — but they have also not been disproven, and the
one [OBSERVED] BigBot spawn this session used exactly this pattern and DID place units. Treat the
literal-vs-key question as OPEN. It might
be 0-based, might be a different numbering, might not correspond in some lobby configurations. It is one
possible contributor to "units silently do not spawn," alongside coordinate validity (§3).

**Before trusting `pairs(Armies)` keys, prove them.** The cheapest proof: spawn one unit with the
literal `1` alongside one using the loop's `armyIndex`, at two visibly separated positions. If the
literal appears and the loop one does not, the key is the bug.

Related, separately confirmed: `army.lobbyOptions` is **nil on at least some army entries** (an AI
army is the confirmed case). An unguarded `army.lobbyOptions.isEmptySlot` **throws**. Always guard:
```lua
local bIsEmptySlot = army.lobbyOptions and army.lobbyOptions.isEmptySlot
```
nil `lobbyOptions` ⇒ treat as **OCCUPIED** (an AI slot IS filled).

## 3. POSITION VALIDATION — the `.bak` did a lot; current code does none

**[CLAIMED]:** the `.bak` spawner never trusted a coordinate. Every unit went through
`FindValidSpot(idealX, idealZ, sizeX, sizeZ, needsWater)`, which checked:
- **Terrain flatness** — `Engine.SampleTerrainNormals`, max **9.0 degrees**, explicitly *"matches
  script.lua's own InitializePlacementGrid"* (the same test the game itself uses).
- **Terrain height** — `Engine.SampleTerrainHeightFromCell`.
- **Water** — `Engine.GetWaterLevel()` / `Engine.HasWater()`. **Only NAVAL units (not amphibious)
  are required to be in water; everything else is placed on dry land.** Naval/amphibious identified
  via the game's own `Tags` table (`Tags.NAVAL[tpId]`, `Tags.AMPHIBIOUS[tpId]`).
- **Occupancy** — a spatial grid of already-placed units and real map props
  (`GameInfo.MapData.props`), with per-prop-family buffer radii.
- **Spiral fallback** — if the ideal spot fails, search outward: `SPIRAL_STEP = 2.0` world units per
  ring, `SPIRAL_MAX_FLAT_TRIES = 300` rings while still requiring flatness, then
  `SPIRAL_MAX_TRIES = 700` once flatness is dropped.
- **Real unit size** — `__Templates.Units[tpId].skirtSize` (structures) or `.footprint`,
  `GAP = 1.5` world units between adjacent footprints. `DEFAULT_FOOTPRINT = {x=4, y=4}` only if a
  blueprint is missing size data.

**UNVERIFIED / current code:** the scenario spawner emits a fixed rectangular grid at hardcoded
pond coordinates with **no flatness check, no water check, no occupancy check, and no spiral
fallback**. If a coordinate is invalid, `CreateUnit` returns falsy, the unit is counted as failed,
and nothing is visible. A grid-spacing analysis (`scratchpad/verify_full_grids.py`) claimed 1/80
ships land on dry land at spacing 8 — but that is a script's opinion about a heightmap, **not an
in-game verification**.

## 4. FAILURE VISIBILITY — the practice worth copying regardless of provenance

**[CLAIMED] — but the practice is sound regardless of whether that file ran:**

> *"Diagnostic: after placement, any unit that still failed to place spawns a small marker
> (tpId `uel1001`) near (750, 900), grouped in 3 rows (structures / other / naval), capped at 60 per
> row. That area empty = everything placed. Markers there = that many are still missing in that
> category."*

```lua
SpawnDeficitMarkers(#structureEntries - structCount, 0)
SpawnDeficitMarkers(#otherEntries    - otherCount,   20)
SpawnDeficitMarkers(#navalEntries    - navalCount,   40)
```

**Why this matters more than it looks:** the F1 in-game console is **unreliable in this build** —
it has shown nothing while confirmed-running code executed, and `game_logs/*.txt` stayed empty
across a whole session. `Log()`/`Warn()` are therefore **not a dependable diagnostic**. Spawning
countable objects on the map is the only signal channel proven to work.

**Current code has no such diagnostic** — `SpawnUnits` logs `placed`/`failed` counts to `Log()`,
i.e. into the unreliable channel. A total failure and a total success look identical in-game.

## 5. Ordering / invocation

**[CLAIMED]:** the `.bak` exposed a global `SpawnArmies()` and was *"Called by
`Pandemonium Isthmus_script.lua`"* — a per-map script file that **no longer exists** in the map
folder. Its invocation path is gone; only the algorithm survives in the `.bak`.

**Current path (structurally sound, independently confirmed):**
`_data.lua`'s single `NewThread` → `Scenario.SpawnMatchedScenarioUnits(area)`.
- `Armies` is **not** populated during `LoadMapData` — unit spawning must be deferred to
  `NewThread`, which fires after `RunMapSetup` on tick 0.
- **Only ONE `NewThread()` per script is honored** — a second call silently never runs. All
  deferred work must live in one callback.
- ⚠️ Units spawned **outside the active playable area are culled** — models *and* strategic icons.
  When using units as a diagnostic signal, always spawn inside the active area.

## 6. tpIds — mixed evidence

| tpId | Unit |
|---|---|
| `ucl4004` | Chosen T4 BigBot — confirmed spawned live this session |
| `ucn1001` | Chosen T1 Frigate — confirmed in the working naval code |
| `ucn3001` | Chosen T3 Dreadnought Battleship — confirmed in the working naval code |
| `uga1201` | Guard T1 Aerofoil AA Fighter — confirmed playable |
| `uel1001` | EDA T1 — used as the deficit/diagnostic marker unit |
| `uga3201` | Guard T3 Contrail — ⚠️ **NOT playable** (`BONE_MISSMATCH` in `UNITS_STATUS.md`) |

Highest validated tier is **T4**, every faction. No T5 exists in this build.

## 7. The diff that matters right now

Current `slots5to8AnyFilled` spawner vs. the verified recipe:

| Aspect | `.bak` [CLAIMED] | Current [UNVERIFIED] | Risk |
|---|---|---|---|
| Army index | literal `1` | `pairs(Armies)` key | ⚠️ **unproven — prime suspect** |
| `lobbyOptions` | never read | read (now guarded) | fixed 2026-08-28 |
| Flatness check | 9° via `SampleTerrainNormals` | none | ships/fighters on invalid ground |
| Water check | naval-only, real water level | fixed `y = 78.0` | battleships may not be in water |
| Occupancy check | spatial grid vs. units + props | none | spawning into occupied cells |
| Spiral fallback | 300 flat + 700 total rings | none | one bad coord = permanent loss |
| Failure signal | **marker units on the map** | `Log()` to unreliable F1 | **failures are invisible** |

## 8. Rules to follow when writing any new spawner

1. **Never assume an army index.** Prove it against the literal that worked (`1`) before trusting
   `pairs(Armies)` keys.
2. **Always guard `army.lobbyOptions`.**
3. **Always check `ok AND unit`** — never `pcall` alone.
4. **Never trust a hardcoded coordinate.** Validate ground/water/occupancy, or accept that failures
   will be silent and invisible.
5. **Always emit a visible on-map failure signal.** `Log()` is not a diagnostic in this build.
6. **One `NewThread` per script**, and spawn inside the active playable area.
7. **Test with an AI player present**, not only all-human — the `lobbyOptions` bug hid for a long
   time precisely because the unit-spawn opt-in (`spawnsUnits`; the retired `navy` at the time)
   was only ever true on an all-human composition.
