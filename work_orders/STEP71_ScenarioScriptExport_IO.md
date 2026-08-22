# STEP71 — `ScenarioScript_Export_IO`: the Map Scenario export orchestrator

**Layer:** IO. **Domain:** Map Scenario Lua-rendering leg orchestration (filesystem writes,
overwrite safety). **Sequence:** Map Scenario IO track, `work_orders/DESIGN_MapScenarioIO_R1.md`
§6, Work-Order 7 of 8 — **the last IO-side ticket in the track** (WO8 is UI Expert's). Depends on
**STEP64** (`GameInstallLocation_IO`, `Io::AppSettings.gameInstallRoot`), **STEP70**
(`ScenarioScript_DataLua_IO::BuildScenarioDataLuaText` + `kScenarioGeneratedFileBannerLine`), and
**WO6** (`ScenarioScript_RuntimeResource_IO`, **not yet authored** — see §0 for the exact contract
this ticket assumes). **Blocked on WO6 landing before implementation can compile** — this document
is written now so both tickets can be reviewed in parallel; a coder must not begin STEP71 until WO6
exists with a matching (or trivially adaptable) signature.

## 0. The WO6 contract this ticket assumes (not built here, not binding on WO6's author)

`DESIGN_MapScenarioIO_R1.md` §3's resolution algorithm, translated into the shape this ticket calls:

```cpp
// ASSUMED — src/io/ScenarioScript_RuntimeResource_IO.h, WO6, not yet authored.
namespace SanmapGen { namespace Io {
struct ScenarioRuntimeResourceResult {
    bool        bSucceeded = false;
    std::string runtimeLuaText;      // the resolved runtime file's full text, INCLUDING its own
                                     // kScenarioGeneratedFileBannerLine first line (STEP70) — WO6's
                                     // bundled resource content must open with that exact literal.
    std::string sourceDescription;   // "bundled" or "override", for the debugLog
    std::string errorMessage;        // ⚠️ AMENDED BY STEP72 — a DIAGNOSTIC/ADVISORY string, not a
                                     // failure-only field. Empty ONLY on a fully clean resolution;
                                     // NON-EMPTY on a successful degrade-to-bundled
                                     // (bSucceeded == true) as well as a hard failure. See step 7.
};
ScenarioRuntimeResourceResult LoadScenarioRuntimeText(const std::string& runtimeResourceDirectory,
                                                      const std::string& runtimeOverridePathOrEmpty);
} }
```

If WO6 ships a different signature, this ticket's one call site needs a trivial adapter — the
overwrite-safety/write-target logic below does not otherwise depend on WO6's internal shape.

## ⚠️ Three corrections applied, not silently followed

Grounding directly against `MAP_SCENARIO_SPEC.md` §2/§2.1/§2.2 (ratified spec, not just the design
doc) surfaced three points where the authoring instructions reproduced the **superseded
two-file-era design**. All three trace to one root cause: `DESIGN_MapScenarioIO_R1.md` §2 predates
`MAP_SCENARIO_SPEC.md` §2's ratification and used the legacy filename
`<MapName>_Scenarios_Script.lua` for the SanGen-owned data file.

1. **"the runtime file is always overwritten [unconditionally]"** — reproduces the superseded design
   table. `MAP_SCENARIO_SPEC.md` §2.1 point 3 is explicit: *"Before writing **either** SanGen-owned
   path, the exporter reads whatever file already exists there... checks for the marker token."*
   "Either" = both `_Scenarios_Runtime.lua` **and** `_Scenarios_Data.lua`. `ARCH_15_04_ThreeFileOnDiskShape.md` §15.4 point 2/3
   agrees ("**Both** SanGen-owned files open with a machine-checkable banner"). **Ruling: symmetric
   banner-gated overwrite safety on both files.** `ScenarioExportResult` therefore carries a
   collision flag for **both**, extending `DESIGN_MapScenarioIO_R1.md` §5's struct (which had only
   `bDataLuaCollisionDetected`).
