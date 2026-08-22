# Work-Order — Step 19: global app-settings persistence — schema-v3 Correction 9

*Constitution §7. Executor: SanGen Coder. Implements `SANMAP_FORMAT_SPEC.md` Correction 9's
positive half: a new, durable, global (not per-map) settings file outside `.sanmap` entirely.
Design obtained from an IO Architecture Expert consult (see "Design" below — binding, not a
starting point) plus two human product decisions (save timing, `FastPreviewMode`'s fate).
Genuinely greenfield: confirmed zero app-settings persistence exists anywhere in this codebase
today — every path/toggle currently resets to empty/default on every launch.*

## Root problem
`sanpackPath` (`Application_UI.h`), `assetCacheDirectory` (`SystemTab_UI.h` SCOPE NOTE 1), and
`environmentPackPath` (`StratumsTab_UI.h` SCOPE NOTE 2) are each caller-owned UI state with their
own comment admitting "no durable home yet." `ApplicationExecutionSettings`'s three GPU/backend
toggles (`bUseGpuTerrain`, `bUseGpuFlow`, `bWysiwygBaking`) already exist and already fan out
correctly via the existing `Ui::ApplyExecutionSettings(...)` — but only on user interaction, never
seeded at startup. `GamedataPath`/`GlobalEnvironmentPath` (the literal v1 fields) and
`GPUPreviewIterations`/`FastPreviewMode` are confirmed to have zero live target anywhere in v2 —
see "Explicit out-of-scope" for why each is excluded rather than resurrected.

## Design (IO Architecture Expert consult — binding)
1. **Type home: a plain IO-owned struct, NOT `Params::`.** `PARAMS_PIPELINE_SPEC.md` already rules
   `GamedataPath`/`GlobalEnvironmentPath`'s home as "(none — app-local, not a recipe field)" — a
   `Params::` type would be structurally wrong by ARCH's own definition (PARAMS depends on
   nothing, serializes only to `.sanmap`). Not `Sys::DispatchPolicy` fields directly either — IO
   may not depend on SYS. Same footing as `MapExportOptions`/`AtlasBuildSettings` — a plain bag of
   scalars translated into `Sys::DispatchPolicy` edits by `Ui::Application`, the one unit that
   already legally touches both IO and SYS.
