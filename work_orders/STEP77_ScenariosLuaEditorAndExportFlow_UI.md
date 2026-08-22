# STEP77 — Runtime Lua editor (`LuaCodeEditor_UI`) + `ExportScenarioScript` Files-tab flow

**Layer:** UI. **Domain:** new shared widget (`ARCH_15_08_ThirdPartyDependencyRuling.md` §15.8), Files tab, `Application` shell.
**Sequence:** Map Scenario track, Work-Order 8 of 8, UI leg, part 2 of 3. **Depends on STEP74**
(`ScenariosTabState`/`ScenarioNeedsSpawnsAcknowledgment`, extends both), **STEP64**
(`Io::AppSettings.gameInstallRoot`/`scenarioRuntimeOverridePath`, `Io::ValidateGameInstallRoot`),
**STEP65** (`Sys::CheckLuaSyntax` + LuaJIT vendoring — this ticket is UI's first consumer, so
STEP65's CMake `PRIVATE` link must resolve for a UI-layer translation unit; if it does not, that is
a STEP65 gap to flag back, not something to route around here), **STEP71**
(`Io::ExportMapScenario`/`ScenarioExportResult`), **STEP72** (`Io::LoadScenarioRuntimeText`, the
bundled resource and its `sangen_lua_resources` staging). **None of STEP64/65/71/72 are landed in
`src/` as of this writing** — this ticket cannot compile until they do, same posture STEP71 took
toward its own WO6 dependency. No dependency on STEP47/50-53 (canvas, STEP78).

## ⚠️ Correction to `DESIGN_ScenariosTabAndLuaEditor_R1.md` §7 — the editor is file-based, not buffer-based

The original design assumed a `ScenarioSettings::runtimeScriptText` buffer inside
`Params::MapRecipe`, edited in place and export-rendered. **That field does not exist anywhere in
the ratified data model** — `ARCH_15_05_ParamsScenariosType.md` §15.5's `Scenarios` struct and `SANMAP_FORMAT_SPEC.md`
Correction 17's JSON shape both carry no such member (confirmed by reading both in full). The
ratified IO shape (STEP71 §0, STEP72) resolves runtime text **from disk** at export time:
`Io::LoadScenarioRuntimeText(runtimeResourceDirectory, scenarioRuntimeOverridePathOrEmpty)` —
override file if set and readable, else the bundled `SanGenScenarioRuntime.lua` staged beside the
executable. **This ticket redesigns the editor around that file, not an embedded buffer.**

