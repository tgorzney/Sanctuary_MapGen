# STEP72 — `ScenarioScript_RuntimeResource_IO`: the bundled Map Scenario runtime — resource, resolver, staging

> **⚠️ CORRECTION NOTE, 2026-08-28 (SanGen IO Architecture Expert).** This ticket originally specified
> a from-scratch build of three parts (Part 1: `resources/lua/SanGenScenarioRuntime.lua`, Part 2:
> `src/io/ScenarioScript_RuntimeResource_IO.h/.cpp`, Part 3: CMake staging). **All three have since
> been implemented and merged, verbatim as originally specified** — confirmed present on disk
> 2026-08-28 (dated 2026-08-22). This rewrite does not restart that work; it corrects what shipped,
> because the design Part 1's Lua content was ported from stopped being true on 2026-08-27: the live
> reference script (`Pandemonium Isthmus_Scenarios_Script.lua`) was rewritten that day and **deleted
> `Scenario.SpawnNavalFleets` and every `NAVAL_*` constant**, replacing them with a generic
> `spawnsUnits`-gated, name-keyed dispatch (`Scenario.SpawnMatchedScenarioUnits` /
> `Scenario.SpawnUnits`). `ARCH_15_05_ParamsScenariosType.md` §15.5 was amended 2026-08-28 to match —
> it retires `ScenarioNavalFleet`/`ScenarioNavalFleetEntry`/`ScenarioNavalPondSide`/
> `ScenarioNavalPondAssignment`/`ScenarioBody::navy` and records two OPEN questions this rewrite does
> not resolve (see "Blocking issue" below).
>
> **Net effect: Parts 2 and 3 need no change at all** — they are agnostic to the Lua content they
> stage/resolve. Only Part 1's Lua text, and one assertion block in its own test, are corrected here —
> and correction of Part 1 cannot reach a fully-working system, because the per-scenario dispatch half
> of the 2026-08-27 replacement has no ratified home under the three-file split. That half is stated
> as a blocker, not guessed at.
>
> **This ticket should NOT be retired.** It still owns real, bounded, actionable work (removing dead
> naval code, renaming `navy`→`spawnsUnits`, adding the generic executor, fixing a test that currently
> pins the dead function name). Retiring it would either lose that correction or force it into another
> ticket with a different declared scope. It stays open, now as a **correction** ticket against
> already-shipped code rather than a **build** ticket.

**Layer:** IO (+ the existing shipped-resource tree `resources/lua/`, already staged).
**Domain:** Map Scenario Lua-rendering leg, the generic runtime algorithm
(`ARCH_15_04_ThreeFileOnDiskShape.md` §15.4 point 2).
**Sequence:** Map Scenario IO track, WO6, **status: shipped, now under correction.** Not blocking
STEP71 for compilation (`ScenarioScript_Export_IO` is also already built and is Lua-content-agnostic —
confirmed: zero occurrences of `navy`/`spawnsUnits`/`navalFleet`/`SpawnNavalFleets` in
`ScenarioScript_Export_IO.cpp`). The correction below **is** blocking for any map that sets
`spawnsUnits = true` and expects units to actually spawn.

## 0. Ruling on the one open question — how the runtime locates its own map's data file

**Unaffected by this correction — still binding.**

**Ruling: `MapUtils.GetMapName()` self-discovery — not a new `_data.lua` argument.** The runtime file
(a byte-identical copy of `resources/lua/SanGenScenarioRuntime.lua`) calls it at its own top level —
during `_data.lua`'s `Import(...)` call — then builds
`Import("maps/" .. mapName .. "/" .. mapName .. "_Scenarios_Data.lua")` from it.
`Scenario.ResolveAndApply`'s signature is `ARCH_15_10_SlotPatternConstructionMoves.md` §15.10 ratified
law, and `MAP_SCENARIO_SPEC.md` §2.2's three-step `_data.lua` hand-edit adds no fifth argument.
Self-discovery needs zero further hand-edits and no new ratification.

⚠️ **Still not independently live-verified.** First place to look if a real map load throws
"File doesn't exist" on the sibling `Import()`. This correction does not touch that code path.

## 1. Root problem

