# DESIGN — Slot-Range Composition Condition for the Scenario System (R1)

*Analysis consult, authored at human request 2026-08-27, in response to: "Add a Scenario to
Pandemonium Isthmus.sanmap where if any slot 5-8 are filled, the play area will be full map
size." NOTHING in this document is ratified. No PARAMS/ARCH/IO type is invented here, only
flagged, per Constitution §7 / `ARCH_08_04_CoderScopeLaw.md` §8.4. No code, `.sanmap`, or `.lua`
file has been touched to produce this — read-only research only, per the human's explicit
request to explain the design before any modification.*

Grounded in `MAP_SCENARIO_SPEC.md`, `Scenario_PARAMS.h`, `ScenarioScript_DataLua_IO.cpp`,
`resources/lua/SanGenScenarioRuntime.lua`, and a live read of the target file:
`E:\...\Sanctuary_Data\Maps\Pandemonium Isthmus\Pandemonium Isthmus.sanmap`.

## 1. How the Scenario system works today

A map's playable area (and per-army spawns/alloys/navy) is resolved once, at map load, by
`Scenario.ResolveAndApply(total, humanCount, aiCount, playersInformation)` — three files per map,
colocated in the game's script tree (`LJ/lua/maps/<MapName>/`), **not** inside the `.sanmap`
asset package:

| File | Content | Owner |
|---|---|---|
| `<MapName>_data.lua` | Orchestrator: reads the lobby, calls `Scenario.ResolveAndApply`, wires the returned area/navy flag into the map. | Hand-authored, never touched by SanGen. |
| `<MapName>_Scenarios_Runtime.lua` | Generic algorithm — byte-identical across every map, copied from `resources/lua/SanGenScenarioRuntime.lua` on every SanGen export. | SanGen-owned, regenerated every export. |
| `<MapName>_Scenarios_Data.lua` | Per-map scenario tables (`PATTERN_SCENARIOS`/`COUNT_SCENARIOS`/`DEFAULT_SCENARIO`/`MAX_ARMY_SLOT_COUNT`), rendered from `Params::Scenarios`. | SanGen-owned, regenerated every export. |

Resolution is a **three-tier** match, first-hit-wins across tiers, checked in this order:

1. **TIER 1 — `PATTERN_SCENARIOS`.** Exact string equality against `slotPattern` (one char per
   army slot, 1..`maxArmySlotCount`, `"h"`/`"A"`/`"-"`). No wildcards.
2. **TIER 2 — `COUNT_SCENARIOS`.** An ordered array; first match wins. Each entry's `conditions`
   are a flat, AND-only list of `{field, comparator, value}` triples
   (`Params::ScenarioCountCondition`, `Scenario_PARAMS.h:74-88`).
3. **TIER 3 — `DEFAULT_SCENARIO`.** Always matches, mandatory singleton fallback.

