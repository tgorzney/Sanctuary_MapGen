# STEP74 — `ScenariosTab_UI`: list management, match-rule authoring, live composition matrix, mandatory-spawns surfacing, `maxArmySlotCount` authoring

**Layer:** UI. **Domain:** new top-level tab, `Params::Scenarios` (`ARCH_15_05_ParamsScenariosType.md` §15.5 / `ARCH_15_06_CountScenariosOrdering.md` §15.6 / `ARCH_15_10_SlotPatternConstructionMoves.md` §15.10).
**Sequence:** Map Scenario track, Work-Order 8 of 8, UI leg, part 1 of 3. **Depends on STEP69**
(`Params::Scenarios`/`MapRecipe::scenarios` must exist in `src/params/` — not landed as of this
writing; this ticket cannot compile until it does). **No dependency on STEP63/64/65/70/71/72** (the
Lua-rendering/export leg, STEP77) or on **STEP47/50-53** (world↔screen projection and the
overlay-icon draw pass, all still **DRAFTED, not landed** — STEP78, canvas editing). This ticket is
the entire non-canvas, non-Lua-editor authoring surface, buildable and testable in complete
isolation from both other legs.

**Supersedes `DESIGN_ScenariosTabAndLuaEditor_R1.md` §1–§6** on every type name/shape: that design's
provisional `Scenario`/`ScenarioSettings`/`ScenarioSpawnsPolicy` types are retired. Author against
`ARCH_15_05_ParamsScenariosType.md` §15.5's `ScenarioBody`/`PatternScenario`/`CountScenario`/`Scenarios` and §15.10's
`maxArmySlotCount` verbatim — no new PARAMS type is introduced by this ticket.

## ⚠️ AMENDED 2026-08-28 — `navy` checkbox → `spawnsUnits`; the fleet editor is BLOCKED

**What this ticket used to specify.** §5's shared `ScenarioBody` field editor drew a `navy`
`Checkbox_UI`, and — shown when `navy == true`, collapsed/de-emphasized otherwise — a `navalFleet`
editor section: a flat list of `{templateIdentifier, count}` rows, a flat list of
`{armyName, side}` rows with a two-option `"West"`/`"East"` Combo, and one float field for
`sideBiasDistance`.

**Why it changed.** The 2026-08-27 rewrite of the live reference script deleted
`Scenario.SpawnNavalFleets` and every `NAVAL_*` constant; the vestigial `navy` field was removed
from the live Lua on 2026-08-28 after being confirmed to have zero readers
(`Pandemonium Isthmus_Scenarios_Script.lua:182-186`).
`ARCH_15_05_ParamsScenariosType.md`'s "RETIRED 2026-08-28" section retires `ScenarioNavalFleet`,
`ScenarioNavalFleetEntry`, `ScenarioNavalPondSide`, `ScenarioNavalPondAssignment`, and both
`ScenarioBody::navalFleet` and `ScenarioBody::navy`. **Those PARAMS members no longer exist, so
the fleet editor has nothing to bind to and would not compile.**

**Resolution, in two parts:**
1. The checkbox becomes `spawnsUnits` (§5), with a mandatory consequence caption — the flag is
   only half of a two-step opt-in and does nothing on its own.
2. ⚠️ **The `navalFleet` editor section is BLOCKED and must not be built or replaced.** Whether
   per-scenario unit spawning has *any* declarative PARAMS form is
   `ARCH_15_05_ParamsScenariosType.md` **OPEN item 1**, explicitly undecided: the live generator
   is hand-authored placement code (live terrain samples, a deepest-water spiral search, a
   rectangular grid layout), not a short list of tunable numbers. This ticket must not invent a
   `ScenarioUnitSpawn` struct, an instruction-list field, or a reuse of the existing
   `Params::UnitGroup`/`UnitTransform` family to keep the section alive. **A UI surface for
   per-scenario unit-spawn content is blocked pending ARCH §15.5 OPEN item 1.**

Everything else in this ticket is unaffected: the three-tier lists, match-rule editors,
reachability badges, spawns-acknowledgment warning, composition matrix, and `maxArmySlotCount`
settings never touched the naval family.

