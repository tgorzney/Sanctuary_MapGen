// TemplateIngestCache_IO_Test.cpp — acceptance test for TemplateIngestCache_IO (STEP88). Every
// scenario the ticket names: full round-trip, mismatched fingerprint, format-version-checked-
// before-fingerprint, missing/malformed/partial-record cache files, and path-digest keying
// distinguishing two different gameInstallRoot values that share a folder-name suffix.
#include "TemplateIngestCache_IO.h"
#include "FilesystemPrimitives_IO.h"
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
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
    std::filesystem::create_directories(folder, pathError);
    return folder.string();
}

std::vector<Io::TemplateRecord> MakeThreeMixedDialectRecords() {
    std::vector<Io::TemplateRecord> records;

    Io::TemplateRecord unit;
    unit.dialectKind = Io::TemplateDialectKind::UnitTemplate;
    unit.templateIdentifier = "edbm0149";
    unit.bTpIdWasDeclared = true;
    unit.bHasFootprint = true;
    unit.baseFootprintWidth = 4.5f;
    unit.baseFootprintDepth = 6.25f;
    unit.tags = { "infantry", "light" };
    unit.sourceLogicalPath = "engine/LJ/lua/common/units/unitsTemplates/edbm0149.santp";
    unit.sourceByteSize = 12345ull;
    unit.sourceModifiedTime = 1700000000ull;
    unit.sourceContentHash = 0xfeedfacecafebeefull;
    unit.sourcePriorityRank = 0;
    records.push_back(unit);

    Io::TemplateRecord prop;
    prop.dialectKind = Io::TemplateDialectKind::PropTemplateLowercase;
    prop.templateIdentifier = "rock01";
    prop.bTpIdWasDeclared = false;
    prop.bHasFootprint = true;
    prop.baseFootprintWidth = 1.0f;
    prop.baseFootprintDepth = 1.0f;
    prop.sourceLogicalPath = "Environment.sanpack!Environment/Desert/Props/rock01.sanprop";
    prop.sourceByteSize = 987ull;
    prop.sourceModifiedTime = 0ull;   // zip entry: no independent mtime (STEP86 precedent)
    prop.sourceContentHash = 0x1122334455667788ull;
    prop.sourcePriorityRank = 2;
    records.push_back(prop);

    Io::TemplateRecord marker;
    marker.dialectKind = Io::TemplateDialectKind::MarkerTemplate;
    marker.templateIdentifier = "spawnMarker01";
    marker.bTpIdWasDeclared = true;
    marker.bHasFootprint = false;
    marker.sourceLogicalPath = "engine/LJ/lua/common/markers/markerTemplates/spawnMarker01.sanprop";
    marker.sourceByteSize = 42ull;
    marker.sourceModifiedTime = 1699999999ull;
    marker.sourceContentHash = 0xffffffffffffffffull;   // exercises the uint64 top-of-range case
    marker.sourcePriorityRank = 1;
    records.push_back(marker);

    return records;
}

void CheckRecordsEqual(const Io::TemplateRecord& expected, const Io::TemplateRecord& actual, const char* label) {
    Check(expected.dialectKind == actual.dialectKind, label);
    Check(expected.templateIdentifier == actual.templateIdentifier, label);
    Check(expected.bTpIdWasDeclared == actual.bTpIdWasDeclared, label);
    Check(expected.bHasFootprint == actual.bHasFootprint, label);
    Check(NearlyEqual(expected.baseFootprintWidth, actual.baseFootprintWidth), label);
    Check(NearlyEqual(expected.baseFootprintDepth, actual.baseFootprintDepth), label);
    Check(expected.tags == actual.tags, label);
    Check(expected.sourceLogicalPath == actual.sourceLogicalPath, label);
    Check(expected.sourceByteSize == actual.sourceByteSize, label);
    Check(expected.sourceModifiedTime == actual.sourceModifiedTime, label);
    Check(expected.sourceContentHash == actual.sourceContentHash, label);
    Check(expected.sourcePriorityRank == actual.sourcePriorityRank, label);
}

