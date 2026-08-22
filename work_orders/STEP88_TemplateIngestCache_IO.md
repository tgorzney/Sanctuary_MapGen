# STEP88 — `TemplateIngestCache_IO`: fingerprinted disk cache for a whole ingestion run

**Layer:** IO. **Domain:** asset-derived cache, same category as `AssetAtlasCache_*`.
**Sequence:** ticket 4 of 8 (85–92). **Real dependency:** ticket 87 (`Io::TemplateRecord`, the type
this cache stores) — `#include "TemplateDialect_IO.h"`. Reuses the real, shipped
`FilesystemPrimitives_IO.h` (`JoinExportPath`/`EnsureFolderExists`/`WriteBinaryFileBytes`/
`ReadTextFileBytes`) and mirrors — without sharing code with — `AssetAtlasCache_Fingerprint_IO.cpp`'s
disk-cache discipline. No dependency on 85/86/89–92.

## Root problem
`DESIGN_SantpFootprintIngestion_R1.md` §4.2 specifies this cache should be "a deliberate mirror of"
`AssetAtlasCache`'s existing disk-cache discipline (caller-supplied cache directory, a
fingerprint-matching skip of expensive work, a format-version stamp checked before the fingerprint,
bounds-checked never-throwing reads) — while explicitly noting **one real difference, not a
copy-paste**: at this corpus's ~2 MB total size, content-hashing every source file by default is
cheap, unlike `AssetAtlasCache`'s 1.75 GB sanpack case where it rightly defaults off.

## Deliberate design decision: JSON, not `AssetAtlasCache`'s binary `ByteCursor` format
`AssetAtlasCache`'s binary blob format exists because it caches genuinely large raw pixel-page data.
This cache stores ~546 small, already-typed structured records (~2 MB total) — squarely
`nlohmann::json`'s existing niche everywhere else in `src/io/` (the `.sanmap` document itself,
`AppSettings.json`). The design doc's own §4.2 "bounds-checked reads that fall through to a rebuild"
language is a LESSON (never read out of bounds, corrupt cache never fatal) applied here via
`nlohmann::json::parse`'s own `try`/`catch` plus explicit `is_object()`/`is_array()`/type checks
before ever indexing — the exact never-throwing degrade pattern `AppSettings_IO.cpp::LoadAppSettings`
already established (confirmed real, read this session), generalized from one flat object to an
array of records. Not a binary `ByteCursor` reimplementation.

## Fix

### 1. New file: `src/io/TemplateIngestCache_IO.h`
```cpp
// TemplateIngestCache_IO.h — a fingerprinted disk cache for a whole ingestion run's TemplateRecord
// set, modelled on AssetAtlasCache's own disk-cache discipline (manifest-style path keying, a
// format-version stamp checked BEFORE the fingerprint, never-throwing bounds-safe reads that fall
// through to "rebuild") — NOT a byte-for-byte copy of its binary ByteCursor blob format; see this
// ticket's own "Deliberate design decision." Layer: IO.
#pragma once
#include "TemplateDialect_IO.h"
#include <cstdint>
#include <string>
#include <vector>

namespace SanmapGen {
namespace Io {

// Cheap validity check for the WHOLE scan — content-hashing every one of ~546 small files is "free"
// at this corpus size (unlike AssetAtlasCache's 1.75 GB sanpack case, which defaults content hashing
// off for exactly the opposite reason), so per-file fingerprints are folded into ONE combined digest
// rather than compared file-by-file — ANY changed, added, or removed source file changes this value,
// triggering a full rebuild (never a partial/incremental one — matches AssetAtlasCache's own
// all-or-nothing rebuild granularity, generalized from one source file to many).
struct TemplateIngestFingerprint {
    int           sourceFileCount     = 0;
    std::uint64_t combinedContentHash = 0;   // FNV-1a fold, in sourceLogicalPath-sorted order, of
                                              // every source file's own {byteSize, modifiedTime,
                                              // contentHash} — sorted so the fold is independent of
                                              // ThreadPool completion order (ticket 89's fan-out).
    bool Matches(const TemplateIngestFingerprint& other) const {
        return sourceFileCount == other.sourceFileCount &&
               combinedContentHash == other.combinedContentHash;
    }
};

struct CachedTemplateIngest {
    TemplateIngestFingerprint   fingerprint;
    std::vector<TemplateRecord> records;   // every bRecognized record from the run that produced
                                            // this cache, INCLUDING skipped-by-design Projectile/
                                            // Marker entries (ticket 89 still wants their counts).
};

// Bump whenever TemplateRecord's fields or TemplateDialect_IO's extraction logic changes — a
// mismatch invalidates the cache unconditionally, before the fingerprint is even compared
// (DESIGN_SantpFootprintIngestion_R1.md §4.2: "a reader-logic change invalidates every cache
// without touching source files").
inline constexpr std::uint32_t kTemplateIngestCacheFormatVersion = 1;

std::string TemplateIngestCachePathFor(const std::string& cacheDirectory, const std::string& gameInstallRoot);

// False + outCache left UNTOUCHED on ANY anomaly (missing file, malformed JSON, format-version
// mismatch, fingerprint mismatch, or a record whose fields fail type-checking) — a corrupt or stale
// cache is never fatal, it is simply not used (Constitution §6; caller falls through to a full
// rebuild, exactly AssetAtlasCache::LoadFromDisk's own contract).
bool LoadTemplateIngestCache(const std::string& cacheDirectory, const std::string& gameInstallRoot,
                              const TemplateIngestFingerprint& expected, CachedTemplateIngest& outCache);

// False (logged to std::cerr — this document sits outside the `.sanmap`/MapImportResult domain,
// same posture AppSettings_IO.cpp already uses) on a write failure — never a partial file
// (WriteBinaryFileBytes's own contract, reused rather than a second write path invented here).
bool SaveTemplateIngestCache(const std::string& cacheDirectory, const std::string& gameInstallRoot,
                              const CachedTemplateIngest& cache);

} // namespace Io
} // namespace SanmapGen
```

