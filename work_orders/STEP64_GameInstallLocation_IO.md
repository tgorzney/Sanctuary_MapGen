# STEP64 — `GameInstallLocation_IO` + `Io::AppSettings` `gameInstallRoot`/`scenarioRuntimeOverridePath`

**Layer:** IO (new validation file), IO+UI touch (existing `AppSettings_IO`/`Application` round
trip). **Domain:** durable global settings (`SANMAP_FORMAT_SPEC.md` Correction 9's family, extended)
+ Map Scenario game-install location. **Sequence:** Map Scenario IO track,
`work_orders/DESIGN_MapScenarioIO_R1.md` §6, Work-Order 3 of 8. No dependency on any other undone
work-order; parallel with STEP63/STEP65.

## Root problem
`ARCH_15_04_ThreeFileOnDiskShape.md` §15.4 requires SanGen to export two Lua files into the engine's own script tree
(`LJ/lua/maps/<MapName>/`) — a location outside the `.sanmap` package entirely, requiring a
**game install root** SanGen does not currently ask for or persist anywhere. `DESIGN_MapScenarioIO_R1.md`
§1/§3 names the two new pieces this needs: a pure validation function
(`ValidateGameInstallRoot`) and two new durable settings fields, since the game install root and any
designer-chosen runtime-resource override must survive across app launches exactly like
`sanpackPath`/`assetCacheDirectory`/`environmentPackPath` already do (`STEP19_AppSettings_IO`).

⚠️ **This ticket edits two pre-existing files, not just adds new ones** — `src/io/AppSettings_IO.h`/
`.cpp` (the struct + JSON round trip) AND `src/ui/Application_UI.h` + `src/ui/Application_AppSettings_UI.cpp`
(the load-at-startup/save-at-shutdown bridge, `STEP19_AppSettings_IO`'s own wiring) — because adding
a field to `AppSettings` alone does nothing: nothing populates it on save or consumes it on load
unless the shell bridge is extended in the same pattern `bUseGpuMarkers` already established
("no UI toggle yet" is fine; "the value silently resets to default every shutdown" is not).

## Correction (amended in place, 2026-08-21) — `ValidateGameInstallRoot`'s second subpath
The `.cpp`-description and acceptance-test snippets below originally specified the second required
subpath as `<root>/Sanctuary_Data/Maps`. **That is wrong; the real Steam Demo install layout has it
one level deeper, at `<root>/engine/Sanctuary_Data/Maps`.** Verified against two independent
ground-truth sources this session: `work_orders/SESSION_HANDOFF_4.md:20` records the real imported
file's path as `...\Sanctuary Shattered Sun Demo\engine\Sanctuary_Data\Maps\Pandemonium
Isthmus\Pandemonium Isthmus.sanmap`, and `work_orders/DESIGN_SantpFootprintIngestion_R1.md` §3.2
independently confirms it from a live read of the real install (`ls <root>/Sanctuary_Data` fails,
`ls <root>/engine/Sanctuary_Data/Maps` lists 20+ map folders). Both trace the error to
`DESIGN_MapScenarioIO_R1.md` §1's table, which is where the wrong path first entered the record and
from which it propagated into this ticket's `.cpp` snippet, its `ARCH rules invoked` citation, and
its acceptance-test snippet — all six occurrences below are now corrected to `engine/Sanctuary_Data/Maps`.

**Decision: fixed in place, not spun into a new standalone ticket.** `DESIGN_SantpFootprintIngestion_R1.md`
§7 proposes this as its own future ticket ("93 — fix `ValidateGameInstallRoot` (IO)"), and a sibling
ticket this session (`STEP83_ReclaimFilterWiring_UI.md`) made an analogous own-ticket-vs-amendment
call the other way. The two situations differ in the way that matters: STEP83 was reconciling **two
already-drafted, unlanded tickets** (STEP50 vs. STEP51/STEP53) whose boundary needed a multi-ticket
argument, a retraction of specific ratified sentences in two separate files, and a blocking
cross-ticket defect that belonged to neither — content substantial enough that no single ticket was
the right home for it. This bug is the opposite shape: **STEP64 itself is unimplemented** (confirmed
by grep — zero hits for `ValidateGameInstallRoot`/`gameInstallRoot` anywhere in `src/`), the wrong
value lives entirely inside this one file's own two snippets, no other ticket depends on or
references the specific wrong string, and nothing has shipped or been retracted — there is no
"landed" ticket to leave undisturbed, only a spec that has not yet been built from. Fixing it here is
correcting the recipe before anything is cooked, not reconciling two dishes already in the oven. A
separate STEP93 ticket would add process overhead (a new file, a new dispatch, a new "why does this
exist" for a future reader) for what is, in substance, a two-character subpath typo caught before
implementation. The alternative (write `work_orders/STEP93_GameInstallRootPathFix_IO.md` as the
design doc's ticket list names it) was considered and rejected on those grounds — noted here so it
is not silently dropped from the record.

## Fix

### 1. New fields — `src/io/AppSettings_IO.h`
Add to `struct AppSettings` (`src/io/AppSettings_IO.h:24-32`), next to the existing fields:
```cpp
struct AppSettings {
    std::string sanpackPath;
    std::string assetCacheDirectory;
    std::string environmentPackPath;
    std::string gameInstallRoot;               // NEW — root of the game install; ValidateGameInstallRoot
                                               // checks <root>/engine/LJ/lua and <root>/engine/Sanctuary_Data/Maps
    std::string scenarioRuntimeOverridePath;   // NEW — empty => bundled SanGenScenarioRuntime.lua default
                                               // (DESIGN_MapScenarioIO_R1.md §3)
    bool bUseGpuTerrain = true;
    bool bUseGpuFlow    = true;
    bool bWysiwygBaking = false;
    bool bUseGpuMarkers = false;
};
```

### 2. Round trip — `src/io/AppSettings_IO.cpp`
`ToJson`/`FromJson` (`src/io/AppSettings_IO.cpp:18-41`) — same flat, direct-1:1, total/never-throwing
pattern every existing field already uses:
```cpp
document["gameInstallRoot"]              = settings.gameInstallRoot;
document["scenarioRuntimeOverridePath"]  = settings.scenarioRuntimeOverridePath;
// ...
ReadJsonText(document, "gameInstallRoot", outSettings.gameInstallRoot);
ReadJsonText(document, "scenarioRuntimeOverridePath", outSettings.scenarioRuntimeOverridePath);
```
A document missing either key leaves the caller's default (empty string) — same partial-document
degrade-gracefully contract every existing field already has (Constitution §6).

### 3. Shell bridge — `src/ui/Application_UI.h` + `src/ui/Application_AppSettings_UI.cpp`
Add two plain caller-owned members to `Application` (`src/ui/Application_UI.h:124`, next to
`bUseGpuMarkers`), same posture — **no picker, no checkbox, no validation call wired in this
ticket**, exactly how `bUseGpuMarkers` shipped with "no UI toggle yet":
```cpp
std::string gameInstallRoot;               // Map Scenario export target root; no picker yet (STEP64)
std::string scenarioRuntimeOverridePath;   // empty => bundled runtime default; no picker yet (STEP64)
```
Extend `LoadAppSettingsAtStartup`/`SaveAppSettingsAtShutdown` (`src/ui/Application_AppSettings_UI.cpp:30-61`)
with the mirrored two lines each — direct copy, no translation, no validation call:
```cpp
// LoadAppSettingsAtStartup, after the existing string seeds:
gameInstallRoot             = loaded.gameInstallRoot;
scenarioRuntimeOverridePath = loaded.scenarioRuntimeOverridePath;

// SaveAppSettingsAtShutdown, after the existing string reads:
current.gameInstallRoot             = gameInstallRoot;
current.scenarioRuntimeOverridePath = scenarioRuntimeOverridePath;
```

### 4. New file: `src/io/GameInstallLocation_IO.h` / `.cpp`
Pure filesystem validation — **no platform dialog** (`FileDialog_IO.h`'s own header comment: "UI
never calls a platform dialog itself"; symmetrically, IO never opens one either — the picker is
strictly UI's, this file only judges a path already chosen).
```cpp
// GameInstallLocation_IO.h — validates a candidate game install root for the Map Scenario Lua
// export leg (`ARCH_15_04_ThreeFileOnDiskShape.md` §15.4, DESIGN_MapScenarioIO_R1.md §1). Pure filesystem check, no platform
// API — unlike AppSettingsLocation_IO.h it needs no `_Shell_IO.cpp` split (Constitution §5): this
// is judgment on a path already chosen, not resolution of a platform-specific bootstrap location.
// UI owns the picker (FileDialog_IO.h's own rule, applied symmetrically); this file only validates.
#pragma once
#include <string>

namespace SanmapGen {
namespace Io {

struct GameInstallRootValidation {
    bool        bValid = false;
    std::string reason;   // populated only when bValid == false; empty on success
};

// A candidate root is valid iff BOTH <candidateRoot>/engine/LJ/lua AND
// <candidateRoot>/engine/Sanctuary_Data/Maps exist as directories (`ARCH_15_04_ThreeFileOnDiskShape.md` §15.4: the engine's script
// tree and the map asset package folder respectively). `reason` names whichever subpath(s) are
// missing -- never a generic "invalid" with no actionable detail (Constitution §6).
GameInstallRootValidation ValidateGameInstallRoot(const std::string& candidateRoot);

} // namespace Io
} // namespace SanmapGen
```
`.cpp`: use `std::filesystem::is_directory` on `JoinExportPath(JoinExportPath(candidateRoot,
"engine"), "LJ/lua")` and `JoinExportPath(JoinExportPath(candidateRoot, "engine"), "Sanctuary_Data/Maps")`
(reuse `FilesystemPrimitives_IO.h`'s `JoinExportPath` rather than hand-rolling path joins). An empty
`candidateRoot` is immediately invalid with `reason = "no game install root was given."`, never a
filesystem call on an empty path.

## Files touched
- `src/io/AppSettings_IO.h` — two new fields on `AppSettings`.
- `src/io/AppSettings_IO.cpp` — `ToJson`/`FromJson` extended.
- `src/ui/Application_UI.h` — two new members on `Application`.
- `src/ui/Application_AppSettings_UI.cpp` — `LoadAppSettingsAtStartup`/`SaveAppSettingsAtShutdown` extended.
- NEW `src/io/GameInstallLocation_IO.h`/`.cpp` — `GameInstallRootValidation`, `ValidateGameInstallRoot`.
- NEW `src/io/GameInstallLocation_IO_Test.cpp`.
- `src/io/AppSettings_IO_Test.cpp` — extend `TestRoundTripSurvivesExactly` with the two new fields.
- `src/ui/ApplicationShell_AppSettings_UI_Test.cpp` — extend the seed/reload assertions with the two
  new fields, mirroring how `bUseGpuMarkers` was added to that same test (`STEP19_AppSettings_IO`'s
  own precedent) — proves the shell bridge, not just the raw JSON round trip, actually carries them.
- `CMakeLists.txt` — one new `add_sangen_test(GameInstallLocation_IO_Test src/io/GameInstallLocation_IO_Test.cpp)`
  line near `WorldFootprintSizeTable_IO_Test`/`FileDialog_IO_Test`.

## Backend policy
N/A — two `std::filesystem::is_directory` calls, called at most once per validation attempt (a UI
button click in some later ticket, not this one). No compute dispatch.

## ARCH rules invoked
- `ARCH_15_04_ThreeFileOnDiskShape.md` §15.4 — the exact two subpaths (`engine/LJ/lua`, `engine/Sanctuary_Data/Maps`) this validator checks.
- `DESIGN_MapScenarioIO_R1.md` §1 — `GameInstallLocation_IO`'s contract, verbatim.
- `STEP19_AppSettings_IO` (`work_orders/STEP19_AppSettings_IO.md`) — the precedent this ticket
  extends rather than duplicates: same flat/direct-1:1 JSON shape, same total/never-throwing load,
  same "seed at startup, flush at clean shutdown" bridge, same "field ships before its UI toggle"
  posture already used for `bUseGpuMarkers`.
- `FileDialog_IO.h`'s own rule ("UI never calls a platform dialog itself") applied symmetrically:
  IO never opens one either — validation only, no browse/pick call anywhere in this ticket.
- Constitution §5 (platform seam) — confirmed NOT triggered here: pure `std::filesystem` is portable,
  so no `_Shell_IO.cpp` split is needed, unlike `AppSettingsLocation_IO.h`'s `SHGetKnownFolderPath` seam.
- Constitution §6 — total, never-throwing validation; an empty/missing path degrades to a specific,
  actionable `reason` string, never a thrown exception or an unexplained `false`.

## Explicit out-of-scope
- **Any platform file/folder picker** — `FileDialog_IO.h`'s `SelectDirectoryPath` already exists;
  wiring a "browse for game install" button is UI Expert's work, not invented here.
- **Calling `ValidateGameInstallRoot` from `LoadAppSettingsAtStartup` or anywhere else automatically**
  — this ticket ships the pure function and the durable field; gating export on a valid root, or
  warning the user about a stale/invalid stored root, is `ScenarioScript_Export_IO`'s (WO7) or the
  UI's job, per `DESIGN_MapScenarioIO_R1.md` §5.
- **`ScenarioScript_Export_IO`, `ScenarioScript_RuntimeResource_IO`, `ScenarioScript_DataLua_IO`,
  `LuaTableWriter_IO`** — separate work-orders (WO5–WO7, STEP63).
- **`resources/lua/SanGenScenarioRuntime.lua` content or its build staging** — WO6, needs ARCH
  sign-off on the `resources/` top-level location first (`DESIGN_MapScenarioIO_R1.md`'s own open item).

## Acceptance test
New `src/io/GameInstallLocation_IO_Test.cpp` (registered in `CMakeLists.txt`):
- A scratch folder with both `engine/LJ/lua` and `engine/Sanctuary_Data/Maps` created under it validates
  `bValid == true`, `reason.empty()`.
- A scratch folder missing `engine/LJ/lua` only validates `bValid == false`, `reason` names that
  specific subpath.
- A scratch folder missing `engine/Sanctuary_Data/Maps` only — mirrored.
- A scratch folder missing both — `reason` names both (never silently one).
- `ValidateGameInstallRoot("")` returns `bValid == false`, `reason == "no game install root was given."`,
  with zero filesystem calls attempted (a nonexistent-but-nonempty candidate root is a separate case
  from an empty one and must not crash either way).
- A candidate root that is itself a FILE, not a directory, validates `bValid == false` (not a crash,
  not a false positive).

Extended `src/io/AppSettings_IO_Test.cpp`: `TestRoundTripSurvivesExactly` sets non-default
`gameInstallRoot`/`scenarioRuntimeOverridePath` and asserts both survive `Save`→`Load` exactly.
`TestMissingDirectoryDegradesToDefaults`/`TestCorruptJsonDegradesToDefaults` assert both new fields
land at their compiled-default empty string alongside the existing checks.

Extended `src/ui/ApplicationShell_AppSettings_UI_Test.cpp`: the seeded fixture sets both new fields
to non-default values; assert `application.gameInstallRoot`/`application.scenarioRuntimeOverridePath`
equal the seed immediately after construction; after mutating both and calling
`SaveAppSettingsAtShutdown()`, assert `Io::LoadAppSettings(scratchDirectory)` reflects the mutated
values, not the original seed — proves the shutdown flush, not just construction, carries them.

## Verify
- New `src/io/GameInstallLocation_IO_Test.cpp` passes.
- `src/io/AppSettings_IO_Test`, `src/ui/ApplicationShell_Layout_UI_Test` (which links
  `ApplicationShell_AppSettings_UI_Test.cpp`) stay green with the extended assertions.
- Full solo rebuild + `ctest -C Debug`: 100% pass, zero unrelated test files edited or broken.
