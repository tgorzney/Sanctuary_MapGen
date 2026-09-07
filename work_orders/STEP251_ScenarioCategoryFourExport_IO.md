# STEP251 — Category-4 scenario generator scaffolding, generic dispatch, and `ScenarioBody::name` path-safety validation

**Layers:** IO, UI. **Domain:** the new category-4 on-disk file (`<MapName>_Scenarios_<ScenarioName>.lua`),
its scaffold-once-if-missing export mechanism, `Scenario.SpawnMatchedScenarioUnits`'s generic lazy
dispatcher in the bundled runtime resource, and `Params::ScenarioBody::name`'s new filesystem-safety
validation. **Sequence:** Map Scenario track (follows STEP209). **Authorized by:**
`ARCH_15_04_ThreeFileOnDiskShape.md`'s "AMENDED 2026-09-03" section (category 4 + the third
overwrite-safety class, binding law) and `ARCH_15_05_ParamsScenariosType.md`'s "AMENDED 2026-09-03"
section (`ScenarioBody::name` charset + uniqueness rule, binding law), both grounded in
`MAP_SCENARIO_SPEC.md` §11.2/§14 (companion documentation, including the drafted dispatcher Lua this
ticket transcribes verbatim). Both ARCH sections already rule every open design question about *what*
category 4 is; this work-order is the mechanical translation into real files plus the judgment calls
those sections deliberately left to implementation (validation enforcement points, the collision-check
mechanism, file layout).

## 0. Why

`ARCH_15_05`'s original ratification left an OPEN item: where does the per-scenario unit-spawn
dispatch/generator code live under the three-file split? `ARCH_15_04`'s 2026-09-03 amendment answers
it: a **fourth** on-disk category, one hand-authored Lua file per `spawnsUnits == true` scenario,
scaffolded once by SanGen and never touched again, discovered at runtime by a **generic** lazy
`Import()`-by-name dispatcher living in the existing bundled runtime resource — never an `if/elseif`
chain, never per-map/per-scenario code in that file. This closes the gap and, as a direct consequence,
makes `ScenarioBody::name` a literal filesystem path component for the first time — which needs new
validation it never needed as a pure log/debug string.

None of this is built yet. `resources/lua/SanGenScenarioRuntime.lua` still carries an explicit comment
declining to add the dispatcher (superseded by the 2026-09-03 ruling). `ScenarioScript_Export_IO.cpp`
has no category-4 write step at all. `ScenarioBody::name` has zero validation anywhere.

## 1. IO — `ScenarioNameValidation_IO.h`/`.cpp` (new files, pure, no filesystem)

