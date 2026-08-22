# STEP82 — `ArmySpawnMarkerValidation_IO`: warn at export when an Army has no matching Spawn marker

*Constitution §7. Author: SanGen Format Expert. Executor: SanGen Coder.*

**Layer:** IO. **Accuracy class:** N/A (no computation — a name-membership scan over `Params::MapRecipe`).
**Domain:** export-time authoring diagnostic. **No new PARAMS field, no new `.sanmap` key, no format
change** — this ticket adds nothing to the wire format and reads nothing that is not already
round-tripped today.

**Sequence: land `work_orders/STEP76_ArmyIdentityNaming_IO.md` FIRST.** STEP76 makes the
engine-facing army identity an auto-generated `ARMY_XX` and normalizes legacy `Army1`-style names on
import, rewriting the matching `markers.Spawn.transforms` keys as it goes. That rewrite *changes
which armies look orphaned*: an army whose name STEP76 normalizes, paired with a spawn key STEP76
rewrites to match, is not an orphan and must not be reported as one. This ticket's scan runs on
whatever names exist at export time and has no opinion about them; running it before STEP76 would
produce a burst of warnings that STEP76 then silences. **Army naming is entirely STEP76's surface —
specify none of it here.**

## Root problem

The `.sanmap`'s `markers.Spawn.transforms` inner dictionary is **keyed by army name**
(`work_orders/STEP49_ManualMarkersUI.md:34-35`, confirmed live in-game). The game resolves an army's
start position by looking that army's own name up as a key in that dictionary. There is no linkage
field in either direction — `Params::Army` (`src/params/Army_PARAMS.h:43-52`) carries no
`spawnMarkerName`, and `Params::MarkerTransform` (`src/params/MarkerInstance_PARAMS.h:17-21`)
carries no army field. The association is **inferred purely by string equality of the two names**,
and it is inferred at load time by the engine, not by SanGen.

The consequence: a designer who authors an Army and forgets its Spawn marker gets a `.sanmap` that
exports cleanly, imports cleanly, round-trips perfectly — and silently gives that army no start
position in-game. Nothing anywhere in SanGen says a word about it. Both halves are written by
independent, name-keyed builders that never consult each other:

- `src/io/MapExporter_Armies_IO.cpp:75` — `armies[army.name] = armyJson;`
- `src/io/MapExporter_Markers_IO.cpp:50` — `markers[group.name] = groupJson;`
- `src/io/MapExporter_Markers_IO.cpp:45` — `transforms[markerTransform.name] = ...`

`work_orders/STEP49_ManualMarkersUI.md:86-87` flagged this as "a possible future ticket, same class
as the existing `blueprintPath` warn-dialog, not built here." `ARCH_16_08_SpawnArmyShrink.md`
routed the decision to the Format Expert. This is that ticket.

### Scope ruling: per-Army, not per-group

The scan iterates **armies**, asking of each "does a Spawn transform with my exact name exist?" It
does **not** iterate marker groups asking "does the Spawn group exist?" The per-Army form subsumes
the missing-group case for free: with no `"Spawn"` group authored at all, every army fails the test
and every army is named in one warning. A per-group check would catch only the total absence and
miss the far more common single-army omission.

## ⚠️ This is UX polish. It is NOT correctness-critical, and it MUST NOT block or mutate anything.

`ARCH_16_08_SpawnArmyShrink.md` §16.8 ratifies that **an Army with no matching Spawn marker is a
legal, tolerated state** — "already a legal, unremarkable state today," with no flag anywhere that
could go stale, because no linkage field exists in either direction. It is a soft in-game degrade
("that army gets no commander" — `STEP49_ManualMarkersUI.md:49-50`), not a corrupt file.

Therefore, and stated plainly so no Coder over-implements this ticket:

- **NEVER auto-create** a Spawn marker for an orphaned army.
- **NEVER auto-delete** an orphaned army.
- **NEVER auto-rename** anything to make the two sides match. (Renaming is STEP76's, and even
  STEP76 does it on *import*, not here.)
- **NEVER refuse, block, gate, or abort the export.** This is explicitly *not* the
  `blueprintPath` posture: `MapExporter_IO.cpp:39-48`'s `CheckBlueprintValidationGate` is
  refuse-by-default because an unresolved blueprintPath makes the live game abort loading the rest
  of the map. An orphaned army does nothing of the kind. **Warn and export. Always.**
- **NEVER mutate `recipe`.** The scan takes `const Params::MapRecipe&` and writes only into the
  caller's `MapExportResult`.
