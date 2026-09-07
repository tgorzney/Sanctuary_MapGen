# STEP253 — Implement `ARCH_15_05`'s `SlotRangeOccupiedCount` condition (Tier 2 slot-identity predicate)

**Layer:** PARAMS + IO/BRIDGE + UI + the bundled Lua runtime. **Domain:** `Params::ScenarioCountField`/
`Params::ScenarioCountCondition` and every reader/writer/evaluator of them. **Sequence:** ratifies
nothing new — `ARCH_15_05_ParamsScenariosType.md`'s `AMENDED 2026-09-04` section is already law
(ratified in full, no corrections needed). This ticket is the first thing that actually implements
it. **Depends on nothing undone in this ticket's own scope**, but see "Sequencing" below for a real
ordering interaction with `STEP252`.

**Not yet built.** Confirmed by direct grep: `SlotRangeOccupiedCount` does not appear anywhere under
`src/` or `resources/` — the ARCH amendment is ratified law, not shipped behavior. Every scenario
condition authored in SanGen today is still limited to `Total`/`HumanCount`/`AiCount`.

**Read `ARCH_15_05_ParamsScenariosType.md`'s `AMENDED 2026-09-04` section in full before starting.**
It is the binding law; this ticket restates its exact shape with real file/line anchors against the
CURRENT tree — it is not a paraphrase to work from independently. Where this ticket's own text and
that ARCH section disagree on anything, the ARCH section wins; flag the discrepancy rather than
silently picking one.

## 0. Why

The live, shipping reference script's own first-priority `COUNT_SCENARIOS` entry,
`slots5to8AnyFilled`, predicates on **which** army slots are occupied — something no existing
`ScenarioCountField` (`Total`/`HumanCount`/`AiCount`, pure aggregate counts) can express. Two players
in slots 5 and 7 are indistinguishable from two players in slots 1 and 2 to every field that exists
today. `ARCH_15_05`'s amendment adds a fourth field, `SlotRangeOccupiedCount`, that closes this —
reusing the existing `ScenarioComparator`/`value` pair rather than inventing a new predicate
vocabulary, so "any of slots 5-8 filled," "all of slots 5-8 filled," and "at least N of slots 5-8
filled" are all expressible with the same one new field plus two new sibling ints.

This is also a real, named blocker: `work_orders/PHASE_A_ScenarioDataMigration_PandemoniumIsthmus.md`
already authors a `slots5to8AnyFilled` entry using this exact shape (`SlotRangeOccupiedCount`,
`slotRangeStart=5`, `slotRangeEnd=8`, `GreaterOrEqual`, `value=1`) — that data cannot round-trip
through SanGen until this ticket lands.

## 1. Sequencing — read before touching anything

**`STEP252` (the `ScenarioSpawnPoint`/`spawnIds` pool) touches the same files this ticket touches**
(`Scenario_PARAMS.h`, `MapExporter_Scenarios_IO.cpp`, `MapImporter_ScenarioRecord_IO.cpp`,
`ScenarioScript_DataLua_IO.cpp`, `resources/lua/SanGenScenarioRuntime.lua`, `ScenariosTab_MatchRules_UI.cpp`
region of the UI file family) and is independently flagged **urgent** (live shipped code is currently
out of ARCH compliance). Per the commit protocol's "claim before writing" step: check whether `STEP252`
has landed before starting this ticket's own edits to any shared file. If `STEP252` is mid-flight in
another session, coordinate rather than editing the same file concurrently — these two tickets are
logically independent (spawn identity vs. count-condition shape) but physically adjacent in several
files, so a naive parallel edit risks a real merge collision, not just a build-directory lock.

## 2. PARAMS — `src/params/Scenario_PARAMS.h`

```cpp
enum class ScenarioComparator { Equal, NotEqual, GreaterThan, GreaterOrEqual, LessThan, LessOrEqual };  // UNCHANGED
enum class ScenarioCountField { Total, HumanCount, AiCount, SlotRangeOccupiedCount };  // AMENDED -- 4th enumerator

struct ScenarioCountCondition {
    ScenarioCountField field           = ScenarioCountField::Total;
    ScenarioComparator comparator      = ScenarioComparator::Equal;
    int                value           = 0;
    int                slotRangeStart  = 1;   // NEW. 1-based, inclusive. Meaningful ONLY when
                                                // field == SlotRangeOccupiedCount.
    int                slotRangeEnd    = 1;   // NEW. 1-based, inclusive. Must satisfy
                                                // 1 <= slotRangeStart <= slotRangeEnd <=
                                                // maxArmySlotCount (validated, never clamped -- §4).
};
```

Follow this struct's existing explicit-default-initializer discipline (every field already has one;
match it for the two new fields). No other field on `ScenarioBody`/`PatternScenario`/
`ScenarioComparator` changes — every existing `Total`/`HumanCount`/`AiCount` condition must round-trip
and evaluate completely unchanged after this ticket.

