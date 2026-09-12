# STEP266 — Files tab "Import Scenario Data" action + review/assign panel

**Layer:** UI. **Domain:** `src/ui/FilesTab_Draw_UI.cpp`, `src/ui/FilesTab_Actions_UI.cpp`,
`src/ui/FilesTabState_UI.h` (or wherever `FilesTabState`/`FilesTabAction`/`FilesTabBrowseKind` are
declared), new `src/ui/ScenariosTab_ImportReview_UI.cpp` (or similar — a new review/assign panel).
**Executor:** SanGen Coder. **Sequence:** depends on `STEP265` (the orchestrator this wires to).
Closes out the scenario-import track (`STEP260`-`STEP266`).

## 0. Why

`STEP265` builds `ImportFullScenarioDataFromScenarioScriptFile`, callable but unreachable from the UI.
This ticket wires it in, following the **already-shipped** area-import precedent
(`FilesTab_Draw_UI.cpp:62-65`, `FilesTab_Actions_UI.cpp:151-158` — `RunImportScenarioAreas`), not the
export-row shape (`FilesTab_ScenarioExportRow_Draw_UI.cpp`), since this is an import action living in
the Files tab's existing "Open" section.

## 1. Files tab wiring

New `FilesTabBrowseKind` entry (e.g. `ScenarioFullDataImportLua`) and a new `FilesTabState` field
(e.g. `scenarioFullDataImportPath`), mirroring `ScenarioAreaImportLua`/`scenarioAreaImportPath`
exactly. New `FilesTabAction::ImportScenarioFullData`, labeled e.g. `"Import Scenario Data (Conditions/
Patterns/Placements)"` in `FilesTabActionLabel` (`FilesTab_Actions_UI.cpp:209-218`).

In `DrawOpenSection` (`FilesTab_Draw_UI.cpp`), add a new path row + button immediately after the
existing area-import row (`:62-65`), same shape:
```cpp
DrawFilesTabPathRow("Scenario Full Data Import Lua", FilesTabBrowseKind::ScenarioFullDataImportLua,
                    state.scenarioFullDataImportPath);
if (ImGui::Button(FilesTabActionLabel(FilesTabAction::ImportScenarioFullData))) {
    const bool bSucceeded = RunFilesTabAction(FilesTabAction::ImportScenarioFullData, state, recipe, ...);
    ...
}
```

New `RunImportScenarioFullData(FilesTabState& state, Params::MapRecipe& recipe)` in
`FilesTab_Actions_UI.cpp`, mirroring `RunImportScenarioAreas` (`:151-158`): calls
`Io::ImportFullScenarioDataFromScenarioScriptFile(state.scenarioFullDataImportPath, recipe.mapSize,
recipe)`, appends `result.debugLog`-style output via `AppendFilesTabLog`, and — new relative to the
area-only action — **stashes the three non-auto-attached candidate lists** (`matchConditions`,
`slotPatterns`, `unitPlacements`) into new `FilesTabState` fields so the review panel (§2) can present
them on the next frame. Wire the new action into `RunFilesTabAction`'s dispatch (`:232-238`).

## 2. Review/assign panel

Given `ARCH_15_14` Part B's explicit ruling that wiring extracted candidates to a scenario is "a
separate human authoring action," this panel is the human-gated wiring step — never automatic.

New section (in the Scenarios tab, or a dedicated panel — coder's call on exact placement, but it must
be reachable without leaving the app after an import runs) showing, per stashed candidate list:

- **Match conditions**: each extracted `std::vector<ScenarioCountCondition>` as one row, with its
  extraction context tag (from `STEP264` §1) shown as a label, and a **target scenario picker**
  (dropdown over `recipe.scenarios.countScenarios[].body.name` /
  `recipe.scenarios.patternScenarios[].body.name` / the default scenario) plus an "Append to selected
  scenario's conditions" button — appends the whole condition-set to that `CountScenario`'s
  `conditions` vector (only meaningful for a `CountScenario` target; disable/hide the button when a
  `PatternScenario`/default-scenario target is selected, since those don't have a `conditions` field).
- **Slot patterns**: each extracted `{name, slotPattern}` pair as one row, with a target
  `PatternScenario` picker and a "Set as selected scenario's slotPattern" button (overwrites, with a
  confirmation if the target already has a non-empty `slotPattern` — this is a destructive overwrite of
  existing authored data, unlike the additive append above).
- **Unit placements**: each extracted `ScenarioUnitPlacement` (or the whole batch at once — coder's
  call, but batch-append is the simpler v1) with a target scenario picker (any of
  `CountScenario`/`PatternScenario`/default, all have `body.unitPlacements`) and an "Append to selected
  scenario's unit placements" button.

No auto-selection of a "best guess" target scenario — the human always picks explicitly, consistent
with `ARCH_15_14`'s "never inferred" posture for this wiring step.

## 3. Partial-success reporting

Extend the existing `debugLog`/`AppendFilesTabLog` banner pattern
(`FilesTab_ScenarioExportRow_Draw_UI.cpp:46-61`'s per-flag banner style) with one line per shape,
never a single pass/fail bool — mirror `ScenarioAreaExtractionResult`'s own multi-field shape, extended
per-shape:
```
Areas: 4 imported, 0 collisions, 0 near-misses.
Match conditions: 7 candidate sets extracted, 1 near-miss (see log for reason).
Slot patterns: 0 candidates extracted.
Unit placements: 0 candidates extracted (no literal-shaped table found -- expected for the reference file).
```
Each near-miss's `reason` string (already produced by the `STEP264`/`STEP265` extractors) is appended
to the log, not swallowed.

## 4. Tests

Given this ticket touches imgui-drawn UI, follow this tab's existing split between pure signal-handling
logic (test directly, no GL frame) and drawing (GL-backed acceptance test only where no such split
exists — check `FilesTab_Actions_UI_Test.cpp` if present for the harness pattern):

1. `RunImportScenarioFullData` on a fixture file populates `state`'s three candidate-list fields and
   `recipe.areas` (via the existing area-import behavior, unchanged) in one call.
2. Appending a candidate match-condition-set to a chosen `CountScenario` adds exactly that vector to
   `body.conditions`, leaving every other scenario's `conditions` untouched.
3. Setting a candidate slot pattern onto a `PatternScenario` that already has a non-empty
   `slotPattern` requires the confirmation step (does not silently overwrite on the first click).
4. Appending a candidate unit-placement batch to a chosen scenario adds exactly those rows to
   `body.unitPlacements`.
5. The log banner reports per-shape counts and near-miss reasons for a fixture exercising all three
   shapes plus at least one near-miss per shape.

## 5. Out of scope

- Any change to the extractors' own recognized grammar — `STEP264`/`STEP265` own that.
- Per-row (rather than per-group) selection UI for match-condition/unit-placement batches — batch-level
  append is sufficient for v1; a finer-grained picker is a future enhancement, not required here.
- Any automatic/inferred target-scenario selection.

## 6. Files touched

**New:** `src/ui/ScenariosTab_ImportReview_UI.cpp` (or coder's chosen name/placement per §2), plus its
test file.

**Modified:** `src/ui/FilesTab_Draw_UI.cpp`, `src/ui/FilesTab_Actions_UI.cpp`, the `FilesTabState`/
`FilesTabAction`/`FilesTabBrowseKind` declaration file(s).