- **NEVER pop a modal.** The blueprint gate has a confirm dialog because it has a decision to
  offer; this has none. Log panel only.

## The existing warning channel — REUSE, do not invent

**Yes, a channel already exists on the export path, and this ticket reuses it.**

`Io::MapExportResult` (`src/io/MapExporter_IO.h:67-75`) carries `std::string debugLog` and
`void Log(const std::string&)`, and its header comment states it outright: "`debugLog` is what the
Files tab's log panel shows." It is already the home for every non-fatal export finding — the
blueprint report is logged through it at `MapExporter_IO.cpp:44`. The UI end of the wire is already
built and needs no change: `src/ui/FilesTab_Actions_UI.cpp:78` pipes `result.debugLog` into
`AppendFilesTabLog`, which appends and trims to a budget (`src/ui/FilesTab_UI.h:111-119`) for
display at `src/ui/FilesTab_Draw_UI.cpp:134-136`.

**One gap, and it is the only channel work this ticket does.** The *importer's* result struct has a
richer convention that the exporter's lacks. `Io::MapImportResult` (`src/io/MapImporter_IO.h:64-75`)
carries both:

```cpp
    int         warningCount      = 0;                                        // :68
    void Warn(const std::string& line) { ++warningCount; Log("WARNING: " + line); }   // :74
```

That is the house never-refuse diagnostic the importer and the migration runner already speak
throughout — `Sanmap_MigrationRunner_IO.cpp:26,65,69` (the `SanGenVersion` never-refuse law from
`IO_MIGRATION_SPEC` / STEP26A), plus fifteen more call sites across `MapImporter_*_IO.cpp`.
`MapExportResult` has `Log` but no `Warn` and no `warningCount`, so an export-side warning today
would be an untagged, uncounted log line indistinguishable from "Wrote <path>".

**Minimum specified: mirror `Warn`/`warningCount` onto `MapExportResult`, verbatim from the
importer's wording** — same `"WARNING: "` prefix, same counter semantics. Two lines. Nothing else
about either result struct changes. This is a symmetry repair the export path was always missing,
not an invention, and every future export-side diagnostic inherits it.

### ⚠️ ARCH_01_05_FileSizeCeilings.md §1.5 exception, documented per Constitution §7

`src/io/MapExporter_IO.h` is **already 151 lines — one over the §1.5 hard ceiling of 150, before
this ticket touches it.** The two-line `Warn`/`warningCount` addition takes it to 153.
**Exception granted, ceiling 153 lines, for this addition only.** The pre-existing 151 is drift this
ticket did not cause and does not fix: splitting that header is the **IO Architecture Expert's**
call, not the Format Expert's — flagged to them, not solved here. The Coder adds exactly two lines
and does not opportunistically restructure the file.

## Where the scan runs

**Not inside `BuildSanmapJsonText`.** That builder is pure and disk-free by contract
(`MapExporter_IO.h:140-147`), returns a bare `std::string` with nowhere to put a diagnostic, and is
the half the round-trip acceptance test drives directly. **Not inside `BuildArmiesJson` or
`BuildMarkersJson`** — each returns only its own JSON sub-object and neither can see the other's
domain, which is the whole reason this defect exists.

