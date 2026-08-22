# STEP86 — `TemplateSourceScan_IO`: resolve and walk the three `.santp`/`.sanprop` template roots

**Layer:** IO. **Domain:** `.santp`/`.sanprop` template ingestion, source resolution only (no
interpretation — see "Deliberate design decision" below). **Sequence:** ticket 2 of 8 (85–92).
**Real dependency:** `STEP64_GameInstallLocation_IO.md` (real, shipped — `gameInstallRoot` /
`Io::ValidateGameInstallRoot`) for the validated-root concept this ticket consumes but does not
re-validate, and the real, shipped `Io::SanpackReader`/`Io::ReadTextFileBytes`/`Io::JoinExportPath`
(`SanpackReader_IO.h`, `FilesystemPrimitives_IO.h`), reused rather than duplicated. **No dependency
on ticket 85** (this ticket never touches Lua — see below) and no dependency on 87–92.

## Root problem
`DESIGN_SantpFootprintIngestion_R1.md` §1.1/§3.3 (live-verified against the real Steam Demo install
this session, re-confirmed by reading the design doc in full) establishes that `.santp`/`.sanprop`
template data ships across **three structurally different sources**, and that no directory-walking
scanner exists anywhere in `src/` today — the only `std::filesystem::directory_iterator` in the whole
tree (`src/io/MapImporter_IO.cpp:56`) is non-recursive and unrelated. §1.6 leaves genuine doubt
whether the `Environment.sanpack.unzipped/` tree ships with the game or is a local-machine extraction
artifact — this ticket must handle both possibilities, never assume either.

## Deliberate design decision: this ticket loads, it never interprets
This file yields **raw file text**, doing zero content interpretation — dialect detection (`.sanprop`
that is actually JSON, root-table-name classification) is `TemplateDialect_IO`'s job (ticket 87).
This is Constitution §1's "IO LOADS, it never simulates/interprets" applied literally to a domain
that could tempt a coder to fold detection logic into the walk itself — the design doc's own trap 1
(§1.5: "`.sanprop` does not imply Lua… a reader must sniff content, never trust the extension") is
explicitly a **ticket 87** concern, not this one; this ticket collects both `.santp` and `.sanprop`
files uniformly and hands their text onward unexamined.

## Fix

