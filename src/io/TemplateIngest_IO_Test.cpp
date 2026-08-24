// TemplateIngest_IO_Test.cpp — acceptance test for TemplateIngest_IO (STEP89), the integration
// point over tickets 85-88. Drives real scratch install layouts under the platform temp directory
// (never a real game install) with hand-built .santp/.sanprop fixtures covering all five real
// dialects, exercised through real LuaJIT evaluation (Sys::EvaluateLuaTableSource, ticket 85) --
// mirrors TemplateSourceScan_IO_Test.cpp / TemplateIngestCache_IO_Test.cpp's own scratch-folder and
// fixture-building conventions.
#include "TemplateIngest_IO.h"
#include "FilesystemPrimitives_IO.h"
#include "WorldFootprintSizeTable_IO.h"
#include "../sys/ThreadPool_SYS.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <miniz.h>
#include <utility>
#include <vector>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

bool NearlyEqual(float left, float right) { return std::fabs(left - right) <= 1.0e-4f; }

std::string ScratchFolderPath(const char* name) {
    std::error_code pathError;
    const std::filesystem::path folder = std::filesystem::temp_directory_path(pathError) / name;
    std::filesystem::remove_all(folder, pathError);
    return folder.string();
}

void WriteTextFile(const std::string& filePath, const std::string& content) {
    std::error_code makeDirectoryError;
    std::filesystem::create_directories(std::filesystem::path(filePath).parent_path(), makeDirectoryError);
    std::ofstream outputStream(filePath, std::ios::binary | std::ios::trunc);
    outputStream << content;
}

bool WriteSyntheticZip(const std::string& zipPath,
                       const std::vector<std::pair<std::string, std::string>>& entries) {
    std::remove(zipPath.c_str());
    std::error_code makeDirectoryError;
    std::filesystem::create_directories(std::filesystem::path(zipPath).parent_path(), makeDirectoryError);
    mz_zip_archive archive;
    std::memset(&archive, 0, sizeof(archive));
    if (!mz_zip_writer_init_file(&archive, zipPath.c_str(), 0)) return false;
    bool bWritten = true;
    for (const std::pair<std::string, std::string>& entry : entries)
        bWritten = bWritten && mz_zip_writer_add_mem(&archive, entry.first.c_str(), entry.second.data(),
                                                      entry.second.size(), MZ_BEST_COMPRESSION);
    bWritten = bWritten && mz_zip_writer_finalize_archive(&archive);
    mz_zip_writer_end(&archive);
    return bWritten;
}

bool FootprintMapsEqual(const std::unordered_map<std::string, Io::TemplateFootprintRecord>& left,
                         const std::unordered_map<std::string, Io::TemplateFootprintRecord>& right) {
    if (left.size() != right.size()) return false;
    for (const auto& entry : left) {
        const auto found = right.find(entry.first);
        if (found == right.end()) return false;
        const Io::TemplateFootprintRecord& a = entry.second;
        const Io::TemplateFootprintRecord& b = found->second;
        if (!NearlyEqual(a.baseFootprintWidth, b.baseFootprintWidth)) return false;
        if (!NearlyEqual(a.baseFootprintDepth, b.baseFootprintDepth)) return false;
        if (a.sourceFingerprint.sourcePath != b.sourceFingerprint.sourcePath) return false;
        if (a.sourceFingerprint.byteSize != b.sourceFingerprint.byteSize) return false;
        if (a.sourceFingerprint.modifiedTime != b.sourceFingerprint.modifiedTime) return false;
        if (a.sourceFingerprint.contentHash != b.sourceFingerprint.contentHash) return false;
    }
    return true;
}

bool TagsMapsEqual(const std::unordered_map<std::string, std::vector<std::string>>& left,
                    const std::unordered_map<std::string, std::vector<std::string>>& right) {
    if (left.size() != right.size()) return false;
    for (const auto& entry : left) {
        const auto found = right.find(entry.first);
        if (found == right.end() || found->second != entry.second) return false;
    }
    return true;
}

