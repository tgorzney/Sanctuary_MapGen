# STEP89 — `TemplateIngest_IO`: the orchestrator, second producer of STEP58's `WorldFootprintSizeTable`

**Layer:** IO. **Domain:** the one entry point tying tickets 85–88 together. **Sequence:** ticket 5 of
8 (85–92), the end of the strict 85→86→87→88→89 chain. **Real dependency:** tickets 85 (via 87's type
only, this file calls `Sys::EvaluateLuaTableSource` directly), 86, 87, 88 — all four, genuinely (this
is the integration point). Also `AssetAtlasCache_IO.h` for the real, shipped `Io::SourceFingerprint`
type (reused verbatim, per `STEP96_FootprintBakeAndStalenessCheck_IO.md` §0's explicit requirement —
confirmed real by reading it this session) and `WorldFootprintSizeTable_IO.h` (STEP58, real, shipped,
**not edited by this ticket**). `Sys::ThreadPool` (`src/sys/ThreadPool_SYS.h`, real, shipped) for the
fan-out.

## Root problem
`DESIGN_SantpFootprintIngestion_R1.md` §4.1's flow diagram names this as the one call site UI reaches
for (ticket 91) and the one type STEP96's already-drafted "Resolve Footprint" bake button reads from.
§5 specifies the supersession mechanism precisely: **STEP58's `WorldFootprintSizeTable_IO.h` is not
edited at all** — this ticket is a SECOND producer over the same type, seeding the placeholder first,
then overlaying real ingested data, relying entirely on `SetFootprint`'s already-documented
last-write-wins policy for the placeholder-vs-real case (zero new policy code in STEP58's file).

## ⚠️ Required addition, folded in at first landing (per `STEP96` §0)
`STEP96_FootprintBakeAndStalenessCheck_IO.md` (real, already drafted, blocked on this ticket landing)
§0 specifies that its own bake button needs a per-`templateIdentifier` accessor this design doc's §3.4
file table never gave `TemplateIngest_IO`'s output type — `WorldFootprintSizeTable::Resolve()` is the
wrong accessor for baking because it silently folds "found" and "fell back to a default" into one
return with no per-entry fingerprint. This ticket ships `TemplateFootprintRecord` +
`TemplateIngestReport::FindByTemplateIdentifier` exactly as STEP96 §0 specifies, at first landing —
not bolted on afterward.

## Fix

### 1. New file: `src/io/TemplateIngest_IO.h`
```cpp
// TemplateIngest_IO.h — the one entry point: composes TemplateSourceScan_IO (86) ->
// LuaTableEvaluate_SYS (85, fanned out over Sys::ThreadPool) -> TemplateDialect_IO (87) ->
// TemplateIngestCache_IO (88), and populates STEP58's Io::WorldFootprintSizeTable as a SECOND
// producer over its existing placeholder seed (STEP58's own file is NOT edited by this ticket).
// Layer: IO. ARCH_18_02_IngestedDataDeterminism.md §18.2 governs every consumer of this file's
// output: PROC must NEVER see Io::TemplateIngestReport or call IngestTemplates — see "Explicit
// out-of-scope" below.
#pragma once
#include "AssetAtlasCache_IO.h"          // Io::SourceFingerprint — real shape this reuses verbatim
#include "TemplateDialect_IO.h"
#include "WorldFootprintSizeTable_IO.h"  // STEP58 — second-producer target, unedited
#include <string>
#include <unordered_map>
#include <vector>

namespace SanmapGen {
namespace Sys { class ThreadPool; }
namespace Io {

// Exactly the shape work_orders/STEP96_FootprintBakeAndStalenessCheck_IO.md §0 requires, folded in
// here at first landing.
struct TemplateFootprintRecord {
    float             baseFootprintWidth = 0.0f;
    float             baseFootprintDepth = 0.0f;
    SourceFingerprint sourceFingerprint;   // AssetAtlasCache_IO.h:38-48's real shape.
};

class TemplateIngestReport {
public:
    int  totalSourceFileCount           = 0;
    int  ingestedFootprintRecordCount   = 0;   // Unit/PropA/PropB records WITH a real footprint
    int  skippedMissingFootprintCount   = 0;   // Unit/PropA/PropB records missing footprint (anomaly)
    int  skippedProjectileOrMarkerCount = 0;   // recognized, skipped by design (no footprint field)
    int  skippedUnrecognizedCount       = 0;   // evaluation failed, or no known root table
    // Carried through from ticket 86's TemplateSourceScanReport (a file that was skipped before
    // evaluation ever ran, so it has no TemplateRecord and doesn't fit the counters above) — closes
    // a gap where an earlier draft of this ticket computed and tested these in ticket 86 but never
    // surfaced them here, silently dropping real diagnostic data on a cache-miss ingest.
    int  skippedOversizeFileCount       = 0;
    int  skippedUnreadableFileCount     = 0;
    bool bGamedataRootPresent           = false;
    bool bUnzippedEnvironmentTreePresent = false;
    bool bLoadedFromDiskCache           = false;
    TpIdCollisionReport tpIdCollisions;
    // Sibling to the footprint map (ARCH_18_03_CatalogDataOwnership.md §18.3's "a sibling table, not
    // a field bolted onto WorldFootprintSizeTable_IO.h" — this IS that sibling table, homed on this
    // ticket's own report type rather than a second standalone file; §18.3 delegates the exact
    // shape to the Format Expert, exercised here).
    std::unordered_map<std::string, std::vector<std::string>> tagsByTemplateIdentifier;
    std::unordered_map<std::string, TemplateFootprintRecord>  footprintByTemplateIdentifier;

    const TemplateFootprintRecord* FindByTemplateIdentifier(const std::string& templateIdentifier) const;
    const std::vector<std::string>* FindTagsByTemplateIdentifier(const std::string& templateIdentifier) const;
    // House warning shape (STEP73 §0 / STEP82's own precedent, reused verbatim): loud, aggregate,
    // non-blocking, one block naming every skip/collision, never auto-fixes.
    std::string SummaryText() const;
};

// gameInstallRoot empty => returns a default-constructed (empty) report immediately, zero
// filesystem calls (DESIGN doc §4.4's "gameInstallRoot empty: ingestion is not attempted" row).
// workerPool nullptr => the per-file evaluate+parse fan-out runs inline via this function's OWN
// explicit sequential loop (see the .cpp implementation contract, step 4) — a null Sys::ThreadPool*
// is not the same thing as a real ThreadPool instance with zero workers (ThreadPool_SYS.h's
// constructor remaps workerCount==0 to hardware_concurrency(), so that state is not reachable via
// normal construction), so this is NOT "reusing ThreadPool's own zero-worker contract" — it is this
// function's own null-check gate, mirroring the real `if (workerPool != nullptr) ... else ...`
// pattern AssetAtlasCache::BuildFromSanpack's decodeOne call site already uses
// (AssetAtlasCache_IO.cpp:60-83) for the identical situation.
TemplateIngestReport IngestTemplates(const std::string& gameInstallRoot, const std::string& cacheDirectory,
                                      Sys::ThreadPool* workerPool = nullptr);

// STEP58's WorldFootprintSizeTable::SetFootprint is ALREADY last-write-wins (unedited, unchanged) —
// this function relies on `report.footprintByTemplateIdentifier` ALREADY being collision-resolved
// (IngestTemplates's own first-write-wins fold, ticket 87's Q7 resolution, does that before
// returning), so insertion order into outTable does not matter here; every tpId appears exactly once.
void PopulateWorldFootprintSizeTable(const TemplateIngestReport& report, WorldFootprintSizeTable& outTable);

} // namespace Io
} // namespace SanmapGen
```

### 2. New file: `src/io/TemplateIngest_IO.cpp` — implementation contract
1. `gameInstallRoot.empty()` → return `TemplateIngestReport{}` immediately.
2. **Always** run `TemplateSourceScan_IO::ScanTemplateSources(gameInstallRoot)` (ticket 86) — this is
   the cheap part (~2 MB sequential read, per ticket 86's own cost basis), never skipped even on a
   cache hit, because computing the CURRENT fingerprint to compare against the cache requires it.
   Fold each `TemplateSourceFile`'s `{byteSize, modifiedTime, contentHash}` into a
   `TemplateIngestFingerprint` (sort files by `logicalPath` first, so the fold is independent of
   scan-internal ordering; `sourceFileCount = scan.files.size()`; `combinedContentHash` = an FNV-1a
   fold over the sorted per-file triples). Also copy `scan.skippedOversizeFileCount`/
   `skippedUnreadableFileCount` straight onto the report's own same-named fields — this happens on
   EVERY call (cache hit or miss), since the scan itself always runs.
3. `TemplateIngestCache_IO::LoadTemplateIngestCache(cacheDirectory, gameInstallRoot, fingerprint,
   cached)`. **On hit**: build the report directly from `cached.records` (step 5 below, skipping steps
   3b/4 entirely — no Lua evaluation runs this call), set `bLoadedFromDiskCache = true`.
4. **On miss**: `if (workerPool != nullptr) workerPool->ParallelFor(0, count, ...) else for (...) { ... }`
   — the real gating pattern `AssetAtlasCache::BuildFromSanpack`'s `decodeOne` call site already
   uses (`AssetAtlasCache_IO.cpp:60-83`), NOT `AssetAtlasCache::PackImages` (an earlier draft of
   this ticket cited `PackImages`/`placeholderFlags` here — wrong; `PackImages` only ever *reads*
   `placeholderFlags[index]` in a plain sequential loop, it never fans out or writes it). Each index
   `i` writes ONLY `outcomes[i]` — call
   `Sys::EvaluateLuaTableSource(scan.files[i].sourceText)` then
   `Io::ParseTemplateSource(evaluated, scan.files[i].logicalPath, static_cast<int>(scan.files[i].sourceRank),
   scan.files[i].byteSize, scan.files[i].modifiedTime, scan.files[i].contentHash)`, storing the
   resulting `TemplateParseOutcome` into `outcomes[i]`. This is the embarrassingly-parallel step —
   `ARCH_18_01`'s "fresh `lua_State` per file" constraint is exactly what makes it safe with zero
   synchronization.
5. **Sequential fold** (both the cache-hit and cache-miss paths converge here, operating on either
   `cached.records` or the `bRecognized` subset of `outcomes`): stable-sort the record list by
   `sourcePriorityRank` ascending (Q7's resolution, ticket 87). Walk in that order; for each record:
   - `dialectKind == Unrecognized` → `++skippedUnrecognizedCount`, skip.
   - `dialectKind == ProjectileTemplate || MarkerTemplate` → `++skippedProjectileOrMarkerCount`, skip
     (still counted, per DESIGN doc §4.4's "skipped silently by design, counted not warned").
   - `!bHasFootprint` (a Unit/PropA/PropB record missing its expected field) →
     `++skippedMissingFootprintCount`; STILL fold its `tags` into `tagsByTemplateIdentifier` if present
     (tags and footprint are independent signals — a record can usefully carry one without the other).
   - Otherwise: if `footprintByTemplateIdentifier` does NOT already contain `templateIdentifier`
     (first-write-wins, by virtue of the priority-sorted walk order), insert
     `TemplateFootprintRecord{baseFootprintWidth, baseFootprintDepth, SourceFingerprint{
     sourceLogicalPath, sourceByteSize, sourceModifiedTime, sourceContentHash}}`,
     `++ingestedFootprintRecordCount`; ALWAYS fold `tags` into `tagsByTemplateIdentifier` under the
     same first-write-wins rule (tags follow the same winning source as footprint, for the same tpId).
   - `totalSourceFileCount = <the full list size>`.
6. `Io::DetectTpIdCollisions` over the FULL (pre-dedup) record list → `report.tpIdCollisions`.
7. **On a cache miss only**: `TemplateIngestCache_IO::SaveTemplateIngestCache(cacheDirectory,
   gameInstallRoot, CachedTemplateIngest{fingerprint, <the full bRecognized record list>})`.
8. `PopulateWorldFootprintSizeTable`: iterate `report.footprintByTemplateIdentifier`, call
   `outTable.SetFootprint(tpId, record.baseFootprintWidth, record.baseFootprintDepth)` for each —
   order is irrelevant, every tpId appears exactly once already.
9. `SummaryText()` — house shape, e.g.:
   ```
   Ingested 481 template footprint(s), 512 tags record(s), from 546 source file(s)
   (loaded from disk cache). 62 projectile(s) and 6 marker(s) skipped by design. 3 file(s) had no
   recognized root table (see log). 1 tpId collision found:
     "Cliff_02": Environment/Pandemonium/Props/Cliff_02/Cliff_02.santp,
                 Environment/Pandemonium/Props/Cliff_03/Cliff_03.sanprop (first source wins)
   ```

## Files touched
- NEW `src/io/TemplateIngest_IO.h`
- NEW `src/io/TemplateIngest_IO.cpp`
- NEW `src/io/TemplateIngest_IO_Test.cpp`
- `CMakeLists.txt` — one new `add_sangen_test(TemplateIngest_IO_Test src/io/TemplateIngest_IO_Test.cpp)`
  — this test needs a real `Sys::ThreadPool` linked (already part of `SanGenV2`, no extra link line)
  and, for its cold-ingest test cases, real LuaJIT execution via ticket 85 — no extra CMake wiring
  needed since `SanGenV2`'s existing `PRIVATE luajit` link already propagates transitively to any
  test executable that links `SanGenV2` (the same `miniz.c` precedent noted in STEP65).

## Backend policy
CPU only. The Lua-evaluation fan-out is the one genuinely parallel step, dispatched via
`Sys::ThreadPool::ParallelFor` (real, shipped, already blocking/deterministic-partition by design —
"partition depends only on range + worker count... safe for the deterministic path," confirmed by
reading `ThreadPool_SYS.h` this session). Cost basis (Constitution §7, direct measurement,
`DESIGN_SantpFootprintIngestion_R1.md` §4.2): 546 files × ~4 KB average — an order of magnitude
smaller than `AssetAtlasCache`'s existing ~37 MB/few-hundred-file budget on the same pool; a cache hit
is a single manifest read of a few tens of KB. No microbenchmark warranted at this scale.

## Layer & accuracy class
IO. Accuracy class: **Visual** — unchanged from STEP58's own assignment
(`ARCH_18_02_IngestedDataDeterminism.md` §18.2 rule 5: "`Io::WorldFootprintSizeTable` stays
Visual-class exactly as STEP58 ships it... its only DIRECT consumer remains the preview's icon-LOD
sizing"). This ticket's output is never Exact/Accurate-class and never will be by itself — only a
future human-triggered PARAMS bake (STEP96, already drafted) may convert a single resolved value into
recipe data.

## ARCH rules invoked
- `ARCH_18_02_IngestedDataDeterminism.md` §18.2, ALL FIVE rules — this is THE ticket §18.2 is most
  binding on: "Ticket 89 must not be dispatched with a design that wires
  `Io::WorldFootprintSizeTable`/`TemplateIngest_IO` directly into any `PROC`/scatter code path — this
  ruling is binding on that ticket's shape." Confirmed: this ticket touches zero files under
  `src/proc/`.
- `ARCH_18_03_CatalogDataOwnership.md` §18.3 — footprint stays exactly where STEP58 put it (IO,
  second-producer, no shape change); the tags sibling table is homed on this ticket's own
  `TemplateIngestReport`, per §18.3's explicit delegation.
- `STEP96_FootprintBakeAndStalenessCheck_IO.md` §0 — the `TemplateFootprintRecord`/
  `FindByTemplateIdentifier` addition, folded in verbatim at first landing rather than retrofitted.
- Constitution §1 layering — IO may depend on SYS (ticket 85's evaluator) and on IO (86/87/88); this
  ticket introduces no new dependency direction.
- Constitution §6 — the whole failure-path table (`DESIGN_SantpFootprintIngestion_R1.md` §4.4) is
  honored: empty root, missing Gamedata, one file's evaluation failure, JSON-not-Lua content,
  Projectile/Marker skip, missing footprint, tpId collision, corrupt cache — none of these ever
  aborts the batch or prevents SanGen from launching/generating/exporting.

## Explicit out-of-scope
- **Any `src/proc/` file reading `Io::TemplateIngestReport`/`Io::WorldFootprintSizeTable` or calling
  `IngestTemplates`.** `ARCH_18_02` forbids this outright; enforced by this ticket touching zero PROC
  files and by the Verify step's grep below.
- **The actual PARAMS bake action** (baking a resolved footprint into `Params::PropRule`/`UnitRule`,
  the "Resolve Footprint" button) — `STEP96_FootprintBakeAndStalenessCheck_IO.md`, already drafted,
  now unblocked by this ticket landing. This ticket does not touch any `src/params/` or `src/ui/` file.
- **`bReclaimable` population** — ticket 92, a separate consumer of this same `TemplateIngestReport`.
- **UI wiring / calling `IngestTemplates` from the shell** — ticket 91.
- **Durable "last ingest" state** — ticket 90; this ticket is a pure, stateless-between-calls function,
  no persistence beyond the disk cache itself.

## Acceptance test
New `src/io/TemplateIngest_IO_Test.cpp` (registered in `CMakeLists.txt`), against real scratch
install layouts (small, hand-built fixtures — a handful of real-shaped `.santp` files, not the live
Steam install):
1. `IngestTemplates("", cacheDir)` returns a default-constructed report, zero filesystem/Lua work.
2. A scratch install with 3 real `UnitTemplate` files (one with `general.tpId`, one without — proving
   the filename-stem fallback fires end-to-end), 2 `propTemplate`/Dialect-A files, 1 `PropTemplate`/
   Dialect-B file, 1 `ProjectileTemplate`, 1 `MarkerTemplate`, 1 `.sanprop` file that is actually JSON
   → `IngestTemplates` (cold, `bLoadedFromDiskCache == false`) yields
   `ingestedFootprintRecordCount == 6`, `skippedProjectileOrMarkerCount == 2`,
   `skippedUnrecognizedCount == 1`, `totalSourceFileCount == 9`, and `FindByTemplateIdentifier` for
   each of the 6 returns the exact seeded footprint values plus a `sourceFingerprint` matching that
   file's real `{path, size, mtime}`.
3. **Cache hit.** A second `IngestTemplates` call against the SAME scratch install (nothing changed)
   returns `bLoadedFromDiskCache == true` and byte-identical `footprintByTemplateIdentifier`/
   `tagsByTemplateIdentifier` contents to run 2 — proves the cache round-trips through
   `TemplateIngest_IO`'s own integration, not just `TemplateIngestCache_IO`'s isolated unit test.
4. **Cache invalidation.** Modifying one source file's content between two calls produces
   `bLoadedFromDiskCache == false` on the second call, and the changed value is reflected.
5. **tpId collision**, end-to-end: two fixture files (one in a `LooseFile`-ranked location, one in a
   `CompressedSanpack`-ranked synthetic sanpack) both declaring the same tpId with DIFFERENT footprint
   values → `report.tpIdCollisions.AnyCollisions() == true` naming both paths, AND
   `FindByTemplateIdentifier` for that tpId returns the LOOSE-FILE value (proves the priority-sorted
   first-write-wins fold, not an arbitrary/last-processed one).
6. `PopulateWorldFootprintSizeTable`: seed a `WorldFootprintSizeTable` with
   `BuildPlaceholderWorldFootprintSizeTable()` (STEP58, real) first, then populate from a report
   containing `"uca1001"` with a DIFFERENT footprint than the placeholder's own hand-seeded value →
   `table.Resolve("uca1001")` returns the INGESTED value, not the placeholder — proves real data wins
   over the placeholder via STEP58's own unedited last-write-wins `SetFootprint`, with zero policy
   code added to STEP58's file (grep it for zero edits, listed in Verify below).
7. **Threaded vs. inline equivalence.** The SAME scratch install ingested once with a real
   `Sys::ThreadPool(4)` and once with `workerPool == nullptr` (inline) produces byte-identical
   reports — proves the fan-out has no data race and no order-dependence.
8. `SummaryText()` on a report with a known skip/collision mix contains every named entity (mirrors
   STEP82 acceptance test 9's "one aggregate warning, not one per offender" — not multiple separate
   messages).
9. Full solo rebuild + `ctest -C Debug`: previously-passing suite (including `WorldFootprintSizeTable_IO_Test`,
   unedited) stays green; the new target passes.

## Verify
- New `src/io/TemplateIngest_IO_Test.cpp` passes, especially tests 3/4 (cache round-trip) and 5/6
  (collision + placeholder-overlay).
- Grep `src/io/WorldFootprintSizeTable_IO.h` — confirm it is byte-for-byte unedited by this ticket
  (STEP58's own file, per this ticket's whole "second producer, not a rewrite" design).
- Grep `src/proc/` for any new reference to `TemplateIngestReport`/`TemplateIngest_IO`/
  `WorldFootprintSizeTable` introduced by this ticket — must be zero (`ARCH_18_02` rule 2).
- Full solo rebuild + `ctest -C Debug`: 100% pass, zero unrelated test files edited or broken.
