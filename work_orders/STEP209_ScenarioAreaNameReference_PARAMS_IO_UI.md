# STEP209 — `ScenarioBody::areaName` named-Area reference (PARAMS shape + export-time resolution + import + UI)

**Layers:** PARAMS, IO, UI. **Domain:** `Params::ScenarioBody`'s new `areaName` field, its export-time
resolution against `recipe.areas`, its `.sanmap` round-trip, and its UI authoring surface.
**Sequence:** Map Scenario track (follows STEP204). **Authorized by:** `ARCH_15_05_ParamsScenariosType.md`
§15.5's "AMENDED 2026-08-28 — `ScenarioBody::areaName`" section (binding law) and
`MAP_SCENARIO_SPEC.md` §6.2 (companion documentation). Both already rule every open design question
this ticket implements — round-trips (not bake-only), stale-reference behavior (fallback + loud
non-blocking warning), and duplicate-name resolution (first-match). Do not re-litigate those; they are
settled. This work-order is the mechanical translation into real files.

## 0. Why

A `Scenario` today holds a fully disconnected private rectangle (`ScenarioBody::area`). The human
ruled a scenario should be able to **reference** a named `Params::MapArea` from `recipe.areas` instead
— so resizing the named Area updates every scenario that references it, at the next export, without
per-scenario re-authoring. `ScenarioBody::areaName` (empty by default = today's exact behavior) is the
reference; it resolves into the flat rect the wire format already understands (`Area`/`area`) **at
export time only**, never bake-once.

## 1. ⚠️ Real-code discrepancy found during verification — read before implementing §5

`ARCH_15_05`'s amendment claims **"Zero changes anywhere else. `ScenarioScript_DataLua_IO.cpp`,
`resources/lua/SanGenScenarioRuntime.lua`, and the runtime's `ResolveAndApply`/`ApplyScenario` never
see `areaName`/`AreaName` at all — they only ever consume the single resolved flat rect the exporter
already writes into `area`/`Area`, exactly as today."**

**This is correct for the STRING (`areaName`/`AreaName` itself never needs to reach Lua — confirmed:
`resources/lua/SanGenScenarioRuntime.lua`'s `ResolveAndApply` returns `chosenArea` straight from the
matched scenario's own `area` field with no name-lookup capability). It is WRONG for the NUMBERS.**

Direct read of the real code proves there is no single shared "the exporter" that resolves the rect
once for both artifacts:

- `MapExporter_Scenarios_IO.cpp`'s `BuildScenarioRecordJson` (`.sanmap` JSON leg) reads
  `body.area.originX`/`originZ`/`width`/`length` directly (lines 65-66) and is called only from
  `BuildScenariosJson`, itself called only from `MapExporter::BuildSanmapJsonText` — the **`.sanmap`
  round-trip artifact**.
- `ScenarioScript_DataLua_IO.cpp`'s `AppendScenarioBodyFields` (lines 92-97) **independently** reads
  `body.area.originX`/`originZ`/`width`/`length` directly, called only from
  `BuildPatternScenariosTable`/`BuildCountScenariosTable`/`BuildDefaultScenarioTable`, themselves
  called only from `BuildScenarioDataLuaText` — the **`<MapName>_Scenarios_Data.lua` artifact the
  running game actually reads** (`MAP_SCENARIO_SPEC.md` §14).
- These two builders live in different files, are never called from a shared parent, and — per
  `ScenarioScript_Export_IO.h`'s own header comment — are triggered by **two entirely separate UI
  actions with two entirely separate result types** (`MapExporter::ExportSanmapOnly`/`ExportAll` →
  `MapExportResult`, vs. `ScenarioScript_Export_IO::ExportMapScenario` → `ScenarioExportResult`,
  "never merged"). There is no code path where resolving the rect once feeds both.

**Consequence if §5 below is skipped:** a user picks a named Area for a scenario, then later resizes
that Area in the Areas tab without reselecting it in the Scenarios tab (a normal workflow — nothing
requires reselection). `body.area` (the UI's "live-preview copy", written only at selection time,
§7 below) is now stale. The `.sanmap` JSON leg (§4) re-resolves fresh against `recipe.areas` at every
export and gets the correct, current rectangle. **The Lua leg, if left untouched, keeps rendering the
stale `body.area` values into `<MapName>_Scenarios_Data.lua` — the file the game actually loads.** The
two SanGen-authored artifacts silently diverge, and the one that matters for gameplay is the wrong one.

**Resolution:** §5 below threads the identical resolved-rect-with-fallback algorithm into the Lua leg
too, via a small dedicated helper duplicated in that file (matching this exact file's own established
"each leg owns its own copy" precedent — see its top-of-file comment re: `kScenarioAlloyModeSpellings`
etc.). This is flagged to the human as a **correction to ARCH_15_05's stated verification**, not an
architectural amendment: the ratified *algorithm* (first-match resolve, fallback on miss, never crash)
is unchanged and is simply applied at a second real call site the ARCH ruling's own verification missed.
No new design question is opened. **If the ARCH Expert wants to record this as a formal correction to
`ARCH_15_05_ParamsScenariosType.md`, that is the ARCH Expert's file to write — flagging it here per the
Format Expert's "operate WITHIN the ARCH, escalate discrepancies" charter, not silently overriding it.**

## 2. PARAMS — `src/params/Scenario_PARAMS.h`

Insert immediately after line 35 (`Params::MapArea area;`) and before line 36
(`bool spawnsUnits = false;`):

```cpp
std::string areaName;                                  // empty (default) = "use area directly" --
                                                        // today's exact behavior, fully backward
                                                        // compatible. Non-empty names a live
                                                        // Params::MapArea::name in recipe.areas,
                                                        // resolved into the wire Area/area rect AT
                                                        // EXPORT TIME ONLY (ARCH_15_05_ParamsScenariosType.md
                                                        // §15.5, "AMENDED 2026-08-28").
```

No other struct in this file changes. `std::string` default-constructs empty; no explicit initializer
needed beyond the field declaration itself (matches every other `std::string` field in this file, e.g.
`name`/`authoringNote`, which also carry no `= ""`).

## 3. IO — export, `.sanmap` JSON leg (`src/io/MapExporter_Scenarios_IO.cpp`)

### 3a. New private resolver (add near the top of the anonymous namespace, after `BuildPositionJson`,
before `BuildScenarioRecordJson`)

```cpp
// Resolves body.areaName against `areas` (first-match by .name, mirroring this exact file family's
// own established idiom -- AreasTab_List_UI.h's ResolveAreaColor, UniqueNameList_UI.h's
// NameIsTakenBefore -- both resolve by first/earliest match, never last-wins). Empty areaName or an
// unresolvable (stale) name both fall back to body.area unchanged -- never crash, never emit garbage
// (ARCH_15_05_ParamsScenariosType.md §15.5 AMENDED 2026-08-28).
Params::MapArea ResolveScenarioAreaRect(const Params::ScenarioBody& body,
                                        const std::vector<Params::MapArea>& areas) {
    if (body.areaName.empty()) return body.area;
    for (const Params::MapArea& area : areas)
        if (area.name == body.areaName) return area;
    return body.area;
}
```

### 3b. `BuildScenarioRecordJson` signature + body (currently lines 60-77)

Widen the signature:

```cpp
nlohmann::ordered_json BuildScenarioRecordJson(const Params::ScenarioBody& body, int mapSize,
                                               const std::vector<Params::MapArea>& areas) {
```

Replace the `Area` emission (currently lines 63-66) with:

```cpp
    // Resolved against recipe.areas when body.areaName names a live entry; falls back to body.area
    // otherwise (empty areaName, or a stale/renamed/deleted reference). See ResolveScenarioAreaRect.
    const Params::MapArea resolvedArea = ResolveScenarioAreaRect(body, areas);
    json["Area"] = { { "x", resolvedArea.originX }, { "y", resolvedArea.originZ },
                     { "width", resolvedArea.width }, { "height", resolvedArea.length } };
    // Sibling of Area, always emitted even when empty -- matches SpawnsUnits/AuthoringNote's own
    // "every scalar field always present" convention (ARCH_15_05_ParamsScenariosType.md §15.5
    // AMENDED 2026-08-28: round-trip the reference, never export-only bake).
    json["AreaName"] = body.areaName;
```

`#include`: none needed — `Params::MapArea` is already visible via this file's existing
`#include "../params/MapRecipe_PARAMS.h"` (which includes `MapArea_PARAMS.h`).

### 3c. All three real call sites inside `BuildScenariosJson` (verified by direct read — these are the
only call sites in the file; there is no fourth)