### 1. New file: `src/io/TemplateSourceScan_IO.h`
```cpp
// TemplateSourceScan_IO.h — resolves the .santp/.sanprop template source roots off a game install
// and yields raw file text, doing NO interpretation of that text (dialect/JSON-sniffing is
// TemplateDialect_IO's job, ticket 87 — Constitution §1 layering: this file only LOADS). Layer: IO.
// First recursive directory walk in src/ (the sole existing std::filesystem::directory_iterator,
// MapImporter_IO.cpp:56, is non-recursive and unrelated).
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace SanmapGen {
namespace Io {

// Lower rank == higher priority when the SAME templateIdentifier is later found in more than one
// source (TemplateDialect_IO's first-write-wins collision policy, ticket 87). Loose files (either
// LJ/lua's own tree or the Gamedata/Props+Pandemonium loose trees) rank above an extracted/unzipped
// pack tree, which ranks above reading the same content out of a compressed .sanpack —
// DESIGN_SantpFootprintIngestion_R1.md §3.3's own "prefer unzipped over sanpack, cheaper, no
// inflate" ordering, generalized across ALL sources by the same "prefer the more direct source" logic.
enum class TemplateSourceRank { LooseFile = 0, UnzippedPackTree = 1, CompressedSanpack = 2 };

struct TemplateSourceFile {
    // Stable identity for caching (ticket 88) and diagnostics — the real filesystem path for a
    // loose/unzipped source, or "<sanpackPath>!<entryName>" for a zip entry (mirrors
    // Io::SourceFingerprint's own sourcePath usage, AssetAtlasCache_IO.h:38-48).
    std::string        logicalPath;
    std::string        sourceText;
    TemplateSourceRank sourceRank = TemplateSourceRank::LooseFile;
    std::uint64_t       byteSize = 0;
    std::uint64_t       modifiedTime = 0;   // 0 for a sanpack-extracted entry (no independent mtime)
    std::uint64_t       contentHash = 0;    // FNV-1a over sourceText's bytes -- computed here because
                                             // the bytes are already resident (§4.2's "content hashing
                                             // is cheap at this corpus size" argument); ticket 88's
                                             // cache validity check and ticket 89's per-record
                                             // Io::SourceFingerprint both consume this field.
};

// Instrumentation + degrade-gracefully signal (Constitution §6 — a missing subtree is a partial
// ingest, never a hard failure; DESIGN_SantpFootprintIngestion_R1.md §4.4).
struct TemplateSourceScanReport {
    std::vector<TemplateSourceFile> files;
    bool bGamedataRootPresent            = false;   // <root>/engine/Sanctuary_Data/Gamedata
    bool bUnzippedEnvironmentTreePresent = false;   // Environment.sanpack.unzipped/Environment
    bool bEnvironmentSanpackPresent      = false;   // Environment.sanpack (fallback source)
    int  skippedOversizeFileCount        = 0;       // file_size() over the safety cap, never opened
    int  skippedUnreadableFileCount      = 0;       // ReadTextFileBytes/SanpackReader failure
};

// gameInstallRoot is validated by GameInstallLocation_IO (STEP64) BEFORE this call — this function
// does NOT re-validate it, only treats an empty root as "scan nothing" (Constitution §6, total
// behaviour). Walks, IN THIS PRIORITY ORDER: the four LJ/lua loose subtrees (units/props/markers/
// projectiles), the Gamedata/Props + Gamedata/Pandemonium loose trees, then the Environment source
// (the unzipped tree if present, else Environment.sanpack via Io::SanpackReader — NEVER both,
// §3.3's own "double-ingesting is a guaranteed tpId collision storm" warning).
TemplateSourceScanReport ScanTemplateSources(const std::string& gameInstallRoot);

} // namespace Io
} // namespace SanmapGen
```