The shipped `resources/lua/SanGenScenarioRuntime.lua` still contains, verbatim, the retired naval-fleet
machinery ported from the live reference **as it existed 2026-08-21** — before the 2026-08-27 rewrite
deleted it from the actual game file:

- `local currentNavalFleet = nil` and its bridging comment.
- The entire "Naval fleet spawning" section: `NAVAL_BATCH_SIZE`, `NAVAL_SPIRAL_STEP`,
  `NAVAL_SPIRAL_MAX_TRIES`, `NAVAL_GAP`, `NAVAL_DEFAULT_FOOTPRINT`, `NAVAL_GRID_CELL`,
  `NAVAL_GIVE_UP_AFTER_MISSES`, `NavalSpiralXZ`, `NavalNewGrid`/`NavalGridKey`/`NavalGridAdd`/
  `NavalGridHasNearby`, `NavalIsInsideArea`, `NavalFindSpot`, `NavalPlaceDeficitMarkers`, and
  `function Scenario.SpawnNavalFleets(area)` itself.
- `Scenario.ResolveAndApply` reads `matchedScenario.navy` (not `.spawnsUnits`), assigns
  `currentNavalFleet = matchedScenario.navalFleet`, and logs `navy=%s`.
- ⚠️ `Scenario.SpawnNavalFleets`'s own `army.lobbyOptions.isEmptySlot` check is **unguarded** — the
  exact live-confirmed bug `MAP_UNIT_SPAWNING_SPEC.md` §5 and `MAP_SCENARIO_SPEC.md` §11 document as
  having silently killed every unit spawn for an AI army (nil `lobbyOptions` throws, `pcall` swallows
  it, nothing spawns, no log reaches the console). **This bundled copy carries the bug forward.**

None of `ScenarioNavalFleet`/`navalFleet`/`navy`/`SpawnNavalFleets` has a reader left in the live,
in-game-confirmed reference. Continuing to bundle this is not neutral — it is dead code carrying a
known live bug, and it means the bundled runtime has **no** implementation of the executor half of the
replacement (`Scenario.SpawnUnits`), which any scenario following `MAP_SCENARIO_SPEC.md` §12's worked
example needs.

The runtime's own acceptance test additionally **pins the dead function name as a requirement**:
`ScenarioScript_RuntimeResource_IO_Test.cpp:149` asserts
`text.find("function Scenario.SpawnNavalFleets") != std::string::npos` — a currently-passing assertion
that would fail correction unless updated in the same change.

## 2. Fix — Part 1 only

### Confirmed unaffected, no action

- **Part 2**, `src/io/ScenarioScript_RuntimeResource_IO.h`/`.cpp` — pure bundled/override text
  resolution, zero Lua-content awareness. Re-read 2026-08-28: no reference to
  `navy`/`navalFleet`/`SpawnNavalFleets`. **No change.**
- **Part 3**, the CMake staging block (`CMakeLists.txt:378-399`, test registered at `:1005`) — stages
  whatever `resources/lua/*.lua` contains; content-agnostic. **No change.**

### Part 1 correction — `resources/lua/SanGenScenarioRuntime.lua`

**Keep exactly as shipped:** the banner comment block, `Scenario = {}`, the `MapUtils.GetMapName()`
self-discovery block and its ⚠️ comment (§0), the `PATTERN_SCENARIOS`/`COUNT_SCENARIOS`/
`DEFAULT_SCENARIO`/`MAX_ARMY_SLOT_COUNT` locals, the `ARMY_ID_TO_NAME`/`KNOWN_ALLOY_MARKERS` block and
its fail-loud `bAlloyRosterAvailable` guard, `IDENTITY_ROTATION`/`IDENTITY_SCALE`, `BuildSlotPattern`,
`EvaluateScenarioCondition`/`EvaluateScenarioConditions`, `FindMatchingScenario`, and `ApplyScenario`
in full — none of these read `navy` or `navalFleet`.

**DELETE in full:** the `local currentNavalFleet = nil` declaration and its comment, and the entire
block from the `-- Naval fleet spawning` section comment through the closing `end` of
`function Scenario.SpawnNavalFleets(area)` — every `NAVAL_*` constant, every `Naval*` helper, and the
function itself. Nothing after it depends on it except the final `return Scenario`, which is kept.