Consequence: editing is **always editing a real file on disk** — either the designer's own override
file (created on first edit if none is set) or, read-only, the bundled default (a "View Bundled
Default" panel, never saved over). There is no "the recipe's own copy" distinct from "the file."

## Root problem
No `LuaCodeEditor_UI` widget exists (`ARCH_15_08_ThirdPartyDependencyRuling.md` §15.8 ratifies its shape but defers wiring to this
consult). No `FilesTabAction::ExportScenarioScript` exists (`FilesTab_UI.h`'s enum has 8 members,
none scenario-related). No UI surface reads/writes `Application::gameInstallRoot`/
`scenarioRuntimeOverridePath` — STEP64 shipped both with "no UI toggle yet"; this ticket is their
first consumer.

## Fix

### 1. `LuaCodeEditor_UI` — new shared widget (`ARCH_15_08_ThirdPartyDependencyRuling.md` §15.8)

`src/ui/LuaCodeEditor_UI.h` (plain settings/state struct, "THE SPLIT" per `ColorSwatch_UI.cpp` —
third-party `TextEditor` code isolated to the `.cpp`) + `LuaCodeEditor_UI.cpp` (vendored
ImGuiColorTextEdit, dark pastel Lua theme, wired to `Sys::CheckLuaSyntax` for its gutter):

```cpp
// src/ui/LuaCodeEditor_UI.h
#pragma once
#include <string>

namespace SanmapGen {
namespace Ui {

struct LuaCodeEditorState {
    std::string bufferText;
    bool        bDirty = false;         // buffer differs from what was last loaded/saved
    int         lastCheckedErrorLine = 0;
    std::string lastCheckedErrorMessage;
    bool        bLastCheckSucceeded  = true;
};

// Draws the editor (a single-instance widget — ordinary per-frame ImGui, NOT the 100k-entity
// bypass toolkit, per ARCH_15_08_ThirdPartyDependencyRuling.md §15.8's own scoping note). Runs Sys::CheckLuaSyntax on FOCUS-LOSS and
// an explicit "Validate Now" button only — never per-keystroke (avoids thrashing an immediate-mode
// loop for text edited in bursts). Errors surface as an inline gutter marker (ImGuiColorTextEdit's
// native error-marker support) plus a one-line status bar. Returns true the frame the buffer text
// actually changed.
bool DrawLuaCodeEditor(const char* label, LuaCodeEditorState& state);

} // namespace Ui
} // namespace SanmapGen
```

**Export gate — deliberate divergence from warn-never-block, named so it doesn't read as an
inconsistency**: a Lua syntax error is not a judgment call a designer might override; the file is
provably broken and the game cannot load it. `!state.bLastCheckSucceeded` is a public, readable
condition both the Scenarios tab and the Files tab consult to hard-disable their export actions,
tooltip pointing at `lastCheckedErrorMessage`/`lastCheckedErrorLine`.

### 2. Hosting — `ScenariosTab_UI.h`/`.cpp` extension (EDIT, STEP74's files)

Add to `ScenariosTabState`:
```cpp
SectionState       runtimeScriptSection;   // collapsed-by-default "Runtime Script (advanced)"
LuaCodeEditorState runtimeScriptEditor;
bool               bRuntimeScriptLoaded = false;   // one-shot load-on-first-open guard
```

New `src/ui/ScenariosTab_RuntimeScript_UI.cpp`:
- On first open (`bRuntimeScriptLoaded == false`), call `Io::LoadScenarioRuntimeText` with the
  resolved `scenarioRuntimeResourceDirectory` (§5) and the current `scenarioRuntimeOverridePath`;
  seed `bufferText`, set the guard. A failed resolution shows `errorMessage` inline instead of an
  editor — never crashes, never shows an empty silent buffer.
- **"Use a custom Runtime Script" toggle** (bound to whether the override path is set): OFF shows
  the resolved bundled text **read-only** (`ImGui::BeginDisabled`); ON shows it editable and, if no
  override path is set yet, prompts via `FilePathPicker_UI` (`.lua` fence) for **where to save the
  new override file** before the editor becomes editable — never silently invents a path.
- **Save** (enabled only while ON with a path set): `Sys::CheckLuaSyntax` first — failure blocks the
  save with the same gutter/status feedback, no file touched. Success calls
  `Io::WriteBinaryFileBytes(...)` (existing primitive; first UI-layer call site, no new IO needed).
- **"Reset to bundled default"** — `ConfirmDialog_UI`, then **clears** `scenarioRuntimeOverridePath`
  (reverting resolution to bundled on the next load). It does **not** delete the override file
  (`MAP_SCENARIO_SPEC.md` §2.2 point 4's never-delete posture, applied by analogy).
- **Bundled-vs-override diff banner** — ⚠️ **simplified from the original design's "newer bundled"
  framing**, which assumed a version/hash baseline this data model has no field to store. What IS
  substantiable: whenever an override is set AND its text differs from the bundled text, show a
  non-blocking dismissible banner: *"Your Runtime Script override differs from SanGen's current
  bundled default."* → **[View Bundled Default]** (read-only panel, no auto-merge — Lua text merging
  stays out of scope) / **[Dismiss]**. Dropped: **[Keep My Version]** and the "newer" qualifier,
  both of which implied a staleness determination this data cannot make. Any real customization
  trips this banner every view — honest given what's knowable, not a regression to hide.
  **Flag to ARCH/IO:** a real "forked from bundled version X, now at Y" mechanism needs a stored
  baseline (e.g. a hash beside `scenarioRuntimeOverridePath`) — not proposed here.

### 3. `FilesTabAction::ExportScenarioScript` (EDIT `FilesTab_UI.h` + `.cpp`s)

```cpp
enum class FilesTabAction {
    OpenSanmap, ImportSupComLua, ExportSanmapOnly, ExportAll,
    ExportHeightmapRaw, ExportSlopeImage, ExportFlowImage, ExportStratumMasks,
    ExportScenarioScript,       // NEW
};
inline constexpr int filesTabActionCount = 9;   // was 8
```
`FilesTabActionLabel(ExportScenarioScript)` → `"Export Scenario Script"`.
`FilesTabActionNeedsBakedFields` stays `false` (reads only `recipe.scenarios`/`recipe.armies`).

`FilesTabState` gains:
```cpp
// Caller-owned pointers into Application-level machine-local settings (STEP64) — same posture as
// `assetPack`: non-null once Application wires them; nullptr degrades the row to a clear
// "not configured" state rather than crashing.
std::string* gameInstallRoot             = nullptr;
std::string* scenarioRuntimeOverridePath = nullptr;
std::string  scenarioRuntimeResourceDirectory;   // resolved once at startup (§5), copied not
                                                 // pointed — it never changes after launch
Io::ScenarioExportResult lastScenarioExportResult;
```

`RunFilesTabAction`'s new branch:
```cpp
case FilesTabAction::ExportScenarioScript: {
    if (state.gameInstallRoot == nullptr || !Io::ValidateGameInstallRoot(*state.gameInstallRoot).bValid) {
        AppendFilesTabLog(state, "Scenario export needs a valid game install root — set one first.");
        return false;
    }
    const std::string overridePath = (state.scenarioRuntimeOverridePath != nullptr)
        ? *state.scenarioRuntimeOverridePath : std::string();
    state.lastScenarioExportResult = Io::ExportMapScenario(*state.gameInstallRoot, recipe,
        state.scenarioRuntimeResourceDirectory, overridePath);
    AppendFilesTabLog(state, state.lastScenarioExportResult.debugLog);
    return state.lastScenarioExportResult.bDataLuaWritten || state.lastScenarioExportResult.bRuntimeCopied;
}
```

**Mandatory-spawns confirm gate wraps the click, not the action** — reuses the tab's existing
`confirmDialogState`/`pendingConfirmAction`/`bConfirmActionPending` machinery (already shipped for
the blueprintPath warning): the draw-site pre-check calls `ScenarioNeedsSpawnsAcknowledgment`
(STEP74) over every `patternScenarios`/`countScenarios` entry (never `defaultScenario`, per STEP74's
exemption); any hit opens the dialog naming every affected scenario, **[Export Anyway]**/
**[Cancel]**, before `RunFilesTabAction` runs. Entries with a non-empty `authoringNote` never
trigger it.

### 4. Row UX + result surfacing (`FilesTab_Draw_UI.cpp`, EDIT)

- **Unset/invalid `gameInstallRoot`**: the row relabels to `"Locate Game Install…"`
  (`FilePathPicker_UI` directory mode) rather than a dead-end disabled button — a redirect, not a
  failure.
- **Set but invalid**: inline red text naming `reason` verbatim, export disabled.
- **Result banner**: surfaces every `lastScenarioExportResult` flag —
  `bDataLuaCollisionDetected`/`bRuntimeCollisionDetected` each get a line naming both the occupied
  original and the `.sangen-pending.lua` sibling, with a text pointer to the pending file so the
  designer knows to reconcile manually (no platform shell-open invented here).
  `bDataLuaSyntaxCheckFailed`/`bRuntimeSyntaxCheckFailed` each surface their own line too — the
  editor's pre-save check should make the latter rare, but IO's belt-and-suspenders check can still
  fire (e.g. an override hand-edited outside SanGen between sessions).

### 5. Resource-directory + settings wiring (EDIT `ApplicationMain_UI.cpp`, `Application_Settings_UI.h`, `Application_UI.h`, `Application_AppSettings_UI.cpp`)

Mirrors the existing shader-directory resolution verbatim:
```cpp
// ApplicationSettings — one new field:
std::string scenarioRuntimeResourceDirectory;

// ApplicationMain_UI.cpp — same shape as ResolveShaderSearchDirectories but single-directory
// (LoadScenarioRuntimeText takes one directory, not a search list):
std::string ResolveScenarioRuntimeResourceDirectory(int argumentCount, char** arguments) {
    return ExecutableDirectory(argumentCount > 0 ? arguments[0] : nullptr) + "sangen_lua_resources";
}
```
`Application` additionally stores `scenarioRuntimeResourceDirectory` (copied at construction, never
a pointer — immutable post-launch). `FilesTabState`'s two pointers are wired to
`&application.gameInstallRoot`/`&application.scenarioRuntimeOverridePath` at the same call site
`assetPack` is wired — **and that is also where a plain `FilePathPicker_UI` row for both belongs**,
satisfying STEP64's "no UI toggle yet" deferral for the first time. Placed on the Files tab (not
buried in Scenarios) since it is machine-local configuration, not recipe content — matching
`ArmiesTabState::gamedataDirectory`'s precedent.