### 2. New file: `src/io/TemplateIngestCache_IO.cpp` — implementation contract
1. **Filename keying** mirrors `AssetAtlasCache_Fingerprint_IO.cpp`'s real, confirmed
   `<stem>_<16-hex-FNV1a-of-fullpath>` scheme (`CacheFileStem`, `AssetAtlasCache_Fingerprint_IO.cpp:45-51`)
   but keyed on `gameInstallRoot` itself (there is no single "source file" for a whole install) —
   `TemplateIngestCachePathFor` returns
   `<cacheDirectory>/<installRootFolderName>_<16-hex-FNV1a-of-absolute-gameInstallRoot-path>.santpingestcache`.
   The FNV-1a text hash is the same trivial algorithm duplicated locally (as ticket 86 also does),
   not exported from `AssetAtlasCache_Fingerprint_IO.cpp`'s private anonymous namespace.
2. **`LoadTemplateIngestCache`**: read the file via `Io::ReadTextFileBytes`; on any read failure,
   return false immediately. `nlohmann::json::parse` inside a `try`/`catch`; on a thrown
   `json::exception`, or `!document.is_object()`, return false. Read `formatVersion`
   (`ReadJsonInteger`-equivalent on an unsigned field, or a plain `document.value("formatVersion",
   0u)` guarded the same way `ReadJsonInteger` already is) — mismatch against
   `kTemplateIngestCacheFormatVersion` → false, cache not even fingerprint-checked. Read the stored
   fingerprint's two fields; `!stored.Matches(expected)` → false. Only then iterate
   `document["records"]` (must be an array — `!is_array()` → false), building one `TemplateRecord`
   per element.

   **⚠️ Correction 2026-08-22 — `ReadJsonInteger` does NOT cover this ticket's `uint64_t` fields,
   real compile blocker in an earlier draft.** The real `JsonPrimitives_IO.h::ReadJsonInteger` takes
   a fixed `int&` out-parameter — it does not overload or template for `std::uint64_t`, and no
   `uint64_t`-capable JSON accessor exists anywhere in `src/io/` today (confirmed by grep). Binding
   `TemplateRecord::sourceByteSize`/`sourceModifiedTime`/`sourceContentHash` or
   `TemplateIngestFingerprint::combinedContentHash` (all `std::uint64_t`) through `ReadJsonInteger`
   is a hard reference-type mismatch that will not compile. Use
   `document.value("sourceByteSize", std::uint64_t{0})`-style direct `nlohmann::json` accessors (or
   `document[key].get<std::uint64_t>()` guarded by `document.contains(key) &&
   document[key].is_number_unsigned()`) for these four fields specifically. `ReadJsonFloat`/
   `ReadJsonText`/`ReadJsonBoolean` are still correct as-is for every other field
   (`baseFootprintWidth`/`baseFootprintDepth` as `float`, `templateIdentifier`/`sourceLogicalPath`
   as `std::string`, `bTpIdWasDeclared`/`bHasFootprint` as `bool`) — only the four `uint64_t` fields
   need the direct-accessor treatment. A single malformed record element still aborts the WHOLE load
   (returns false), never a partially-filled `outCache` (Constitution §6's "never a partial success"
   applied to a cache specifically, since a half-loaded ingestion table is worse than none — it
   would silently under-report real coverage).
