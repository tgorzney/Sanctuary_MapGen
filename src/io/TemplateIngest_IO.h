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