// Writes the real 9-fixture install this ticket's own scratch-install scenario (§ acceptance test 2)
// names: 3 UnitTemplate (one general.tpId, one path-stem fallback), 2 propTemplate (Dialect A),
// 1 PropTemplate (Dialect B), 1 ProjectileTemplate, 1 MarkerTemplate, 1 .sanprop that is actually
// JSON.
void WriteNineFixtureInstall(const std::string& root) {
    const std::string luaCommon = Io::JoinExportPath(root, "engine/LJ/lua/common");
    WriteTextFile(Io::JoinExportPath(luaCommon, "units/unitsTemplates/unit1.santp"),
        R"(UnitTemplate = { general = { tpId = "uca1001" }, footprint = { x = 1.2, y = 1.2 } })");
    WriteTextFile(Io::JoinExportPath(luaCommon, "units/unitsTemplates/uca2002.santp"),
        R"(UnitTemplate = { footprint = { x = 2.0, y = 2.0 } })");   // no general -> filename-stem fallback
    WriteTextFile(Io::JoinExportPath(luaCommon, "units/unitsTemplates/unit3.santp"),
        R"(UnitTemplate = { general = { tpId = "uca3003" }, footprint = { x = 3.0, y = 3.0 },
                             tags = { "infantry" } })");
    WriteTextFile(Io::JoinExportPath(luaCommon, "props/propsTemplates/propA1.sanprop"),
        R"(propTemplate = { general = { tpId = "epx1001" }, footprint = { x = 1.0, y = 1.0 } })");
    WriteTextFile(Io::JoinExportPath(luaCommon, "props/propsTemplates/propA2.sanprop"),
        R"(propTemplate = { general = { tpId = "epx1002" }, footprint = { x = 1.5, y = 1.5 } })");
    WriteTextFile(Io::JoinExportPath(luaCommon, "props/propsTemplates/propB1.sanprop"),
        R"(PropTemplate = { general = { tpId = "epx2001" }, footprint = { x = 2.5, y = 2.5 } })");
    WriteTextFile(Io::JoinExportPath(luaCommon, "projectiles/projectilesTemplates/projectile1.santp"),
        R"(ProjectileTemplate = { general = { tpId = "proj001" } })");
    WriteTextFile(Io::JoinExportPath(luaCommon, "markers/markerTemplates/marker1.sanprop"),
        R"(MarkerTemplate = { general = { tpId = "spawn001" } })");
    WriteTextFile(Io::JoinExportPath(luaCommon, "props/propsTemplates/fakejson.sanprop"),
        R"({ "tpId": "epx9999", "footprint": { "x": 1.0, "y": 1.0 } })");
}

void CheckNineFixtureCounts(const Io::TemplateIngestReport& report, const char* label) {
    char message[256];
    std::snprintf(message, sizeof(message), "%s: totalSourceFileCount == 9", label);
    Check(report.totalSourceFileCount == 9, message);
    std::snprintf(message, sizeof(message), "%s: ingestedFootprintRecordCount == 6", label);
    Check(report.ingestedFootprintRecordCount == 6, message);
    std::snprintf(message, sizeof(message), "%s: skippedProjectileOrMarkerCount == 2", label);
    Check(report.skippedProjectileOrMarkerCount == 2, message);
    std::snprintf(message, sizeof(message), "%s: skippedUnrecognizedCount == 1", label);
    Check(report.skippedUnrecognizedCount == 1, message);
}

void TestEmptyRootIsANoOp() {
    const Io::TemplateIngestReport report = Io::IngestTemplates("", "C:/nowhere");
    Check(report.totalSourceFileCount == 0, "empty-root: totalSourceFileCount == 0");
    Check(report.ingestedFootprintRecordCount == 0, "empty-root: ingestedFootprintRecordCount == 0");
    Check(!report.bLoadedFromDiskCache, "empty-root: bLoadedFromDiskCache false");
    Check(report.footprintByTemplateIdentifier.empty(), "empty-root: footprint map empty");
}

