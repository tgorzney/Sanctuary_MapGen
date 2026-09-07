# DRAFT — UNRATIFIED — for the ARCH Expert's dedicated setup conversation

**Status: UNRATIFIED DRAFT.** Produced 2026-09-04 by an advisory (read-only) consult on the SanGen
ARCH Expert's behalf — this document is **not** itself authoritative and binds nothing. It exists so
the human can carry a precise, ready-to-ratify text into the ARCH Expert's own dedicated setup
conversation. Nothing in `ARCH.md`, `ARCH_15_05_ParamsScenariosType.md`,
`ARCH_15_04_ThreeFileOnDiskShape.md`, `sangen_arch_pack/`, `Scenario_PARAMS.h`, or
`resources/lua/SanGenScenarioRuntime.lua` has been touched to produce this file. Until ratified
there, treat every clause below as a proposal, not law.

Full background/reasoning trail (not repeated here): `work_orders/DESIGN_ScenarioSlotRangeCondition_R1.md`
("Path B"), `ARCH_15_05_ParamsScenariosType.md` §15.5's original ruling and its 2026-08-28/2026-09-03
amendments, `ARCH_15_06_CountScenariosOrdering.md`, `sangen_arch_pack/specs/MAP_SCENARIO_SPEC.md`
§4/§5/§5.1/§5.2/§6/§11.2, the live `Pandemonium Isthmus_Scenarios_Script.lua`'s `slots5to8AnyFilled`
entry (the real, shipping, checked-first COUNT_SCENARIOS rule this amendment exists to make
authorable in SanGen).

---

## Proposed amendment to §15.5 (`ARCH_15_05_ParamsScenariosType.md`)

### AMENDED 2026-09-04 — `ScenarioCountField::SlotRangeOccupiedCount` (Tier 2 gains a slot-identity predicate; resolves `DESIGN_ScenarioSlotRangeCondition_R1.md` Path B)

**This is a shape amendment to an already-ratified type, recorded the same way the "RETIRED"/
"AMENDED" corrections above are** — auditable, not a silent bolt-on.