2. **"the live `Pandemonium Isthmus_Scenarios_Script.lua`... first export hits the collision path"**
   — wrong mechanism. `MAP_SCENARIO_SPEC.md` §2.1 point 1 (filename disjointness) and §2.2 are both
   explicit that the legacy file **is never at overwrite risk and never touched at all** — its
   filename collides with **neither** ratified SanGen-owned path
   (`_Scenarios_Runtime.lua`/`_Scenarios_Data.lua`), by design. SanGen never reads or banner-checks
   it; it simply isn't a candidate write path. **Ruling: the legacy-map acceptance test verifies the
   legacy file is byte-identical before/after export and that no collision flag fires** — a
   *separate* test from the synthetic banner-collision tests, which use the *new* ratified filenames
   deliberately pre-seeded with foreign content.
3. **The "call-site invariant... `_data.lua`'s `Import()` line never changes"** — also superseded.
   `MAP_SCENARIO_SPEC.md` §2 ("Link mechanism, extended") states the opposite: `<MapName>_data.lua`'s
   `Import()` target **moves** to `_Scenarios_Runtime.lua`, "a hand-edit, once per map, part of the
   migration in §2.2." This ticket never writes `_data.lua` under any code path regardless, so the
   correction changes no write behavior — only what the migration test asserts (a one-time human
   edit is expected, not a preserved call site).

## Root problem
`ARCH_15_04_ThreeFileOnDiskShape.md` §15.4 requires SanGen to write two files into the engine's own script tree
(`LJ/lua/maps/<MapName>/`) — outside the `.sanmap` package, gated on a game install root (STEP64),
never touching the hand-authored orchestrator. No orchestrating write path exists in `src/` today —
`BuildScenarioDataLuaText` (STEP70) is pure/disk-free by design and never writes a file itself.

## Fix

### 1. New file: `src/io/ScenarioScript_Export_IO.h`
```cpp
// ScenarioScript_Export_IO.h — the Map Scenario export orchestrator: given a validated game install
// root and a Params::MapRecipe, writes <MapName>_Scenarios_Runtime.lua and
// <MapName>_Scenarios_Data.lua into LJ/lua/maps/<MapName>/ under the banner-gated overwrite-safety
// rule (`ARCH_15_04_ThreeFileOnDiskShape.md` §15.4, MAP_SCENARIO_SPEC.md §2.1). Layer: IO. The ONE entry point UI calls for the
// scenario export leg.
//
// SEPARATE from MapExporter_IO::ExportAll/ExportSanmapOnly, by design (DESIGN_MapScenarioIO_R1.md
// §5) -- a scenario-leg failure NEVER sets MapExportResult::bSucceeded = false, and a .sanmap/asset
// export failure never blocks this. Two independent calls, two independent result types, never
// merged.
#pragma once
#include <string>
#include <vector>

namespace SanmapGen {
namespace Params { struct MapRecipe; }
namespace Io {

// Mirrors MapExportResult's shape but is a DISTINCT type -- never merged with it.
struct ScenarioExportResult {
    bool bDataLuaWritten           = false;  // <MapName>_Scenarios_Data.lua written this export
    bool bRuntimeCopied            = false;  // <MapName>_Scenarios_Runtime.lua written this export
    bool bOrchestratorPresent      = false;  // <MapName>_data.lua exists; false => warn, files inert
    bool bDataLuaCollisionDetected = false;  // unrecognized content occupied the Data.lua path
    bool bRuntimeCollisionDetected = false;  // unrecognized content occupied the Runtime.lua path
                                             // (symmetric with the data flag -- see correction 1)
    bool bDataLuaSyntaxCheckFailed = false;  // LuaSyntaxCheck_SYS rejected the RENDERED data text;
                                             // write refused -- separate from a collision
    bool bRuntimeSyntaxCheckFailed = false;  // LuaSyntaxCheck_SYS rejected the resolved runtime
                                             // text (bundled or override); write refused
    std::vector<std::string> writtenFilePaths;
    std::string              debugLog;

    void Log(const std::string& line) { debugLog += line; debugLog += '\n'; }
};

// gameInstallRoot: validated by this function itself via
// GameInstallLocation_IO::ValidateGameInstallRoot (STEP64). recipe.mapName names the target
// subfolder and the three files' shared filename prefix. runtimeResourceDirectory/
// runtimeOverridePathOrEmpty pass through verbatim to WO6's LoadScenarioRuntimeText (§0).
ScenarioExportResult ExportMapScenario(const std::string& gameInstallRoot,
                                       const Params::MapRecipe& recipe,
                                       const std::string& runtimeResourceDirectory,
                                       const std::string& runtimeOverridePathOrEmpty);

} // namespace Io
} // namespace SanmapGen
```