- Line 87 (inside the `patternScenarios` loop):
  `BuildScenarioRecordJson(pattern.body, mapSize)` → `BuildScenarioRecordJson(pattern.body, mapSize, recipe.areas)`

  Concretely: `recipe.areas` — `recipe` is already `BuildScenariosJson`'s own parameter, already in
  scope at every one of these three call sites; the local `mapSize` variable already reads
  `recipe.geometry.mapSize` at the top of the function.
- Line 96 (inside the `countScenarios` loop):
  `BuildScenarioRecordJson(countScenario.body, mapSize)` → `BuildScenarioRecordJson(countScenario.body, mapSize, recipe.areas)`
- Line 110 (`defaultScenario`):
  `BuildScenarioRecordJson(scenarios.defaultScenario, mapSize)` → `BuildScenarioRecordJson(scenarios.defaultScenario, mapSize, recipe.areas)`

`BuildScenariosJson(const Params::MapRecipe& recipe)`'s own public signature (declared
`MapExporter_Recipe_IO.h:150`) is **unchanged** — only the file-local (anonymous-namespace)
`BuildScenarioRecordJson` widens; it is never declared in a header, so no other file needs editing for
this signature change.

## 4. IO — export-time stale-reference warning (new files, mirroring `MapExporter_ArmySpawnMarkerValidation_IO.h`/`.cpp` exactly)