Each matched record carries `area` (world-space rect — this is the "play area"), `navy`,
`alloyMode`, and (per §6's hard requirement) should carry explicit `spawns` for any composition
that needs deterministic spawn points.

## 2. Why "any of slots 5-8 filled" does not fit today's authorable model

**Tier 1 is out.** An exact-pattern match would need one literal entry per every possible
`slotPattern` where at least one of positions 5-8 is non-`-` — with `maxArmySlotCount` defaulting
to 16, that's thousands of combinations. Not authorable as literal patterns.

**Tier 2 is out, as currently shaped.** `Params::ScenarioCountField` (`Scenario_PARAMS.h:74`) is:

```cpp
enum class ScenarioCountField { Total, HumanCount, AiCount };
```

That's it — three fields, all of them aggregate counts. There is no field that can reference
*which* slots are filled. This is not just a UI gap: it's load-bearing all the way down to the
generated runtime. `EvaluateScenarioCondition` in `resources/lua/SanGenScenarioRuntime.lua:93-104`
only ever receives `(condition, total, humanCount, aiCount)` — **`slotPattern` is never passed
into it**, even though `FindMatchingScenario` (line 140) has `slotPattern` in scope at the call
site (line 150) and simply doesn't forward it. The live reference this was ported from *did*
support arbitrary slot inspection (Tier 2 `match` was a raw Lua closure taking `slotPattern` as
its 4th argument, per `MAP_SCENARIO_SPEC.md` §4) — that expressiveness was deliberately narrowed
to `{field, comparator, value}` **data** when SanGen took over authoring it (§4's own commentary:
"the ratified SanGen-rendered contract is DATA instead"), and slot-range conditions were never
added back.

**Conclusion: this specific rule cannot be authored through the SanGen UI today, in any tier, as
the system is currently shaped.** It requires a real (small) capability extension, not just data
entry.

**⚠️ This is not an oversight — it is a prior ratified ARCH decision, on record as the human's
own.** `ARCH_15_05_ParamsScenariosType.md` §15.5 states, verbatim: *"Comparator vocabulary is
deliberately small (`Equal`/`NotEqual`/`GreaterThan`/`GreaterOrEqual`/`LessThan`/`LessOrEqual` ×
`Total`/`HumanCount`/`AiCount`, conjunction-only) — per the human's own settled framing: 'every
live predicate is a simple conjunction over total/human/AI counts, so a small vocabulary
suffices.'"* (dated 2026-08-21). Confirmed: this repo's `MAP_SCENARIO_SPEC.md` and
`DESIGN_ScenariosTabAndLuaEditor_R1.md` are pre-ARCH design consults, but §15's PARAMS shape
itself (the part that actually governs what's authorable) **is** fully ratified — `ARCH.md`
lists all ten `ARCH_15_01`–`ARCH_15_10` subsection files, and `Scenario_PARAMS.h` transcribes
§15.5 verbatim. Path B below is therefore not a gap-fill but an explicit **amendment** to a
settled ruling, and needs to be raised with the ARCH Expert as such — including reconciling it
with §15.5's stated rationale, not just adding a field around it.

## 3. The target map's current state

`Pandemonium Isthmus.sanmap` (the live game-install copy) has **no `"Scenarios"` and no
`"armies"` top-level key at all** — it is `fileVersion: 3` (the engine's native map-package
version, unrelated to SanGen's own schema versioning) and has never been round-tripped through
SanGen's exporter with scenario data. This checks out: per `MAP_SCENARIO_SPEC.md`, this exact map
is the **reference implementation** the whole spec was extracted from, and per §2.2 it is still
on the **legacy two-file shape** (`Pandemonium Isthmus_data.lua` +
`Pandemonium Isthmus_Scenarios_Script.lua`, hand-authored, mixing the generic algorithm and the
per-map tables in one file) — it has not yet been migrated to the three-file SanGen-owned design.
The map does already define armies `ARMY_01`..`ARMY_16` as spawn/alloy markers, so slots 5-8 are
real, occupiable army slots.

## 4. Two viable paths

### Path A — Hand-edit the legacy script directly (no SanGen code changes)
The live `Pandemonium Isthmus_Scenarios_Script.lua`'s `COUNT_SCENARIOS` entries are still raw Lua
closures with full `(total, humanCount, aiCount, slotPattern)` access (this file predates
SanGen's data-only contract and is exempt from it). A new entry could be added directly, e.g.:

```lua
{
    name = "slots5to8AnyFilled_FullMap",
    match = function(total, humanCount, aiCount, slotPattern)
        return slotPattern:sub(5, 8):find("[^-]") ~= nil
    end,
    area = { x = 0, y = 0, width = 2048, height = 2048 },  -- full map, matches width/length above
    navy = false,        -- or whatever's appropriate
    alloyMode = "occupancy",
    -- spawns = { ... }  -- §6 hard requirement: must be supplied explicitly, or the rule must
                          -- explicitly document that it intends to inherit the .sanmap baseline
},
```
placed **above** any broader existing fallback rule in `COUNT_SCENARIOS` (first-match-wins,
order is priority).

- ✅ Fast, zero SanGen code changes, uses a capability the legacy file already has.
- ❌ Lives entirely outside SanGen: invisible in the Scenarios tab, not regenerated/protected by
  SanGen's overwrite-safety marker, and orphaned forever if this map is ever migrated to the
  three-file design (§2.2) — the migration only carries forward what's authored in SanGen.

### Path B — Extend the Scenario system properly (SanGen-native, durable)
1. **ARCH Expert** ratifies a new Tier-2 predicate shape — e.g. a `SlotRange` condition kind
   carrying `slotStart`/`slotEnd` plus an "any filled" (or "all filled" / count-in-range)
   semantic, added alongside `ScenarioCountField` or as a sibling clause type on
   `ScenarioCountCondition`.
2. **Format Expert** rules the `.sanmap` JSON spelling for the new field (the `"Scenarios"`
   section's format-truth).
3. **IO Architecture Expert** briefs the render/evaluate change: `ScenarioScript_DataLua_IO.cpp`
   emits the new condition shape into `<MapName>_Scenarios_Data.lua`, and
   `resources/lua/SanGenScenarioRuntime.lua`'s `EvaluateScenarioCondition`/
   `EvaluateScenarioConditions` need `slotPattern` threaded in as a parameter (a small, contained
   change — `FindMatchingScenario` already has it in scope) plus a new branch to test the range.
4. **UI Expert** extends the Tier-2 clause editor (`ScenariosTab_MatchRules_UI.cpp`) with a
   slot-range picker for the new field, and rules how/whether it participates in the live
   composition matrix (`ScenariosTab_Matrix_UI.cpp`) — likely the same "hatch overlay, may
   pre-empt" treatment Tier-1 patterns already get, since a slot-range predicate isn't a pure
   function of (total, human, ai).
5. **Coder** implements from the resulting ratified work-order(s).
6. **Then**, as a one-time human action, migrate this specific map from its current legacy
   two-file shape to the three-file design (`MAP_SCENARIO_SPEC.md` §2.2: author its existing
   `PATTERN_SCENARIOS`/`COUNT_SCENARIOS`/`DEFAULT_SCENARIO` into SanGen, export once, hand-retarget
   `_data.lua`'s `Import()` and `ResolveAndApply` call site), authoring the new slot-range rule
   as part of that same pass.

- ✅ The rule becomes real SanGen data: visible/editable in the Scenarios tab, protected by the
  overwrite-safety marker, survives every future re-export.
- ❌ Real scope: a genuine ARCH-level capability extension across PARAMS/IO/UI, plus the map's
  one-time legacy→three-file migration, before this one rule can be authored.

## 4.5 Correction — the system is further along than §4 assumed, and a third path exists

Checked `work_orders/HANDOFF_TRACK_ScenarioScripting.md` and confirmed against `src/`: **STEP69,
70, 71, 72, 73, 74, 77, 78 are all landed** (not just ratified on paper) — `Params::Scenarios`,
the Lua-rendering IO, the bundled runtime resource, the full `ScenariosTab_UI`, and — the piece
that changes the recommendation below — **`LuaCodeEditor_UI` + the per-map "Runtime Script
(advanced)" override system (STEP77)** are real, shipped code (`src/ui/LuaCodeEditor_UI.*`,
`src/ui/ScenariosTab_RuntimeScript_UI.cpp`, `src/io/ScenarioScript_RuntimeResource_IO.*`,
`src/io/ScenarioScript_Export_IO.*`). This is almost certainly the "very elaborate, very
customizable system" the human recalled.

**What it actually does:** every map can carry its own override of the entire
`<MapName>_Scenarios_Runtime.lua` algorithm — edited in-app (LuaJIT compile-validated, hard export
block on a syntax error), saved to a designer-chosen file, and copied on export in place of the
bundled default. `EvaluateScenarioCondition`/`FindMatchingScenario` are not special, sealed
code — they're ordinary lines in that override, editable like anything else in the file.

### Path C — SanGen-native override (uses only shipped, ratified mechanisms — no ARCH amendment)
1. In the Scenarios tab, author a scenario record (any tier — its `pattern`/`conditions` never
   need to literally match) carrying the target data: `area` = full map, `alloyMode`, `navy`,
   explicit `spawns` per §6. Give it a distinctive `name`, e.g. `"Slots5to8AnyFilled_FullMap"`.
2. Open "Runtime Script (advanced)", toggle "Use a custom Runtime Script", and add ~4 lines to the
   generated `FindMatchingScenario` (or just above its normal three-tier body):
   ```lua
   if slotPattern:sub(5, 8):find("[^-]") then
       for _, scenario in ipairs(PATTERN_SCENARIOS) do
           if scenario.name == "Slots5to8AnyFilled_FullMap" then return scenario end
       end
   end
   ```
   (`PATTERN_SCENARIOS`/`COUNT_SCENARIOS`/`DEFAULT_SCENARIO` are already in scope as locals in the
   generated file — this reads the SanGen-authored data table by name, it doesn't invent a new one.)
3. Export normally (`Files tab → Export Scenario Script`) — both files are written together,
   collision-checked, banner-warned on drift from the bundled default.

- ✅ Zero ARCH amendment, zero `src/` C++ changes, no new PARAMS field. Fully inside the shipped,
  ratified system — visible/editable in SanGen, protected by the overwrite-safety marker, survives
  every re-export (the override is remembered, not lost).
- ⚠️ The matching *logic* for this one rule lives in hand-edited Lua text (the override), not in
  structured `Params::ScenarioCountCondition` data — so the live composition matrix / reachability
  badges (which only understand structured conditions) can't visualize or flag it. Same class of
  limitation §3's DESIGN doc already flagged for Tier-1 patterns ("may pre-empt", hatch overlay),
  just with no visualization at all here.

### Real risk specific to this map, for any SanGen-native path (B or C)
Per `HANDOFF_TRACK_ScenarioScripting.md` §G.4/§G.5/E.3, `Pandemonium Isthmus` is the **live,
currently-deployed reference map**, still on the hand-authored legacy two-file shape — it has
**never been round-tripped through SanGen's exporter at all** (confirmed independently in §3
above: no `"Scenarios"`/`"armies"` key in the live `.sanmap`). Bringing it into SanGen for the
first time surfaces two unresolved, known bugs before this feature could even be tested:
- **`NextArmyName` emits `Army1`-style names** (§G.5), not the `ARMY_01`-style names the live map
  and the whole scenario system's `armyName` matching require. Unfixed as of the handoff.
- **D.5**: `ArmiesExceedingSlotCount`'s army-ID assumption (positional) vs. the engine's actual
  alphabetical-sort assignment (§G.6) — unreconciled.

Neither blocks Path A (no SanGen involvement at all). Both are real landmines for Path B or Path C
on *this specific map* until confirmed fixed or worked around.

## 4.6 Confirmed against the live reference file directly

Read the live `Pandemonium Isthmus_Scenarios_Script.lua` in full (2026-08-27). `PATTERN_SCENARIOS`
is genuinely empty, with a placeholder comment anticipating exactly this shape:
```lua
-- { name="4human-slots5-8", pattern="----hhhh--------", area=..., navy=..., alloyMode=..., spawns=..., alloys=... },
```
This is an **exact**-pattern example (Tier 1) — "exactly 4 humans, precisely slots 5-8, nothing
else filled" — not "any of slots 5-8 filled" (which admits AI in those slots, other slots also
filled, partial occupancy of 5-8, etc.). `FindMatchingScenario` (line 239 of the live file) still
does plain `scenario.pattern == slotPattern` string equality, confirming no wildcard support ever
existed, live or ratified. Also grepped the whole repo (`ARCH_15_10`, `STEP74`, `ScenariosTab_
Settings_UI.cpp`, every other scenario-adjacent file) for any first-class "slot range"/"any slot"
predicate — the only hits are the unrelated `maxArmySlotCount`-vs-roster-size warning. **There is
no structured, data-driven way to express "any of slots N..M filled" anywhere in the shipped or
ratified system** — the only two places `slotPattern` is ever inspectable by arbitrary logic are
(a) a raw Lua `match` closure, which only the still-legacy `Pandemonium Isthmus_Scenarios_Script.lua`
supports (SanGen's rendered `COUNT_SCENARIOS` closures are structural now, `{field,comparator,
value}` data only — SanGen itself cannot emit a closure like this), or (b) hand-editing the
generated runtime algorithm itself via SanGen's Runtime Script override (§4.5, Path C).

## 5. Recommendation

Path B is the only option that matches "SanGen compatibility" as the spec defines it — Path A
puts the rule permanently outside SanGen's reach. But Path B is real, multi-layer scope, not a
one-line change. Recommend routing Path B through the ARCH Expert first (the predicate-shape
decision in step 1) before any PARAMS/IO/UI/Lua file is touched.

## ❓ Open questions for the human
- Path A (fast, off-SanGen) or Path B (proper extension, more scope) — or Path A now as a bridge,
  Path B later?
- If Path B: exact semantics wanted — "any of 5-8 filled" (≥1), or should the new predicate
  support "all filled" / "at least N filled" too, since the runtime work is nearly the same cost
  either way?
- "Full map size" — confirm this means `{x:0, y:0, width:mapSize, height:mapSize}` (2048×2048 per
  the file's own `width`/`length`), not some other definition of "full."