### 2. New primitive: `ReadTextFileBytes` — `src/io/FilesystemPrimitives_IO.h`/`.cpp` (EDIT)

⚠️ **STEP72 (WO6) specs this same primitive and lands FIRST in build order.** Check whether
`ReadTextFileBytes` already exists before adding it — if STEP72 has landed, this section is already
satisfied and must not be duplicated. The rationale below stands either way.

No existing read counterpart to `WriteBinaryFileBytes` exists in this file (confirmed by grep —
`AppSettings_IO.cpp` does its own inline `ifstream` read rather than going through a shared
primitive). This ticket is the **second** caller needing "read an existing small text file's full
contents, total, never throwing" (banner detection on both the Data and Runtime paths), crossing the
"recurs across two or more call sites → shared primitive" bar `FilesystemPrimitives_IO.h`'s own
header comment sets as its reason for existing. Add:

```cpp
// Reads filePath's entire contents as text. false + outText left EMPTY if the file does not exist
// or cannot be opened for read -- never throws, never partial-fills outText on failure. The read
// counterpart to WriteBinaryFileBytes, first needed by ScenarioScript_Export_IO's (STEP71)
// overwrite-safety banner check on an already-existing SanGen-owned scenario file.
bool ReadTextFileBytes(const std::string& filePath, std::string& outText);
```

`.cpp`: `std::ifstream inputStream(filePath, std::ios::binary); if (!inputStream) return false;
outText.assign(std::istreambuf_iterator<char>(inputStream), std::istreambuf_iterator<char>());
return true;` — the same idiom `AppSettings_IO.cpp:49-55` and `MapExporter_IO_Test.cpp:33-37`
already use inline, now given one shared home.

### 3. `ExportMapScenario` — sequence

1. `const auto rootValidation = ValidateGameInstallRoot(gameInstallRoot);` (STEP64). **Invalid →
   log `rootValidation.reason` verbatim, return immediately with every field at default (all
   `false`, empty `writtenFilePaths`)** — never a crash, never a silent skip. The debugLog is the
   only signal; UI surfacing is WO8's job.
2. `const std::string mapScriptDirectory = JoinExportPath(JoinExportPath(JoinExportPath(
   gameInstallRoot, "engine"), "LJ/lua/maps"), recipe.mapName);` — the exact
   `<gameInstallRoot>/engine/LJ/lua/maps/<MapName>/` path `ARCH_15_04_ThreeFileOnDiskShape.md` §15.4/STEP64 name.
   `EnsureFolderExists(mapScriptDirectory, errorMessage)` — **created if absent, not an error**; a
   genuine creation failure logs `errorMessage` and returns with all fields at default, same posture
   as step 1.
3. `bOrchestratorPresent = std::filesystem::exists(JoinExportPath(mapScriptDirectory,
   recipe.mapName + "_data.lua"))`. **SanGen never opens, reads, or writes this path under any code
   path in this function** — existence-only check. `false` → `result.Log("scenario files written but
   inert until " + recipe.mapName + "_data.lua exists")` — **never blocks the two writes below.**