// Acceptance tests 2 + 3: cold ingest with exact category counts, then a cache-hit round-trip
// byte-identical to the cold run.
void TestColdIngestThenCacheHitRoundTrip() {
    const std::string root = ScratchFolderPath("SanGenTemplateIngestTest_NineFiles");
    const std::string cacheDirectory = ScratchFolderPath("SanGenTemplateIngestTest_NineFiles_Cache");
    WriteNineFixtureInstall(root);

    const Io::TemplateIngestReport cold = Io::IngestTemplates(root, cacheDirectory);
    Check(!cold.bLoadedFromDiskCache, "cold-ingest: bLoadedFromDiskCache false");
    CheckNineFixtureCounts(cold, "cold-ingest");

    const std::pair<const char*, std::pair<float, float>> expected[6] = {
        {"uca1001", {1.2f, 1.2f}}, {"uca2002", {2.0f, 2.0f}}, {"uca3003", {3.0f, 3.0f}},
        {"epx1001", {1.0f, 1.0f}}, {"epx1002", {1.5f, 1.5f}}, {"epx2001", {2.5f, 2.5f}},
    };
    for (const auto& entry : expected) {
        const Io::TemplateFootprintRecord* found = cold.FindByTemplateIdentifier(entry.first);
        char message[128];
        std::snprintf(message, sizeof(message), "cold-ingest: %s found", entry.first);
        Check(found != nullptr, message);
        if (found == nullptr) continue;
        Check(NearlyEqual(found->baseFootprintWidth, entry.second.first) &&
              NearlyEqual(found->baseFootprintDepth, entry.second.second), "cold-ingest: footprint value");
        Check(found->sourceFingerprint.byteSize > 0, "cold-ingest: sourceFingerprint byteSize > 0");
        Check(!found->sourceFingerprint.sourcePath.empty(), "cold-ingest: sourceFingerprint sourcePath set");
    }
    Check(cold.FindTagsByTemplateIdentifier("uca3003") != nullptr &&
          *cold.FindTagsByTemplateIdentifier("uca3003") == std::vector<std::string>{"infantry"},
          "cold-ingest: uca3003 tags == [infantry]");

    const Io::TemplateIngestReport hit = Io::IngestTemplates(root, cacheDirectory);
    Check(hit.bLoadedFromDiskCache, "cache-hit: bLoadedFromDiskCache true");
    CheckNineFixtureCounts(hit, "cache-hit");
    Check(FootprintMapsEqual(cold.footprintByTemplateIdentifier, hit.footprintByTemplateIdentifier),
          "cache-hit: footprintByTemplateIdentifier byte-identical to the cold run");
    Check(TagsMapsEqual(cold.tagsByTemplateIdentifier, hit.tagsByTemplateIdentifier),
          "cache-hit: tagsByTemplateIdentifier byte-identical to the cold run");
}

// Acceptance test 4: modifying one source file's content between two calls invalidates the cache.
void TestCacheInvalidatesOnContentChange() {
    const std::string root = ScratchFolderPath("SanGenTemplateIngestTest_Invalidation");
    const std::string cacheDirectory = ScratchFolderPath("SanGenTemplateIngestTest_Invalidation_Cache");
    const std::string filePath =
        Io::JoinExportPath(root, "engine/LJ/lua/common/units/unitsTemplates/unit1.santp");
    WriteTextFile(filePath, R"(UnitTemplate = { general = { tpId = "uca1001" }, footprint = { x = 1.2, y = 1.2 } })");

    const Io::TemplateIngestReport first = Io::IngestTemplates(root, cacheDirectory);
    Check(!first.bLoadedFromDiskCache, "invalidation: first call is a cold ingest");
    const Io::TemplateIngestReport second = Io::IngestTemplates(root, cacheDirectory);
    Check(second.bLoadedFromDiskCache, "invalidation: second call (unchanged) hits the cache");

    WriteTextFile(filePath, R"(UnitTemplate = { general = { tpId = "uca1001" }, footprint = { x = 9.9, y = 9.9 } })");
    const Io::TemplateIngestReport third = Io::IngestTemplates(root, cacheDirectory);
    Check(!third.bLoadedFromDiskCache, "invalidation: third call (content changed) misses the cache");
    const Io::TemplateFootprintRecord* found = third.FindByTemplateIdentifier("uca1001");
    Check(found != nullptr && NearlyEqual(found->baseFootprintWidth, 9.9f) &&
          NearlyEqual(found->baseFootprintDepth, 9.9f), "invalidation: changed footprint value reflected");
}

