# DRAFT — Format Expert spec-refresh pass for the 2026-09-03 scenario file-split amendment

**Status: UNAPPLIED DRAFT.** Produced by the Format Expert (advisory/read-only consult) on
2026-09-03, to be applied by the ARCH Expert (the only agent permitted to write under
`sangen_arch_pack/`) or by a human. This file is not itself authoritative — it exists so the
edits below can be reviewed and applied verbatim or near-verbatim, then deleted.

**Why this exists:** `ARCH_15_04_ThreeFileOnDiskShape.md` and `ARCH_15_05_ParamsScenariosType.md`
were amended 2026-09-03 to add a fourth on-disk category (per-scenario hand-authored generator
files) and resolve `ARCH_15_05`'s OPEN item 2. `sangen_arch_pack/specs/MAP_SCENARIO_SPEC.md` and
`sangen_arch_pack/INDEX.md`'s topic row still describe the pre-amendment three-file shape and need
to catch up. The ratifying session flagged this explicitly as "not fixed here — not my file."

---

## 1. Edits to `sangen_arch_pack/specs/MAP_SCENARIO_SPEC.md`

### §2 — replace the "File shape" row (line 42)

Old:
```
| File shape | 2 files: `_data.lua` + `_Scenarios_Script.lua` | 3 files: `_data.lua` + `_Scenarios_Runtime.lua` + `_Scenarios_Data.lua` | `ARCH_15_04` — not migrated |
```

New:
```
| File shape | 2 files: `_data.lua` + `_Scenarios_Script.lua` | 4 categories: `_data.lua` + `_Scenarios_Runtime.lua` + `_Scenarios_Data.lua` + one `_Scenarios_<ScenarioName>.lua` per `spawnsUnits == true` scenario (splittable to `_<FunctionName>.lua`) | `ARCH_15_04` (amended 2026-09-03, category 4) — not migrated |
```

**Optional bonus fix (flagged, not required):** line 31, "Two live files today, three under ratified
ARCH law (§9)." → "…four categories under ratified ARCH law (§14)." The `§9` citation also looks
like a pre-existing wrong section reference (§9 is the `ARMY_XX` naming section; file shape is §14).

### New §11.2 — insert after line 509, before `## 12. Complete worked example…`

Keep §11/§11.1 exactly as-is (still correct: the live reference file's hardcoded `if/elseif` chain
is exactly what runs today, un-migrated). Insert:

```markdown
### 11.2 Ratified target — category 4 + generic lazy dispatch (`ARCH_15_04` amended 2026-09-03, resolves `ARCH_15_05`'s OPEN item 2)

The live `if/elseif` chain in §11 above is what runs today, in the un-migrated two-file live
reference — nothing changes there until a human migrates the map (§14, §2.2). The **ratified
target** it migrates to eliminates the `if/elseif` chain entirely, replacing §11's Step 2 with a
different mechanism:

- **Step 1 is unchanged.** `spawnsUnits = true` on the scenario record is still the opt-in flag,
  still meaningless alone.
- **Step 2 is no longer "add a branch."** Every `spawnsUnits == true` scenario instead gets its own
  hand-authored file, `<MapName>_Scenarios_<ScenarioName>.lua` (splittable to
  `<MapName>_Scenarios_<ScenarioName>_<FunctionName>.lua` under a 100-line-soft/150-line-hard
  ceiling — `ARCH_15_04`'s category 4), exposing a **global** `GenerateScenarioUnits(area)` that
  returns the same flat `{armyIndex, templateIdentifier, x, y, z}` instruction array
  `Scenario.SpawnUnits` already consumes. SanGen scaffolds this file once if missing, then never
  touches it again (§14's third overwrite-safety class) — the direct replacement for hand-editing
  an `elseif` branch into the old monolithic script.
- **Dispatch becomes generic and lives entirely in `_Scenarios_Runtime.lua`** (category 2), never
  edited per map or per scenario:

  ```lua
  function Scenario.SpawnMatchedScenarioUnits(area)
      if not currentMatchedScenarioName then return end
      local path = string.format("maps/%s/%s_Scenarios_%s.lua",
          currentMapName, currentMapName, currentMatchedScenarioName)
      local importOk, generatorModule = pcall(Import, path)
      if not importOk or not generatorModule or not generatorModule.GenerateScenarioUnits then
          return  -- no such file: this scenario did not opt into unit spawning -- not an error
      end
      local buildOk, instructions = pcall(generatorModule.GenerateScenarioUnits, area)
      if buildOk and instructions then
          Scenario.SpawnUnits(instructions)
      end
  end
  ```

  `Import()`'d **only for the one matched scenario** (`currentMatchedScenarioName`) — never eager,
  never a name→function table built up front, never an enumeration of the full authored
  `Scenarios` set. A missing file for a `spawnsUnits == false` scenario is the silent, expected
  common case; a missing file for `spawnsUnits == true` is a real authoring gap that `pcall`
  degrades to "no units spawned," not an abort — the same per-call `pcall` ordering law §3.1
  already states.
- The **executor/generator split** (`Scenario.SpawnUnits`, §11's closing paragraph) is unaffected —
  only the mechanism that decides which generator function runs for the matched scenario changes.

This closes the gap §11.1 below used to flag as an open ARCH question: where per-scenario dispatch
and generator code lives under the file split now has a ratified answer (`ARCH_15_04`'s category 4;
`ARCH_15_05`'s matching `ScenarioBody::name` filesystem-safety-charset amendment, §6). It is not yet
built or migrated (§2) — the live reference's hardcoded chain in §11 above is exactly what still
runs today.
```