4. `const std::string dataLuaText = BuildScenarioDataLuaText(recipe);` (STEP70).
5. **Syntax pre-check:** `const auto dataSyntax = Sys::CheckLuaSyntax(dataLuaText);` —
   `!dataSyntax.bSucceeded` → `bDataLuaSyntaxCheckFailed = true`, log the exact `lineNumber`/
   `message`, **do not write the Data.lua path at all** (skip to step 7) — never a partial/corrupt
   write of text SanGen itself knows is broken.
6. **Write-target safety for `<MapName>_Scenarios_Data.lua`** (only reached if step 5 passed):
   `const std::string dataLuaPath = JoinExportPath(mapScriptDirectory, recipe.mapName +
   "_Scenarios_Data.lua"); std::string existingText; const bool bExists =
   ReadTextFileBytes(dataLuaPath, existingText);`
   - `!bExists` **or** `existingText` starts with `kScenarioGeneratedFileBannerLine` (STEP70) →
     `WriteBinaryFileBytes(dataLuaPath, dataLuaText.data(), dataLuaText.size())`,
     `bDataLuaWritten = true`, record the written path.
   - Else (present, unrecognized) → write `dataLuaText` to the sibling
     `<MapName>_Scenarios_Data.sangen-pending.lua` instead, `bDataLuaCollisionDetected = true`, log a
     specific error naming **both** paths (the occupied original and the pending sibling) — never a
     silent skip, never touching the original (`MAP_SCENARIO_SPEC.md` §2.1 point 3, `ARCH_15_04_ThreeFileOnDiskShape.md` §15.4).