// Acceptance test 5: a loose-file source and a compressed-sanpack source both declare the same
// tpId with different footprints -- the priority-sorted first-write-wins fold must pick the
// LOOSE-FILE value, not whichever the fan-out happened to finish last.
void TestTpIdCollisionResolvesToHighestPriority() {
    const std::string root = ScratchFolderPath("SanGenTemplateIngestTest_Collision");
    const std::string cacheDirectory = ScratchFolderPath("SanGenTemplateIngestTest_Collision_Cache");
    WriteTextFile(Io::JoinExportPath(root, "engine/LJ/lua/common/units/unitsTemplates/dup.santp"),
        R"(UnitTemplate = { general = { tpId = "dupId001" }, footprint = { x = 5.0, y = 5.0 } })");

    const std::string gamedataRoot = Io::JoinExportPath(root, "engine/Sanctuary_Data/Gamedata");
    std::error_code makeDirectoryError;
    std::filesystem::create_directories(gamedataRoot, makeDirectoryError);
    const std::string sanpackPath = Io::JoinExportPath(gamedataRoot, "Environment.sanpack");
    const std::string sanpackSource =
        R"(propTemplate = { general = { tpId = "dupId001" }, footprint = { x = 9.0, y = 9.0 } })";
    Check(WriteSyntheticZip(sanpackPath, {{"Environment/Props/dup.sanprop", sanpackSource}}),
          "collision: synthetic Environment.sanpack written");

    const Io::TemplateIngestReport report = Io::IngestTemplates(root, cacheDirectory);
    Check(report.tpIdCollisions.AnyCollisions(), "collision: AnyCollisions true");
    bool bFoundCollisionGroup = false;
    for (const Io::TpIdCollision& collision : report.tpIdCollisions.collisions)
        if (collision.templateIdentifier == "dupId001" && collision.conflictingSourcePaths.size() == 2)
            bFoundCollisionGroup = true;
    Check(bFoundCollisionGroup, "collision: dupId001 group names both sources");

    const Io::TemplateFootprintRecord* found = report.FindByTemplateIdentifier("dupId001");
    Check(found != nullptr && NearlyEqual(found->baseFootprintWidth, 5.0f) &&
          NearlyEqual(found->baseFootprintDepth, 5.0f),
          "collision: FindByTemplateIdentifier returns the LOOSE-FILE value (priority-sorted first-write-wins)");
}

// Acceptance test 6: real ingested data overrides STEP58's own hand-seeded placeholder value via
// SetFootprint's already-documented last-write-wins policy -- zero new policy code.
void TestPopulateWorldFootprintSizeTableOverridesPlaceholder() {
    Io::WorldFootprintSizeTable table = Io::BuildPlaceholderWorldFootprintSizeTable();
    Check(NearlyEqual(table.Resolve("uca1001").baseFootprintWidth, 1.2f), "placeholder: uca1001 starts at 1.2");

    Io::TemplateIngestReport report;
    Io::TemplateFootprintRecord record;
    record.baseFootprintWidth = 7.0f;
    record.baseFootprintDepth = 7.0f;
    report.footprintByTemplateIdentifier["uca1001"] = record;

    Io::PopulateWorldFootprintSizeTable(report, table);
    const Io::WorldFootprintSize_IO resolved = table.Resolve("uca1001");
    Check(NearlyEqual(resolved.baseFootprintWidth, 7.0f) && NearlyEqual(resolved.baseFootprintDepth, 7.0f),
          "populate: ingested value (7.0) wins over the placeholder (1.2)");
}