### §11.1 — replace the final bullet (lines 504-509)

Old:
```
- **`ARCH_15_05` §15.5's `ScenarioNavalFleet`/`ScenarioNavalPondSide`/`ScenarioNavalPondAssignment`
  types and `ScenarioBody::navy` were shaped on 2026-08-21 from a live read of a `SpawnNavalFleets`
  body that no longer exists.** This spec does not and cannot amend the ARCH. **Action required:
  the ARCH Expert should review §15.5 against the current `spawnsUnits` model.** The parallel
  `Params::Scenarios` field is `navy` + `navalFleet`; the live field is `spawnsUnits` + a
  name-keyed dispatch.
```

New:
```
- **`ARCH_15_05` §15.5's `ScenarioNavalFleet`/`ScenarioNavalPondSide`/`ScenarioNavalPondAssignment`
  types and `ScenarioBody::navy` were shaped on 2026-08-21 from a live read of a `SpawnNavalFleets`
  body that no longer exists.** **Resolved:** the ARCH Expert's review landed as `ARCH_15_05`'s own
  "RETIRED 2026-08-28" section (formally retiring all four types and `navy`) plus its "AMENDED
  2026-09-03" section resolving where per-scenario dispatch/generator code lives under the file
  split (§11.2 above, `ARCH_15_04`'s category 4). The parallel `Params::Scenarios` field is now
  `spawnsUnits` + a per-scenario category-4 file, matching the live `spawnsUnits` + name-keyed
  dispatch shape.
```

### §6 — replace the `name` field row (line 269)

Old:
```
| `name` | string | all | `SCEN:378`, `:399`, `:404` | Log identifier **and** the dispatch key for unit spawning (§11) — not cosmetic. |
```

New:
```
| `name` | string | all | `SCEN:378`, `:399`, `:404` | Log identifier **and** the dispatch key for unit spawning (§11) — not cosmetic. **Ratified target only** (`ARCH_15_05`, amended 2026-09-03 — not yet migrated, §2): under the category-4 file split, `name` is also string-formatted directly into a filesystem path (`<MapName>_Scenarios_<name>.lua`, §11.2) and therefore carries a validation rule the live two-file shape does not yet enforce — safe-filename charset `^[A-Za-z0-9_]+$` (digits may lead; every live name including `1v1` already satisfies it) plus case-insensitive uniqueness across the whole `Scenarios` set. |
```

### §13 — add a new row (insert after the existing "Three-file split…" row, after line 631)

```
| §2/§14 described the ratified target as a fixed three-file split with no answer for where per-scenario dispatch/generator code lives | `ARCH_15_04` amended 2026-09-03: a fourth category, `_Scenarios_<ScenarioName>.lua` (splittable to `_<FunctionName>.lua`), holds per-scenario generator code; dispatch is a generic lazy `Import()`-by-name mechanism living in `_Scenarios_Runtime.lua`, never an `elseif` chain, in the ratified target | Still not migrated — the live reference script's hardcoded `if/elseif` chain (§11) is exactly what runs today. §2, §11.2, §14 |
```

### §14 — full replacement of lines 639-673