3. **`SaveTemplateIngestCache`**: build the mirror JSON document (`formatVersion`, `fingerprint`
   object, `records` array — one object per `TemplateRecord`, PascalCase members, e.g.
   `"TemplateIdentifier"`, `"DialectKind"` written as its integer value, `"BaseFootprintWidth"`, etc.);
   `EnsureFolderExists(cacheDirectory, ...)` first (existing, reused); `WriteBinaryFileBytes` the
   `.dump(4)` text (existing, reused, same trunc-write posture `AppSettings_IO.cpp` already uses for
   its own convenience-state document — Constitution §6: this is a rebuildable cache, not `.sanmap`
   map data, an atomic-write upgrade is not warranted here any more than it was for `AppSettings.json`).

## Files touched
- NEW `src/io/TemplateIngestCache_IO.h`
- NEW `src/io/TemplateIngestCache_IO.cpp`
- NEW `src/io/TemplateIngestCache_IO_Test.cpp`
- `CMakeLists.txt` — one new `add_sangen_test(TemplateIngestCache_IO_Test src/io/TemplateIngestCache_IO_Test.cpp)`.

## Backend policy
CPU only. One JSON parse/dump per `Load`/`Save` call, at most once per ingestion run (ticket 89 calls
this at most twice per session: once to check, once to save on a miss). ~2 MB JSON parse is
sub-100ms on any modern machine — no microbenchmark warranted (Constitution §7, direct measurement
basis, same as ticket 86's own cost argument).

## ARCH rules invoked
- `ASSET_LOADING_SPEC.md` "Disk cache" — the fingerprint-then-skip-expensive-work discipline this
  ticket generalizes from `AssetAtlasCache`'s one-source-file case to this ticket's many-source-file
  case.
- Constitution §6 — total, never-throwing: every failure path returns false with `outCache`
  untouched; no partial fill, ever.
- Constitution §7 — the format-version stamp is the mechanism that makes "a reader-logic change
  invalidates every cache" a real, testable property rather than an aspiration.
- §1.5 size ceilings — both new files budgeted comfortably under the 100-line soft ceiling.

## Explicit out-of-scope
- **Computing the fingerprint from a live scan** — that is `TemplateSourceScan_IO`'s output
  (ticket 86) plus ticket 89's own fold into a `TemplateIngestFingerprint`; this ticket only
  compares an already-computed `expected` value against what is stored on disk.
- **Populating `Io::WorldFootprintSizeTable`** — ticket 89.
- **Per-file incremental caching** (rebuilding only the changed subset) — deliberately not built;
  any change triggers a full rebuild, matching `AssetAtlasCache`'s own all-or-nothing granularity.
  A future ticket may revisit this if the corpus ever grows enough to matter — not indicated by
  today's ~2 MB size.
- **A binary `ByteCursor` blob format** — explicitly rejected, see "Deliberate design decision" above.

## Acceptance test
New `src/io/TemplateIngestCache_IO_Test.cpp` (registered in `CMakeLists.txt`):
- Save a `CachedTemplateIngest` with 3 `TemplateRecord` entries (mixed dialect kinds, one with
  `tags` populated) to a scratch cache directory, then `LoadTemplateIngestCache` with a MATCHING
  expected fingerprint returns `true` and every field of every record round-trips exactly
  (floats via `NearlyEqual`, strings/vectors/enums exact).
- `LoadTemplateIngestCache` with a MISMATCHED fingerprint (different `sourceFileCount` or
  `combinedContentHash`) on an otherwise-valid cache file returns `false`, `outCache` untouched.
- A cache file whose `formatVersion` does not match `kTemplateIngestCacheFormatVersion` returns
  `false` even when the fingerprint WOULD have matched — proves the version stamp is checked first.
- A missing cache file, a cache file containing malformed JSON, and a cache file whose `records`
  array contains one element missing a required field all return `false`, `outCache` untouched, no
  crash/throw in any case.
- `TemplateIngestCachePathFor` called twice with two DIFFERENT `gameInstallRoot` values (even if
  their folder names happen to match, e.g. two different drives both named `.../SteamGames/Sanctuary`)
  produces two DIFFERENT paths — proves the path-digest keying actually distinguishes installs, the
  same collision-avoidance property `AssetAtlasCache`'s own scheme guarantees.
- Full solo rebuild + `ctest -C Debug`: previously-passing suite stays green; the new target passes.

## Verify
- New `src/io/TemplateIngestCache_IO_Test.cpp` passes.
- Grep this ticket's `.cpp` for any raw byte-offset arithmetic outside `nlohmann::json` calls — should
  be none, confirming the JSON-not-binary design decision was actually followed.
- Full solo rebuild + `ctest -C Debug`: 100% pass, zero unrelated test files edited or broken.