## 3. IO — `.sanmap` JSON leg (`src/io/MapExporter_Scenarios_IO.cpp` / `src/io/MapImporter_ScenarioRecord_IO.cpp`)

- `ScenarioCountField`'s spelling table gains a 4th entry: `"SlotRangeOccupiedCount"` (PascalCase,
  same convention as `"Total"`/`"HumanCount"`/`"AiCount"`).
- The condition object's writer gains two new sibling int keys, `"SlotRangeStart"`/`"SlotRangeEnd"`,
  **always emitted** regardless of `"Field"` — matching this record's "every scalar field is always
  present" convention (the same posture `"Value"` already has for conditions that don't need it).
- The condition object's reader reads `"SlotRangeStart"`/`"SlotRangeEnd"` into the new struct fields
  the same way every other scalar field in that reader is read. An absent key (every pre-existing
  `.sanmap`) leaves both fields at their struct default (1, 1) — legacy files unaffected, no migration
  entry needed, no `SanGenVersion` bump.

## 4. IO — `<MapName>_Scenarios_Data.lua` leg (`src/io/ScenarioScript_DataLua_IO.cpp`)

- The Lua-rendering field-spelling table gains the matching `"SlotRangeOccupiedCount"` entry.
- The condition row's renderer gains the matching lowercase keys, `slotRangeStart = N, slotRangeEnd = M`,
  also always emitted alongside the existing `field`/`comparator`/`value` triple, for the same
  fixed-row-shape reason as §3.

## 5. Runtime — `resources/lua/SanGenScenarioRuntime.lua`

`EvaluateScenarioCondition`/`EvaluateScenarioConditions` widen by one parameter, `slotPattern`,
forwarded unchanged through the one existing `FindMatchingScenario` call site (which already has
`slotPattern` in scope but currently drops it before reaching Tier 2). No new call site.

```lua
local function EvaluateScenarioCondition(condition, total, humanCount, aiCount, slotPattern)
    local fieldValue
    if condition.field == "Total" then
        fieldValue = total
    elseif condition.field == "HumanCount" then
        fieldValue = humanCount
    elseif condition.field == "AiCount" then
        fieldValue = aiCount
    elseif condition.field == "SlotRangeOccupiedCount" then
        local occupiedCount = 0
        for slotIndex = condition.slotRangeStart, condition.slotRangeEnd do
            if slotPattern:sub(slotIndex, slotIndex) ~= "-" then
                occupiedCount = occupiedCount + 1
            end
        end
        fieldValue = occupiedCount
    else
        Warn("SANGEN: scenario condition named unknown field '"..tostring(condition.field).."' -- treated as non-matching.")
        return false
    end
    -- comparator branch below is UNCHANGED -- SlotRangeOccupiedCount reuses it verbatim
    ...
end

local function EvaluateScenarioConditions(conditions, total, humanCount, aiCount, slotPattern)
    for _, condition in ipairs(conditions) do
        if not EvaluateScenarioCondition(condition, total, humanCount, aiCount, slotPattern) then
            return false
        end
    end
    return true
end
```

`FindMatchingScenario`'s Tier-2 call site widens to forward `slotPattern` — its own parameter,
already in scope, simply not passed through until now. No other function in this file changes.
Governed by `ARCH_15_04` category-2 posture (generic, byte-identical across every map) — stays inside
this one bundled file, no new category, no overwrite-safety change, no size-ceiling exception needed
(this file's existing ceiling exemption is unaffected by a few added lines).

## 6. Validation — bounds, loud/logged, never silently clamped

`1 <= slotRangeStart <= slotRangeEnd <= scenarios.maxArmySlotCount`, enforced at the same two points
and posture as `ARCH_15_10` point 2's roster-vs-slot-count check and `ScenarioBody::name`'s
charset/uniqueness check (`STEP251` §1, once landed) already use: UI-authoring time and export time,
named-by-scenario, logged loud, never silently reordered/clamped/truncated. Model the export-time
validator on `MapExporter_ScenarioAreaNameValidation_IO.h`'s shape (a report struct + one-wording
`SummaryText()`, pure/disk-free) — a new, small, dedicated file (e.g.
`ScenarioSlotRangeValidation_IO.h`/`.cpp`) rather than growing an existing near-ceiling file, matching
this codebase's one-file-per-validation-concern pattern.

## 7. UI

### 7a. Tier-2 clause editor (`ScenariosTab_MatchRules_UI.cpp`, `DrawScenarioCountConditionsEditor`)