### 2. New file: `src/io/TemplateSourceScan_IO.cpp` — implementation contract
1. `gameInstallRoot.empty()` → return a default-constructed report immediately, zero filesystem calls.
2. Real subpaths (verified live this session, per the design doc's §1.1 table — note these correct
   `GAMEDATA_LAYOUT_SPEC.md`'s own wrong nesting claim, flagged to ARCH separately, not amended here):
   - `<root>/engine/LJ/lua/common/units/unitsTemplates/`
   - `<root>/engine/LJ/lua/common/props/propsTemplates/`
   - `<root>/engine/LJ/lua/common/markers/markerTemplates/`
   - `<root>/engine/LJ/lua/common/projectiles/projectilesTemplates/`
   - `<root>/engine/Sanctuary_Data/Gamedata/Props/`
   - `<root>/engine/Sanctuary_Data/Gamedata/Pandemonium/`
   - `<root>/engine/Sanctuary_Data/Gamedata/Environment.sanpack.unzipped/Environment/`
   - `<root>/engine/Sanctuary_Data/Gamedata/Environment.sanpack`
3. Check `<root>/engine/Sanctuary_Data/Gamedata` (`std::filesystem::is_directory`) FIRST — set
   `bGamedataRootPresent`. If absent, skip the four Gamedata-rooted sources entirely (logged via the
   report's own booleans, never a hard error) — the design doc's own §3.2 recommendation that this
   third subpath be checked **separately** from `ValidateGameInstallRoot`, since the Map Scenario
   export leg has no business failing because Gamedata is missing.
4. For each loose subtree present: `std::filesystem::recursive_directory_iterator`, filter to regular
   files whose extension (case-insensitively) is `.santp` or `.sanprop` — mirrors
   `SanpackEntryFilter::Accepts`'s own case-insensitive contract (`SanpackReader_IO.h`) without
   depending on that type. For each match: `std::filesystem::file_size` first; if it exceeds 64 MB
   (matching `SanpackSafetyLimits::maximumEntryByteSize`'s existing default value,
   `SanpackReader_IO.h` — reused as a value, not a shared type, since these files are observed at
   35.8 KB max and this is purely a defensive sanity cap) → `++skippedOversizeFileCount`, never
   opened. Otherwise `Io::ReadTextFileBytes` (existing, `FilesystemPrimitives_IO.h`); a read failure
   → `++skippedUnreadableFileCount`. On success, compute `contentHash` (FNV-1a over the read bytes —
   the SAME algorithm `AssetAtlasCache_Fingerprint_IO.cpp`'s private `HashBytes` already implements;
   duplicated as a small local helper here rather than exported from that file, since it is a five-line
   standard hash, not worth a cross-file dependency for) and append a `TemplateSourceFile` with
   `sourceRank = LooseFile`.
5. Environment source resolution: `is_directory(Environment.sanpack.unzipped/Environment)` → walk it
   exactly as step 4 (`sourceRank = UnzippedPackTree`), set `bUnzippedEnvironmentTreePresent`. ELSE
   `exists(Environment.sanpack)` (a file) → set `bEnvironmentSanpackPresent`, open via
   `Io::SanpackReader`, `Open()` + `ReadCentralDirectoryOnce()`, then
   `ExtractFiltered(SanpackEntryFilter{{}, {".santp", ".sanprop"}}, SanpackSafetyLimits{}, payloads)`
   (existing types, reused verbatim — this IS the "single-pass memory-mapped sanpack ingestion" the
   Format Expert's own charter names as a Fix-target, already solved by reusing `SanpackReader`
   unedited). Each `bValid == true` payload becomes a `TemplateSourceFile` with
   `logicalPath = sanpackPath + "!" + payload.name`, `sourceRank = CompressedSanpack`,
   `modifiedTime = 0` (a zip entry carries no independent mtime — its own `contentHash` still applies,
   computed over `payload.bytes`). A `bValid == false` payload is counted as
   `++skippedUnreadableFileCount`, never as a "record" (mirrors the reader's own documented
   "invalid record is reported, never trusted into a placeholder here" contract). ELSE (neither
   present) — this source contributes zero files, both booleans stay false, logged, not an error.

## Files touched
- NEW `src/io/TemplateSourceScan_IO.h`
- NEW `src/io/TemplateSourceScan_IO.cpp`
- NEW `src/io/TemplateSourceScan_IO_Test.cpp`
- `CMakeLists.txt` — one new `add_sangen_test(TemplateSourceScan_IO_Test src/io/TemplateSourceScan_IO_Test.cpp)`.
  This test writes real scratch files/folders (a synthetic install layout) and needs no `miniz`
  vendored-header include the way `AssetPipeline_IO_Test` does, UNLESS its sanpack-fallback test case
  writes a real synthetic zip — if so, add
  `target_include_directories(TemplateSourceScan_IO_Test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/core)`
  mirroring `AssetPipeline_IO_Test`'s own line (`CMakeLists.txt:409`).

## Backend policy
CPU only, single-pass, no `Dispatch_SYS` involvement. Cost basis (Constitution §7, direct
measurement): 546 files, 2.12 MB total, 3.9 KB average — this walk is I/O-bound and trivially fast
even sequential; no microbenchmark warranted at this scale (the same basis STEP58 already used for
its own small-N lookup).

## ARCH rules invoked
- `ARCH_18_SantpFootprintIngestion.md` — the overall ruling authorizing tickets 85–92.
- `STEP64_GameInstallLocation_IO.md` — the validated `gameInstallRoot` concept this ticket consumes,
  unedited.
- `SanpackReader_IO.h`'s own contract — single-pass, memory-mapped, filter→sort→inflate, reused
  unedited (Fix-target from this expert's own charter: "single-pass memory-mapped sanpack ingestion
  (never 2 GB in RAM)" — already solved by `SanpackReader`, this ticket adds zero new zip code).
- Constitution §1 — IO loads, never interprets; dialect/content sniffing is explicitly out of scope
  here (see "Deliberate design decision").
- Constitution §6 — every external file validated: the oversize guard, the read-failure counters, the
  "missing subtree is a partial ingest, not a failure" posture throughout.

## Explicit out-of-scope
- **Dialect/root-table detection, `.sanprop`-that-is-actually-JSON sniffing, footprint/tpId/tags
  extraction** — `TemplateDialect_IO`, ticket 87.
- **Lua evaluation of any kind** — `LuaTableEvaluate_SYS`, ticket 85; this ticket never links or calls
  it.
- **Caching the scan result** — `TemplateIngestCache_IO`, ticket 88.
- **Threaded fan-out** — this scan is a fast sequential walk (§ cost basis above); parallelism belongs
  to the per-file Lua evaluation stage, ticket 89's own `ThreadPool::ParallelFor` over this ticket's
  output.
- **Re-validating `gameInstallRoot`** — `Io::ValidateGameInstallRoot` (STEP64) is the caller's job,
  before this function is ever called.

## Acceptance test
New `src/io/TemplateSourceScan_IO_Test.cpp` (registered in `CMakeLists.txt`):
- `ScanTemplateSources("")` returns a default-constructed report (`files.empty()`, every bool false,
  every counter 0), zero filesystem calls attempted.
- A scratch install with all four LJ/lua subtrees populated with a mix of `.santp`/`.sanprop`/unrelated
  extensions (`.sanmodel`) yields exactly the `.santp`/`.sanprop` files, each with `sourceRank ==
  LooseFile`, correct `byteSize`/`contentHash` (re-hash independently in the test and compare), and
  the unrelated-extension files excluded.
- A scratch install with NO `Sanctuary_Data/Gamedata` at all: `bGamedataRootPresent == false`, and
  the Gamedata-rooted sources contribute zero files — LJ/lua sources still populate normally (proves
  the partial-ingest posture).
- A scratch install with `Environment.sanpack.unzipped/Environment/<Biome>/Props/**/*.santp` present
  AND a (deliberately different-content) `Environment.sanpack` also present: only the unzipped tree's
  files appear in the result (`sourceRank == UnzippedPackTree`), `bUnzippedEnvironmentTreePresent ==
  true`, `bEnvironmentSanpackPresent == false` — proves "prefer unzipped, never both."
- A scratch install with ONLY a real (synthetic, miniz-written) `Environment.sanpack`, no unzipped
  tree: files come from the sanpack path (`sourceRank == CompressedSanpack`,
  `logicalPath` containing `"!"`, `modifiedTime == 0`), `bEnvironmentSanpackPresent == true`.
- A file exceeding the 64 MB oversize guard (a synthetic sparse/truncated-claim file, not a literal
  64 MB write) increments `skippedOversizeFileCount` and is never opened/appended.
- An unreadable file (e.g. a directory masquerading with a `.santp` name, or a permission-denied
  scratch file where the platform allows constructing one) increments `skippedUnreadableFileCount`.
- Two files with byte-identical content produce the same `contentHash`; a one-byte content difference
  produces a different hash — proves the hash is a real content function, not a placeholder.
- Full solo rebuild + `ctest -C Debug`: previously-passing suite stays green; the new target passes.

## Verify
- New `src/io/TemplateSourceScan_IO_Test.cpp` passes.
- Grep this ticket's `.cpp` for any Lua/`lua.h`/`Sys::` reference — must have none (confirms the
  "no dependency on ticket 85" claim above).
- Full solo rebuild + `ctest -C Debug`: 100% pass, zero unrelated test files edited or broken.
