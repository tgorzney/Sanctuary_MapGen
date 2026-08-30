# STEP224 — "Import Areas from Scenario Script..." button (Files tab)

## Summary
The human: "Need to import areas from scenarios in map _data and _script files" / "When opening a
map file, I want to import the areas from the _data." SanGen's OWN native `.sanmap` format already
round-trips its `areas` JSON section automatically on every load (`ParseEntityDomainsJson` ->
`ReadAreasJson`, unconditional, no work needed there). What does NOT yet exist is any UI path to
`Io::ImportAreaRectanglesFromScenarioScriptFile` (`ScenarioScript_AreaImport_IO.h`, STEP215/ARCH
§15.11) — a fully built and tested IO entry point for extracting Area rectangles out of a FOREIGN
(non-SanGen) scenario's `_data.lua`/`_script.lua`-style file, which today has ZERO callers anywhere
in the codebase (confirmed by a full-repo search this session).

Ruled by the SanGen Format Expert (this session): **firing this automatically as part of opening
any map — native or foreign — is NOT compliant with the existing ARCH ruling and is not built by
this ticket.** `ARCH_15_11_ForeignScenarioAreaImport.md` item 8 is explicit: "Invocation is an
explicit authoring action... forbidden to run implicitly on map open, on export, on generate, or on
any dirty-hash recompute." That binds on the function itself, not on the literal phrase "map open"
— bundling it as an automatic side effect of ANY other click (including a hypothetical dedicated
foreign-import action) would still convert one human decision into two, which is exactly what item
8's "no default/implicit invocation path" and item 9's "no provenance, no re-sync" language exist to
prevent. Wiring it into `RunOpenSanmap`/native Open is explicitly forbidden outright.

What §15.11 ALREADY authorizes, with no ARCH change needed, is exactly what its own header comment
anticipates (`ScenarioScript_AreaImport_IO.h:50-53`, "UI wiring is explicit follow-up, not built by
this ticket"): a brand-new, separately-clicked Files-tab action where a human explicitly picks a
`.lua` file and clicks Import — satisfying "human-triggered, one-shot, no live binding" exactly as
written. This ticket builds that action. If the human insists on literal automatic-on-open wiring
later, that requires a fresh ARCH ruling from the SanGen ARCH Expert first — not something a
work-order can authorize on its own.

## Required reading
- `src/io/ScenarioScript_AreaImport_IO.h` (full file, 59 lines) — the entry point this ticket wires
  up: `Io::ImportAreaRectanglesFromScenarioScriptFile(const std::string& sourceFilePath,
  Params::MapRecipe& recipe) -> ScenarioAreaImportResult`.
- `src/ui/FilesTab_UI.h` (full file) — `FilesTabAction` enum, `FilesTabState`, `RunFilesTabAction`.
- `src/ui/FilesTab_Actions_UI.cpp` (full file) — existing action dispatch, especially
  `RunImportSupComLua` (lines 84-98) as the closest existing shape (a path field + an injected/IO
  call + a logged result), and `RunFilesTabAction`'s own switch (lines 159-176).
- `src/ui/FilesTab_Draw_UI.cpp:37-62` (`DrawOpenSection`) — where the new row is added, and the
  existing `DrawFilesTabPathRow("SupCom Save Lua", ...)` + `DrawActionButton(...)` pair (lines
  57-58) as the exact shape to mirror.
- `src/ui/FilesTab_Browse_UI.h` (full file) — `FilesTabBrowseKind` enum.
- `src/ui/FilesTab_Browse_UI.cpp` (full file) — per-kind dialog title/filter table
  (`BuildDialogRequest`, `BuildPickerOptions`) — note the ALREADY-GENERIC `luaDialogFilters` table
  (lines 20-21, `{"Lua script", "*.lua"}, {"All files", "*.*"}`), already used for
  `ScenarioRuntimeOverrideLua` — reuse it, do not add a third near-duplicate filter table.

## 1. `src/ui/FilesTab_Browse_UI.h` — one new browse kind

Add a new enumerator to `FilesTabBrowseKind` (after `ScenarioRuntimeOverrideLua`):
```cpp
enum class FilesTabBrowseKind {
    SanmapDocument, SupComLuaDocument, ExportFolder, GameInstallRoot,
    ScenarioRuntimeOverrideLua,
    ScenarioAreaImportLua,   // STEP224: a FOREIGN scenario `_data`/`_script` `.lua` (ARCH §15.11)
};
```

## 2. `src/ui/FilesTab_Browse_UI.cpp` — wire the new kind into the existing per-kind tables

In `BuildDialogRequest`, extend the existing `SupComLuaDocument`/`ScenarioRuntimeOverrideLua`
branch's condition to include the new kind, and add its own title:
```cpp
} else if (kind == FilesTabBrowseKind::SupComLuaDocument
        || kind == FilesTabBrowseKind::ScenarioRuntimeOverrideLua
        || kind == FilesTabBrowseKind::ScenarioAreaImportLua) {
    request.title            = kind == FilesTabBrowseKind::SupComLuaDocument
                                   ? "Import Supreme Commander Lua"
                                   : kind == FilesTabBrowseKind::ScenarioAreaImportLua
                                       ? "Import Areas from Scenario Script"
                                       : "Locate Runtime Script Override";
    request.defaultExtension = ".lua";
    request.filters          = luaDialogFilters;
    request.filterCount      = 2;
}
```
In `BuildPickerOptions`, extend the `.lua`-fence condition the same way:
```cpp
options.allowedExtensions = (kind == FilesTabBrowseKind::SupComLuaDocument
                             || kind == FilesTabBrowseKind::ScenarioRuntimeOverrideLua
                             || kind == FilesTabBrowseKind::ScenarioAreaImportLua)
                                ? ".lua" : nullptr;
```

## 3. `src/ui/FilesTab_UI.h` — new action, new path field, new result-log surface

Add a new enumerator to `FilesTabAction` and bump the count:
```cpp
enum class FilesTabAction {
    OpenSanmap, ImportSupComLua, ImportScenarioAreas, ExportSanmapOnly, ExportAll,
    ExportHeightmapRaw, ExportSlopeImage, ExportFlowImage, ExportStratumMasks,
    ExportScenarioScript,
};
inline constexpr int filesTabActionCount = 10;
```
Add the path field to `FilesTabState`, beside `supComLuaPath`:
```cpp
std::string scenarioAreaImportPath;   // STEP224: a FOREIGN scenario `.lua` (ARCH §15.11)
```
`FilesTabActionLabel` gains one case (in `FilesTab_Actions_UI.cpp`, see below) — no other header
change; `RunFilesTabAction`'s existing signature is unchanged (the new action needs only `recipe`,
which every existing action already receives).