**The gap this closes.** `ScenarioCountField` (`Total`/`HumanCount`/`AiCount`) can only express
aggregate-count conjunctions. The live, shipping reference script's own **first-priority**
`COUNT_SCENARIOS` entry, `slots5to8AnyFilled` (`MAP_SCENARIO_SPEC.md` §5.1/§5.2 — "`pattern:sub(5,8):
find("[^-]") ~= nil`", `area = AREA_FULL`, `spawnsUnits = true`, the one entry that opts into unit
spawning), predicates on **which slots** are occupied, not merely how many. No conjunction over
`Total`/`HumanCount`/`AiCount` can express it — two players in slots 5 and 7 are indistinguishable
from two players in slots 1 and 2 to every existing field. This is not authorable through SanGen
today, in any tier (§2 of the design doc).

**What changes.** One new enumerator on `ScenarioCountField`, and two new sibling fields on
`ScenarioCountCondition`, meaningful only when that enumerator is selected — the same "flat sibling
fields, only the ones matching the record's own mode are meaningful" idiom this file already uses
for `ScenarioBody`'s `alloys`/`alloysToAdd`/`alloysToRemove` (§15.5 above), applied one type deeper
rather than introduced as a new pattern:

```cpp
enum class ScenarioComparator { Equal, NotEqual, GreaterThan, GreaterOrEqual, LessThan, LessOrEqual };  // UNCHANGED
enum class ScenarioCountField { Total, HumanCount, AiCount, SlotRangeOccupiedCount };  // AMENDED — 4th enumerator

struct ScenarioCountCondition {
    ScenarioCountField field           = ScenarioCountField::Total;
    ScenarioComparator comparator      = ScenarioComparator::Equal;
    int                value           = 0;
    int                slotRangeStart  = 1;   // NEW. 1-based, inclusive. Meaningful ONLY when
                                                // field == SlotRangeOccupiedCount — ignored
                                                // otherwise, same "meaningful only for the
                                                // matching mode" idiom as ScenarioBody's alloy
                                                // fields above. Default 1, not 0: this codebase's
                                                // slot indexing is 1-based throughout
                                                // (slotPattern, ARCH_15_10's BuildSlotPattern,
                                                // ARMY_ID_TO_NAME) — a 0 default would be an
                                                // out-of-range value dressed as a harmless default.
    int                slotRangeEnd    = 1;   // NEW. 1-based, inclusive (matches Lua's
                                                // slotPattern:sub(a,b) convention exactly — the
                                                // live reference's own "slots 5 to 8" reads as
                                                // slotRangeStart=5, slotRangeEnd=8 with zero
                                                // translation). Must satisfy
                                                // 1 <= slotRangeStart <= slotRangeEnd <=
                                                // maxArmySlotCount — see "Bounds" below.
};
struct CountScenario { ScenarioBody body; std::vector<ScenarioCountCondition> conditions; };  // UNCHANGED, TIER 2, AND'd
```

**Why "one more countable quantity" and not a sibling variant/tagged-union condition kind.** The
design doc's own open question asked whether to support just "any filled" (≥1), or also "all
filled"/"at least N filled," noting runtime cost is nearly identical either way. The chosen shape
answers this **without inventing a second predicate vocabulary**: `SlotRangeOccupiedCount` is
compared via the *same, already-ratified* `ScenarioComparator`/`value` pair every other field already
uses. This gives every semantic the design doc considered, for free, with no dedicated enum of range
semantics:
- "Any of slots 5-8 filled" → `field=SlotRangeOccupiedCount, slotRangeStart=5, slotRangeEnd=8, comparator=GreaterOrEqual, value=1`
- "All of slots 5-8 filled" → same range, `comparator=Equal (or GreaterOrEqual), value=4` (the range's own width)
- "At least 2 of slots 5-8 filled" → same range, `comparator=GreaterOrEqual, value=2`

A separate discriminated `SlotRangeCondition` type (sibling variant on `CountScenario`, or a
`std::variant` on the condition itself) was considered and **rejected**: it would duplicate the
comparator/value machinery under a different name for no additional expressiveness, add a second
wire shape, a second UI clause type, and a second evaluator branch structure — real surface area
this codebase's existing "no `std::variant` precedent in `src/params/`" posture (§15.5 above,
"Why a flat sibling field set, not a tagged union") already argues against. A bare boolean-only
"any filled" field (no comparator) was also considered and rejected as under-general: it would
special-case exactly one of the three semantics the design doc flagged as equal-cost, for no
implementation savings.

**Bounds — validated, never silently clamped, against `maxArmySlotCount`, not a hardcoded 16.**
`1 <= slotRangeStart <= slotRangeEnd <= scenarios.maxArmySlotCount` is enforced at the same two
points, and with the same posture, `ARCH_15_10`'s `maxArmySlotCount`-vs-army-roster check and this
file's own `areaName`/`name`-uniqueness checks already use: **UI-authoring time and export time,
loud-logged on violation, never silently reordered/clamped/truncated by SanGen.** A range that
exceeds `maxArmySlotCount` at export time is the same class of authoring gap `ARCH_15_10` point 2
already names for the roster-vs-slot-count mismatch — non-blocking, named-by-scenario, logged loud.
This is a new instance of an existing enforcement shape, not a new mechanism.

**Wire spellings — additive, no version bump**, following this exact record family's established
"additive key, no `SanGenVersion` bump" posture (§15.5's `AreaName` amendment, above):
- `ScenarioCountField`'s spelling table (`kCountFieldSpellings`/`kScenarioCountFieldSpellings`,
  `MapExporter_Scenarios_IO.cpp`/`ScenarioScript_DataLua_IO.cpp`) gains a 4th entry,
  `"SlotRangeOccupiedCount"` — same PascalCase convention as `"Total"`/`"HumanCount"`/`"AiCount"`.
- The `.sanmap` JSON leg's condition object gains two new sibling int keys, `"SlotRangeStart"`/
  `"SlotRangeEnd"`, **always emitted** (matching this record's own "every scalar field is always
  present" convention, e.g. `SpawnsUnits`/`AuthoringNote`) — meaningless when `"Field"` is not
  `"SlotRangeOccupiedCount"`, exactly as `value` is already meaningless-but-present for conditions
  that don't need it.
- The `<MapName>_Scenarios_Data.lua` leg's condition row gains the matching lowercase keys,
  `slotRangeStart = N, slotRangeEnd = M`, also always emitted alongside the existing
  `field/comparator/value` triple, for the same fixed-row-shape reason.
- `MapImporter_ScenarioRecord_IO.cpp`'s condition reader gains two lines reading
  `"SlotRangeStart"`/`"SlotRangeEnd"` into the new struct fields, the same idiom as every other
  scalar field in that reader. An absent key (every pre-existing `.sanmap`) leaves both fields at
  their struct default (1, 1) — legacy files unaffected, no migration entry needed, no existing
  authored condition changes shape or behavior.

**Runtime evaluation — `resources/lua/SanGenScenarioRuntime.lua` signature widens by one parameter,
threading a value already in scope.** `FindMatchingScenario(total, humanCount, aiCount, slotPattern)`
already receives `slotPattern` (it is the file's TIER 1 exact-match input) but never forwards it into
TIER 2's evaluator — that is the entire mechanical gap the design doc identified (§2). This amendment
closes it by threading it one call deeper, not by adding a new call site:

```lua
-- EvaluateScenarioCondition gains a 5th parameter, slotPattern, and one new field branch:
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

-- EvaluateScenarioConditions gains the same 5th parameter, forwarding it unchanged:
local function EvaluateScenarioConditions(conditions, total, humanCount, aiCount, slotPattern)
    for _, condition in ipairs(conditions) do
        if not EvaluateScenarioCondition(condition, total, humanCount, aiCount, slotPattern) then
            return false
        end
    end
    return true
end

-- FindMatchingScenario's TIER 2 call site widens to forward slotPattern -- its own parameter,
-- already in scope, simply not passed through until now:
local evalOk, matched = pcall(EvaluateScenarioConditions, scenario.conditions, total, humanCount, aiCount, slotPattern)
```

No other call site changes. `Scenario.ResolveAndApply` already builds `slotPattern` via
`BuildSlotPattern` (`ARCH_15_10`) before calling `FindMatchingScenario` — nothing upstream of
`FindMatchingScenario` needs to change. This is governed by the same `ARCH_15_04` category-2
("generic runtime algorithm, byte-identical across every map") posture already in force — the
widened functions stay inside the one bundled `_Scenarios_Runtime.lua` file, no new category, no
overwrite-safety change, no new file.

**Reconciling with this file's own "small vocabulary suffices" rationale, not silently replacing
it.** The original ruling states, verbatim: *"Comparator vocabulary is deliberately small
(...) — per the human's own settled framing: 'every live predicate is a simple conjunction over
total/human/AI counts, so a small vocabulary suffices.'"* That premise was **incomplete, not
wrong**, for what it actually covered: it was derived before the 2026-08-27 live-reference rewrite
that introduced `slots5to8AnyFilled` as the script's own new first-priority rule — every *other*
live `COUNT_SCENARIOS` entry (`1v1`, `2h1ai`, `4human`, `1h3ai`, `6total`, `2hRestAI`, `floor169`,
`MAP_SCENARIO_SPEC.md` §5.2) genuinely is, and remains, a pure conjunction over
`Total`/`HumanCount`/`AiCount`, and this amendment changes none of their shape or the two enumerators
that express them. What was missing was not a bigger comparator vocabulary — it was one more
**countable quantity**: "how many slots in an authored range are occupied," alongside the three
existing quantities. The small comparator vocabulary the human ruled on stands unmodified; only the
set of things it can be asked to count grows from three to four. `Total`/`HumanCount`/`AiCount`
remain the common case; `SlotRangeOccupiedCount` is the deliberately narrow escape hatch for the one
class of predicate — identity, not cardinality — the original ruling did not have a live example of
yet.

**Rejected alternatives, recorded so a future reader does not re-propose them:**
1. A discriminated `SlotRangeCondition` sibling/variant type — rejected; duplicates the
   comparator/value machinery for no added expressiveness (see above).
2. A boolean-only "any filled" field with no comparator — rejected; under-general for zero
   implementation savings versus reusing the existing comparator/value pair.
3. Hardcoding the bound check against a literal `16` — rejected; must check against the map's own
   authored `maxArmySlotCount` (`ARCH_15_10`), which may legitimately exceed or fall short of 16.
4. Silently clamping an out-of-range `slotRangeEnd` at export time — rejected; violates this file
   family's "loud, logged, never silent, never auto-clamped" posture (`ARCH_15_10` point 2).

**No other field on `ScenarioBody`/`PatternScenario`/`ScenarioComparator` changes.** This amendment
is additive to `ScenarioCountField`/`ScenarioCountCondition` only; every existing authored
`Total`/`HumanCount`/`AiCount` condition round-trips and evaluates completely unchanged.

---

## What `ARCH_15_04` needs to say differently

**Nothing shape-level.** The widened `EvaluateScenarioCondition`/`EvaluateScenarioConditions`
functions stay inside `_Scenarios_Runtime.lua` — the same category-2 file `ARCH_15_04` already
governs as "generic runtime algorithm, byte-identical across every map." No new category, no new
overwrite-safety class, no change to the category-4 dispatch mechanism (which is orthogonal — it
fires after a scenario is already matched, regardless of which Tier matched it). Flagging only for
completeness; the ARCH Expert may want one clause noting the category-2 file's function signatures
are not frozen by `ARCH_15_04`'s own text — but this is optional editorial tidiness, not a
correctness gap.

## What `MAP_SCENARIO_SPEC.md` needs to say differently

Flagging only — not fully drafted, per scope:
- **§4's field/comparator vocabulary table** (wherever it enumerates `Total`/`HumanCount`/`AiCount`)
  needs a fourth row for `SlotRangeOccupiedCount`, including the 1-based/inclusive
  `slotRangeStart`/`slotRangeEnd` convention and the `maxArmySlotCount` bound.
- **§5.1's `slots5to8AnyFilled` worked example** should gain a forward-note that, under this
  amendment, the rule becomes expressible as native Tier 2 structured data
  (`SlotRangeOccupiedCount ≥ 1` over slots 5-8) rather than only as a raw Lua closure — the
  **ordering** discipline §5.1 documents (identity rules must be authored above aggregate-count
  rules they could collide with, `ARCH_15_06`) is completely unaffected and still applies verbatim:
  a `SlotRangeOccupiedCount` condition must still be placed above `1v1`-style total-only rules it
  could otherwise be shadowed by.
- **§5.2's live table** describes the still-unmigrated legacy two-file script and needs no edit
  until the map is actually migrated (`§2.2`); a forward-pointer to the new field is enough.
- **§6's field table** (`name`'s existing "Ratified target only" annotation style) gains a row, or an
  annotation on the existing `match`/condition row, describing `SlotRangeOccupiedCount` and its two
  new sibling keys.
- **§11.2** needs no change — unit-spawn dispatch is orthogonal to which tier/field matched.

## UI / composition-matrix implication (recommendation, not a work order)

**Tier 2 clause editor (`ScenariosTab_MatchRules_UI.cpp`, `DrawScenarioCountConditionsEditor`).**
`kScenarioCountFieldCount` grows to 4 (`scenarioCountFieldLabels` gains e.g. `"Slot Range"`); the
per-condition row gains a conditional slot-range picker (two bounded `ImGui::InputInt` "From"/"To"
fields, consistent with the existing `ImGui::InputInt("Value", ...)` idiom already on the same row,
clamped to `[1, maxArmySlotCount]`) shown only when `condition.field == SlotRangeOccupiedCount` —
the function widens to take `maxArmySlotCount` as a parameter, which it does not take today. This is
UI-Expert-owned implementation detail, flagged here, not drafted.

**Composition matrix (`ScenariosTab_Matrix_UI.cpp` / `ScenariosTab_Reachability_UI.cpp`).**
Confirmed how Tier 1 patterns are shown today: `AnyPatternScenarioMayPreempt` computes a pure
function of a cell's `(human, ai)` pair (a registered pattern's own letter counts), and
`DrawScenarioMatrixCell` renders it as a black top-left corner-triangle hatch plus a tooltip
sentence — the resolved headline color is computed by the normal Tier2→Default walk, **ignoring**
Tier 1 entirely, with the hatch as a separate "may actually be pre-empted" warning layered on top.

**The same treatment extends cleanly to `SlotRangeOccupiedCount`, but is a distinct source of
uncertainty from Tier 1's and should get its own, visually distinct flag, not reuse Tier 1's exact
mark:** `MatchesScenarioConditions(conditions, total, human, ai)` today is a pure function of the
triple, but a `CountScenario` carrying a `SlotRangeOccupiedCount` clause cannot be conclusively
resolved from the triple alone whenever every other clause in its conjunction already passes (if any
non-range clause already fails, the whole conjunction is definitely false regardless of slot
identity — no ambiguity, short-circuits exactly as today). Recommend: extend the evaluator to a
tri-state (definitely-true / definitely-false / ambiguous — ambiguous only when every non-range
clause passes AND at least one range clause's truth depends on identity the triple can't supply);
`ResolvedScenarioNameForTriple`'s walk treats "ambiguous" as "does not match, keep walking" (mirrors
Tier 1's own posture: show what happens without the identity-dependent rule); the cell then gets a
**second, visually distinct hatch mark** (e.g. a bottom-right corner triangle, mirroring Tier 1's
top-left one, rather than reusing the same corner) so the two independent sources of uncertainty
(Tier 1 exact-pattern pre-emption vs. a Tier 2 slot-range clause's own identity-dependence) stay
legible and combinable rather than conflated into one ambiguous signal. Full implementation is
UI-Expert-owned; flagged here with the exact semantics needed so it isn't reinvented differently.

---

## Resolves

**`work_orders/DESIGN_ScenarioSlotRangeCondition_R1.md` Path B** — the live, shipping
`slots5to8AnyFilled` scenario becomes authorable as native `Params::Scenarios` structured data
(visible/editable in the Scenarios tab, protected by the overwrite-safety marker, survives every
re-export, visible in the composition matrix) via one additive enumerator and two additive sibling
fields, with zero shape change to any existing `Total`/`HumanCount`/`AiCount` condition, and with the
Path C Lua Runtime Script override no longer needed for this specific rule once ratified and
implemented.

## Scope for implementation (pointer only, not a work order)

Once ratified, the coder-facing work order should cover, at minimum:
- `Scenario_PARAMS.h`: add `ScenarioCountField::SlotRangeOccupiedCount` and the two new
  `ScenarioCountCondition` fields (with the STEP69-style explicit default-initializer discipline
  this file already follows for every field in this struct).
- `MapExporter_Scenarios_IO.cpp` / `MapImporter_ScenarioRecord_IO.cpp`: the `.sanmap` JSON leg's
  `kCountFieldSpellings` table and the condition read/write of `"SlotRangeStart"`/`"SlotRangeEnd"`.
- `ScenarioScript_DataLua_IO.cpp`: the Lua-data-leg `kScenarioCountFieldSpellings` table and the
  condition row's `slotRangeStart`/`slotRangeEnd` keys.
- `resources/lua/SanGenScenarioRuntime.lua`: the `EvaluateScenarioCondition`/
  `EvaluateScenarioConditions`/`FindMatchingScenario` signature widening and new field branch shown
  above (bundled resource — content governed by this ARCH family, `ARCH_15_04`).
- `ScenariosTab_MatchRules_UI.cpp`: the 4th field label, the conditional slot-range picker, bounds
  clamping against the map's own `maxArmySlotCount` (threaded into the function, not hardcoded).
- `ScenariosTab_Matrix_UI.cpp` / `ScenariosTab_Reachability_UI.cpp`: the tri-state evaluator and
  second, distinct hatch mark described above.
- Export-time validation: `slotRangeStart`/`slotRangeEnd` bounds against `maxArmySlotCount`, same
  enforcement posture as `ARCH_15_10`'s roster-vs-slot-count check and this file's `name`/`areaName`
  checks (loud, logged, non-blocking, never auto-clamped).
- No changes to `MapImporter_ScenarioRecord_IO.cpp`'s read-back scope beyond the two new scalar
  fields — this amendment adds no new "read Lua back" capability anywhere, consistent with
  `ARCH_15_03`.