Modelled directly on `MapExporter_ScenarioAreaNameValidation_IO.h`/`.cpp` (STEP209): a report struct
with a one-wording `SummaryText()`, plus a pure `Validate*` free function, never called from inside
`BuildSanmapJsonText`/`BuildScenarioDataLuaText`. **Shared** by the UI (live, per-frame, non-blocking
inline warning, §4 below) and IO (§2's export-time write-refusal gate) — one validator, two consumers,
matching the areaName validator's own "SHARED by both export legs" precedent.

**Runs over the WHOLE `Params::Scenarios` set regardless of `spawnsUnits`.** `MAP_SCENARIO_SPEC.md`
§11.2's dispatcher (§3 below) builds an `Import()` path from **whichever scenario matched**, whether or
not that scenario opted into spawning — a missing file for a `spawnsUnits == false` scenario is a
silent, expected no-op, but a malformed `name` (a space, a path separator) is a live runtime-path risk
regardless of `spawnsUnits`. Only the category-4 **write** (§2) is gated on `spawnsUnits == true`, since
that is the only place an actual file write is ever attempted.

### 1a. `src/io/ScenarioNameValidation_IO.h`

```cpp
// ScenarioNameValidation_IO.h -- pure, disk-free validation of ScenarioBody::name against the
// filesystem-safety charset + case-insensitive cross-set uniqueness ARCH_15_05_ParamsScenariosType.md's
// "AMENDED 2026-09-03" section rules (name is now substituted directly into a category-4 Import() path,
// ARCH_15_04_ThreeFileOnDiskShape.md's "AMENDED 2026-09-03" section). Layer: IO. Modelled directly on
// the sibling MapExporter_ScenarioAreaNameValidation_IO.h: a report struct with a one-wording
// SummaryText(), plus a pure Validate* free function, same tier as recipe.IsValid(), never called from
// inside BuildSanmapJsonText/BuildScenarioDataLuaText.
//
// SHARED by the UI (live, per-frame, non-blocking inline warning) and IO (the category-4 export gate,
// ScenarioScript_CategoryFourExport_IO) -- one validator, two consumers, never two independently
// invented copies of the rule (same posture as MapExporter_ScenarioAreaNameValidation_IO.h's own
// "SHARED by both export legs" note).
//
// Runs over the WHOLE Scenarios set regardless of spawnsUnits -- see this file's own header comment in
// the work-order that authored it (STEP251 §1) for why validation is never gated on spawnsUnits even
// though the category-4 WRITE it feeds (a separate file) is.
#pragma once
#include <string>
#include <vector>

namespace SanmapGen {
namespace Params { struct Scenarios; }
namespace Io {

struct ScenarioNameValidationReport {
    struct Violation {
        std::string scenarioDescriptor;   // body.name if non-empty else a tier+index fallback,
                                          // mirroring ScenarioAreaNameDescriptor's "never blank" idiom
                                          // (MapExporter_ScenarioAreaNameValidation_IO.cpp)
        std::string name;                 // the offending name itself, as-authored, byte-for-byte
        std::string reason;               // one full sentence -- see ValidateScenarioNames' three
                                          // reason texts in the .cpp
    };
    std::vector<Violation> violations;
    bool AllNamesValid() const { return violations.empty(); }
    // Returns the first violation reason recorded for `name`, or nullptr if `name` has none. The
    // per-scenario query ScenarioScript_CategoryFourExport_IO and the UI's inline warning both use
    // this instead of re-deriving the rule.
    const std::string* FindViolationReasonForName(const std::string& name) const;
    std::string SummaryText() const;   // ONE wording -- shared by every call site
};

// Pure/read-only, touches no disk, no Lua. Walks patternScenarios, then countScenarios, then
// defaultScenario (same tier order this file family already establishes elsewhere) -- runs over EVERY
// ScenarioBody regardless of spawnsUnits (see this header's own top note for why).
ScenarioNameValidationReport ValidateScenarioNames(const Params::Scenarios& scenarios);

} // namespace Io
} // namespace SanmapGen
```

### 1b. `src/io/ScenarioNameValidation_IO.cpp` — algorithm

Three independent checks, evaluated **per scenario body, first-failing-rule-wins** (a name failing
charset is never also charset-checked-then-duplicate-checked — one violation per scenario, matching
this codebase's general first-match precedent):

1. **Charset.** `^[A-Za-z0-9_]+$` — non-empty, ASCII letters/digits/underscore only. No `<regex>`
   anywhere in this codebase today (`grep -rn "#include <regex>" src/` returns nothing) — implement as
   a manual per-character loop (`std::isalnum(static_cast<unsigned char>(c)) || c == '_'`), matching
   this codebase's minimal-dependency convention. Reason text on failure: `"empty, or contains a
   character outside [A-Za-z0-9_]"`.
2. **Reserved name.** Case-insensitive equality (ASCII `std::tolower`, the exact idiom
   `TemplateSourceScan_IO.cpp` already uses for its extension check) against `"runtime"` or `"data"` —
   **this is a corollary of `ARCH_15_04`'s own already-ratified filename-disjointness principle (its
   point 1, and the category-4 amendment's own "excluding the two category-2/3 literal names" phrasing
   for the collision-detection glob), applied one level up: a scenario literally named `"Runtime"` or
   `"Data"` (any case) would produce a category-4 filename
   (`<MapName>_Scenarios_Runtime.lua`/`<MapName>_Scenarios_Data.lua`) that COLLIDES with SanGen's own
   always-regenerated category-2/3 files.** This exact reserved-word check is not spelled out verbatim
   as a numbered rule in `ARCH_15_05`'s charset/uniqueness text — it is implemented here because leaving
   it out is a real correctness bug (a scenario named "Data" would have its own hands-off category-4
   scaffold either overwritten by the next category-3 regeneration, or vice versa, silently corrupting
   one of the two). **Flagged for the ARCH Expert's attention** — if they want this formally appended to
   `ARCH_15_05` as a named sub-rule, that is their file to write; this ticket implements it as a
   necessary consequence of already-ratified law, not as new architecture. Reason text on failure:
   `"'<name>' collides with SanGen's own reserved <MapName>_Scenarios_Runtime.lua / _Data.lua filenames
   (case-insensitive) -- rename this scenario"`.
3. **Cross-set uniqueness, case-insensitive.** Among every charset-valid, non-reserved name remaining,
   group by lowercased value; every scenario whose lowercased name occurs more than once across
   `patternScenarios` + `countScenarios` + `defaultScenario` combined gets a violation. Reason text:
   `"duplicates another scenario's name, case-insensitively, elsewhere in this Scenarios set"`.

`SummaryText()`: mirror `ScenarioAreaNameValidationReport::SummaryText()`'s shape — empty when
`AllNamesValid()`; otherwise a count line, one indented `<descriptor> -> "<name>": <reason>` line per
violation, and a closing sentence: `"A scenario's name is substituted directly into a game-loaded file
path (<MapName>_Scenarios_<Name>.lua) -- fix it in the Scenarios tab before exporting the scenario
script. Only that scenario's own category-4 generator file write is refused; the .sanmap and
<MapName>_Scenarios_Data.lua exports are unaffected."`

## 2. IO — `ScenarioScript_CategoryFourExport_IO.h`/`.cpp` (new files, filesystem-touching)

**Why a new file, not grown into `ScenarioScript_Export_IO.cpp`:** that file is already ~140 lines
against the `ARCH_01_05_FileSizeCeilings.md` §1.5 **150-line hard ceiling** (never retroactively raised
for this ticket — category 4's own ceiling exception is scoped to the *generated* Lua files, not to
SanGen's own C++). Extracting this concern into its own file also matches this exact file family's
established pattern: `MapExporter_ArmySpawnMarkerValidation_IO`, `MapExporter_ScenarioAreaNameValidation_IO`
— one small file per independent validation/write concern, wired into the orchestrator via one call
(`ARCH_05_GodObjectDismemberment.md`).

### 2a. `src/io/ScenarioScript_CategoryFourExport_IO.h`

```cpp
// ScenarioScript_CategoryFourExport_IO.h -- category-4 on-disk shape (ARCH_15_04_ThreeFileOnDiskShape.md
// "AMENDED 2026-09-03", MAP_SCENARIO_SPEC.md §11.2/§14 point 4): the per-scenario hand-authored
// unit-spawn generator file, <MapName>_Scenarios_<ScenarioName>.lua. Layer: IO. Scaffold-once-if-missing,
// then permanently hands-off -- a THIRD overwrite-safety class, distinct from both
// ScenarioScript_Export_IO's existing banner-gated always-regenerate class (categories 2-3) and
// category 1's never-write-ever posture. Sole caller: ScenarioScript_Export_IO::ExportMapScenario,
// called once per export, after the Data.lua/Runtime.lua legs.
#pragma once
#include <string>
#include <vector>

namespace SanmapGen {
namespace Params { struct MapRecipe; }
namespace Io {

// The weaker banner every category-4 scaffold opens with. UNLIKE kScenarioGeneratedFileBannerLine
// (categories 2-3), this banner is NEVER read back or compared against to decide whether to overwrite
// -- once a category-4 file exists in ANY state (this banner, hand-edited, no banner at all), SanGen
// never touches it again. It exists purely so the scaffold's own text carries a consistent,
// machine-greppable literal a human can search for across every map.
inline constexpr const char* kScenarioCategoryFourScaffoldBannerLine =
    "-- SANGEN-CREATED STARTING POINT -- freely hand-edit -- never regenerated";

struct ScenarioCategoryFourExportReport {
    std::vector<std::string> scaffoldedFilePaths;   // new generator-file scaffolds written this export
                                                     // -- one entry per spawnsUnits==true scenario whose
                                                     // file did not yet exist
    std::vector<std::string> writeRefusals;         // spawnsUnits==true scenarios whose own scaffold
                                                     // write was refused this export (an invalid name,
                                                     // or -- structurally near-unreachable given
                                                     // upstream charset validation -- a syntax-check
                                                     // failure on the rendered scaffold text), each a
                                                     // fully-formatted message
};

// mapScriptDirectory: LJ/lua/maps/<MapName>/, already created by the caller (ExportMapScenario's own
// step 2) -- this function never creates it itself. For every ScenarioBody across
// recipe.scenarios.patternScenarios/countScenarios/defaultScenario with spawnsUnits == true:
//   1. Validate its own name via Io::ValidateScenarioNames(recipe.scenarios) (computed ONCE per call,
//      not once per scenario). An invalid name refuses THAT scenario's write only -- appends a message
//      to writeRefusals and moves on; every other scenario's export is unaffected.
//   2. std::filesystem::exists on the literal path mapScriptDirectory/<MapName>_Scenarios_<Name>.lua --
//      present in ANY state -> skip silently (no read, no comparison, never touched again, not even
//      logged as a no-op -- this is the expected common case on every export after the first). Absent
//      -> render the minimal scaffold, run it through Sys::CheckLuaSyntax (defensive -- structurally
//      unreachable given every rendered placeholder is charset-restricted to [A-Za-z0-9_] and only ever
//      appears inside `--` comment lines, but Constitution §6 asks SanGen to validate what it itself
//      writes too), write it, append its path to scaffoldedFilePaths.
// Total: never throws, never touches a category-4 file already on disk in any state.
ScenarioCategoryFourExportReport ExportScenarioCategoryFourScaffolds(
    const std::string& mapScriptDirectory, const Params::MapRecipe& recipe);

} // namespace Io
} // namespace SanmapGen
```

### 2b. `src/io/ScenarioScript_CategoryFourExport_IO.cpp` — exact scaffold text

```cpp
#include "ScenarioScript_CategoryFourExport_IO.h"
#include "FilesystemPrimitives_IO.h"
#include "ScenarioNameValidation_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../sys/LuaSyntaxCheck_SYS.h"
#include <filesystem>

namespace SanmapGen {
namespace Io {
namespace {

std::string BuildScaffoldText(const std::string& mapName, const std::string& scenarioName) {
    return std::string(kScenarioCategoryFourScaffoldBannerLine) + "\n"
        "--\n"
        "-- " + mapName + "_Scenarios_" + scenarioName + ".lua -- hand-authored unit-spawn generator for\n"
        "-- the \"" + scenarioName + "\" scenario (`ARCH_15_04_ThreeFileOnDiskShape.md` category 4,\n"
        "-- `MAP_SCENARIO_SPEC.md` \xC2\xA7 11.2/\xC2\xA7 14). SanGen wrote this file ONCE, because\n"
        "-- " + mapName + "_Scenarios_Data.lua's \"" + scenarioName + "\" scenario record has\n"
        "-- spawnsUnits = true and no generator file existed yet at export time. SanGen will NEVER\n"
        "-- overwrite this file again, in any state -- edit freely, it is entirely yours from here.\n"
        "--\n"
        "-- CONTRACT: must expose a GLOBAL function GenerateScenarioUnits(area) returning a flat array\n"
        "-- of rows shaped { armyIndex = <int>, templateIdentifier = \"<string>\", x = <num>, y = <num>,\n"
        "-- z = <num> }. Scenario.SpawnMatchedScenarioUnits (in the sibling\n"
        "-- " + mapName + "_Scenarios_Runtime.lua) Import()s this file LAZILY -- only when \""
        + scenarioName + "\"\n"
        "-- is the scenario that actually matched the current lobby -- and passes whatever it returns\n"
        "-- straight to Scenario.SpawnUnits, which calls CreateUnit for you. Returning an empty table is\n"
        "-- legal: it spawns nothing, exactly like this stub does until you fill it in.\n"
        "--\n"
        "-- See MAP_SCENARIO_SPEC.md \xC2\xA7 12 for a complete worked example, and MAP_UNIT_SPAWNING_SPEC.md\n"
        "-- for the CreateUnit contract: army indices come from pairs(Armies), never hardcoded; guard\n"
        "-- army.lobbyOptions before reading isEmptySlot; never call CreateUnit directly -- only through\n"
        "-- Scenario.SpawnUnits.\n"
        "\n"
        "function GenerateScenarioUnits(area)\n"
        "    return {}\n"
        "end\n";
}

std::string CategoryFourFilePath(const std::string& mapScriptDirectory, const std::string& mapName,
                                 const std::string& scenarioName) {
    return JoinExportPath(mapScriptDirectory, mapName + "_Scenarios_" + scenarioName + ".lua");
}

void ExportOneScenarioBody(const Params::ScenarioBody& body, const std::string& mapScriptDirectory,
                           const std::string& mapName, const ScenarioNameValidationReport& nameReport,
                           ScenarioCategoryFourExportReport& outReport) {
    if (!body.spawnsUnits) return;

    const std::string* invalidReason = nameReport.FindViolationReasonForName(body.name);
    if (invalidReason != nullptr) {
        outReport.writeRefusals.push_back(
            "category-4 generator file for scenario '" + body.name + "' was NOT written: its own name "
            "failed validation (" + *invalidReason + ") -- fix the name, then re-export.");
        return;
    }

    const std::string filePath = CategoryFourFilePath(mapScriptDirectory, mapName, body.name);
    std::error_code existenceError;
    if (std::filesystem::exists(std::filesystem::path(filePath), existenceError)) return; // hands-off

    const std::string scaffoldText = BuildScaffoldText(mapName, body.name);
    const Sys::LuaSyntaxCheckResult syntax = Sys::CheckLuaSyntax(scaffoldText);
    if (!syntax.bSucceeded) {
        outReport.writeRefusals.push_back(
            "category-4 scaffold for scenario '" + body.name + "' failed its own syntax pre-check -- "
            "not written. This should be structurally unreachable; report it if seen.");
        return;
    }

    WriteBinaryFileBytes(filePath, scaffoldText.data(), scaffoldText.size());
    outReport.scaffoldedFilePaths.push_back(filePath);
}

} // namespace

ScenarioCategoryFourExportReport ExportScenarioCategoryFourScaffolds(
        const std::string& mapScriptDirectory, const Params::MapRecipe& recipe) {
    ScenarioCategoryFourExportReport report;
    const ScenarioNameValidationReport nameReport = ValidateScenarioNames(recipe.scenarios);

    for (const Params::PatternScenario& entry : recipe.scenarios.patternScenarios)
        ExportOneScenarioBody(entry.body, mapScriptDirectory, recipe.mapName, nameReport, report);
    for (const Params::CountScenario& entry : recipe.scenarios.countScenarios)
        ExportOneScenarioBody(entry.body, mapScriptDirectory, recipe.mapName, nameReport, report);
    // Defense-in-depth: the UI never exposes a spawnsUnits checkbox for Tier 3 (ScenariosTab_Lists_UI.cpp's
    // own "Tier 3 is never spawns-flagged" comment) but the PARAMS field still exists and a hand-edited
    // or imported .sanmap could set it -- treat the default scenario symmetrically, never assume the
    // UI-level convention holds at the PARAMS/IO layer.
    ExportOneScenarioBody(recipe.scenarios.defaultScenario, mapScriptDirectory, recipe.mapName, nameReport, report);

    return report;
}

} // namespace Io
} // namespace SanmapGen
```

Note the `\xC2\xA7` UTF-8 byte sequence for `§` inside the scaffold's comment text — matches this
codebase's own established idiom for a non-ASCII character inside a plain C++ string literal embedded
in generated text (see `ScenariosTab_Detail_UI.cpp`'s `"\xE2\x9A\xA0"` warning-triangle literal for the
same pattern with a different codepoint). Keep the comment lines ASCII-safe otherwise, matching
`kScenarioGeneratedFileBannerLine`'s own "ASCII-only by deliberate choice" rule.

## 3. IO — wire category-4 into `ScenarioScript_Export_IO.cpp`

Add `#include "ScenarioScript_CategoryFourExport_IO.h"`. Insert as a new **step 8**, between the
existing step 7 (runtime resolution/write) and the existing step 8 ("Return", renumbered to 9):

```cpp
    // 8. Category-4 scaffold export (STEP251) -- runs even if the Data.lua/Runtime.lua legs above
    // failed; a category-4 scaffold's own name-validation gate is per-scenario, independent of
    // whether the map's other two SanGen-owned files wrote cleanly this export.
    const ScenarioCategoryFourExportReport categoryFourReport =
        ExportScenarioCategoryFourScaffolds(mapScriptDirectory, recipe);
    for (const std::string& scaffoldedPath : categoryFourReport.scaffoldedFilePaths) {
        result.writtenFilePaths.push_back(scaffoldedPath);
        result.Log("Scaffolded new category-4 generator file: " + scaffoldedPath);
    }
    for (const std::string& refusal : categoryFourReport.writeRefusals) {
        result.Log(refusal);
    }
```

No new fields added to `ScenarioExportResult` — matching STEP209's own minimal-additive-field
precedent (that ticket added zero new fields, routing everything through `Log`/`debugLog` and the
pre-existing `writtenFilePaths`). The Files tab's existing raw-`debugLog` display panel (referenced by
`FilesTab_ScenarioExportRow_Draw_UI.cpp`'s own header comment: "the exact paths already live in
`state.debugLog`... which this points at rather than re-deriving") surfaces every line above with zero
new UI plumbing.

## 4. IO — wire name validation into the `.sanmap` JSON leg too (`MapExporter_IO.cpp`)

A scenario name is substituted into a runtime `Import()` path regardless of which export action
produced the `.sanmap` — a human authoring in SanGen without ever touching the Files tab's separate
"Export Scenario Script" button should still see this warning on a plain `.sanmap` export. Mirror
`ReportScenarioAreaNameReferences` (lines 18-27, STEP209) exactly:

```cpp
// STEP251 -- warn, never block. An invalid/duplicate/reserved ScenarioBody::name is caught here too
// (not only in ScenarioScript_Export_IO's category-4 gate) because the .sanmap leg is a separate,
// independently-triggerable export action -- Constitution §6's "loud, logged" posture applies
// regardless of which button produced the document.
void ReportScenarioNameViolations(const Params::MapRecipe& recipe, MapExportResult& result) {
    const ScenarioNameValidationReport report = ValidateScenarioNames(recipe.scenarios);
    if (report.AllNamesValid()) return;
    result.Warn(report.SummaryText());
}
```

Add `#include "ScenarioNameValidation_IO.h"`. Call it inside `WriteSanmapDocument`, immediately after
the existing `ReportScenarioAreaNameReferences(recipe, result);` (line 38):

```cpp
    ReportScenarioNameViolations(recipe, result);
```

## 5. IO — `resources/lua/SanGenScenarioRuntime.lua` (bundled runtime resource content)

**Replace lines 302-306** (the comment block beginning `-- \`Scenario.SpawnMatchedScenarioUnits(area)\`
... is deliberately NOT added here ...`) **with the real function**, transcribed verbatim from
`MAP_SCENARIO_SPEC.md` §11.2 / `ARCH_15_04`'s own drafted Lua. Both `currentMapName` (line 37) and
`currentMatchedScenarioName` (line 255, set inside `ResolveAndApply` at line 261) are **already
declared as locals earlier in this same file** — the new function reuses them directly, no new
plumbing:

```lua
-- ============================================================================
-- Generic per-scenario dispatch (`ARCH_15_04_ThreeFileOnDiskShape.md` "AMENDED 2026-09-03",
-- MAP_SCENARIO_SPEC.md §11.2). Lazily Import()s ONLY the one matched scenario's own category-4
-- generator file, <MapName>_Scenarios_<ScenarioName>.lua -- never eager, never a name->function table
-- built up front, never an enumeration of the whole authored Scenarios set. A missing file for a
-- spawnsUnits == false scenario is the silent, expected common case; a missing file for
-- spawnsUnits == true is a real authoring gap that pcall degrades to "no units spawned," never an
-- abort -- the same per-call pcall ordering law this file's own FindMatchingScenario already follows.
-- ============================================================================
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

Place this immediately before the file's final `return Scenario` line (currently 308-309). This file's
own **100/150-line ceiling exception does NOT apply here** — `ARCH_15_04`'s ceiling is scoped only to
category-4 files, explicitly not retroactive to this file (already 309 lines pre-amendment; growing to
~327 is fine and expected).

No other line in this file changes. `_data.lua` (category 1, never SanGen-written) is responsible for
actually *calling* `Scenario.SpawnMatchedScenarioUnits(chosenArea)` inside its own `NewThread` — that is
existing, already-specified orchestrator wiring (`MAP_UNIT_SPAWNING_SPEC.md`), out of this ticket's
scope (SanGen never writes `_data.lua`, under any code path).

## 6. UI — enforcement point for name validation: BOTH edit-time and export-time (recommendation + reasoning)

**Recommendation: both**, matching this exact codebase's own established dual-enforcement pattern for
every comparable rule in this file family (`ScenarioBody::areaName` staleness — UI shows no-match via
the Combo's own selection-resolution loop, export logs the authoritative warning;
`ArmySpawnMarkerValidationReport` — same shape; `maxArmySlotCount`, `ARCH_15_10` point 2 — "loud, logged,
never blocks"). There is no stated project-wide "validate only at the edit surface" or "validate only at
export" preference to defer to instead — every precedent in this exact subsystem is dual, so category-4
naming follows the same shape rather than introducing a new posture.

- **UI (edit-time): non-blocking, informational, red inline text**, drawn under the Name field,
  reusing this file's own established `TextColored` idiom — never blocking a keystroke, never
  auto-correcting/renaming/truncating/deduplicating (explicit ARCH ruling: "never silently renamed,
  truncated, or deduplicated by SanGen"). `TextInputRules` (`TextInput_UI.h`) carries no charset-filter
  field today and this ticket does not add one — extending a widget every other name field in the app
  shares, just for this one field's rule, would be scope creep on a shared control; a purely additive,
  read-only warning line achieves the same "loud, logged" goal with zero shared-widget risk.
- **Export-time (hard, but scoped): refuses ONLY that one scenario's category-4 scaffold write** — never
  the whole export, never the `.sanmap`/`_Scenarios_Data.lua` legs (which render `name` as plain text
  data with no filesystem risk). This is the exact refusal shape `ARCH_15_04` already uses for a foreign
  marker collision on categories 2-3 — a new instance of an existing shape, not a new mechanism.

### 6a. `ScenariosTab_Detail_UI.cpp` — the inline warning

Add two file-local `ImVec4` constants to this file's own anonymous namespace, duplicating
`FilesTab_ScenarioExportRow_Draw_UI.cpp`'s exact values (same established "each file owns its own copy"
precedent this ARCH family already uses repeatedly — promoting these to a shared header is a
cheap-but-optional follow-up, not required here):

```cpp
const ImVec4 kErrorTextColor(0.95f, 0.35f, 0.35f, 1.0f);
```

Immediately after the existing `DrawTextInput("Name", body.name, nameRules);` (line 175), add:

```cpp
if (const std::string* invalidReason = nameReport.FindViolationReasonForName(body.name))
    ImGui::TextColored(kErrorTextColor, "%s", ("\xE2\x9A\xA0 " + *invalidReason).c_str());
```

### 6b. Update the stale "Spawns Units" warning (lines 184-186)

This text currently describes the now-superseded `if/elseif` dispatch ("a matching branch...keyed off
this scenario's Name"). Replace with:

```cpp
ImGui::TextWrapped("%s", "\xE2\x9A\xA0 Setting this alone spawns nothing. SanGen scaffolds "
    "<MapName>_Scenarios_<Name>.lua on the next Scenario Script export if it does not exist yet -- "
    "open that file and fill in GenerateScenarioUnits(area) to actually spawn units.");
```

### 6c. Thread `ScenarioNameValidationReport` down to `DrawScenarioBodyFields`

Compute it **once per frame**, at the top of `DrawScenariosTab` (`ScenariosTab_Lists_UI.cpp`, currently
lines 90-100) — never once per body (that would be needless repeated O(n) work per row):

```cpp
void DrawScenariosTab(Params::MapRecipe& recipe, ScenariosTabState& state, Pipeline::PreviewDriver*) {
    ImGui::PushID("scenariosTab");
    Params::Scenarios& scenarios = recipe.scenarios;
    const Io::ScenarioNameValidationReport nameReport = Io::ValidateScenarioNames(scenarios);
    DrawScenarioSettings(scenarios, state.settingsSection, recipe.armies);
    DrawScenarioPatternTier(scenarios, state, recipe.armies, recipe.areas, nameReport);
    DrawScenarioCountTier(scenarios, state, recipe.armies, recipe.areas, nameReport);
    DrawScenarioDefaultTier(scenarios, state, recipe.armies, recipe.areas, nameReport);
    DrawScenarioMatrix(scenarios, state.matrixSection);
    DrawScenarioRuntimeScriptSection(state);
    ImGui::PopID();
}
```

Widen every function on the chain to add `const Io::ScenarioNameValidationReport& nameReport` as a new
trailing parameter (exact same threading shape STEP209 already used for `areas` through this identical
call chain — add it, don't reorder existing params):

- `ScenariosTab_UI.h`: `DrawScenarioBodyFields` declaration (line 145) — add the param.
- `ScenariosTab_Detail_UI.cpp`: `DrawScenarioBodyFields` definition (line 169) — add the param; use it
  at §6a's insertion point.
- `ScenariosTab_Lists_UI.cpp`: `DrawScenarioPatternTier` (line 39), `DrawScenarioCountTier` (line 56),
  `DrawScenarioDefaultTier` (line 74) — each adds the param and forwards it to its own
  `DrawScenarioPatternList`/`DrawScenarioCountList`/`DrawScenarioBodyFields` call (lines 48, 65, 82-83).
- `ScenariosTab_ListMechanics_UI.h`: `DrawScenarioPatternList` (line 83) and `DrawScenarioCountList`
  (line 106) — each adds the param and forwards it to its own `DrawScenarioBodyFields` call (lines 100,
  122).

`#include "../io/ScenarioNameValidation_IO.h"` needs adding wherever `Io::ScenarioNameValidationReport`
is not already visible (at minimum `ScenariosTab_UI.h`, since it appears in that header's own
declaration).

## 7. Tests

### 7a. New file `src/io/ScenarioNameValidation_IO_Test.cpp` (pure, no imgui/GL)

1. Empty `Scenarios` (no pattern/count entries, default has its own default-constructed empty `name`)
   — the empty default `name` itself is a charset violation (empty string). Assert exactly one
   violation, for the default scenario, charset reason.
2. Valid, unique names across all three tiers (`"1v1"`, `"slots5to8AnyFilled"`, `"4human"`, etc. — the
   exact live reference names from `MAP_SCENARIO_SPEC.md` §5.2) — assert `AllNamesValid() == true`,
   proving the corrected (leading-digit-permitting) charset from `ARCH_15_05`'s amendment is actually
   implemented, not the earlier rejected leading-letter-required draft.
3. **`"1v1"` passes, `"My Scenario!"` fails** (the task's own required examples) — assert the space+`!`
   name is flagged with the charset reason, `"1v1"` is not.
4. **Two names differing only by case fail** (`"FooBar"` in `patternScenarios[0]`, `"foobar"` in
   `defaultScenario`) — assert BOTH appear as violations with the duplicate reason, and
   `FindViolationReasonForName("FooBar")`/`FindViolationReasonForName("foobar")` both return non-null.
5. Reserved-name cases: `"Runtime"`, `"runtime"`, `"RUNTIME"`, `"Data"`, `"data"` each individually —
   assert each is flagged with the reserved-name reason (case-insensitive).
6. A charset-invalid name is NOT also flagged as a duplicate/reserved violation even if it happens to
   collide with another entry (first-failing-rule-wins) — construct a case that would trip two rules at
   once and assert exactly one violation for that scenario.
7. `SummaryText()` on a clean report is empty; on a dirty report contains every violated name and its
   reason substring.
8. `FindViolationReasonForName` returns `nullptr` for a name with zero violations.

### 7b. New file `src/io/ScenarioScript_CategoryFourExport_IO_Test.cpp` (filesystem-touching, scratch-folder
pattern per `ScenarioScript_Export_IO_Test.cpp:26-32`)

9. **Scaffold created when missing.** One `spawnsUnits = true` default scenario, valid name, empty
   `mapScriptDirectory`. Assert the file `<MapName>_Scenarios_<Name>.lua` now exists, its content opens
   with `kScenarioCategoryFourScaffoldBannerLine`, contains `"function GenerateScenarioUnits(area)"`,
   and `report.scaffoldedFilePaths` contains exactly that path.
10. **Scaffold skipped when present, in any state.** Pre-seed the target path with (a) an untouched
    prior scaffold's own text, and separately (b) arbitrary hand-edited content with no banner at all.
    In both cases, assert the file's bytes are byte-identical before/after the call, and
    `report.scaffoldedFilePaths` is empty.
11. **`spawnsUnits == false` scenarios are never written**, valid or invalid name alike — assert no file
    appears for them regardless of name validity.
12. **Invalid name refuses only that scenario's write.** Two `spawnsUnits = true` scenarios, one with a
    valid name, one with `"My Scenario!"` — assert the valid one's file is written, the invalid one's is
    not, and `report.writeRefusals` names the invalid scenario and its reason substring.
13. **Default scenario is exported symmetrically** (defense-in-depth, §2b's own comment) — a
    `spawnsUnits = true` `defaultScenario` with a valid name gets its own scaffold, exactly like a
    pattern/count entry would.
14. Dispatcher's generated Lua text is syntactically valid: assert
    `Sys::CheckLuaSyntax(scaffoldFileContentReadBack).bSucceeded == true` on the file actually written in
    test 9 — the same gate this file family already applies to every other Lua text it writes.

### 7c. Extend `src/io/ScenarioScript_Export_IO_Test.cpp`

15. **End-to-end wiring.** A recipe with one `spawnsUnits = true`, validly-named scenario, run through
    the real `ExportMapScenario`. Assert the category-4 file exists on disk at the expected path,
    `result.writtenFilePaths` contains it, and `result.debugLog` contains "Scaffolded new category-4".
16. **Invalid-name wiring.** Same but the scenario's name is `"Bad Name"`. Assert `result.debugLog`
    contains the refusal message and no category-4 file was written, while `result.bDataLuaWritten` and
    `result.bRuntimeCopied` both stay `true` (an invalid scenario name never blocks the other two legs).

### 7d. Extend `src/io/ScenarioScript_RuntimeResource_IO_Test.cpp`'s `TestRealBundledResourceSelfCheck`

17. Add `Check(text.find("function Scenario.SpawnMatchedScenarioUnits") != std::string::npos, "the
    generic per-scenario dispatcher is defined");`.
18. Add a negative assertion that the old declining comment is gone:
    `Check(text.find("is deliberately NOT added here") == std::string::npos, "the ARCH_15_05 OPEN-item
    placeholder comment was replaced, not merely supplemented");`.
19. **Resolve this file's own pre-existing TODO (d)** — `Sys::CheckLuaSyntax` (STEP65) now exists in this
    worktree (it did not when that TODO was written): add
    `Check(Sys::CheckLuaSyntax(text).bSucceeded, "the real bundled resource is syntactically valid Lua");`
    and `#include "../sys/LuaSyntaxCheck_SYS.h"`. This closes a stale TODO this ticket's own dependency
    (`Sys::CheckLuaSyntax`) happens to unblock — flag it in the PR/commit notes as an incidental fix, not
    silently bundled.

### 7e. Extend `src/io/MapExporter_IO_Test.cpp`

20. **`.sanmap`-leg wiring** (mirrors STEP209 item 11 exactly). A recipe with one invalidly-named
    scenario, exported via `Io::MapExporter::ExportSanmapOnly`. Assert `result.bSucceeded == true`
    (never blocks), `result.warningCount >= 1`, and `result.debugLog` contains the offending name.

## 8. CMakeLists.txt

Add two new test targets, mirroring the `add_sangen_test(MapExporter_ScenarioAreaNameValidation_IO_Test
...)` entry (lines 1005-1009) and `ScenarioScript_Export_IO_Test`'s entry (line 1144) exactly:

```cmake
# STEP251_ScenarioCategoryFourExport_IO: pure charset/uniqueness/reserved-name validation of
# ScenarioBody::name, shared by the UI's inline warning and the category-4 export gate.
add_sangen_test(ScenarioNameValidation_IO_Test src/io/ScenarioNameValidation_IO_Test.cpp)

# STEP251_ScenarioCategoryFourExport_IO: the scaffold-once-if-missing category-4 file writer. Pure
# string-building + std::filesystem + Sys::CheckLuaSyntax, no JSON type in sight -- no nlohmann link
# needed, same posture as ScenarioScript_Export_IO_Test.
add_sangen_test(ScenarioScript_CategoryFourExport_IO_Test src/io/ScenarioScript_CategoryFourExport_IO_Test.cpp)
```

## 9. Verify

- Full solo rebuild + `ctest -C Debug`: 100% pass, including every new/extended test above.
- `grep -rn "SpawnMatchedScenarioUnits" resources/lua/SanGenScenarioRuntime.lua` shows the real function
  definition, not the declining comment.
- `grep -rn "ScenarioNameValidation_IO.h" src/` shows exactly: the new `.h`/`.cpp`/`_Test.cpp`,
  `ScenarioScript_CategoryFourExport_IO.cpp`, `MapExporter_IO.cpp`, and every UI file touched in §6c —
  never inside `ScenarioScript_DataLua_IO.cpp` or `MapExporter_Scenarios_IO.cpp` (name rendering itself
  is unchanged; only a new, separate validation pass is added).
- Manually export a recipe with a `spawnsUnits = true`, freshly-added scenario through the real Files
  tab flow (build only — do not hand-run the resulting map, per this project's "no manual/interactive
  testing" policy): confirm the scaffold file appears once, re-exporting twice more never changes its
  bytes even after hand-editing it between exports.

## 10. Out of scope

- Whether per-scenario unit-spawn generator logic ever becomes declarative `PARAMS` data instead of
  hand-authored Lua (`ARCH_15_05`'s still-OPEN item 1) — untouched by this ticket; category 4 stays
  hand-authored Lua exactly as ratified.
- Migrating the live `Pandemonium Isthmus` map's existing `_Scenarios_Script.lua` content into the new
  four-file shape — `MAP_SCENARIO_SPEC.md` §14's own "one-time human action," never automated by SanGen.
- Promoting `kErrorTextColor`/`kWarningTextColor` out of per-file duplication into a shared UI header —
  flagged as a cheap, optional follow-up in §6a, not required here.
- A "reveal in Explorer" / "open generator file" convenience button next to a scaffolded category-4
  file. **Flagged as an optional nice-to-have**, not required: it would need only the already-computed
  category-4 file path (trivially derivable from `recipe.mapName` + `body.name`) and the platform's
  existing file-reveal primitive (if one already exists elsewhere in this codebase's Files tab — not
  verified here). Do not build it as part of this ticket without a separate go-ahead.
- Adding a charset-filter capability to the shared `TextInputRules`/`DrawTextInput` widget — §6's own
  reasoning explains why a read-only inline warning was chosen instead.
- Formally amending `ARCH_15_05_ParamsScenariosType.md`'s text to record the reserved-name guard (§1b
  point 2) as a named sub-rule — that is the ARCH Expert's file; this ticket only flags it for that
  expert's attention and implements the necessary consequence now.