2. **File location: `%APPDATA%\SanGen\AppSettings.json`** (Windows `FOLDERID_RoamingAppData` via
   `SHGetKnownFolderPath`, same header this codebase's `FileDialog_Shell_IO.cpp` already links). A
   fixed, always-resolvable bootstrap anchor — solves the chicken-and-egg problem a fully
   "user-chosen SanGen folder" would create (you can't ask the user where settings live before
   you've loaded settings). If a larger user-chosen data folder is wanted later (e.g. for the
   icon/thumbnail cache), it becomes ONE FIELD inside this bootstrap file, not the file's own
   location.
3. **Format:** `nlohmann::json`, flat top-level object, direct 1:1 with C++ member names — no
   PascalCase-section convention (that's an ARCH_01_06_SanmapKeyCasing.md §1.6 `.sanmap` rule; this isn't a `.sanmap`
   document and carries no `SanGenVersion`).
4. **Load/save timing:**
   - Load inside `Ui::Application`'s constructor/early `Initialize()` — mirror where
     `LoadAssetAtlas()` already runs. **Never in `ApplicationMain_UI.cpp`** — that file's own
     header comment says it is deliberately "nothing but the entry point" (ARCH_05_GodObjectDismemberment.md §5.5 retired the
     v1 pattern of loading everything in `main.cpp`; loading settings there would resurrect it).
   - After load: seed `ApplicationExecutionSettings`'s three fields + `SystemTabState::
     bDeterministic` from the loaded struct, then call the ALREADY-EXISTING
     `Ui::ApplyExecutionSettings(...)` once at startup. Zero new PIPELINE/SYS plumbing needed.
   - **Save: on clean shutdown only** (human-ratified) — not autosave-on-every-change. Bundle the
     flush with however `Ui::Application::Run()`'s normal exit path already unwinds.
5. **File-write safety:** direct `ofstream` trunc-write is sufficient — matches this codebase's
   own precedent (even the primary `.sanmap` writer isn't atomic/temp-file+rename). A missing or
   corrupt settings file is a logged fallback to compiled defaults, never a hard failure —
   mirrors the total, never-throwing `ReadJson*` contract already established everywhere else.
   Reuse `EnsureExportFolderExists`'s folder-creation logic rather than duplicating it; if its
   `MapExportResult&`-specific signature doesn't fit cleanly, a small result-agnostic refactor is
   in scope (flag it, don't hack around it).
6. **Files, flat in `src/io/`, no subfolder:**
   - `AppSettings_IO.h`/`.cpp` — `struct AppSettings { ... };` (see field list below) plus
     `Load(directory)`/`Save(directory, settings)`, directory supplied by the caller (mirrors
     `AssetAtlasCache::LoadFromDisk`/`SaveToDisk`'s own shape) — the struct itself does not
     hardcode the bootstrap path.
   - `AppSettingsLocation_IO.h` + `AppSettingsLocation_Shell_IO.cpp` — the one new platform
     touchpoint, `std::string DefaultAppSettingsDirectory();`, `#ifdef _WIN32` implementation —
     direct structural twin of the existing `FileDialog_IO.h`/`FileDialog_Shell_IO.cpp` split
     (Constitution §5's platform seam).
   - **Deliberately NOT named with the `MapExporter_<Domain>_IO`/`MapImporter_<Domain>_IO`
     pairing** — that convention is reserved for `.sanmap` top-level sections per
     `IO_MIGRATION_SPEC.md` §1's own definition of `Domain`. This file is outside the
     `.sanmap`/`SanGenVersion`/migration system entirely.

## Target files
New: `src/io/AppSettings_IO.h`/`.cpp`, `src/io/AppSettingsLocation_IO.h`,
`src/io/AppSettingsLocation_Shell_IO.cpp`.

Modified: `src/ui/Application_UI.h`/`.cpp` (or `Application_Assets_UI.cpp`/wherever
initialization logic lives) — load `AppSettings` at startup, seed `sanpackPath`/
`assetCacheDirectory`/`environmentPackPath`/`ApplicationExecutionSettings` from it, call
`ApplyExecutionSettings` once; save on clean shutdown. `src/io/MapExporter_IO.h`/`.cpp` — only if
`EnsureExportFolderExists` needs the result-agnostic refactor noted in Design item 5.

## Layer & accuracy class
IO/BRIDGE (new subsystem, outside the `.sanmap` domain) + a small UI-layer startup/shutdown wiring
touch. Accuracy class: Exact for the round-trip; best-effort/degrade-gracefully for a missing or
corrupt file (never a hard failure — this is convenience state, not map data).

## Backend policy
CPU only, one-time at startup/shutdown — not a per-frame or per-generation concern.

## ARCH rules invoked
- `SANMAP_FORMAT_SPEC.md` Correction 9, `PARAMS_PIPELINE_SPEC.md` (both already rule these fields
  out of `Params::`/`.sanmap` scope — this ticket builds the replacement home, doesn't re-litigate
  the exclusion).
- Constitution §5 (platform seam) — the `#ifdef _WIN32` split mirrors `FileDialog_IO.h`'s existing
  precedent exactly.
- Constitution §6 — total, never-throwing load; a bad/missing file degrades to compiled defaults,
  logged, never crashes the app.
- ARCH_05_GodObjectDismemberment.md §5.5 — `ApplicationMain_UI.cpp` stays entry-point-only; do not load settings there.

## Solution — `AppSettings` field list
```cpp
struct AppSettings {
    std::string sanpackPath;
    std::string assetCacheDirectory;
    std::string environmentPackPath;
    bool bUseGpuTerrain = true;    // ARCH Expert-confirmed real default, Application_Execution_UI.h:36
    bool bUseGpuFlow    = true;    // ARCH Expert-confirmed real default, Application_Execution_UI.h:37
    bool bWysiwygBaking = false;   // ARCH Expert-confirmed real default, Application_Execution_UI.h:38
    bool bUseGpuMarkers = false;   // NEW — see "Flagged, not blocking" below; no existing default to
                                    // match since no toggle exists yet, false is this ticket's own
                                    // conservative choice
};
```

**Flagged, not blocking this ticket:** `bUseGpuMarkers` has a real live target
(`placementStage`'s own `Sys::DispatchPolicy`, per ARCH_04_DispatchContract.md §4.2) but no UI toggle wired to it yet.
This ticket gives it a settings-file home and seeds it into that policy at startup (the same
one-line pattern `bUseGpuTerrain`/`bUseGpuFlow`/`bWysiwygBaking` already use via
`ApplyExecutionSettings`) — but does NOT add a checkbox to `SystemTab_UI`; that's separate UI
work. If `ApplyExecutionSettings` doesn't currently take a markers-GPU flag, extend its signature
minimally to accept one rather than leaving the loaded value unused.

## Explicit out-of-scope
- **`GamedataPath`/`GlobalEnvironmentPath`** (the literal v1 field names) — confirmed empty in
  every real map, zero code references anywhere in v2. Do not resurrect them; `environmentPackPath`
  (the live v2 equivalent) is what's actually included above.
- **`GPUPreviewIterations`** — confirmed retired (`Thermal_Kernel_PROC.h`'s own comment: collapsed
  into one shared per-project `iterationCount` constant, one value for both backends now). Do not
  carry it forward as a dead field.
- **`FastPreviewMode`** — human-ratified drop (no live v2 target, possibly subsumed by ARCH_04_DispatchContract.md §4.4's
  idle-escalation auto-refine, not confirmed either way). Not included in `AppSettings`.
- **A UI toggle for `bUseGpuMarkers`** — the settings-file plumbing is in scope, the checkbox is
  not (see "Flagged, not blocking" above).
- **Wiring `sanpackPath`/`assetCacheDirectory`/`environmentPackPath` into the tabs that currently
  own them as caller-owned state** beyond seeding their initial value at startup and saving their
  final value at shutdown — the tabs' own edit/commit UI is untouched.
- **A "user-chosen SanGen folder" for the icon/thumbnail cache** — the bootstrap file's own
  location is fixed (`%APPDATA%\SanGen\`); a separate, larger user-relocatable data folder concept
  is explicitly deferred (Design item 2's "becomes one field inside this file, later").

## Acceptance test
`AppSettings_IO_Test.cpp`: a populated `AppSettings` survives `Save`→`Load` exactly. Loading from a
missing directory/file falls back to default-constructed `AppSettings`, logged, no crash. Loading
a corrupt/malformed JSON file does the same. `DefaultAppSettingsDirectory()` returns a non-empty,
plausible path on Windows (exact value not asserted, since it's environment-dependent — assert
shape/non-emptiness, not a literal string). An `Ui::Application` startup/shutdown integration test
(if one already exists for this class, extend it; if not, a minimal one) confirms: constructing
`Application` with a pre-existing settings file seeds `sanpackPath`/`ApplicationExecutionSettings`
correctly, and a clean `Run()`-exit writes the current values back out. Full `SanGenV2` build stays
clean; every existing test continues to pass.