Do **not** thread `MapExportResult`/`ScenarioExportResult` into the pure JSON/Lua builders above — both
`BuildSanmapJsonText` and `BuildScenarioDataLuaText` are documented, established "pure, disk-free, no
result object" builders (see each file's own header comment) and stay that way, matching the existing
precedent that blueprintPath/army-spawn-marker validation are **separate pre-flight passes**, not
folded into the JSON/Lua builders.

### 4a. New file `src/io/MapExporter_ScenarioAreaNameValidation_IO.h`

```cpp
// MapExporter_ScenarioAreaNameValidation_IO.h -- `ScenarioAreaNameValidationReport` + the export-time
// ScenarioBody::areaName -> recipe.areas membership scan (STEP209). Layer: IO. Modelled directly on
// the sibling MapExporter_ArmySpawnMarkerValidation_IO.h: a report struct with a one-wording
// SummaryText(), plus a pure Validate* free function, same tier as recipe.IsValid(), never called
// from inside BuildSanmapJsonText/BuildScenarioDataLuaText.
//
// SHARED by both export legs (the .sanmap JSON leg, MapExporter_IO.cpp, and the Lua-rendering leg,
// ScenarioScript_Export_IO.cpp) -- unlike the per-leg spelling-table duplication precedent elsewhere
// in this file family, this validator carries no wire-format-specific content (no JSON, no Lua text),
// so there is no reason to fork it; it is pure Params-level logic.
#pragma once
#include <string>
#include <vector>

namespace SanmapGen {
namespace Params { struct MapRecipe; }
namespace Io {

// One export-time areaName -> recipe.areas membership pass. WARN-ONLY: a stale reference (the named
// Area was renamed or deleted after a scenario picked it) is a legal, tolerated state
// (ARCH_15_05_ParamsScenariosType.md §15.5 AMENDED 2026-08-28) -- never auto-cleared, never blocking.
struct ScenarioAreaNameValidationReport {
    std::vector<std::string> staleReferences;   // one entry per scenario with a non-empty areaName
                                                 // not found in recipe.areas, in tier-then-vector
                                                 // order (pattern, then count, then default)
    bool AllReferencesResolve() const { return staleReferences.empty(); }
    std::string SummaryText() const;   // ONE wording -- shared by every call site
};

// Pure/read-only, touches no disk, never called from inside BuildSanmapJsonText/
// BuildScenarioDataLuaText -- same tier as recipe.IsValid().
ScenarioAreaNameValidationReport ValidateScenarioAreaNameReferences(const Params::MapRecipe& recipe);

} // namespace Io
} // namespace SanmapGen
```