⚠️ **STATUS — this is a retirement pass over LANDED code, not a fresh build.** The ticket body
below still reads as "no scenario-authoring UI exists" (true when authored); it is no longer.
`src/ui/ScenariosTab_UI.h`, `ScenariosTab_Detail_UI.cpp`, and a dedicated
**`src/ui/ScenariosTab_DetailNaval_UI.cpp` (87 lines)** all exist and bind the retired members,
against `src/params/Scenario_PARAMS.h`'s still-present naval family. **The fleet-editor
translation unit is the thing to delete** — not a section to leave unbuilt. Scoping that deletion
across the landed PARAMS/IO/UI files is not authored here and needs its own work-order.

## Root problem
No scenario-authoring UI exists anywhere in `src/ui/` (confirmed: no `Scenario` match under
`src/ui/`). `Params::Scenarios` (STEP69) is otherwise reachable only by hand-editing a `.sanmap`'s
`Scenarios` JSON section — there is no in-app surface for the three-tier list, the mandatory-`spawns`
risk `MAP_SCENARIO_SPEC.md` §6 documents a real live regression from, or `maxArmySlotCount`.

## ⚠️ Two corrections to the original design, applied here, not silently followed

**1. `ScenarioSpawnsPolicy` does not exist in ratified PARAMS.** `ARCH_15_05_ParamsScenariosType.md` §15.5's `ScenarioBody`
carries `spawns` (a plain vector) and `authoringNote` (a plain string) — no tri-state intent enum,
confirmed by grep across `ARCH.md`/`SANMAP_FORMAT_SPEC.md`/STEP69 (zero matches for
`SpawnsPolicy`). **Resolution — derive the signal from existing fields, no new PARAMS:**

```cpp
// ScenariosTab_UI.h
// A Tier 1/2 scenario risks the live-documented 2h1ai regression (MAP_SCENARIO_SPEC.md §6) when
// it has no explicit spawns AND has not documented that as intentional in its own authoringNote
// (the exact mechanism ARCH_15_05_ParamsScenariosType.md §15.5 ratified `authoringNote` FOR — "carries forward §6's 'document
// that intent in the entry's own comment' as real data"). Tier 3 (defaultScenario) is NEVER
// flagged: its own empty `spawns` correctly means "inherit the .sanmap baseline," which IS its
// ratified, intended behavior (Format Expert answer 6) — flagging it would be a false-positive
// nag on every map.
inline bool ScenarioNeedsSpawnsAcknowledgment(const Params::ScenarioBody& body) {
    return body.spawns.empty() && body.authoringNote.empty();
}
```

The `[I understand, inherit baseline]` action (§4) satisfies this by writing a canned sentence into
`authoringNote` — real exported data (`AuthoringNote` JSON key, `authoringNote` Lua field, STEP70),
not a UI-only flag, so the acknowledgment survives save/reopen/export like every other field.

**2. `maxArmySlotCount < recipe.armies.size()`'s export-time warning has no IO-side home.**
`ARCH_15_10_SlotPatternConstructionMoves.md` §15.10 point 2 requires it; STEP70 §2b punted to "(c) let STEP71's orchestrator perform
this validation" as its recommended default, but the already-authored STEP71 contains **no mention
of `maxArmySlotCount` anywhere** — confirmed by reading it in full. A real gap in the authored IO
track, not something this ticket can assume covered. **Resolution: this ticket surfaces it itself**,
computed off `Params::MapRecipe` (UI already holds it), as an always-visible banner — not gated on
an export click, so it is seen the moment it becomes true:

```cpp
// Names armies at 1-based roster position > maxArmySlotCount. ⚠️ ASSUMES "army ID" == 1-based
// position in recipe.armies (declaration order) — the ONLY convention consistent with
// ARCH_15_10_SlotPatternConstructionMoves.md §15.10's own "maxArmySlotCount < recipe.armies.size()" framing. NOTE: STEP73 subsequently
// established that the engine derives slot order by ALPHABETICAL Army::name sort, so this
// positional assumption must be reconciled against STEP73's BuildArmyIdToNameTable when both
// land — flagged so that reconciliation is deliberate, not discovered.
inline std::vector<std::string> ArmiesExceedingSlotCount(const std::vector<Params::Army>& armies,
                                                         int maxArmySlotCount) {
    std::vector<std::string> affected;
    for (std::size_t index = static_cast<std::size_t>(maxArmySlotCount < 0 ? 0 : maxArmySlotCount);
         index < armies.size(); ++index)
        affected.push_back(armies[index].name.empty() ? "(unnamed army)" : armies[index].name);
    return affected;
}
```