```markdown
## 14. SanGen ownership and on-disk shape (ratified law — not yet built)

Per `ARCH_15_03`/`ARCH_15_04`/`ARCH_15_10`. SanGen owns scenario **data**, rendered to Lua on
export; it never parses Lua back (option (c) — no Lua parser in the import direction), with exactly
one narrow carve-out ratified 2026-08-29: `ARCH_15_11` permits a human-triggered, non-executing
extraction of **area rectangles only** from a foreign scenario `.lua` SanGen never writes.

**Four on-disk categories**, per `ARCH_15_04` (amended 2026-09-03 to add category 4):

| # | File | Role | Written by SanGen? |
|---|---|---|---|
| 1 | `<MapName>_data.lua` | Orchestrator — load-scope gate, lobby read, counts, `NewThread`, wiring | **Never, under any code path.** |
| 2 | `<MapName>_Scenarios_Runtime.lua` | Generic algorithm — `BuildSlotPattern`, `FindMatchingScenario`, `ApplyScenario`, `ResolveAndApply`, the spawn executor, the generic lazy per-scenario dispatcher (`Scenario.SpawnMatchedScenarioUnits`, §11.2), any universal per-scenario helpers, all tuning constants. Identical across every map. | Yes — bundled resource, copied per export. |
| 3 | `<MapName>_Scenarios_Data.lua` | Per-map tables — `PATTERN_SCENARIOS`/`COUNT_SCENARIOS`/`DEFAULT_SCENARIO`/`MAX_ARMY_SLOT_COUNT`, rendered from `Params::Scenarios` (`ARCH_15_05`, `ARCH_15_10` §2). | Yes — fully regenerated per export, never hand-edited, never read back. |
| 4 | `<MapName>_Scenarios_<ScenarioName>.lua` (splittable to `<MapName>_Scenarios_<ScenarioName>_<FunctionName>.lua`) | One per `spawnsUnits == true` scenario — hand-authored per-scenario unit-spawn generator exposing a global `GenerateScenarioUnits(area)` (§11.2). Added 2026-09-03; resolves the "where does per-scenario generator code live" gap §11.1 used to flag as open. | **Scaffold-once-if-missing, then never again** — a third, distinct overwrite-safety posture, below. |

- **All four categories colocated in `LJ/lua/maps/<MapName>/`.** Cross-tree `Import` is impossible —
  the map's asset folder (`Sanctuary_Data/Maps/<MapName>/`) is unreachable
  (`MAP_UNIT_SPAWNING_SPEC` §3, disproved via `Engine.FileExists`). This is not a preference; none
  of the four categories can be separated from the others.
- The generated data file (category 3) declares its tables as **globals**, for the same `Import()`
  global-capture reason as `Scenario` (§10). Category 4 files are `Import()`'d the same way and
  expose their `GenerateScenarioUnits` global identically.
- **Overwrite safety — three classes** (`ARCH_15_04`, amended 2026-09-03 to add the third):
  1. **Categories 2-3 (`_Scenarios_Runtime.lua`/`_Scenarios_Data.lua`) — always regenerate, refuse
     on foreign-marker collision.** (a) filename disjointness — SanGen writes only these two
     literal paths, never `_data.lua` nor the legacy `_Scenarios_Script.lua`; (b) a
     machine-checkable generated-file banner token in both; (c) on an unrecognized occupant,
     refuse to write **that file only**, log loudly by path, and continue the rest of the export.
  2. **Category 1 (`_data.lua`) — never write, ever, under any code path.**
  3. **Category 4 (`_Scenarios_<ScenarioName>.lua`) — scaffold-once-if-missing, then permanently
     hands-off.** Collision detection is a **glob/pattern match**
     (`<MapName>_Scenarios_*.lua`, excluding the two category-2/3 literal names), not the literal
     two-name check categories 2-3 use — used only to detect "does a generator file already exist
     for this scenario," never to decide whether to overwrite it. On export, for every
     `spawnsUnits == true` scenario whose expected generator file does not yet exist, SanGen writes
     a minimal scaffold (`GenerateScenarioUnits(area) return {} end`) under a distinct, weaker
     banner (`-- SANGEN-CREATED STARTING POINT -- freely hand-edit -- never regenerated`), once. On
     every subsequent export, if the file exists in any state (untouched stub, hand-edited, no
     marker at all), SanGen never touches it again. Creating a scaffold is logged exactly as loudly
     as writing categories 2-3; skipping an already-present category-4 file is a quiet, expected
     no-op — the common case on every export after the first.
  All three classes are write-target safety refusals — distinct from Constitution §6's import-time
  "a version marker is never grounds to refuse the file."
- **File-size ceiling — category 4 only.** Soft 100 / hard 150 lines per category-4 file, functions
  ≤ 40 lines; a generator function that would not fit splits one-file-per-function
  (`<ScenarioName>_<FunctionName>.lua`). Explicitly **not** retroactive to `_Scenarios_Runtime.lua`
  (already 309 lines) or `_Scenarios_Data.lua`'s renderer output.
- **`COUNT_SCENARIOS` array order is the authoring action for match priority** (`ARCH_15_06`), so the
  UI surface must be a reorderable list — never a set or an unordered table. §5.1 is why.
- ⚠️ **`<map>_data.lua` is engine-writable.** `host/testUtils.lua` can `table.save` over it as
  machine-generated `MapData = { … }`, destroying every hand-written line
  (`MAP_UNIT_SPAWNING_SPEC` §2a). Keep backups.
- **Migration of the live map is a one-time human action** (`ARCH_15_04`, `ARCH_15_10` §1): author the
  data in SanGen preserving `COUNT_SCENARIOS` order exactly and set `maxArmySlotCount = 16`; export
  once; then hand-edit `_data.lua` to retarget its `Import()`, change the `ResolveAndApply` call to
  pass `playersInformation`, and delete its own `BuildSlotPattern`. For any scenario with
  `spawnsUnits = true` (today, `slots5to8AnyFilled`), the human must also hand-carry its existing
  generator function (e.g. `BuildSlots5to8Instructions`) out of the legacy monolithic script into
  its own new `<MapName>_Scenarios_<ScenarioName>.lua` file, renaming its entry point to the
  required global `GenerateScenarioUnits(area)` (§11.2) — SanGen's create-if-missing scaffold only
  writes an empty stub, it never migrates existing logic for you. SanGen never deletes the orphaned
  legacy file — as forbidden as overwriting one.
```