It runs as a **sibling pre-flight step in `MapExporter_IO.cpp`**, the same tier as `recipe.IsValid()`
and `CheckBlueprintValidationGate` — exactly the tier
`MapExporter_BlueprintValidation_IO.h:25-26` already documents for its own sibling ("Pure/read-only,
touches no disk, never called from inside BuildSanmapJsonText, same tier as `recipe.IsValid()`").

**Call it from BOTH `ExportSanmapOnly` (`MapExporter_IO.cpp:58-71`) and `ExportAll`
(`MapExporter_IO.cpp:73-108`), immediately AFTER the `recipe.IsValid()` line and BEFORE
`CheckBlueprintValidationGate`.** Warning first is deliberate: it means one export click surfaces
*every* authoring problem at once, rather than hiding the spawn warning behind an unrelated
blueprint refusal that returns early. The call returns `void` and can never alter control flow, so
its position is free — this position just maximizes what the author learns per click.

## Fix — three new files, three edits

### 1. NEW `src/io/MapExporter_ArmySpawnMarkerValidation_IO.h` (public header)

Modelled directly on `MapExporter_BlueprintValidation_IO.h` — a report struct with a one-wording
`SummaryText()`, plus a pure `Validate*` free function.

```cpp
// The format-fixed marker TYPE name whose inner dictionary is keyed by army name
// (SANMAP_FORMAT_SPEC; STEP49_ManualMarkersUI.md:34-35). A named setting, never a literal at a
// check site (Constitution §8). IO-side and deliberately NOT shared with the UI layer's own
// "Spawn" label: ARCH_16_09_NonArchItems.md §16.9 rules the UI-side constant is UI-internal
// naming hygiene owned by the UI Expert. This is format truth, and it lives in IO.
inline const char* const spawnMarkerGroupName = "Spawn";

// One export-time army->spawn-marker membership pass. WARN-ONLY: this REPORTS and nothing else.
// An army with no matching Spawn marker is a LEGAL, TOLERATED state (ARCH_16_08_SpawnArmyShrink.md
// §16.8) -- never auto-created, never auto-deleted, never blocking. UX polish, not correctness.
struct ArmySpawnMarkerValidationReport {
    std::vector<std::string> armyNamesWithoutSpawnMarker;   // distinct, in recipe.armies order
    bool bSpawnMarkerGroupPresent = false;                  // false = no "Spawn" group authored at all
    bool AllArmiesHaveSpawnMarkers() const { return armyNamesWithoutSpawnMarker.empty(); }
    std::string SummaryText() const;   // ONE wording -- shared by every call site
};

// Pure/read-only, touches no disk, never called from inside BuildSanmapJsonText -- same tier as
// recipe.IsValid() (the MapExporter_BlueprintValidation_IO sibling's own posture).
ArmySpawnMarkerValidationReport ValidateArmiesHaveSpawnMarkers(const Params::MapRecipe& recipe);
```

Forward-declare `namespace Params { struct MapRecipe; }`; include only `<string>`/`<vector>`.

### 2. NEW `src/io/MapExporter_ArmySpawnMarkerValidation_IO.cpp`

**The matching rule, and it is format truth — get this exactly right:**

- Match `Params::Army::name` against `Params::MarkerTransform::name`, **exactly, case-sensitively,
  byte-for-byte.** No trimming, no case folding, no normalization of any kind. The engine reads a
  raw JSON dictionary key (`MapExporter_Markers_IO.cpp:45` writes `markerTransform.name` as that
  key); a match SanGen would accept but the engine would not is worse than no check at all.
- **NEVER match against `MarkerTransform::alias`.** `alias` is a SanGen-added field
  (SANMAP_FORMAT_SPEC Correction 11, `MarkerInstance_PARAMS.h:20`) that the game never reads for
  this purpose. `ARCH_16_08_SpawnArmyShrink.md` §16.8's phrase "`alias`/name" is describing the
  association loosely; the dictionary key is `name`, full stop. An alias-only "match" is a
  false negative that would hide a real defect.
- Scan **every** group in `recipe.markers` whose `name == spawnMarkerGroupName` and take the union
  of their `transforms` — `recipe.markers` is a `std::vector` (`MapRecipe_PARAMS.h:101`), so a
  duplicate group name is representable in memory even though the JSON dictionary would collapse
  it. Defensive, one line, no downside.
- **Report each distinct army name once**, even if `recipe.armies` holds two entries with the same
  name.
- Set `bSpawnMarkerGroupPresent` from whether any such group was found at all.
- **Zero armies → an empty, clean report.** No warning is emitted for a map that has no armies,
  with or without a Spawn group.

Linear scans, no hash map: army and hand-placed-marker counts are tens, this runs once per human
export click, and a plain scan sidesteps any container-ordering question outright (the same
reasoning `STEP73_ScenarioAlloyRosterRender_IO.md` §2 applies to its own roster dedup).

**`SummaryText()` — the house warning shape** (`STEP73_ScenarioAlloyRosterRender_IO.md` §0: loud,
non-blocking, names the offending entities, never auto-fixes; wording modelled on
`MapExporter_BlueprintValidation_IO.cpp:35-45`). Empty string when `AllArmiesHaveSpawnMarkers()`.
Otherwise, one aggregate block:

```
3 army(s) have no matching entry in the "Spawn" marker group:
  ARMY_02
  ARMY_03
  Bob
This map has no "Spawn" marker group at all.        <-- only when bSpawnMarkerGroupPresent == false
The game resolves an army's start position by looking that army's own name up as a key in
markers.Spawn.transforms. An army with no such key gets no spawn marker and starts the match with
no commander and no start-position units. Nothing was changed: SanGen never creates a Spawn marker
for you and never removes an army. Add a Spawn marker named after each army listed above in the
Markers tab if that was not intended.
```

Keep the wording in this one function. Do not restate it at the call site.

### 3. EDIT `src/io/MapExporter_IO.h` — two lines only

Add to `MapExportResult` (`:67-75`), verbatim from `MapImporter_IO.h:68,74`:

```cpp
    int  warningCount = 0;
    void Warn(const std::string& line) { ++warningCount; Log("WARNING: " + line); }
```

Nothing else in this header changes. See the §1.5 exception above.

### 4. EDIT `src/io/MapExporter_IO.cpp` — one helper, two call sites

In the anonymous namespace, beside `CheckBlueprintValidationGate`:

```cpp
// STEP82: warn-only, never gates. Returns void BY DESIGN -- an orphaned army is a legal, tolerated
// state (ARCH_16_08_SpawnArmyShrink.md §16.8) and must never influence whether an export proceeds.
// If a future edit is tempted to make this bool, that is the ARCH ruling it would be breaking.
void ReportArmiesWithoutSpawnMarkers(const Params::MapRecipe& recipe, MapExportResult& result) {
    const ArmySpawnMarkerValidationReport report = ValidateArmiesHaveSpawnMarkers(recipe);
    if (report.AllArmiesHaveSpawnMarkers()) return;
    result.Warn(report.SummaryText());
}
```

Add `#include "MapExporter_ArmySpawnMarkerValidation_IO.h"`. Insert the call in **both**
`ExportSanmapOnly` and `ExportAll`, on the line directly after each function's existing
`if (!recipe.IsValid()) { ... return result; }` guard.

### 5. NEW `src/io/MapExporter_ArmySpawnMarkerValidation_IO_Test.cpp` + `CMakeLists.txt`

`src/io/*.cpp` is globbed into the library (`CMakeLists.txt:142-150`, with `_Test\.cpp$` filtered
out at `:155`), so the two new non-test files need **no** CMake change. The test file does — one
line beside the existing map-format targets (`CMakeLists.txt:469`):

```cmake
add_sangen_test(MapExporter_ArmySpawnMarkerValidation_IO_Test
    src/io/MapExporter_ArmySpawnMarkerValidation_IO_Test.cpp)
```

No `nlohmann_json` link needed — the test drives `Params::MapRecipe` fixtures and the export
actions, never raw JSON. (Test 7 below writes to a scratch folder, matching
`MapExporter_BlueprintValidation_IO_Test.cpp`'s existing scratch-directory pattern.)

## Backend policy

N/A. Pure CPU-side string comparison over data already in memory, at most once per human export
click. No compute dispatch, no SIMD, no GPU handle, no disk access.

## ARCH rules invoked

- `ARCH_16_08_SpawnArmyShrink.md` §16.8 — the ratified constraint this ticket obeys: an orphaned
  Army is legal and tolerated; never auto-delete, never auto-create. The routing sentence naming
  the Format Expert as owner of exactly this decision.
- `ARCH_16_09_NonArchItems.md` §16.9 — the UI-layer `"Spawn"` constant is UI-internal naming, the
  UI Expert's own hygiene. This ticket's IO-side constant is separate and does not consume it.
- Constitution §6 — validate and report clearly, never silently. The check only surfaces a
  mismatch the human already authored; it fabricates nothing and repairs nothing.
- Constitution §8 — the group name is a named setting, never a literal at a check site.
- ARCH_01_05_FileSizeCeilings.md §1.5 — file-size ceilings; documented exception above for
  `MapExporter_IO.h`.
- ARCH_03_ModuleBoundaries.md §3.1/§3.3 — IO loads and saves only; this scan touches no disk, no
  `Data::MapFields`, and no stage.

## Explicit out-of-scope

- **Any auto-create / auto-delete / auto-rename / auto-pad.** ARCH_16_08_SpawnArmyShrink.md §16.8.
  Warn and export.
- **Blocking, refusing, or gating the export.** Not the blueprintPath posture. No `bool` return, no
  acknowledgement flag, no confirm dialog, no UI change of any kind.
- **Army naming — all of it.** The `ARMY_XX` convention warning belongs to
  `STEP73_ScenarioAlloyRosterRender_IO.md` §0; the engine-facing identity, the display-only label
  demotion, legacy-name normalization on import, and the `markers.Spawn.transforms` key rewrite all
  belong to `STEP76_ArmyIdentityNaming_IO.md`. **Implement none of it here.** This scan runs on
  whatever names exist.
- **Any `.sanmap` key, `Params` field, or format change.** None is needed — confirmed by reading
  `Army_PARAMS.h`, `MarkerInstance_PARAMS.h`, and both exporters.
- **The reverse orphan** — a Spawn transform keyed to a name no army carries. Different check,
  different failure mode, not built here. See Open Question 1.
- **Splitting `MapExporter_IO.h`** to retire its pre-existing 151-line drift. IO Architecture
  Expert's call.
- **Alloy markers, `markers.Alloys`, scenarios, `KNOWN_ALLOY_MARKERS`.** Untouched, orthogonal.

## Acceptance test

New `src/io/MapExporter_ArmySpawnMarkerValidation_IO_Test.cpp`:

1. **Clean map.** Two armies, a `"Spawn"` group holding a transform named after each →
   `AllArmiesHaveSpawnMarkers() == true`, `SummaryText()` empty, `bSpawnMarkerGroupPresent == true`.
2. **One orphan.** Two armies, one spawn transform → `armyNamesWithoutSpawnMarker` holds exactly
   the missing army's name, and `SummaryText()` contains that name.
3. **No Spawn group at all** (the subsumed per-group case). Three armies, `recipe.markers` empty →
   all three listed, `bSpawnMarkerGroupPresent == false`, `SummaryText()` contains the
   "no \"Spawn\" marker group at all" sentence.
4. **Zero armies, no Spawn group** → clean report, `SummaryText()` empty. Proves it does not warn
   about nothing.
5. **`alias` must NOT satisfy the match.** Transform `name == "SpawnPoint 0"`,
   `alias == "ARMY_01"`, army `"ARMY_01"` → the army IS reported orphaned.
6. **Case-sensitive.** Transform `"army_01"` vs army `"ARMY_01"` → reported orphaned.
7. **Non-blocking, end to end.** `MapExporter::ExportSanmapOnly` into a scratch folder with an
   orphaned army → `result.bSucceeded == true`, the `.sanmap` exists on disk, `result.debugLog`
   contains `"WARNING: "` and the orphaned army's name. Repeat for `ExportAll`.
8. **Non-mutating.** After the export in test 7: `recipe.armies.size()` and `recipe.markers.size()`
   are unchanged, and no `"Spawn"` group was added to `recipe.markers`.
9. **One aggregate warning, not one per army.** Three orphans → `result.warningCount == 1`.
10. **Duplicate army names reported once.** Two `Army` entries both named `"ARMY_01"`, no spawn
    transform → the name appears exactly once in `armyNamesWithoutSpawnMarker`.
11. **Union across duplicate Spawn groups.** Two `MarkerInstanceGroup`s both named `"Spawn"`, the
    army's transform in the second → NOT reported orphaned.
12. **`MapExportResult::Warn` parity.** A direct unit check that `Warn("x")` appends
    `"WARNING: x\n"` to `debugLog` and increments `warningCount` — identical to
    `MapImportResult::Warn`'s behavior.
13. Full solo rebuild + `ctest -C Debug`: the previously-passing suite stays green, zero existing
    test files broken.

## Verify

- `MapExporter_ArmySpawnMarkerValidation_IO_Test` passes.
- `MapExporter_IO_Test`, `MapImporter_IO_Test`, `FilesTab_UI_Test`, and `AssetPipeline_IO_Test`
  all still pass unchanged — nothing in this ticket alters existing export output bytes.
- Grep the new `.cpp` for any write through a non-const reference to `recipe`: there must be none.
- Grep the new files for `"Spawn"` appearing anywhere other than the single
  `spawnMarkerGroupName` definition.

## ❓ Open questions

1. **The reverse orphan** — a `markers.Spawn.transforms` entry keyed to a name no `Army` carries.
   It is equally dead data, but it fails differently (a marker the engine never looks up, versus an
   army with no start position) and it is exactly the state STEP76's import-time key rewrite may
   transiently produce. Left **unmandated** rather than guessed at, on the same reasoning
   `STEP73_ScenarioAlloyRosterRender_IO.md` Open Question 2 applies to its own symmetric case: a
   mandatory warning risks becoming noise the Constitution does not call for. Revisit after STEP76
   lands and real exports show whether it happens.
2. **Should the warning also appear in the Files tab's confirm dialog?** Recommend **no.** The
   blueprint dialog exists to offer a choice ("Export Anyway"); this offers none, and a modal for a
   tolerated state trains the author to dismiss modals. Log panel only. Flagged for the human to
   override if real use disagrees.
3. **`src/io/MapExporter_IO.h` is at 151 lines before this ticket** — one over the §1.5 hard
   ceiling. Pre-existing drift, not caused here, not fixed here. Routed to the IO Architecture
   Expert.