## Files touched
- NEW `src/ui/LuaCodeEditor_UI.h`/`.cpp`, `LuaCodeEditor_UI_Test.cpp`,
  `ScenariosTab_RuntimeScript_UI.cpp`.
- EDIT `src/ui/ScenariosTab_UI.h` (new state members), `FilesTab_UI.h` (action + state fields),
  `FilesTab_Actions_UI.cpp` (new branch), `FilesTab_Draw_UI.cpp` (row UX + banner),
  `ApplicationMain_UI.cpp`, `Application_Settings_UI.h`, `Application_UI.h`,
  `Application_AppSettings_UI.cpp`.
- EDIT `CMakeLists.txt` — vendor ImGuiColorTextEdit (`ARCH_15_08_ThirdPartyDependencyRuling.md` §15.8; same `FetchContent`/
  `src/third_party/` coder's-call precedent as STEP65's LuaJIT), one new `add_sangen_test`.

## Backend policy
N/A — imgui-frame CPU work plus at most one `Sys::CheckLuaSyntax` compile per focus-loss/button
click and one filesystem write per Save; not a per-frame hot path.

## ARCH rules invoked
- `ARCH_15_08_ThirdPartyDependencyRuling.md` §15.8 — `LuaCodeEditor_UI`'s home, shape, and "single-instance, ordinary imgui, not the
  bypass toolkit" scoping, implemented verbatim.