### §15 — replace the `ARCH_15_MapScenarioSystem.md` bullet (lines 685-686)

Old:
```
- `ARCH_15_MapScenarioSystem.md` §15 and subsections §15.1-§15.11 — the binding law. §15.5 needs
  review against §11.1 of this spec.
```

New:
```
- `ARCH_15_MapScenarioSystem.md` §15 and subsections §15.1-§15.11 — the binding law.
  `ARCH_15_04_ThreeFileOnDiskShape.md` §15.4 and `ARCH_15_05_ParamsScenariosType.md` §15.5 were
  both amended 2026-09-03, adding the category-4 on-disk file (§11.2, §14) and the matching
  `ScenarioBody::name` filesystem-safety-charset/uniqueness validation rule (§6) — resolving the
  "where does per-scenario dispatch/generator code live" question this spec previously flagged
  (§11.1) as needing ARCH-Expert review.
```

---

## 2. Edit to `sangen_arch_pack/INDEX.md`

Only line 12 (the topic-row) needs a change — its own narrative at lines 257-279 already documents
the amendment correctly and explicitly says line 12 is the outstanding gap.

Old (line 12):
```
| the Map Scenario system — `<MapName>_data.lua`/`<MapName>_Scenarios_Runtime.lua`/`<MapName>_Scenarios_Data.lua` three-file split, module API contract, three-tier scenario matching, `alloyMode` semantics, the mandatory-`spawns` hard requirement, execution/timing law, the ratified export-only IO design (`Params::Scenarios`, overwrite safety, ARCH §15) | `specs/MAP_SCENARIO_SPEC.md` |
```

New (line 12):
```
| the Map Scenario system — `<MapName>_data.lua`/`<MapName>_Scenarios_Runtime.lua`/`<MapName>_Scenarios_Data.lua`/`<MapName>_Scenarios_<ScenarioName>.lua` four-category on-disk split (category 4 added 2026-09-03, `ARCH_15_04`), module API contract, three-tier scenario matching, `alloyMode` semantics, the mandatory-`spawns` hard requirement, execution/timing law, the ratified export-only IO design (`Params::Scenarios`, overwrite safety incl. the category-4 scaffold-once class, ARCH §15) | `specs/MAP_SCENARIO_SPEC.md` |
```

Once this lands, the "Not yet updated by this ratification pass" clause at `INDEX.md` lines
276-279 is stale and should be deleted or marked resolved — that's the ARCH Expert's own narrative
prose about the ARCH's own history, not `.sanmap`/format truth, so it wasn't drafted here.

---

## 3. Flags for the ARCH Expert / human to double-check before applying

1. **File location check**: `ARCH_15_04_ThreeFileOnDiskShape.md` and `ARCH_15_05_ParamsScenariosType.md`
   physically resolve at repo root, not under `sangen_arch_pack/`, even though `INDEX.md`'s
   citations imply they live alongside every other `ARCH_NN_*.md`. Worth confirming this is
   intentional (not a stray/duplicate copy) before treating the Format Expert's reads as
   authoritative.
2. Two additions beyond a literal, minimal diff: the "File-size ceiling — category 4 only" bullet
   in §14, and one added sentence in §14's migration bullet about hand-carrying
   `BuildSlots5to8Instructions` into a new category-4 file. Both check out against the ratified
   ARCH text; drop them if a tighter diff is preferred.
3. Optional bonus fix at MAP_SCENARIO_SPEC.md line 31 (see §1 above) — a pre-existing wrong
   section citation (`§9` should be `§14`), unrelated to this amendment; fix opportunistically or
   leave for a separate pass.