`kScenarioCountFieldCount` grows to 4; `scenarioCountFieldLabels` gains e.g. `"Slot Range"`. The
per-condition row gains a conditional slot-range picker — two bounded `ImGui::InputInt` "From"/"To"
fields, consistent with the existing `ImGui::InputInt("Value", ...)` idiom already on the same row,
clamped to `[1, maxArmySlotCount]` — shown only when `condition.field == SlotRangeOccupiedCount`. The
function widens to take `maxArmySlotCount` as a parameter (it does not take one today).

### 7b. Composition matrix (`ScenariosTab_Matrix_UI.cpp` / `ScenariosTab_Reachability_UI.cpp`)

Per the ratified advisory draft's recommendation (`ARCH_15_05`'s amendment, "Follow-ups" section):
extend `MatchesScenarioConditions`'s evaluator to a tri-state (definitely-true / definitely-false /
ambiguous — ambiguous only when every non-range clause in a conjunction passes AND at least one
range clause's truth depends on slot identity the `(total, human, ai)` triple can't supply).
`ResolvedScenarioNameForTriple`'s walk treats "ambiguous" as "does not match, keep walking" — mirrors
Tier 1's own existing "may pre-empt" posture (show what happens without the identity-dependent rule).
The affected cell gets a **second, visually distinct hatch mark** (e.g. a bottom-right corner
triangle, mirroring Tier 1's existing top-left one, never reusing the same corner) so the two
independent sources of uncertainty — Tier 1 exact-pattern pre-emption vs. a Tier 2 slot-range clause's
own identity-dependence — stay legible and combinable rather than conflated into one signal.

## 8. Tests

1. **PARAMS round-trip**: a `ScenarioCountCondition` with `field=SlotRangeOccupiedCount,
   slotRangeStart=5, slotRangeEnd=8, comparator=GreaterOrEqual, value=1` round-trips through both IO
   legs unchanged; every existing `Total`/`HumanCount`/`AiCount` condition still round-trips unchanged
   (regression coverage).
2. **Legacy `.sanmap` compatibility**: a condition object with no `"SlotRangeStart"`/`"SlotRangeEnd"`
   keys reads back as `(1, 1)` — the struct default — not an error, not a crash.
3. **Wire spelling**: `"SlotRangeOccupiedCount"` appears verbatim in both the `.sanmap` JSON leg and
   the `<MapName>_Scenarios_Data.lua` leg's rendered text for a condition using it; `SlotRangeStart`/
   `SlotRangeEnd` (or `slotRangeStart`/`slotRangeEnd`) keys are present and correctly valued on every
   condition row, including ones that don't use the new field (value `1, 1`).
4. **Runtime evaluator, via `Sys::CheckLuaSyntax` + direct interpretation if this project's Lua test
   harness supports executing snippets** (check `ScenarioScript_RuntimeResource_IO_Test.cpp` for the
   existing pattern): `EvaluateScenarioCondition` with `field="SlotRangeOccupiedCount"`,
   `slotRangeStart=5, slotRangeEnd=8`, against a `slotPattern` with slots 5 and 7 occupied
   (`"----h-h---------"`), `comparator="GreaterOrEqual"`, `value=1` → true. Same pattern with
   `slotRangeStart=1, slotRangeEnd=4` → false (nothing occupied in 1-4). An "all filled" case
   (`value` equal to range width, `comparator="Equal"`) with every slot in range occupied → true, one
   slot empty → false.
5. **Bounds validation**: `slotRangeStart > slotRangeEnd`, `slotRangeEnd > maxArmySlotCount`, and
   `slotRangeStart < 1` each produce a named, logged violation and are never silently clamped or
   reordered; a violation refuses only that condition's row (mirror `STEP251`'s per-scenario refusal
   posture — the rest of the scenario's other conditions/fields are unaffected).
6. **UI matrix tri-state**: a `SlotRangeOccupiedCount` clause whose conjunction partner clauses all
   pass at a given `(total, human, ai)` cell renders the new, second hatch mark; a clause whose
   conjunction partner already fails (e.g. `Total == 99`) does NOT render it (definitely-false,
   no ambiguity).

## 9. Out of scope

- Any change to `ARCH_15_04`'s file-category shape — this ticket is a Tier-2 PARAMS/IO/UI/runtime
  change only, orthogonal to the category-4 file split (`STEP251`).
- Migrating the live `Pandemonium Isthmus` map's actual `slots5to8AnyFilled` data — that is
  `work_orders/PHASE_A_ScenarioDataMigration_PandemoniumIsthmus.md`'s job, blocked on this ticket
  and `STEP251`/`STEP252` landing first, not part of this ticket's own scope.
- `sangen_arch_pack/specs/MAP_SCENARIO_SPEC.md`'s own documentation of this field (§4/§5.1/§6) —
  Format-Expert-owned spec prose, not confirmed as one done as of this ticket's authoring; flag to
  the Format Expert in parallel, don't block this ticket's code on it.