**REPLACE `Scenario.ResolveAndApply` with:**

```lua
-- Set inside ResolveAndApply, read by a future Scenario.SpawnMatchedScenarioUnits (NOT defined in
-- this file -- see the Blocking issue in STEP72; forward-compatible plumbing only, harmless with no
-- consumer, and does not itself decide where that consumer lives).
local currentMatchedScenarioName = nil

function Scenario.ResolveAndApply(total, humanCount, aiCount, playersInformation)
    local slotPattern = BuildSlotPattern(playersInformation, MAX_ARMY_SLOT_COUNT)
    local matchedScenario = FindMatchingScenario(total, humanCount, aiCount, slotPattern)
    local chosenArea, spawnsUnitsEnabled = matchedScenario.area, matchedScenario.spawnsUnits
    currentMatchedScenarioName = matchedScenario.name
    ApplyScenario(matchedScenario, total, slotPattern)

    Log(string.format(
        "SANGEN: %d occupied slot(s) (%d human, %d AI, pattern=%s) -> scenario=%s spawnsUnits=%s, playable area x=%d y=%d w=%d h=%d",
        total, humanCount, aiCount, slotPattern, tostring(matchedScenario.name),
        tostring(spawnsUnitsEnabled), chosenArea.x, chosenArea.y, chosenArea.width, chosenArea.height))

    return chosenArea, spawnsUnitsEnabled
end
```

The `matchedScenario.navy` → `matchedScenario.spawnsUnits` rename and the log-line rename are ported
verbatim from the live reference and `ARCH_15_05` §15.5's ratified rename. `currentMatchedScenarioName`
is ported verbatim as inert bridging state.

**ADD immediately before the final `return Scenario`** — the generic executor half of the replacement.
This part IS generic, IS identical across every map, and has no per-scenario knowledge, so it belongs
in this file with no open question attached:

