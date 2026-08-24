// TemplateIngest_IO.cpp — orchestration half: fingerprint the scan, check/save the disk cache, and
// the ThreadPool-or-inline evaluate+parse fan-out. The sequential record-fold half (priority-sorted
// classification into the report's counters/maps) is the ARCH §1.5 aspect split
// TemplateIngest_Fold_IO.cpp -- kept separate so this file stays under the hard 150-line ceiling,
// mirroring TemplateIngestCache_IO.cpp / TemplateIngestCache_Record_IO.cpp's own split (ticket 88).
#include "TemplateIngest_IO.h"
#include "TemplateIngest_Fold_IO.h"
#include "TemplateIngestCache_IO.h"
#include "TemplateSourceScan_IO.h"
#include "../sys/LuaTableEvaluate_SYS.h"
#include "../sys/ThreadPool_SYS.h"
#include <algorithm>

namespace SanmapGen {
namespace Io {
namespace {

// The same five-line FNV-1a fold every sibling ticket in this chain already re-implements locally
// rather than exporting cross-file (TemplateSourceScan_IO.cpp's own HashBytes precedent).
std::uint64_t FoldValue(std::uint64_t digest, std::uint64_t value) {
    for (int byteIndex = 0; byteIndex < 8; ++byteIndex) {
        digest ^= (value >> (byteIndex * 8)) & 0xFFull;
        digest *= 1099511628211ull;
    }
    return digest;
}

// Sorts by logicalPath FIRST so the fold is independent of scan-internal (and, downstream, of
// ThreadPool completion) ordering — ticket 88's own documented requirement on this fingerprint.
TemplateIngestFingerprint ComputeFingerprint(const TemplateSourceScanReport& scan) {
    std::vector<const TemplateSourceFile*> sortedFiles;
    sortedFiles.reserve(scan.files.size());
    for (const TemplateSourceFile& file : scan.files) sortedFiles.push_back(&file);
    std::sort(sortedFiles.begin(), sortedFiles.end(),
              [](const TemplateSourceFile* left, const TemplateSourceFile* right) {
                  return left->logicalPath < right->logicalPath;
              });

    TemplateIngestFingerprint fingerprint;
    fingerprint.sourceFileCount = static_cast<int>(scan.files.size());
    std::uint64_t digest = 14695981039346656037ull;
    for (const TemplateSourceFile* file : sortedFiles) {
        digest = FoldValue(digest, file->byteSize);
        digest = FoldValue(digest, file->modifiedTime);
        digest = FoldValue(digest, file->contentHash);
    }
    fingerprint.combinedContentHash = digest;
    return fingerprint;
}

// The embarrassingly-parallel step: each index writes ONLY outcomes[index], so the ThreadPool
// fan-out needs no synchronization -- safe because Sys::EvaluateLuaTableSource gets a fresh
// lua_State per call (ARCH_18_01's own contract). Mirrors AssetAtlasCache::BuildFromSanpack's real
// decodeOne call-site gating pattern (AssetAtlasCache_IO.cpp:60-83), NOT PackImages.
std::vector<TemplateRecord> EvaluateAndParseAllSources(const TemplateSourceScanReport& scan,
                                                        Sys::ThreadPool* workerPool) {
    const int fileCount = static_cast<int>(scan.files.size());
    std::vector<TemplateParseOutcome> outcomes(static_cast<std::size_t>(fileCount));
    const auto evaluateOne = [&](int index) {
        const TemplateSourceFile& file = scan.files[static_cast<std::size_t>(index)];
        const Sys::LuaTableEvaluateResult evaluated = Sys::EvaluateLuaTableSource(file.sourceText);
        outcomes[static_cast<std::size_t>(index)] =
            ParseTemplateSource(evaluated, file.logicalPath, static_cast<int>(file.sourceRank),
                                 file.byteSize, file.modifiedTime, file.contentHash);
    };
    if (workerPool != nullptr) workerPool->ParallelFor(0, fileCount, evaluateOne);
    else for (int index = 0; index < fileCount; ++index) evaluateOne(index);

    std::vector<TemplateRecord> recognizedRecords;
    recognizedRecords.reserve(outcomes.size());
    for (const TemplateParseOutcome& outcome : outcomes)
        if (outcome.bRecognized) recognizedRecords.push_back(outcome.record);
    return recognizedRecords;
}

} // namespace

TemplateIngestReport IngestTemplates(const std::string& gameInstallRoot, const std::string& cacheDirectory,
                                      Sys::ThreadPool* workerPool) {
    TemplateIngestReport report;
    if (gameInstallRoot.empty()) return report;   // step 1: zero filesystem/Lua work

    // Step 2: ALWAYS scan -- computing the current fingerprint to compare against the cache needs
    // it, even on a hit.
    const TemplateSourceScanReport scan = ScanTemplateSources(gameInstallRoot);
    report.totalSourceFileCount = static_cast<int>(scan.files.size());
    report.bGamedataRootPresent = scan.bGamedataRootPresent;
    report.bUnzippedEnvironmentTreePresent = scan.bUnzippedEnvironmentTreePresent;
    report.skippedOversizeFileCount = scan.skippedOversizeFileCount;
    report.skippedUnreadableFileCount = scan.skippedUnreadableFileCount;
    const TemplateIngestFingerprint fingerprint = ComputeFingerprint(scan);

    // Step 3: disk cache check.
    CachedTemplateIngest cached;
    const bool bCacheHit = LoadTemplateIngestCache(cacheDirectory, gameInstallRoot, fingerprint, cached);
    report.bLoadedFromDiskCache = bCacheHit;

    // Step 4 (miss only) / step 3 (hit): either path converges on one bRecognized record list.
    std::vector<TemplateRecord> recognizedRecords =
        bCacheHit ? cached.records : EvaluateAndParseAllSources(scan, workerPool);

    // skippedUnrecognizedCount is derivable on EITHER path from the same two already-known
    // quantities (total scanned vs. bRecognized count) -- ticket 88's cache stores only the
    // bRecognized subset, so a cache hit has no raw per-file outcome list to re-count from directly.
    report.skippedUnrecognizedCount =
        report.totalSourceFileCount - static_cast<int>(recognizedRecords.size());

    // Step 5: sequential fold (both paths converge here) -- the ARCH §1.5 aspect split.
    FoldRecordsIntoReport(recognizedRecords, report);

    // Step 6: over the FULL pre-dedup record list.
    report.tpIdCollisions = DetectTpIdCollisions(recognizedRecords);

    // Step 7: save on a miss only.
    if (!bCacheHit) {
        CachedTemplateIngest toSave;
        toSave.fingerprint = fingerprint;
        toSave.records = recognizedRecords;
        SaveTemplateIngestCache(cacheDirectory, gameInstallRoot, toSave);
    }

    return report;
}

// Step 8: relies entirely on STEP58's own already-existing last-write-wins SetFootprint.
void PopulateWorldFootprintSizeTable(const TemplateIngestReport& report, WorldFootprintSizeTable& outTable) {
    for (const auto& entry : report.footprintByTemplateIdentifier)
        outTable.SetFootprint(entry.first, entry.second.baseFootprintWidth, entry.second.baseFootprintDepth);
}

} // namespace Io
} // namespace SanmapGen