### 4b. New file `src/io/MapExporter_ScenarioAreaNameValidation_IO.cpp`

Walk `recipe.scenarios.patternScenarios`, then `.countScenarios`, then `.defaultScenario` (same tier
order §6.1/§14 of `MAP_SCENARIO_SPEC.md` already establish elsewhere in this file family). For each
`ScenarioBody` with non-empty `areaName`, first-match-scan `recipe.areas` for `.name == areaName`
(same idiom as §3a's `ResolveScenarioAreaRect` — this file owns its own copy of the lookup, matching
the established per-file-duplication precedent for the *lookup*, even though the *report/warning* type
itself is shared per 4a's note). On a miss, append one entry to `staleReferences` naming the scenario
(`body.name`, falling back to a tier+index descriptor if `name` is empty — mirror
`ScenarioRowLabel`'s "never blank" posture) and the missing `areaName`.

`SummaryText()`: mirror `ArmySpawnMarkerValidationReport::SummaryText()`'s wording shape exactly —
empty string when `AllReferencesResolve()`; otherwise a count line, one indented line per stale entry,
and a closing sentence explaining the fallback-to-last-known-rectangle behavior so the warning is
actionable, not just alarming. E.g.:

```
"N scenario(s) reference an Area name not found in recipe.areas:\n  <scenario> -> \"<areaName>\"\n...
The exported rectangle falls back to that scenario's own last-known Area values; the reference itself
is kept as-authored (never silently cleared). Re-pick the Area in the Scenarios tab, or re-add an Area
with that name, to resolve it."
```

### 4c. Wire into `MapExporter_IO.cpp`

Add `#include "MapExporter_ScenarioAreaNameValidation_IO.h"`. Add a small wrapper (same shape as the
existing `ReportArmiesWithoutSpawnMarkers`, lines 73-80):

```cpp
// STEP209 -- warn, never block. A stale ScenarioBody::areaName is a legal, tolerated state (the named
// Area was renamed/deleted after the scenario picked it) -- ARCH_15_05_ParamsScenariosType.md §15.5
// AMENDED 2026-08-28 rules "never crash, never emit garbage", exporting the scenario's own last-known
// rectangle instead. This function only reports.
void ReportScenarioAreaNameReferences(const Params::MapRecipe& recipe, MapExportResult& result) {
    const ScenarioAreaNameValidationReport report = ValidateScenarioAreaNameReferences(recipe);
    if (report.AllReferencesResolve()) return;
    result.Warn(report.SummaryText());
}
```

Call it inside `WriteSanmapDocument` (line 17-53), immediately after the existing
`CheckArmyIdentitiesWellFormed(recipe.armies, result);` at line 25:

```cpp
    ReportScenarioAreaNameReferences(recipe, result);
```

This fires for both `ExportSanmapOnly` and `ExportAll` automatically (both call `WriteSanmapDocument`),
exactly like `CheckArmyIdentitiesWellFormed` already does.

## 5. IO — export, Lua-rendering leg (`src/io/ScenarioScript_DataLua_IO.cpp` + `ScenarioScript_Export_IO.cpp`)

**This section exists because of §1's discrepancy — it is real, required work, not "zero changes".**

### 5a. New private resolver (add near `FlipPositionZ`, lines 37-39 — same file, same "each leg owns
its own copy" precedent this file's top-of-file comment already documents for the spelling tables)

```cpp
// Duplicate of MapExporter_Scenarios_IO.cpp's own ResolveScenarioAreaRect (STEP209) -- this file
// never #includes that one (established precedent, see this file's own top-of-file note on
// kScenarioAlloyModeSpellings). Resolves body.areaName against `areas` (first-match by .name);
// empty or stale falls back to body.area unchanged.
Params::MapArea ResolveScenarioAreaRect(const Params::ScenarioBody& body,
                                        const std::vector<Params::MapArea>& areas) {
    if (body.areaName.empty()) return body.area;
    for (const Params::MapArea& area : areas)
        if (area.name == body.areaName) return area;
    return body.area;
}
```

### 5b. `AppendScenarioBodyFields` (currently lines 89-114) — widen signature and use the resolved rect

```cpp
void AppendScenarioBodyFields(std::string& out, int indentLevel, const Params::ScenarioBody& body,
                              int mapSize, const std::vector<Params::MapArea>& areas) {
    AppendKeyValueLine(out, indentLevel, "name", QuotedLuaString(body.name));

    const Params::MapArea resolvedArea = ResolveScenarioAreaRect(body, areas);
    OpenTable(out, indentLevel, "area");
    AppendKeyValueLine(out, indentLevel + 1, "x", RenderLuaNumber(resolvedArea.originX));
    AppendKeyValueLine(out, indentLevel + 1, "y", RenderLuaNumber(resolvedArea.originZ));
    AppendKeyValueLine(out, indentLevel + 1, "width", RenderLuaNumber(resolvedArea.width));
    AppendKeyValueLine(out, indentLevel + 1, "height", RenderLuaNumber(resolvedArea.length));
    CloseTable(out, indentLevel, true);
    // ... rest of the function (spawnsUnits through authoringNote) unchanged.
```

No `areaName`/`AreaName` key is ever rendered here — this is the part of ARCH_15_05's claim that IS
correct; only the four `area` numbers change source.

### 5c. Thread `areas` through the three tier builders and their one caller

- `BuildPatternScenariosTable` (line 121): add `const std::vector<Params::MapArea>& areas` param; its
  call at line 128 becomes `AppendScenarioBodyFields(out, 1, entry.body, mapSize, areas);`
- `BuildCountScenariosTable` (line 137): same — add `areas`; its call at line 156 becomes
  `AppendScenarioBodyFields(out, 1, entry.body, mapSize, areas);`
- `BuildDefaultScenarioTable` (line 165): same — add `areas`; its call at line 168 becomes
  `AppendScenarioBodyFields(out, 1, body, mapSize, areas);`
- `BuildScenarioDataLuaText` (line 257): its three call sites (lines 276, 278, 280) pass `recipe.areas`
  — `recipe` is already this function's own parameter, no new plumbing needed above it.

None of these four functions is declared in `ScenarioScript_DataLua_IO.h` (only `BuildScenarioDataLuaText`
is, and its own signature is unchanged) — all four signature widenings are file-local to
`ScenarioScript_DataLua_IO.cpp`.

### 5d. Wire the same warning into `ScenarioScript_Export_IO.cpp`

Add `#include "MapExporter_ScenarioAreaNameValidation_IO.h"` (the shared header from §4a — reused as-is,
not duplicated: it carries no JSON/Lua-specific content).

Immediately after line 82 (`const std::string dataLuaText = BuildScenarioDataLuaText(recipe);`), add:

```cpp
    const ScenarioAreaNameValidationReport areaNameReport = ValidateScenarioAreaNameReferences(recipe);
    if (!areaNameReport.AllReferencesResolve()) result.Log(areaNameReport.SummaryText());
```

`ScenarioExportResult` has no `Warn`/`warningCount` (only `Log`/`debugLog`, per its own struct shape,
`ScenarioScript_Export_IO.h` lines 20-35) — use `result.Log(...)` here, matching this type's own
existing convention (every other finding in this file uses `result.Log`, never a `Warn` that does not
exist on this type).

## 6. IO — import (`src/io/MapImporter_ScenarioRecord_IO.cpp`)

`ReadScenarioBodyJson` (lines 80-101): insert immediately after the existing `Area` block (line 88)
and before `SpawnsUnits` (line 90):

```cpp
    // Absent key (every pre-STEP209 .sanmap) -> stays at the struct default, empty. Never an error --
    // same idiom as every other plain string field in this function (e.g. AuthoringNote, line 100).
    ReadJsonText(json, "AreaName", body.areaName);
```

This is the exact idiom `ReadJsonText(json, "AuthoringNote", body.authoringNote)` (line 100) already
uses for a plain optional string: no presence check, no special missing-key branch — `ReadJsonText`
itself no-ops when the key is absent, leaving the struct default (empty) in place.

## 7. UI (`src/ui/ScenariosTab_Detail_UI.cpp` + its threading chain)

### 7a. `DrawScenarioAreaFields` (currently lines 60-69) — widen and add the Combo + read-only gating

Add `#include "AreasTab_List_UI.h"` (for `AreaRowLabel`, reused directly per the ARCH ruling — do not
reinvent it).

Replace the function:

```cpp
// The Combo is NOT DrawArmyNameField's exact shape: empty areaName is a real, permanent, authored
// state here ("this scenario owns its own private rectangle"), unlike DrawArmyNameField's transient
// "not chosen yet" -- so this needs one extra leading sentinel entry DrawArmyNameField does not have
// (ARCH_15_05_ParamsScenariosType.md §15.5 AMENDED 2026-08-28).
void DrawScenarioAreaFields(Params::ScenarioBody& body, const std::vector<Params::MapArea>& areas) {
    std::vector<const char*> labels;
    labels.reserve(areas.size() + 1u);
    labels.push_back("-- Custom (no Area reference) --");
    int selectedIndex = body.areaName.empty() ? 0 : -1;   // -1 = stale reference, matches
                                                          // DrawArmyNameField's own no-match idiom
    for (std::size_t index = 0u; index < areas.size(); ++index) {
        labels.push_back(AreaRowLabel(areas[index]));
        if (!body.areaName.empty() && areas[index].name == body.areaName)
            selectedIndex = static_cast<int>(index) + 1;
    }
    ComboOptions options; options.labels = labels.data(); options.count = static_cast<int>(labels.size());
    if (DrawCombo("Reference Area", selectedIndex, options).bCommitted) {
        if (selectedIndex == 0) {
            body.areaName.clear();
        } else if (selectedIndex > 0) {
            const Params::MapArea& picked = areas[static_cast<std::size_t>(selectedIndex - 1)];
            body.areaName = picked.name;
            // Live-preview copy: the four rect scalars only -- NEVER picked.name (Scenario_PARAMS.h's
            // own comment: area.name is never populated/read anywhere on the wire; leaving it alone
            // keeps that invariant true after a Combo pick, not just at default-construction).
            body.area.originX = picked.originX;
            body.area.originZ = picked.originZ;
            body.area.width   = picked.width;
            body.area.length  = picked.length;
        }
    }

    // Read-only while referenced (ARCH ruling: rejected alternative was editable-with-silent-clear-
    // on-edit -- a slider nudge silently detaching a scenario from its named Area is worse than a
    // slider that visibly refuses input). Sliders stay VISIBLE (never hidden) so resolved numbers are
    // never a black box -- only interaction is disabled.
    const bool bReferenced = !body.areaName.empty();
    ImGui::BeginDisabled(bReferenced);
    const ScalarSliderRange range = ScenarioWorldPositionRange();
    RealtimeToggle originXToggle, originZToggle, widthToggle, lengthToggle;
    DrawSliderScalar("Area Origin X", body.area.originX, range, originXToggle, WidgetStyle(), "%.1f");
    DrawSliderScalar("Area Origin Z", body.area.originZ, range, originZToggle, WidgetStyle(), "%.1f");
    DrawSliderScalar("Area Width", body.area.width, range, widthToggle, WidgetStyle(), "%.1f");
    DrawSliderScalar("Area Length", body.area.length, range, lengthToggle, WidgetStyle(), "%.1f");
    ImGui::EndDisabled();
}
```

Update its one call site (line 140, inside `DrawScenarioBodyFields`):
`DrawScenarioAreaFields(body.area);` → `DrawScenarioAreaFields(body, areas);`

### 7b. Thread `areas` through `DrawScenarioBodyFields` and every caller

- `ScenariosTab_UI.h` (declaration, lines 145-148) and `ScenariosTab_Detail_UI.cpp` (definition, line
  132): add `const std::vector<Params::MapArea>& areas` as a new parameter (place it next to `armies`
  for readability — exact position is the coder's call, not load-bearing).
- `ScenariosTab_ListMechanics_UI.h` (private to `ScenariosTab_Lists_UI.cpp`):
  - `DrawScenarioPatternList` (line 83-86): add `const std::vector<Params::MapArea>& areas` param;
    its `DrawScenarioBodyFields` call (lines 99-100) passes it through.
  - `DrawScenarioCountList` (line 105-107): same; its call (lines 120-121) passes it through.
- `ScenariosTab_Lists_UI.cpp`:
  - `DrawScenarioPatternTier` (line 39-40), `DrawScenarioCountTier` (line 55-56),
    `DrawScenarioDefaultTier` (line 72-73): each adds a `const std::vector<Params::MapArea>& areas`
    param and passes it to its respective `DrawScenarioPatternList`/`DrawScenarioCountList`/
    `DrawScenarioBodyFields` call.
  - `DrawScenariosTab` (line 87-97): pass `recipe.areas` to all three Tier calls (lines 91-93) —
    `recipe` is already this function's own parameter, `recipe.areas` needs no new plumbing above it.

None of `DrawScenarioPatternTier`/`DrawScenarioCountTier`/`DrawScenarioDefaultTier`/
`DrawScenarioPatternList`/`DrawScenarioCountList` is declared in any header — they are anonymous-
namespace/private-header-inline functions local to their one translation unit each, so this threading
never ripples beyond the two files named above plus `ScenariosTab_UI.h`'s one `DrawScenarioBodyFields`
declaration.

`Params::MapArea`/`std::vector` are already visible in every file touched here (via each file's own
existing `#include "ScenariosTab_UI.h"` → `"../params/MapRecipe_PARAMS.h"` → `MapArea_PARAMS.h` chain,
or `ScenariosTab_UI.h`'s own direct include) — no new `#include` needed beyond §7a's `AreasTab_List_UI.h`.

## 8. Tests and fixtures

### 8a. `src/io/MapImporter_Scenarios_IO_Test.cpp` (pure, no imgui/GL — "THE SPLIT")

Do **not** add a non-empty `areaName` to the existing full-coverage `PopulateFullScenarioBody` fixture
(it would make every existing round-trip check ambiguous about which code path produced the numbers) —
instead add **new, dedicated tests**:

1. **Hit resolves correctly.** `recipe.areas = {{name="Foo", originX=5,originZ=6,width=7,length=8}}`;
   `defaultScenario.body.areaName = "Foo"`; `defaultScenario.body.area` set to deliberately different
   stale values (e.g. all `0.0f`, so a passthrough bug is caught, not masked). Call
   `Io::BuildScenariosJson(recipe)`. Assert `["DefaultScenario"]["Area"]` == `{5,6,7,8}` (NOT the stale
   zeros) and `["AreaName"] == "Foo"`.
2. **Empty `areaName` behaves identically to today.** `areaName` empty, `body.area` some rect. Assert
   exported `Area` == `body.area` verbatim, `AreaName == ""`.
3. **Unresolvable/stale falls back correctly.** `areaName = "DoesNotExist"`, not present in
   `recipe.areas`. Assert exported `Area` == `body.area`'s own values verbatim, `AreaName ==
   "DoesNotExist"` (never cleared, per the ARCH ruling's "never silently cleared" requirement).
4. **Duplicate names resolve first-match.** `recipe.areas = {{name="Dup", originX=1,...}, {name="Dup",
   originX=99,...}}`; `areaName = "Dup"`. Assert resolved `Area.x == 1` (the FIRST entry), proving
   first-match-wins, not last-wins.
5. **Import round trip.** Build a document with `"AreaName": "Foo"` present on `DefaultScenario`.
   `Io::ReadScenariosJson(document, loaded, result)`. Assert `loaded.scenarios.defaultScenario.areaName
   == "Foo"`.
6. **Absent `AreaName` key (legacy fixture).** Extend `RunAbsentKeyDefaultsTest` (or add a sibling
   check) asserting `loaded.scenarios.defaultScenario.areaName.empty()` and `result.warningCount == 0`
   — a pre-STEP209 `.sanmap` has no such key at all.
7. **Full round trip via the live document path.** Extend `RunLiveDocumentIntegrationTest`
   (`BuildSanmapJsonText` → `ParseSanmapJsonText`) with one scenario carrying a resolved `areaName`,
   confirming `ScenarioBody::areaName` survives end to end (this is the round-trip test the dispatch
   explicitly asked for — do it via the live document path, not only the pure `BuildScenariosJson`/
   `ReadScenariosJson` path, since that is what actually proves the whole pipe).

### 8b. New file `src/io/MapExporter_ScenarioAreaNameValidation_IO_Test.cpp` (mirrors
`MapExporter_ArmySpawnMarkerValidation_IO_Test.cpp`'s structure — check that file for the exact
scaffold/`main()` shape before writing this one)

8. Hit case and empty-`areaName` case both produce `report.AllReferencesResolve() == true`.
9. Stale case produces exactly one `staleReferences` entry; `SummaryText()` contains both the
   scenario's name and the missing `areaName` substring.
10. Multiple scenarios (one pattern, one count, the default) each with a different stale `areaName`
    produce three distinct entries, in tier order (pattern, then count, then default).
11. **Wiring test** (extends or sits alongside `MapExporter_IO_Test.cpp`'s scratch-folder pattern,
    `MapExporter_IO_Test.cpp:190-213`'s style): a recipe with one stale `areaName`, exported via
    `Io::MapExporter::ExportSanmapOnly`. Assert `result.bSucceeded == true` (never blocks),
    `result.warningCount >= 1`, and `result.debugLog` contains the stale scenario/area names — proves
    `ReportScenarioAreaNameReferences` is actually wired into `WriteSanmapDocument`, not just present
    as dead code.

### 8c. `src/io/ScenarioScript_DataLua_IO_Test.cpp` (pure, no imgui/GL)

12. **Hit case.** `recipe.areas` has a named Area with known values; `defaultScenario.areaName` names
    it; `defaultScenario.area` deliberately stale/different. Assert the rendered Lua text's `area = {`
    block contains the *resolved* numbers (substring/`std::string::find` check per this file's own
    existing convention), not the stale ones.
13. **Empty case unchanged** — a scenario with empty `areaName` renders its own `body.area` verbatim
    (regression guard against the new resolver accidentally engaging when it shouldn't).
14. **Stale case falls back.** `areaName` names nothing in `recipe.areas` — rendered numbers match
    `body.area` verbatim (same fallback behavior as the JSON leg, proving the two legs agree).
15. **Negative assertion.** `BuildScenarioDataLuaText`'s output never contains the substring
    `"areaName"` or `"AreaName"` anywhere — confirms the correctly-zero-Lua-field part of ARCH_15_05
    stays true even after this ticket's real changes to the *numbers*.

### 8d. `src/io/ScenarioScript_Export_IO_Test.cpp`

16. A recipe with one stale `areaName`, run through `ExportMapScenario` (scratch-directory pattern this
    file already uses, lines 26-39). Assert `result.debugLog` contains the stale scenario/area names —
    proves §5d's wiring, using this type's own `Log` (not `Warn`, which does not exist on
    `ScenarioExportResult`).

## 9. Verify

- Full solo rebuild + `ctest -C Debug`: 100% pass, including all new/extended tests above.
- `grep -rn "areaName\|AreaName" src/` should show it present in exactly: `Scenario_PARAMS.h`,
  `MapExporter_Scenarios_IO.cpp`, `MapImporter_ScenarioRecord_IO.cpp`, `ScenariosTab_Detail_UI.cpp`,
  and the new/extended test files above — **never** in `ScenarioScript_DataLua_IO.cpp`,
  `ScenarioScript_Export_IO.cpp`, or `resources/lua/SanGenScenarioRuntime.lua` (confirms §1's "correct
  half" of the ARCH claim stays true).
- Manually diff one exported `.sanmap`'s `Scenarios[...].Area` against the SAME recipe's exported
  `<MapName>_Scenarios_Data.lua`'s `area = {...}` table for a scenario with a resolved `areaName`: the
  four numbers must match byte-for-byte (proves §5 actually closed the discrepancy, not just added a
  parallel code path that still disagrees).

## 10. Out of scope

- Anything in `ARCH_15_05`'s own still-OPEN items (per-scenario unit-spawn generator placement,
  three-file-split dispatch-branch location) — untouched by this ticket.
- The coordinate-flip-unconfirmed-for-Scenarios question (both files' own ATTENTION comments) — this
  ticket does not touch position Z-flip logic at all, only the Area rect's x/y/width/height, which
  (per both files' own existing code) is never flipped in the first place.
- Formally amending `ARCH_15_05_ParamsScenariosType.md`'s text to record §1's correction — that is the
  ARCH Expert's file; this work-order only flags it for that expert's attention.