## 4. `src/ui/FilesTab_Actions_UI.cpp` — the action itself

Add a new function beside `RunImportSupComLua` (same file, same anonymous namespace), following
its exact shape (a path check, one IO call, log the result):
```cpp
// ARCH §15.11 — human-triggered, one-shot: this action exists ONLY as an explicit click. It must
// never be called from RunOpenSanmap or from anywhere else automatic (see this ticket's own
// header comment for why).
bool RunImportScenarioAreas(FilesTabState& state, Params::MapRecipe& recipe) {
    if (state.scenarioAreaImportPath.empty()) {
        AppendFilesTabLog(state, "Import refused: no scenario script .lua path is set.");
        return false;
    }
    const Io::ScenarioAreaImportResult result =
        Io::ImportAreaRectanglesFromScenarioScriptFile(state.scenarioAreaImportPath, recipe);
    AppendFilesTabLog(state, result.debugLog);
    return !result.bRefusedGeneratedFile && !result.bRefusedUnreadableFile
        && !result.bRefusedOversizedFile && !result.writtenNames.empty();
}
```
Add `#include "../io/ScenarioScript_AreaImport_IO.h"` to this file's include block.

`FilesTabActionLabel` gains:
```cpp
case FilesTabAction::ImportScenarioAreas: return "Import Areas from Scenario Script";
```

`RunFilesTabAction`'s switch gains, alongside the existing `ImportSupComLua` line:
```cpp
if (action == FilesTabAction::ImportScenarioAreas) return RunImportScenarioAreas(state, recipe);
```
Note this action carries no `fields`/baked-image dependency at all — `FilesTabActionNeedsBakedFields`
must NOT be extended for it (it stays false, the same as `ImportSupComLua`).

## 5. `src/ui/FilesTab_Draw_UI.cpp` — the row