void TestRoundTripWithMatchingFingerprint() {
    const std::string cacheDirectory = ScratchFolderPath("SanGenTemplateIngestCacheTest_RoundTrip");
    Io::CachedTemplateIngest cache;
    cache.fingerprint.sourceFileCount = 3;
    cache.fingerprint.combinedContentHash = 0x9988776655443322ull;
    cache.records = MakeThreeMixedDialectRecords();

    Check(Io::SaveTemplateIngestCache(cacheDirectory, "C:/SteamGames/Sanctuary", cache),
          "round-trip: save succeeds");

    Io::CachedTemplateIngest loaded;
    const bool bLoaded = Io::LoadTemplateIngestCache(cacheDirectory, "C:/SteamGames/Sanctuary",
                                                      cache.fingerprint, loaded);
    Check(bLoaded, "round-trip: load succeeds with a matching fingerprint");
    Check(loaded.records.size() == 3, "round-trip: exactly 3 records loaded");
    if (loaded.records.size() == 3) {
        CheckRecordsEqual(cache.records[0], loaded.records[0], "round-trip: record 0 (UnitTemplate) exact");
        CheckRecordsEqual(cache.records[1], loaded.records[1], "round-trip: record 1 (PropTemplateLowercase) exact");
        CheckRecordsEqual(cache.records[2], loaded.records[2], "round-trip: record 2 (MarkerTemplate) exact");
    }
}

void TestMismatchedFingerprintReturnsFalseUntouched() {
    const std::string cacheDirectory = ScratchFolderPath("SanGenTemplateIngestCacheTest_Mismatch");
    Io::CachedTemplateIngest cache;
    cache.fingerprint.sourceFileCount = 3;
    cache.fingerprint.combinedContentHash = 0x1111111111111111ull;
    cache.records = MakeThreeMixedDialectRecords();
    Check(Io::SaveTemplateIngestCache(cacheDirectory, "C:/Game", cache), "mismatch fixture: save succeeds");

    Io::TemplateIngestFingerprint wrongCount = cache.fingerprint;
    wrongCount.sourceFileCount = 4;
    Io::CachedTemplateIngest outCache;
    outCache.fingerprint.sourceFileCount = -99;   // sentinel: must survive untouched
    Check(!Io::LoadTemplateIngestCache(cacheDirectory, "C:/Game", wrongCount, outCache),
          "mismatch: different sourceFileCount returns false");
    Check(outCache.fingerprint.sourceFileCount == -99, "mismatch: outCache untouched on sourceFileCount mismatch");
    Check(outCache.records.empty(), "mismatch: outCache.records untouched (still empty)");

    Io::TemplateIngestFingerprint wrongHash = cache.fingerprint;
    wrongHash.combinedContentHash = 0x2222222222222222ull;
    Io::CachedTemplateIngest outCache2;
    Check(!Io::LoadTemplateIngestCache(cacheDirectory, "C:/Game", wrongHash, outCache2),
          "mismatch: different combinedContentHash returns false");
    Check(outCache2.records.empty(), "mismatch: outCache2 untouched on hash mismatch");
}

void TestFormatVersionCheckedBeforeFingerprint() {
    const std::string cacheDirectory = ScratchFolderPath("SanGenTemplateIngestCacheTest_VersionFirst");
    Io::CachedTemplateIngest cache;
    cache.fingerprint.sourceFileCount = 1;
    cache.fingerprint.combinedContentHash = 0xaaaaaaaaaaaaaaaaull;
    cache.records = MakeThreeMixedDialectRecords();
    const std::string gameInstallRoot = "C:/VersionCheck";
    Check(Io::SaveTemplateIngestCache(cacheDirectory, gameInstallRoot, cache), "version-first fixture: save succeeds");

    // Rewrite the saved file with a bumped formatVersion but the SAME fingerprint -- the fingerprint
    // WOULD match if it were ever compared, proving the version stamp is checked first.
    const std::string filePath = Io::TemplateIngestCachePathFor(cacheDirectory, gameInstallRoot);
    std::string fileText;
    Check(Io::ReadTextFileBytes(filePath, fileText), "version-first fixture: cache file readable");
    nlohmann::json document = nlohmann::json::parse(fileText);
    document["formatVersion"] = document["formatVersion"].get<std::uint32_t>() + 1u;
    const std::string rewritten = document.dump(4);
    Check(Io::WriteBinaryFileBytes(filePath, rewritten.data(), rewritten.size()), "version-first fixture: rewrite succeeds");

    Io::CachedTemplateIngest outCache;
    Check(!Io::LoadTemplateIngestCache(cacheDirectory, gameInstallRoot, cache.fingerprint, outCache),
          "version-first: mismatched formatVersion returns false even though the fingerprint would match");
    Check(outCache.records.empty(), "version-first: outCache untouched");
}