**Flag to IO Architecture / ARCH Expert:** confirm STEP71 (or a follow-up) should ALSO emit this
into `ScenarioExportResult::debugLog` per §15.10's original text — a log-only record is still useful
even once the UI shows it live. Not built here; noted so it isn't lost between two experts' tickets.

## Fix

### 1. Tab shell — `src/ui/ScenariosTab_UI.h` (NEW)

```cpp
#pragma once
#include <string>
#include <vector>
#include "Combo_UI.h"
#include "ConfirmDialog_UI.h"
#include "DraggableListWidget_UI.h"
#include "Section_UI.h"
#include "UniqueNameList_UI.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../params/Scenario_PARAMS.h"       // STEP69

namespace SanmapGen {
namespace Pipeline { class PreviewDriver; }
namespace Ui {

enum class ScenarioSelectedTier { None, Pattern, Count, Default };

struct ScenariosTabState {
    SectionState settingsSection;      // maxArmySlotCount + its warning banner
    SectionState patternSection;       // Tier 1 DraggableList
    SectionState countSection;         // Tier 2 DraggableList
    SectionState defaultSection;       // Tier 3 fixed panel
    SectionState matrixSection;        // live composition preview
    // STEP77 adds its own runtime-script state as a SIBLING member here once it lands — this
    // ticket does not reserve a forward-declared, unused slot for it.

    ScenarioSelectedTier selectedTier  = ScenarioSelectedTier::None;
    int                  selectedIndex = -1;   // position in patternScenarios/countScenarios;
                                               // ignored when selectedTier == Default
};

// --- pure helpers, no imgui, testable headless ---
inline std::string NextScenarioName(int existingCount) { return NextUniqueLabel("Scenario", existingCount); }
inline const char* ScenarioRowLabel(const Params::ScenarioBody& body) {
    return body.name.empty() ? "Scenario" : body.name.c_str();
}
bool ScenarioNeedsSpawnsAcknowledgment(const Params::ScenarioBody& body);
std::vector<std::string> ArmiesExceedingSlotCount(const std::vector<Params::Army>&, int);
std::string ScenarioPriorityBadge(int zeroBasedRowIndex);   // "1st checked" / "2nd checked" / ...

// The pure conjunction evaluator mirroring the runtime's Lua EvaluateScenarioConditions EXACTLY
// (same field/comparator semantics) -- drives BOTH the live matrix (§6) and the reachability
// badges (§3), so they cannot disagree by construction (one function, two call sites).
// ⚠️ FLAGGED FOR ARCH: placed here, UI-local, on the same footing as ArmiesTab_UI.h's existing
// pure non-presentation helpers -- UI-authoring-only logic with no cross-domain reuse case, not a
// MATH/PIPELINE concern (it never touches DATA, never runs during generation). Confirm this
// placement rather than requiring MATH promotion (the original design's non-binding suggestion).
bool MatchesScenarioConditions(const std::vector<Params::ScenarioCountCondition>& conditions,
                               int total, int human, int ai);

void DrawScenariosTab(Params::MapRecipe& recipe, ScenariosTabState& state,
                      Pipeline::PreviewDriver* previewDriver);

} // namespace Ui
} // namespace SanmapGen
```

`previewDriver` is accepted for interface parity with every other tab but **never called** —
`recipe.scenarios` feeds no PROC stage, so no edit here trips a dirty flag or requests a regen.

### 2. `ScenariosTab_Lists_UI.cpp` (NEW) — the three tiers

- **Tier 1 "Exact Slot Patterns"** (`recipe.scenarios.patternScenarios`) —
  `DraggableList<Params::PatternScenario>::Render`. Reorder is **cosmetic only** — apply
  `ApplyDraggableListSignal` anyway (uniform widget contract) but the section header states:
  *"Order here is cosmetic — exact-match only, first-and-only match wins regardless of position."*
  No priority badge on these rows.
- **Tier 2 "Composition Rules"** (`recipe.scenarios.countScenarios`) — same widget; **every row
  label is `"<n>. <name>"` where `<n>` is `ScenarioPriorityBadge(rowIndex)`**, so label and position
  always agree (`ARCH_15_06_CountScenariosOrdering.md` §15.6 — array order **is** priority; `ApplyDraggableListSignal` on the
  real vector is the entire authoring mechanism, no shadow copy).