// Acceptance test 7: the SAME install ingested once through a real 4-worker ThreadPool and once
// inline (workerPool == nullptr) -- two FRESH cache directories so both runs actually evaluate,
// proving the fan-out itself, not a cache short-circuit -- must produce byte-identical reports.
void TestThreadedAndInlineFanOutAreByteIdentical() {
    const std::string root = ScratchFolderPath("SanGenTemplateIngestTest_ThreadedVsInline");
    WriteNineFixtureInstall(root);
    const std::string cacheDirectoryThreaded = ScratchFolderPath("SanGenTemplateIngestTest_ThreadedVsInline_A");
    const std::string cacheDirectoryInline = ScratchFolderPath("SanGenTemplateIngestTest_ThreadedVsInline_B");

    Sys::ThreadPool pool(4);
    const Io::TemplateIngestReport threaded = Io::IngestTemplates(root, cacheDirectoryThreaded, &pool);
    const Io::TemplateIngestReport inline_ = Io::IngestTemplates(root, cacheDirectoryInline, nullptr);

    Check(threaded.totalSourceFileCount == inline_.totalSourceFileCount, "fan-out: totalSourceFileCount matches");
    Check(threaded.ingestedFootprintRecordCount == inline_.ingestedFootprintRecordCount,
          "fan-out: ingestedFootprintRecordCount matches");
    Check(threaded.skippedProjectileOrMarkerCount == inline_.skippedProjectileOrMarkerCount,
          "fan-out: skippedProjectileOrMarkerCount matches");
    Check(threaded.skippedUnrecognizedCount == inline_.skippedUnrecognizedCount,
          "fan-out: skippedUnrecognizedCount matches");
    Check(FootprintMapsEqual(threaded.footprintByTemplateIdentifier, inline_.footprintByTemplateIdentifier),
          "fan-out: footprintByTemplateIdentifier byte-identical between threaded and inline");
    Check(TagsMapsEqual(threaded.tagsByTemplateIdentifier, inline_.tagsByTemplateIdentifier),
          "fan-out: tagsByTemplateIdentifier byte-identical between threaded and inline");
}

// Acceptance test 8: one aggregate SummaryText block names every skip/collision category, mirroring
// STEP82 acceptance test 9's "one aggregate warning, not one per offender."
void TestSummaryTextNamesEveryCategory() {
    Io::TemplateIngestReport report;
    report.totalSourceFileCount = 10;
    report.ingestedFootprintRecordCount = 4;
    report.skippedMissingFootprintCount = 1;
    report.skippedProjectileOrMarkerCount = 2;
    report.skippedUnrecognizedCount = 1;
    report.skippedOversizeFileCount = 1;
    report.skippedUnreadableFileCount = 1;
    report.tagsByTemplateIdentifier["uca1001"] = { "infantry" };
    Io::TpIdCollision collision;
    collision.templateIdentifier = "Cliff_02";
    collision.conflictingSourcePaths = { "Props/Cliff_02/Cliff_02.santp", "Props/Cliff_03/Cliff_03.sanprop" };
    report.tpIdCollisions.collisions.push_back(collision);

    const std::string summary = report.SummaryText();
    Check(summary.find("4 template footprint") != std::string::npos, "summary: ingested count");
    Check(summary.find("2 projectile/marker") != std::string::npos, "summary: projectile/marker count");
    Check(summary.find("1 file(s) had no recognized root table") != std::string::npos, "summary: unrecognized count");
    Check(summary.find("1 record(s) recognized but missing a footprint") != std::string::npos, "summary: missing-footprint count");
    Check(summary.find("1 oversize file") != std::string::npos, "summary: oversize count");
    Check(summary.find("1 unreadable file") != std::string::npos, "summary: unreadable count");
    Check(summary.find("Cliff_02") != std::string::npos, "summary: collision tpId named");
    Check(summary.find("Cliff_02/Cliff_02.santp") != std::string::npos, "summary: first collision path named");
    Check(summary.find("Cliff_03/Cliff_03.sanprop") != std::string::npos, "summary: second collision path named");
}

} // namespace

int main() {
    TestEmptyRootIsANoOp();
    TestColdIngestThenCacheHitRoundTrip();
    TestCacheInvalidatesOnContentChange();
    TestTpIdCollisionResolvesToHighestPriority();
    TestPopulateWorldFootprintSizeTableOverridesPlaceholder();
    TestThreadedAndInlineFanOutAreByteIdentical();
    TestSummaryTextNamesEveryCategory();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