void TestMissingMalformedAndPartialRecordCacheFilesFailCleanly() {
    const std::string cacheDirectory = ScratchFolderPath("SanGenTemplateIngestCacheTest_Malformed");
    const Io::TemplateIngestFingerprint expected{ 1, 0x1234ull };

    // Missing file entirely.
    Io::CachedTemplateIngest outMissing;
    Check(!Io::LoadTemplateIngestCache(cacheDirectory, "C:/NoSuchInstall", expected, outMissing),
          "malformed: missing cache file returns false, no crash");
    Check(outMissing.records.empty(), "malformed: outMissing untouched");

    // Malformed JSON text.
    const std::string malformedRoot = "C:/MalformedJson";
    const std::string malformedPath = Io::TemplateIngestCachePathFor(cacheDirectory, malformedRoot);
    Check(Io::WriteBinaryFileBytes(malformedPath, "{ this is not valid json", 24),
          "malformed fixture: garbage bytes written");
    Io::CachedTemplateIngest outMalformed;
    Check(!Io::LoadTemplateIngestCache(cacheDirectory, malformedRoot, expected, outMalformed),
          "malformed: invalid JSON text returns false, no crash/throw");
    Check(outMalformed.records.empty(), "malformed: outMalformed untouched");

    // Valid JSON, valid fingerprint, but one records[] element missing a required field.
    const std::string partialRoot = "C:/PartialRecord";
    nlohmann::json document;
    document["formatVersion"] = Io::kTemplateIngestCacheFormatVersion;
    document["fingerprint"] = { { "sourceFileCount", 1 }, { "combinedContentHash", 0x1234ull } };
    nlohmann::json badRecord;
    badRecord["DialectKind"] = 0;
    // "TemplateIdentifier" deliberately omitted -- a required field.
    badRecord["TpIdWasDeclared"] = false;
    badRecord["HasFootprint"] = false;
    badRecord["BaseFootprintWidth"] = 0.0f;
    badRecord["BaseFootprintDepth"] = 0.0f;
    badRecord["Tags"] = nlohmann::json::array();
    badRecord["SourceLogicalPath"] = "x";
    badRecord["SourceByteSize"] = 0ull;
    badRecord["SourceModifiedTime"] = 0ull;
    badRecord["SourceContentHash"] = 0ull;
    badRecord["SourcePriorityRank"] = 0;
    document["records"] = nlohmann::json::array({ badRecord });
    const std::string partialText = document.dump(4);
    const std::string partialPath = Io::TemplateIngestCachePathFor(cacheDirectory, partialRoot);
    Check(Io::WriteBinaryFileBytes(partialPath, partialText.data(), partialText.size()),
          "partial-record fixture: written");
    Io::CachedTemplateIngest outPartial;
    Check(!Io::LoadTemplateIngestCache(cacheDirectory, partialRoot, expected, outPartial),
          "malformed: one record missing a required field fails the WHOLE load");
    Check(outPartial.records.empty(), "malformed: outPartial untouched");
}

void TestDifferentGameInstallRootsProduceDifferentPaths() {
    const std::string cacheDirectory = ScratchFolderPath("SanGenTemplateIngestCacheTest_PathKeying");
    const std::string pathA = Io::TemplateIngestCachePathFor(cacheDirectory, "C:/Games/SteamGames/Sanctuary");
    const std::string pathB = Io::TemplateIngestCachePathFor(cacheDirectory, "D:/Games/SteamGames/Sanctuary");
    Check(pathA != pathB, "path keying: two different gameInstallRoot values with the same folder-name suffix "
                          "produce two different cache paths");
}

} // namespace

int main() {
    TestRoundTripWithMatchingFingerprint();
    TestMismatchedFingerprintReturnsFalseUntouched();
    TestFormatVersionCheckedBeforeFingerprint();
    TestMissingMalformedAndPartialRecordCacheFilesFailCleanly();
    TestDifferentGameInstallRootsProduceDifferentPaths();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