```lua
-- ============================================================================
-- Generic unit-spawn executor (MAP_SCENARIO_SPEC.md §11). Replaces the retired naval-only
-- SpawnNavalFleets. Input: a flat array of {armyIndex, templateIdentifier, x, y, z}. Output: those
-- units exist. Knows NOTHING about water, terrain, navmesh, or unit type -- pure input -> output.
-- Never call CreateUnit outside this function.
--
-- Checks BOTH `ok` and `unit` -- pcall alone reports a falsy-but-non-throwing CreateUnit result as
-- success (MAP_UNIT_SPAWNING_SPEC.md §5).
-- ============================================================================
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

**`Scenario.SpawnMatchedScenarioUnits(area)` — the dispatcher that would call this — is deliberately
NOT added.** See below.

## 3. ⚠️ Blocking issue — `Scenario.SpawnMatchedScenarioUnits` has no ratified home

`ARCH_15_05_ParamsScenariosType.md`'s OPEN section (amended 2026-08-28), item 2:
`Scenario.SpawnMatchedScenarioUnits` and its per-scenario generator functions (e.g.
`BuildSlots5to8Instructions`) are **per-map, per-scenario, AND procedural Lua** — a category neither
ratified file role covers. `<MapName>_Scenarios_Runtime.lua` is defined as generic and byte-identical
across every map; `<MapName>_Scenarios_Data.lua` is defined as pure per-map data tables, never
algorithm code. A hand-authored per-scenario placement generator (deepest-water search, terrain
sampling, squadron layout) fits neither.

**Concrete consequence:** `_data.lua`'s orchestrator calls
`pcall(Scenario.SpawnMatchedScenarioUnits, chosenArea)` whenever `spawnsUnitsEnabled` is true. With
this correction applied, the bundled runtime defines `Scenario.SpawnUnits` but **not**
`Scenario.SpawnMatchedScenarioUnits` — that `pcall` calls a nil value, throws, and is **silently
swallowed** (errors inside the `NewThread` callback are dropped, and `Log`/`Warn` do not function in
this build). That is exactly the silent-wrong-result class Constitution §6 forbids, and it is a real
live-breakage risk for any exported map with a `spawnsUnits = true` scenario.

**This ticket stops here rather than inventing a home.** Candidates, none evaluated or chosen — do not
pick one without an ARCH ruling:

1. A fourth file, outside the ratified three-file split, for per-map procedural dispatch.
2. Loosening the Data file's "pure tables, never algorithm code" rule to allow bounded per-scenario Lua.
3. Keeping per-scenario dispatch/generator code permanently hand-authored beside `_data.lua`, i.e.
   accepting that this part stays outside SanGen's export surface entirely.

Each has different consequences for `ARCH_15_04`'s overwrite-safety mechanism and for whether
`Params::Scenarios` ever needs a declarative shape for unit-spawn placement (`ARCH_15_05` OPEN item 1,
also unresolved). **Action required: the ARCH Expert rules before any coder work-order adds
`Scenario.SpawnMatchedScenarioUnits` to the bundled runtime or renders per-scenario generators.**

## 4. ⚠️ Second cross-cutting dependency — the Data.lua renderer and PARAMS shape are also stale

Confirmed by reading the real files 2026-08-28: `src/params/Scenario_PARAMS.h` still declares
`bool navy = false;` and `ScenarioNavalFleet navalFleet;` on `ScenarioBody`, and
`src/io/ScenarioScript_DataLua_IO.cpp` (STEP70) still renders both — `AppendKeyValueLine(out,
indentLevel, "navy", RenderLuaBoolean(body.navy));` (line 127) and `AppendNavalFleetTable(out,
indentLevel, body.navalFleet);` (line 140), unconditionally, into every exported
`<MapName>_Scenarios_Data.lua`.

**The corrected runtime and the currently-shipping data renderer would then speak different
vocabularies**, independent of the blocker above: corrected `ResolveAndApply` reads
`matchedScenario.spawnsUnits`, but a real export sets `navy`/`navalFleet` and never `spawnsUnits` — so
every matched scenario resolves `spawnsUnitsEnabled = nil` (falsy), **silently disabling unit spawning
for every scenario**, including ones authored with intent to spawn.

`Scenario_PARAMS.h` and `ScenarioScript_DataLua_IO.cpp` need their own correction (retire
`navy`/`ScenarioNavalFleet` family, add `spawnsUnits`, per `ARCH_15_05`'s ratified shape), **landed
together with this one** — not by this ticket, which is scoped to the Runtime resource only.

## 5. Files touched

- **EDIT** `resources/lua/SanGenScenarioRuntime.lua` — per §2. Was authored as NEW by the original
  ticket; already exists, so this is now an edit.
- **EDIT** `src/io/ScenarioScript_RuntimeResource_IO_Test.cpp` — the self-check
  (`TestRealBundledResourceSelfCheck`, ~lines 147-150) currently asserts
  `text.find("function Scenario.SpawnNavalFleets") != std::string::npos`. See §7.
- **No change** to `src/io/ScenarioScript_RuntimeResource_IO.h`/`.cpp` or `CMakeLists.txt`.
- **NOT touched here, flagged as required siblings:** `src/params/Scenario_PARAMS.h`,
  `src/io/ScenarioScript_DataLua_IO.cpp` (§4).

## 6. Backend policy

Unchanged — N/A. A handful of `ifstream` reads, at most once per export attempt.

## 7. Acceptance test (delta from the shipped test)

Items 1-5, 7, 8 are pure C++ resolver behaviour, none Lua-content-dependent — unchanged. Item 6 needs:

1. Replace the `function Scenario.SpawnNavalFleets` positive assertion with
   `function Scenario.SpawnUnits`; keep `function Scenario.ResolveAndApply`.
2. Add negative assertions `text.find("SpawnNavalFleets") == std::string::npos` and
   `text.find("NAVAL_") == std::string::npos` — proves the dead code was removed, not merely supplemented.
3. Add positive `text.find("spawnsUnits") != std::string::npos` and negative
   `text.find("matchedScenario.navy") == std::string::npos` — pins the rename.
4. Item 7b (alloy-roster fail-loud guard) unaffected — keep verbatim.
5. Full solo rebuild + `ctest -C Debug`. No other test in the suite references naval content
   (confirmed by grep across `src/io/*_Test.cpp`).

## 8. Verify

- Corrected test passes with new positive and negative assertions.
- Full solo rebuild + `ctest -C Debug`: 100% pass.
- Grep confirms `resources/lua/SanGenScenarioRuntime.lua` has zero occurrences of
  `Naval`/`NAVAL_`/`navalFleet`/`navy` and exactly one `function Scenario.SpawnUnits`.
- ⚠️ **This correction alone does NOT make unit-spawning scenarios work end-to-end.** That needs the
  sibling `Scenario_PARAMS.h`/`ScenarioScript_DataLua_IO.cpp` correction (§4) in the same batch, and
  the ARCH ruling on `Scenario.SpawnMatchedScenarioUnits`'s home (§3).

## 9. ARCH rules invoked

- `ARCH_15_03_ExportOnlyLuaRatified.md` §15.3 — export-only; unaffected.
- `ARCH_15_04_ThreeFileOnDiskShape.md` §15.4 point 2 — the Runtime file role; its "generic, identical
  across every map" clause is exactly what §3's gap sits against.
- `ARCH_15_05_ParamsScenariosType.md` §15.5 — **the amended version**: the `spawnsUnits` rename, the
  RETIRED section, and the OPEN section (items 1 and 2) cited as the blocker.
- `ARCH_15_10_SlotPatternConstructionMoves.md` §15.10 — `BuildSlotPattern`'s ratified location and
  `MAX_ARMY_SLOT_COUNT`; unaffected.
- `MAP_SCENARIO_SPEC.md` §3/§3.1/§4/§5/§10/§11/§11.1 — execution/timing law, module API contract,
  `alloyMode` semantics, and the primary source for what replaced the naval machinery.
- `MAP_UNIT_SPAWNING_SPEC.md` §5 — the `ok`/`unit` double-check and the never-hardcode-an-army-index
  rule, both present in the executor being added.
- Constitution §6 — fail-closed on unknown condition field/comparator; §3 above applies the same
  loud-not-silent rule to a gap this ticket refuses to paper over.

## 10. Explicit out-of-scope

- **Adding `Scenario.SpawnMatchedScenarioUnits` or any per-scenario generator, in any form, including
  a no-op stub.** Blocked (§3). A no-op stub was considered and rejected: it still invents a home
  (silence over throw) and masks the gap instead of surfacing it.
- **`Scenario_PARAMS.h`'s shape** — ARCH Expert's call, already made in `ARCH_15_05`.
- **`ScenarioScript_DataLua_IO.cpp`'s renderer (STEP70)** — required sibling correction, not performed
  here.
- **`ScenarioScript_Export_IO` (STEP71)** — confirmed unaffected, already shipped.
- **Rendering `ARMY_ID_TO_NAME`/`KNOWN_ALLOY_MARKERS`** — separate open gap, STEP70's domain.
- **`GameInstallLocation_IO`, `Io::AppSettings`, `Sys::CheckLuaSyntax`** — STEP64/65, unaffected.
- **`LuaCodeEditor_UI` and any UI wiring** — WO8, UI Expert.

## 11. ❓ Open questions

1. `MapUtils.GetMapName()`'s exact return spelling / early-call-site safety — unaffected, still open.
2. **Where `Scenario.SpawnMatchedScenarioUnits` and per-scenario generator code live under the
   three-file split** — `ARCH_15_05` OPEN item 2. Blocking; needs an ARCH ruling.
3. Whether per-scenario unit-spawn placement should ever become author-able `Params::Scenarios` data
   at all — `ARCH_15_05` OPEN item 1, unresolved.

## 12. ⚠️ Problems / gaps flagged, not solved here

1. **`ARMY_ID_TO_NAME`/`KNOWN_ALLOY_MARKERS` still have no ratified renderer.** Predates and is
   independent of the naval retirement; still blocks live `explicit`/`occupancy` alloyMode on a real map.
2. **`Scenario_PARAMS.h` / `ScenarioScript_DataLua_IO.cpp` are stale relative to `ARCH_15_05`'s
   amendment** (§4) — needs its own correction ticket, must land alongside this one.
3. **The blocking issue itself** (§3) — the biggest open item in this ticket.
4. **`.claude/agents/sangen-coder.md` briefing check, 2026-08-28:** re-read; its
   `MAP_UNIT_SPAWNING_SPEC.md` bullet is current and needs no edit — it does not mention
   `navy`/`navalFleet`/`SpawnNavalFleets`. No briefing change required.