- **Tier 3 "Default (always matches)"** — one fixed `Section_UI` panel over
  `recipe.scenarios.defaultScenario`; no list widget, no create/duplicate/delete.
- **Create**: appends a fresh entry (name via `NextScenarioName`, `alloyMode` at its struct default
  `Occupancy`).
- **Duplicate** (new; no existing tab has it): deep-copies the selected row **into the position
  immediately below it in the same tier**, then uniquifies names. ⚠️ `MakeNamesUnique<T>` cannot be
  used directly — the `.name` member is nested at `.body.name`; add a two-line domain-local loop
  against `.body.name` or a thin adapter (coder's call; not worth generalizing
  `UniqueNameList_UI.h` for one nested-field case).
- **Delete**: forbidden on `defaultScenario` — no button drawn at all, not merely disabled.
- Selection: any row's Select signal sets `selectedTier`/`selectedIndex`; clicking inside the fixed
  Default panel sets `selectedTier = Default` directly.

### 3. `ScenariosTab_MatchRules_UI.cpp` (NEW) — Tier 1/2 rule editors + reachability badges

- **Tier 1 slot-pattern toggle row**: a bespoke `ImDrawList` row of `maxArmySlotCount` three-state
  buttons (click cycles `-`→`h`→`A`→`-`), tinted by `recipe.armies[i].armyColor` where
  `i < recipe.armies.size()`, grey beyond. Two pure helpers:
  ```cpp
  std::string BuildSlotPatternFromToggles(const std::vector<char>& toggles);
  std::vector<char> ParseSlotPatternToToggles(const std::string& pattern, int maxArmySlotCount);
  // Length mismatch (pattern authored before a maxArmySlotCount edit) degrades gracefully: short
  // pattern pads with '-', long pattern truncates -- NEVER crashes, NEVER silently writes back the
  // repaired string until the user actually edits a toggle (Constitution §6 — a load must not
  // mutate what it didn't touch).
  ```