- `MAP_SCENARIO_SPEC.md` §2.1 — the overwrite-safety/collision surfacing this ticket reports (never
  re-implements — IO already performed the write-or-refuse).
- Constitution §6 — hard export-block on a Lua syntax error (the named, deliberate exception), loud
  never-silent surfacing of every `ScenarioExportResult` flag.

## Explicit out-of-scope
- **`Params::Scenarios`-shaped rendering, `BuildScenarioDataLuaText`, the banner constant** — STEP70.
- **`Io::ExportMapScenario`'s write/overwrite-safety logic** — STEP71; only called here.
- **`Io::LoadScenarioRuntimeText`'s resolution logic, the bundled resource content** — STEP72.
- **A real staleness/version-diff mechanism for the bundled-vs-override banner** — flagged, not built.
- **Any platform "reveal in file manager" call** — text-only pointer.
- **Interactive canvas marker editing** — STEP78.

## Acceptance test
New `src/ui/LuaCodeEditor_UI_Test.cpp`:
1. Validation runs on a simulated focus-loss transition, not on every keystroke (headless-testable
   by driving the state struct and calling the validation entry point the `.cpp` exposes for
   testing — exact seam is the coder's call, but it must be testable without a real imgui frame).
2. A syntactically invalid buffer sets `bLastCheckSucceeded == false` with a non-zero
   `lastCheckedErrorLine` matching the injected error's line.
3. A valid buffer (including empty) sets `bLastCheckSucceeded == true`.

Extended `FilesTab_UI_Test.cpp` / new `FilesTab_ScenarioExport_UI_Test.cpp`:
4. `filesTabActionCount == 9`; `FilesTabActionLabel(ExportScenarioScript)` non-empty and unique.
5. `gameInstallRoot == nullptr` → `RunFilesTabAction` logs and returns `false`, no crash.
6. A stubbed valid root + fixture recipe → returns true when the stubbed `ExportMapScenario` reports
   either file written; `lastScenarioExportResult` reflects the stub verbatim (no reinterpretation).
7. Mandatory-spawns pre-check: a fixture with one empty-spawns/empty-note `CountScenario` → the
   pure pre-check function reports the gate should open, naming that scenario; a fixture where every
   Tier 1/2 entry has spawns or a note → gate does not open.
8. Full solo rebuild + `ctest -C Debug`: previously-passing suite stays green; new targets pass.

## Verify
- New and extended tests pass. Full solo rebuild + `ctest -C Debug`: 100% pass, zero pre-existing
  test files edited or broken.
- Confirm ImGuiColorTextEdit vendoring resolves on a from-scratch configure, same bar STEP65 set for
  LuaJIT.