In `DrawOpenSection` (lines 37-62), add a new path row + action button after the existing SupCom
Lua row (after line 60, before `DrawSectionEnd()`), mirroring that exact pair:
```cpp
    ImGui::Separator();
    DrawFilesTabPathRow("Scenario Area Import Lua", FilesTabBrowseKind::ScenarioAreaImportLua,
                        state.scenarioAreaImportPath);
    if (ImGui::Button(FilesTabActionLabel(FilesTabAction::ImportScenarioAreas))) {
        const bool bSucceeded = RunFilesTabAction(FilesTabAction::ImportScenarioAreas, state, recipe,
                                                  fields);
        if (bSucceeded && previewDriver != nullptr) previewDriver->RequestMapUpdate();
    }
```
This is a plain inline button (not `DrawActionButton`) because `DrawActionButton`'s own
`bImported` check (line 32-33) only recognizes `OpenSanmap`/`ImportSupComLua` — rather than widen
that helper's notion of "imported" for a third, narrower case, this ticket draws its own three-line
button block, matching the size of the change to the size of the need (Constitution — no
unnecessary abstraction widening). `previewDriver->RequestMapUpdate()` is warranted: a newly
imported set of areas is exactly the kind of "everything moved at once" change
`AreasTab_UI.h`'s own SCOPE NOTE 1 already documents needing a map update, not a mere recolor.

## ARCH rules invoked
- ARCH §15.11 (`ARCH_15_11_ForeignScenarioAreaImport.md` item 8) — human-triggered, one-shot, no
  live binding, no default/implicit invocation path. This ticket's ENTIRE reason for existing is to
  satisfy this rule (an explicit button), not to work around it.
- Constitution §6 — a missing path is refused with a logged reason, never silently skipped.
- ARCH §1.5 — no new file needed; the addition is small enough to fit the existing
  `FilesTab_Actions_UI.cpp`/`FilesTab_Draw_UI.cpp` split without approaching either ceiling.

## Explicit out-of-scope
- No change to `RunOpenSanmap`, `MapImporter::LoadSanmap`, or anything in the native `.sanmap` open
  path — that path already imports `recipe.areas` automatically and correctly; this ticket does not
  touch it.
- No automatic/implicit invocation of `ImportAreaRectanglesFromScenarioScriptFile` from any other
  action, dialog, or event — forbidden outright per the Format Expert's ruling above. If the human
  wants that later, it requires a fresh ARCH ruling first, not a coder decision.
- No change to `ScenarioScript_AreaImport_IO.h/.cpp` or `ScenarioScript_AreaRectangleExtract_IO.h`
  — both are already complete and tested; this ticket only adds a caller.
- No result-detail UI (a dialog listing `writtenNames`/`skippedCollisionNames`/`nearMisses`) beyond
  the existing plain-text debug log every other Files-tab action already uses — matches
  `RunImportSupComLua`'s own precedent exactly; a richer result dialog is a separate future ticket
  if the human asks for one.

## Acceptance test
- `FilesTab_UI_Test.cpp` (or a new sibling test file if the existing one is at its §1.5 ceiling):
  construct a `FilesTabState` with `scenarioAreaImportPath` pointing at a small fixture foreign
  scenario `.lua` (reuse whatever fixture `ScenarioScript_AreaImport_IO_Test.cpp` already has, or
  write an equivalent minimal one), call `RunFilesTabAction(FilesTabAction::ImportScenarioAreas,
  ...)`, and assert: (a) `recipe.areas` gains the expected rectangle(s) by name, (b) an empty
  `scenarioAreaImportPath` is refused with a logged reason and touches nothing, (c) a
  `FilesTabActionLabel(FilesTabAction::ImportScenarioAreas)` string is non-empty.
- Full existing test suite: zero regressions (this is a strictly additive enum value + one new
  function + one new browse kind; no existing call site's behavior changes).

## Interpretation calls made
1. The new action returns success only when at least one area name was actually written
   (`!result.writtenNames.empty()`) — a call that refuses for a structural reason
   (`bRefusedGeneratedFile`/`bRefusedUnreadableFile`/`bRefusedOversizedFile`) or that parses a file
   with zero extractable rectangles both correctly report "nothing happened" rather than a false
   "succeeded." A file whose rectangles all collided with existing names
   (`skippedCollisionNames` nonempty, `writtenNames` empty) is therefore also reported as not
   succeeded even though the import ran without error — the log line from `result.debugLog`
   already explains why, matching the existing plain-log-only precedent.
2. No new `FilesTabBrowseKind` filter table — reused the existing generic `luaDialogFilters`
   already shared by `ScenarioRuntimeOverrideLua`, per the Required Reading note above, rather than
   adding a third near-identical `{"Lua script", "*.lua"}` array.