- **Tier 2 clause table**: one row per `ScenarioCountCondition` — `Combo_UI` over the 3
  `ScenarioCountField` labels `{"Total","Human","AI"}`, `Combo_UI` over the 6 comparator display
  labels `{"=","≠",">","≥","<","≤"}` (**display only** — the enum index is stored, per Correction
  17's own note that the UI may render friendlier symbols without changing disk format), integer
  stepper for `value`. `+`/`x` add/remove. **AND-of-clauses only** (matches the flat vector and
  §15.5's ruling) — no OR-group UI; named future extension. Below: a read-only auto-generated
  summary, e.g. `"Matches when: total == 3 and human == 2"`.
- **Reachability badge** (Tier 2 rows and the Default row):
  ```cpp
  // Brute-forces every (total, human, ai) triple in [0, maxArmySlotCount] (human+ai == total, both
  // >= 0) and asks whether entryIndex is the FIRST scenario (by array order) matching ANY triple.
  // If none, the entry is unreachable -- either shadowed by an earlier rule or self-contradictory
  // (e.g. total==5 AND total==3); both surface identically (cause is not distinguished — keeps the
  // model simple). Tier 1 exact-pattern entries are DELIBERATELY excluded: a (total,human,ai)
  // triple maps to MANY slot arrangements, only some with a Tier-1 match — this answers
  // reachability against Tier 2/3 ONLY, matching the matrix's "may pre-empt" framing.
  bool IsCountScenarioReachable(const Params::Scenarios& scenarios, int countScenarioIndex);
  bool IsDefaultScenarioReachable(const Params::Scenarios& scenarios);
  ```
  Row label gains `" ⚠ Unreachable (shadowed by <name>)"` when false — "which earlier entry" is a
  second pass recording the first entry winning each contested triple, reporting the most common
  (tie-break is cosmetic, coder's call).

### 4. `ScenariosTab_SpawnsWarning_UI.cpp` (NEW) — three visibility tiers

1. **List-row badge**: prepend `"⚠ "` to any Tier 1/2 row where
   `ScenarioNeedsSpawnsAcknowledgment(entry.body)` is true (never the Default row).
2. **Detail-panel banner**: persistent amber banner for the selected Tier 1/2 scenario when true:
   *"No explicit spawn positions. This scenario will use whatever the .sanmap's shared baseline
   spawn currently is — which changes if ANY other scenario's baseline edit touches it."*
   - **[Set Explicit Spawns]** — seeds `body.spawns` from `recipe.armies` (one `ScenarioSpawn` per
     army, positions `0,0,0`). ⚠️ There is no "current baseline spawn" value reachable from
     `Params::MapRecipe` for this ticket to read; the canvas mode (STEP78) is what seeds REAL
     baseline positions. Until then this is a zeroed starting point the designer hand-edits via §5
     — a known, temporary UX gap, stated not hidden.
   - **[I understand, inherit baseline]** — appends (only if not already present, substring check)
     `"Acknowledged: intentionally inherits the .sanmap baseline spawn."` to `body.authoringNote`.
3. **Export-time gate** — **not built here**; STEP77 wires the predicate into the Files tab's
   existing `ConfirmDialogState`/`pendingConfirmAction` machinery. This ticket exports the predicate
   publicly (§1) so STEP77 calls it rather than duplicating the logic.

### 5. `ScenariosTab_Detail_UI.cpp` (NEW) — the shared `ScenarioBody` field editor

`DrawScenarioBodyFields(Params::ScenarioBody& body, const std::vector<Params::Army>& armies)`,
called by whichever tier's detail panel is open:

- `name` — text input.
- `area` — four float fields (`originX`/`originZ`/`width`/`length`), same per-field pattern
  `AreasTab_UI` uses. **`area.name` is never shown or edited** (STEP69 §1: no JSON counterpart,
  must stay empty).
- `spawnsUnits` — `Checkbox_UI` (amended 2026-08-28; was `navy`). **Requires an always-visible
  consequence caption directly beneath it** (Constitution §8 — a checkbox whose true state does
  nothing on its own is exactly the case that rule exists for):
  *"⚠ Two-step opt-in. Ticking this alone spawns nothing. The map's runtime script must also
  contain a branch matching this scenario's `name` — see MAP_SCENARIO_SPEC.md §11."*
  The caption must reference the scenario's **`name`** field specifically, since `name` is the
  dispatch key, not a cosmetic label. Do **not** attempt to validate that a matching branch
  exists: the branch lives in Lua text this ticket cannot see, and the file it lives in is itself
  unresolved (`ARCH_15_05` OPEN item 2). A caption, never a checked precondition.
- `alloyMode` — `Combo_UI` over four labels, **paired with an always-visible consequence card**
  (Constitution §8 label table):
  - `explicit` — "You list every army's alloys below. Any army NOT listed loses its alloy markers entirely."
  - `occupancy` — "Uses the map's own baked alloy positions. Empty army slots lose their markers; filled slots keep them."
  - `keepAll` — "Uses the map's own baked alloy positions. Nothing is ever deleted, even for empty slots."
  - `delta` — "⚠ Reserved — not yet used by any shipped scenario. Only listed Adds/Removes apply."
- `spawns` — flat list editor, no drag-reorder (order not load-bearing): `Combo_UI` over
  `armies[i].name` for `armyName` (falls back to a text field when `armies` is empty — never blocks
  authoring) + three float fields; `+ Add Spawn` / row `X`. Cardinality is tens of rows — plain
  loop, **no `VirtualListWidget_UI`** (that is for 100k rows; flagged so nobody over-engineers).
- `alloys`/`alloysToAdd` — same shape plus a free-text `markerName` per row. ⚠️ **Not** a validated
  picker against real placed markers — that fidelity is STEP78's; this is the honest lower-fidelity
  fallback that keeps authoring possible without it.
- `alloysToRemove` — `armyName` + `markerName` only (no position, matches the struct).
- `authoringNote` — `ImGui::InputTextMultiline`.
- ~~`navalFleet` — flat list of `{templateIdentifier, count}`; flat list of `{armyName, side}`
  (two-option Combo "West"/"East"); one float for `sideBiasDistance`.~~
  ⚠️ **BLOCKED — do not build, do not replace. 2026-08-28.** The bound members
  (`ScenarioBody::navalFleet` and its `ScenarioNavalFleet`/`ScenarioNavalFleetEntry`/
  `ScenarioNavalPondSide`/`ScenarioNavalPondAssignment` types) are retired by
  `ARCH_15_05_ParamsScenariosType.md`'s RETIRED section and no longer exist in `Params`.
  **Blocking item: `ARCH_15_05` OPEN item 1** — whether per-scenario unit spawning should have
  any declarative PARAMS form at all is an open design question, deliberately not decided; the
  live model is hand-authored Lua placement code plus inlined per-generator constants. **This UI
  section is blocked pending that ruling.** The coder implements the rest of `DrawScenarioBodyFields`
  with this bullet simply absent — no placeholder panel, no "coming soon" affordance, no
  re-shaped substitute editor. The `spawnsUnits` checkbox above is the entire unit-spawning
  surface this ticket ships.

### 6. `ScenariosTab_Matrix_UI.cpp` (NEW) — live composition preview

Triangular `ImDrawList` grid: rows = `total` (`0..maxArmySlotCount`), columns = `human`
(`0..total`), `ai` implied. Each cell resolved by walking `patternScenarios`→`countScenarios`→
`defaultScenario` in order (`MatchesScenarioConditions` for the Tier 2/3 pass). Tier 1 is **never
resolved by count alone** — a triple does not determine a slot pattern — so Tier 1 registration is a
**hatch overlay on the Tier2/3-resolved base color**, not a distinct fill, with an on-screen
tooltip: *"Hatched cells have at least one registered exact-pattern scenario that MAY pre-empt this
result — Tier 1 depends on which slots are filled, not just how many."* Hover shows the resolved
name. At the default 16 the grid is ≤17×17; no virtualization needed.

### 7. `ScenariosTab_Settings_UI.cpp` (NEW) — `maxArmySlotCount` + its warning

- Integer stepper bound to `recipe.scenarios.maxArmySlotCount`, range `[1, 64]` — the `64` is a
  **UI convenience ceiling only**, not a PARAMS/ARCH constraint (`ARCH_15_10_SlotPatternConstructionMoves.md` §15.10: "no fixed upper
  bound is imposed"); flagged so no coder mistakes it for ratified law.
- Amber banner, always visible (not export-gated) when `ArmiesExceedingSlotCount(...)` is non-empty:
  *"maxArmySlotCount (<n>) is smaller than the authored army roster (<m>). These armies can never
  appear in any slotPattern: <names>."* Never blocks, never auto-raises (`ARCH_15_10_SlotPatternConstructionMoves.md` §15.10 point 2).

## Files touched
- NEW `src/ui/ScenariosTab_UI.h`, `ScenariosTab_Lists_UI.cpp`, `ScenariosTab_MatchRules_UI.cpp`,
  `ScenariosTab_SpawnsWarning_UI.cpp`, `ScenariosTab_Detail_UI.cpp`, `ScenariosTab_Matrix_UI.cpp`,
  `ScenariosTab_Settings_UI.cpp`, `ScenariosTab_UI_Test.cpp`.
- EDIT `src/ui/Application_Panels_UI.h` — add `Scenarios` to `ApplicationPanel`, grouped in
  `ApplicationPanelGroup::Environment` immediately after `Areas`.
- EDIT `src/ui/Application_TabState_UI.h` — include + `ScenariosTabState scenarios;`.
- EDIT `src/ui/Application_PanelEnvironment_UI.cpp` — one new `case`, mirroring `Areas`/`Armies`.

## Backend policy
N/A — pure imgui-frame CPU work; `recipe.scenarios` edits never touch `Data::MapFields`, never call
`RequestMapUpdate()`/`NotifyParametersChanged()`, no compute dispatch.

## ARCH rules invoked
- `ARCH_15_05_ParamsScenariosType.md` §15.5 — every PARAMS shape edited here, verbatim, no new type.
- `ARCH_15_06_CountScenariosOrdering.md` §15.6 — reorder via `DraggableList` IS the match-priority authoring action; labels
  always agree with position.
- `ARCH_15_10_SlotPatternConstructionMoves.md` §15.10 — `maxArmySlotCount`'s field, default, never-clamp/never-auto-raise posture.
- `MAP_SCENARIO_SPEC.md` §6 — the hard mandatory-`spawns` requirement §4 implements.
- Constitution §8 — every alloyMode/comparator/field value gets a real label table.

## Explicit out-of-scope
- **Runtime Lua editor, `ExportScenarioScript`, `gameInstallRoot` UI, result surfacing** — STEP77.
- **Interactive canvas marker editing** — STEP78, gated on STEP47/50-53.
- **Any new PARAMS field**, including a resurrected `ScenarioSpawnsPolicy` — Correction 1 is the
  ruling implemented instead. Equally: **no resurrection of the retired `ScenarioNavalFleet`
  family under a renamed shape**, and no substitute per-scenario unit-spawn content editor —
  blocked on `ARCH_15_05` OPEN item 1 (see the 2026-08-28 amendment).
- **A real baseline-spawn seed for [Set Explicit Spawns]** — flagged gap, needs STEP78.
- **OR-of-clause-groups for Tier 2** — named future extension.

## Acceptance test
New `src/ui/ScenariosTab_UI_Test.cpp`:
1. `ScenarioNeedsSpawnsAcknowledgment`: empty spawns + empty note → true; empty spawns + non-empty
   note → false; non-empty spawns + anything → false.
2. `ArmiesExceedingSlotCount`: 5-army roster, `maxArmySlotCount = 3` → exactly indices 3/4, in
   order. `>= 5` → empty. Negative treated as 0 (never crashes, never negative-indexes).
3. `BuildSlotPatternFromToggles`/`ParseSlotPatternToToggles` round-trip for a mixed length-16
   pattern. Shorter stored pattern pads with `-`; longer truncates — neither crashes.
4. `MatchesScenarioConditions`: conjunction semantics for all 6 comparators × all 3 fields, matching
   the runtime's Lua truth table exactly. Empty `conditions` is vacuously true.
5. `IsCountScenarioReachable`: `[total==3]` then `[total>=0]` → first entry reachable (checked
   first, wins those triples). `[total>=0]` then `[total==3]` → **second** entry unreachable — the
   exact shadowing case `MAP_SCENARIO_SPEC.md` §4 names.
6. `ScenarioPriorityBadge(0)` == `"1st checked"`, `(1)` == `"2nd checked"`.
7. Duplicate inserts an equal-content copy at index 1, names uniquified.
8. Selecting the Default panel sets `selectedTier == Default`; `selectedIndex` never read for it.
9. Full solo rebuild + `ctest -C Debug`: previously-passing suite stays green; new target passes.

## Verify
- New test passes. Full solo rebuild + `ctest -C Debug`: 100% pass, zero pre-existing test files
  edited or broken.
- Confirm `ApplicationPanel::Scenarios` reaches the left column and dispatches without touching any
  other panel's `case` (grep-checkable).

---

## Amendment (STEP76)

**The ⚠️ ASSUMPTION flagged inside `ArmiesExceedingSlotCount` (§ "Two corrections", correction 2) is
now RECONCILED — both tickets were always correct, and the flag can be cleared at implementation
time.**

That comment flagged a conflict it could not resolve on its own: this ticket assumes **"army ID" ==
1-based position in `recipe.armies`** (declaration order), while
`work_orders/STEP73_ScenarioAlloyRosterRender_IO.md` §0 established that the engine derives slot
order by an **alphabetical `Army::name` sort**. Two different-looking conventions, with no rule
forcing them to agree.

`work_orders/STEP76_ArmyIdentityNaming_IO.md` supplies the missing rule. Under STEP76, `Army::name`
becomes a machine-owned, never-human-settable `ARMY_XX` identity, minted from the army's 1-based
roster position and zero-padded so that **an alphabetical sort of the roster's names is identical to
roster order** — that equality is STEP76 ruling 4, and it is the single invariant STEP76's primary
acceptance test exists to protect (at 9, 10, 11, and 16 armies).

So once STEP76 lands, alphabetical order and roster position are **the same thing**, by
construction. `ArmiesExceedingSlotCount`'s positional walk and STEP73's `BuildArmyIdToNameTable`'s
alphabetical sort provably return the same answer. Neither needs to change.

**At implementation time:** delete the `⚠️ ASSUMES ... must be reconciled against STEP73's
BuildArmyIdToNameTable when both land` sentence from that comment and replace it with a reference to
this reconciliation — e.g. *"1-based roster position == alphabetical name order, guaranteed by
STEP76's machine-minted `ARMY_XX` identity (STEP76 ruling 4)."* Keep the function body exactly as
written; the code was never wrong, only unproven.

**Nothing else in STEP74 changes.** One knock-on worth knowing but explicitly NOT this ticket's to
handle: STEP76 splits the human-authored label off `Army::name` onto a new `Army::displayName`. Any
STEP74 surface that shows an army to the user (the slot-toggle row tints, the `[Set Explicit Spawns]`
seeding) should render `displayName` while continuing to store `Army::name` as the `armyName` key —
STEP76 §3b establishes that display/identity split for `ArmiesTab_UI`, and STEP74 follows the same
convention when it is built. STEP74's stored data is unaffected either way.