7. **Runtime resolution + write, symmetric to steps 5-6:** `const auto runtimeResult =
   LoadScenarioRuntimeText(runtimeResourceDirectory, runtimeOverridePathOrEmpty);` (§0's assumed
   contract). `!bSucceeded` → log `runtimeResult.errorMessage`, `bRuntimeCopied` stays `false`,
   **do not fall through to a write** (skip to step 8 — a missing/unreadable runtime never blocks
   the Data.lua write already completed in step 6, and never crashes the export). `bSucceeded` →
   `const auto runtimeSyntax = Sys::CheckLuaSyntax(runtimeResult.runtimeLuaText);` — failure →
   `bRuntimeSyntaxCheckFailed = true`, log, skip the write (this is the ticket's actual pre-write
   safety net on a designer-supplied **override** file, `DESIGN_MapScenarioIO_R1.md` §4's stated
   reason `LuaSyntaxCheck_SYS` exists at this call site). Syntax pass → same banner-gated
   read/overwrite/collision logic as step 6, against `<MapName>_Scenarios_Runtime.lua`, setting
   `bRuntimeCopied`/`bRuntimeCollisionDetected` and logging `runtimeResult.sourceDescription`
   ("bundled"/"override") either way.
   ⚠️ **Also log `runtimeResult.errorMessage` whenever it is non-empty — NOT gated on
   `!bSucceeded`** (STEP72's amendment to the contract). A successful degrade-to-bundled sets
   `bSucceeded == true` *and* a non-empty `errorMessage`; gating the log on failure alone would
   make a skipped override silent, exactly what Constitution §6 forbids.
8. Return `result`.

**Ruling on `LuaSyntaxCheck_SYS` (STEP65) as a pre-write safety net — YES, call it on both texts
before either write** (Constitution §6 "validate all input," applied to a write that lands in the
player's live game install): the Data text is SanGen's own render (cheap insurance against a
rendering-bug regression) and the Runtime text may be a hand-authored override (genuinely likely to
contain a real mistake) — both deserve the same loud, refuse-only-that-file treatment a banner
collision already gets, never a silent write of text SanGen or the resolver flagged as invalid Lua.

## Files touched
- NEW `src/io/ScenarioScript_Export_IO.h`/`.cpp` — per §1/§3.
- NEW `src/io/ScenarioScript_Export_IO_Test.cpp`.
- EDIT `src/io/FilesystemPrimitives_IO.h`/`.cpp` — `ReadTextFileBytes` (§2).
- `CMakeLists.txt` — one new `add_sangen_test(ScenarioScript_Export_IO_Test
  src/io/ScenarioScript_Export_IO_Test.cpp)`. No direct new link expected (mirrors STEP65's "no
  extra `target_link_libraries` line" precedent — this ticket calls `Sys::CheckLuaSyntax` and
  `Io::LoadScenarioRuntimeText` only through their `std::string`-shaped contracts).

## Backend policy
N/A — a handful of `std::filesystem`/`ifstream`/`ofstream` calls plus two `Sys::CheckLuaSyntax`
compiles, at most once per user-initiated scenario export (a UI button click in WO8) — not a
per-frame path, no compute dispatch.

## ARCH rules invoked
- `ARCH_15_04_ThreeFileOnDiskShape.md` §15.4 — the three-file shape and the full overwrite-safety mechanism implemented here
  verbatim (both files banner-gated symmetrically — correction 1).
- `MAP_SCENARIO_SPEC.md` §2.1 — the same mechanism in ratified spec wording; direct source of
  corrections 1/2.
- `MAP_SCENARIO_SPEC.md` §2.2 — the legacy-migration behavior the acceptance test verifies
  (untouched, not auto-migrated, not a collision).
- `ARCH_15_08_ThirdPartyDependencyRuling.md` §15.8 / STEP65 — `LuaSyntaxCheck_SYS`'s two named call sites; this is the IO-side one,
  wired here for the first time.
- `DESIGN_MapScenarioIO_R1.md` §5 — the `ScenarioExportResult` shape extended here (adding the
  runtime-side collision/syntax flags, per correction 1).
- Constitution §6 — total, never-crashing degrade on every failure path (invalid root, folder
  creation failure, runtime-resolution failure, syntax-check failure, banner collision) — none ever
  aborts the other file's write or throws.

## Explicit out-of-scope
- **`ExportAll`/`ExportSanmapOnly`, or any change to `MapExportResult`** — untouched;
  `ScenarioExportResult` is separate by design, never merged.
- **`BuildScenarioDataLuaText`'s rendering logic, the banner constant's definition** — STEP70; this
  ticket only consumes and compares against it.
- **`ScenarioScript_RuntimeResource_IO`, the bundled resource content, its CMake staging** — WO6;
  §0 states the assumed contract only.
- **`GameInstallLocation_IO`'s validation logic, `Io::AppSettings` fields** — STEP64; this ticket
  only calls `ValidateGameInstallRoot`.
- **Any UI wiring** — the Files tab's second export call, `gameInstallRoot` prompt, result-banner
  surfacing, the `.sangen-pending.lua` review UX — WO8, UI Expert.
- **Automatic migration of a pre-ratification map** — `MAP_SCENARIO_SPEC.md` §2.2 rules this out
  explicitly; implement the one-time-human-action posture, never an automatic one.
- **Deleting any file, including an orphaned legacy `_Scenarios_Script.lua`** — forbidden, exactly
  as forbidden as overwriting one (`MAP_SCENARIO_SPEC.md` §2.2 point 4).

## Acceptance test
New `src/io/ScenarioScript_Export_IO_Test.cpp` (registered in `CMakeLists.txt`), scratch-directory
pattern per `MapExporter_IO_Test.cpp:25-31` (`std::filesystem::temp_directory_path() /
"SanGenScenarioExportTest"`, `remove_all` before each test):

1. **Invalid `gameInstallRoot`** (missing both required subfolders) → every field at default
   `false`, `writtenFilePaths.empty()`, `debugLog` contains `rootValidation.reason`. Zero files
   created anywhere.
2. **Clean export, fresh folder** (no `_data.lua`, no pre-existing scenario files).
   `bDataLuaWritten == true`, `bRuntimeCopied == true` (stub a trivial `LoadScenarioRuntimeText`
   fixture returning `bSucceeded = true` with syntactically-valid banner-prefixed text, since WO6
   doesn't exist yet — this test necessarily stubs its dependency), `bOrchestratorPresent == false`,
   log contains the "files written but inert" phrasing. Both files exist, both open with
   `kScenarioGeneratedFileBannerLine`.
3. **Re-export over its own prior output** (both files carry the banner already). Both overwrite
   cleanly, `bDataLuaWritten == bRuntimeCopied == true`, zero collision flags.
4. **Synthetic banner collision — Data.lua.** Pre-seed `<MapName>_Scenarios_Data.lua` with
   unrecognized content (no banner). `bDataLuaWritten == false`, `bDataLuaCollisionDetected ==
   true`, `<MapName>_Scenarios_Data.sangen-pending.lua` exists with the freshly-rendered text, and —
   **critical** — the original is **byte-identical** to what was pre-seeded (untouched).
5. **Synthetic banner collision — Runtime.lua.** Mirrored, proving the symmetric treatment from
   correction 1: `bRuntimeCopied == false`, `bRuntimeCollisionDetected == true`, a
   `.sangen-pending.lua` sibling exists, original untouched.
6. **⚠️ Legacy-map migration — named test, distinct from 4/5.** Pre-seed the scratch folder with a
   file literally named `<MapName>_Scenarios_Script.lua` (the legacy filename, no banner, arbitrary
   hand-authored-looking content) alongside a real `<MapName>_data.lua`. Run a normal export.
   Assert: `bDataLuaWritten == true`, `bRuntimeCopied == true` (both NEW files written cleanly — no
   collision at all, proving filename disjointness, not a banner check, is what protects the legacy
   file), **`bDataLuaCollisionDetected == false` and `bRuntimeCollisionDetected == false`** (the
   legacy file never triggers either flag — it was never a candidate path), and the legacy file's
   bytes are identical before and after (SanGen never opened it). `bOrchestratorPresent == true`.
7. **Syntax-check refusal.** Forcing `BuildScenarioDataLuaText` to produce broken text is
   impractical (STEP70's renderer is total over any valid `Params::Scenarios`) — instead test the
   **runtime** side: stub `LoadScenarioRuntimeText` to return `bSucceeded = true` with deliberately
   malformed Lua (e.g. an unterminated `function`). Assert `bRuntimeSyntaxCheckFailed == true`,
   `bRuntimeCopied == false`, no file written to the Runtime path, and the Data.lua write from the
   same export still succeeds (`bDataLuaWritten == true`) — proves one file's refusal never blocks
   the other's write.
8. **`LoadScenarioRuntimeText` failure** (neither bundled nor override readable). Stub
   `bSucceeded = false`. `bRuntimeCopied == false`, no crash, `debugLog` contains
   `runtimeResult.errorMessage`, `bDataLuaWritten` from the same call still succeeds independently.
9. **Folder auto-creation.** A `gameInstallRoot` valid per STEP64 but whose
   `engine/LJ/lua/maps/<MapName>/` subfolder does not yet exist → export still succeeds end-to-end
   (folder created, not an error).
10. Full solo rebuild + `ctest -C Debug`: previously-passing suite stays green; the new target
    passes. **Confirm this ticket does not compile/link until WO6 exists** — if the coder reaches
    this ticket before WO6 lands, the correct action is to stop and flag it, not invent a
    placeholder `LoadScenarioRuntimeText` inside `src/io/` proper (a test-local stub, as tests 2-8
    use, is fine; a production stub is not).

## Verify
- New `src/io/ScenarioScript_Export_IO_Test.cpp` passes (all 10 items), including both
  collision-path tests (4/5) and the legacy-migration test (6) as **separate, independently
  asserted** scenarios.
- `ReadTextFileBytes` gains coverage in whichever suite hosts `FilesystemPrimitives_IO`'s existing
  tests: missing file → `false`/`outText` unchanged; existing file → `true`/exact byte match,
  including a file with embedded `\0` bytes if that suite already exercises binary-safety elsewhere.
- Full solo rebuild + `ctest -C Debug`: 100% pass, zero pre-existing test files edited or broken.
