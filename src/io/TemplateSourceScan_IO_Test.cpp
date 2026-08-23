// TemplateSourceScan_IO_Test.cpp — acceptance test for ScanTemplateSources (STEP86). Drives real
// scratch install layouts under the platform temp directory (never a real game install), including
// a genuine miniz-written zip for the sanpack-fallback case, through every case the ticket names:
// the empty-root no-op, mixed-extension filtering with independently-recomputed hashes, the
// missing-Gamedata partial-ingest posture, "prefer unzipped, never both," the sanpack-only
// fallback, the oversize guard, the unreadable-file counter, and hash-as-real-content-function.
#include "TemplateSourceScan_IO.h"
#include "FilesystemPrimitives_IO.h"
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <utility>
#include <vector>
#include <miniz.h>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

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

// The SAME FNV-1a ScanTemplateSources itself computes, re-implemented independently here so the
// test proves a real content function rather than trusting the production code's own arithmetic.
std::uint64_t IndependentHash(const std::string& text) {
    std::uint64_t digest = 14695981039346656037ull;
    for (const unsigned char byte : text) {
        digest ^= byte;
        digest *= 1099511628211ull;
    }
    return digest;
}

bool EndsWith(const std::string& text, const std::string& suffix) {
    return text.size() >= suffix.size() && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

const Io::TemplateSourceFile* FindFileEndingWith(const Io::TemplateSourceScanReport& report, const std::string& suffix) {
    for (const Io::TemplateSourceFile& file : report.files)
        if (EndsWith(file.logicalPath, suffix)) return &file;
    return nullptr;
}

bool WriteSyntheticZip(const std::string& zipPath,
                       const std::vector<std::pair<std::string, std::string>>& entries) {
    std::remove(zipPath.c_str());
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

void TestEmptyRootReturnsDefaultReport() {
    const Io::TemplateSourceScanReport report = Io::ScanTemplateSources("");
    Check(report.files.empty(), "empty root: no files");
    Check(!report.bGamedataRootPresent, "empty root: Gamedata flag false");
    Check(!report.bUnzippedEnvironmentTreePresent, "empty root: unzipped flag false");
    Check(!report.bEnvironmentSanpackPresent, "empty root: sanpack flag false");
    Check(report.skippedOversizeFileCount == 0, "empty root: no oversize skips");
    Check(report.skippedUnreadableFileCount == 0, "empty root: no unreadable skips");
}

void TestMixedExtensionFilteringAndContentHash() {
    const std::string root = ScratchFolderPath("SanGenTemplateScanTest_MixedExtension");
    const std::string luaCommon = Io::JoinExportPath(root, "engine/LJ/lua/common");
    WriteTextFile(Io::JoinExportPath(luaCommon, "units/unitsTemplates/alpha.santp"), "shared template body");
    WriteTextFile(Io::JoinExportPath(luaCommon, "units/unitsTemplates/alpha_copy.sanprop"), "shared template body");
    WriteTextFile(Io::JoinExportPath(luaCommon, "units/unitsTemplates/alpha_diff.santp"), "shared template bodyX");
    WriteTextFile(Io::JoinExportPath(luaCommon, "units/unitsTemplates/ignored.sanmodel"), "not a template at all");
    WriteTextFile(Io::JoinExportPath(luaCommon, "props/propsTemplates/beta.sanprop"), "beta content");
    WriteTextFile(Io::JoinExportPath(luaCommon, "markers/markerTemplates/gamma.SANTP"), "gamma content");
    WriteTextFile(Io::JoinExportPath(luaCommon, "projectiles/projectilesTemplates/delta.SanProp"), "delta content");

    const Io::TemplateSourceScanReport report = Io::ScanTemplateSources(root);
    Check(report.files.size() == 6, "mixed extension: exactly the 6 .santp/.sanprop files, .sanmodel excluded");
    Check(FindFileEndingWith(report, "ignored.sanmodel") == nullptr, "unrelated extension never appears");
    Check(FindFileEndingWith(report, "gamma.SANTP") != nullptr, "extension match is case-insensitive (upper)");
    Check(FindFileEndingWith(report, "delta.SanProp") != nullptr, "extension match is case-insensitive (mixed)");

    const Io::TemplateSourceFile* alpha = FindFileEndingWith(report, "alpha.santp");
    const Io::TemplateSourceFile* alphaCopy = FindFileEndingWith(report, "alpha_copy.sanprop");
    const Io::TemplateSourceFile* alphaDiff = FindFileEndingWith(report, "alpha_diff.santp");
    Check(alpha != nullptr && alphaCopy != nullptr && alphaDiff != nullptr, "all three alpha-family files found");
    if (alpha != nullptr) {
        Check(alpha->sourceRank == Io::TemplateSourceRank::LooseFile, "loose file ranks LooseFile");
        Check(alpha->byteSize == std::string("shared template body").size(), "byteSize matches written content");
        Check(alpha->contentHash == IndependentHash("shared template body"),
              "contentHash matches an independently-computed FNV-1a");
    }
    if (alpha != nullptr && alphaCopy != nullptr)
        Check(alpha->contentHash == alphaCopy->contentHash, "byte-identical content produces the same hash");
    if (alpha != nullptr && alphaDiff != nullptr)
        Check(alpha->contentHash != alphaDiff->contentHash, "a one-byte difference produces a different hash");
}

void TestMissingGamedataIsPartialIngestNotFailure() {
    const std::string root = ScratchFolderPath("SanGenTemplateScanTest_NoGamedata");
    const std::string luaCommon = Io::JoinExportPath(root, "engine/LJ/lua/common");
    WriteTextFile(Io::JoinExportPath(luaCommon, "units/unitsTemplates/soldier.santp"), "soldier body");
    WriteTextFile(Io::JoinExportPath(luaCommon, "markers/markerTemplates/spawn.sanprop"), "spawn body");
    // Deliberately no engine/Sanctuary_Data/Gamedata anywhere under root.

    const Io::TemplateSourceScanReport report = Io::ScanTemplateSources(root);
    Check(!report.bGamedataRootPresent, "missing Gamedata: flag is false");
    Check(report.files.size() == 2, "missing Gamedata: the two LJ/lua files still populated");
    Check(FindFileEndingWith(report, "soldier.santp") != nullptr, "unitsTemplates source still present");
    Check(FindFileEndingWith(report, "spawn.sanprop") != nullptr, "markerTemplates source still present");
    Check(!report.bUnzippedEnvironmentTreePresent && !report.bEnvironmentSanpackPresent,
          "missing Gamedata: Environment flags both false, not attempted");
}

void TestUnzippedPreferredOverSanpackNeverBoth() {
    const std::string root = ScratchFolderPath("SanGenTemplateScanTest_UnzippedPreferred");
    const std::string gamedataRoot = Io::JoinExportPath(root, "engine/Sanctuary_Data/Gamedata");
    WriteTextFile(Io::JoinExportPath(gamedataRoot, "Environment.sanpack.unzipped/Environment/Desert/Props/rock.santp"),
                  "rock unzipped content");
    WriteTextFile(Io::JoinExportPath(gamedataRoot, "Environment.sanpack"), "not a real zip -- deliberately different content");

    const Io::TemplateSourceScanReport report = Io::ScanTemplateSources(root);
    Check(report.bGamedataRootPresent, "unzipped-preferred: Gamedata root present");
    Check(report.bUnzippedEnvironmentTreePresent, "unzipped-preferred: unzipped flag true");
    Check(!report.bEnvironmentSanpackPresent, "unzipped-preferred: sanpack flag stays false -- never both");
    Check(report.files.size() == 1, "unzipped-preferred: exactly one file, from the unzipped tree");
    const Io::TemplateSourceFile* rock = FindFileEndingWith(report, "rock.santp");
    Check(rock != nullptr && rock->sourceRank == Io::TemplateSourceRank::UnzippedPackTree,
          "unzipped-preferred: sourceRank == UnzippedPackTree");
}

void TestSanpackOnlyFallback() {
    const std::string root = ScratchFolderPath("SanGenTemplateScanTest_SanpackFallback");
    const std::string gamedataRoot = Io::JoinExportPath(root, "engine/Sanctuary_Data/Gamedata");
    std::error_code makeDirectoryError;
    std::filesystem::create_directories(gamedataRoot, makeDirectoryError);   // Gamedata present, no unzipped tree
    const std::string sanpackPath = Io::JoinExportPath(gamedataRoot, "Environment.sanpack");
    const std::vector<std::pair<std::string, std::string>> entries = {
        { "Environment/Desert/Props/rock.santp", "rock sanpack content" },
        { "Environment/Desert/Props/rock2.sanprop", "rock2 sanpack content" },
        { "Environment/readme.txt", "should be filtered out, not a template extension" },
    };
    Check(WriteSyntheticZip(sanpackPath, entries), "synthetic Environment.sanpack written with the miniz writer");

    const Io::TemplateSourceScanReport report = Io::ScanTemplateSources(root);
    Check(!report.bUnzippedEnvironmentTreePresent, "sanpack-only: unzipped flag false");
    Check(report.bEnvironmentSanpackPresent, "sanpack-only: sanpack flag true");
    Check(report.files.size() == 2, "sanpack-only: the two template entries, readme.txt filtered out");
    const Io::TemplateSourceFile* rock = FindFileEndingWith(report, "!Environment/Desert/Props/rock.santp");
    Check(rock != nullptr, "sanpack-only: logicalPath is \"<sanpackPath>!<entryName>\"");
    if (rock != nullptr) {
        Check(rock->sourceRank == Io::TemplateSourceRank::CompressedSanpack, "sanpack-only: sourceRank == CompressedSanpack");
        Check(rock->modifiedTime == 0, "sanpack-only: a zip entry carries no independent mtime");
        Check(rock->contentHash == IndependentHash("rock sanpack content"),
              "sanpack-only: contentHash matches the decompressed bytes, independently re-hashed");
    }
}

void TestOversizeFileNeverOpened() {
    const std::string root = ScratchFolderPath("SanGenTemplateScanTest_Oversize");
    const std::string huge = Io::JoinExportPath(root, "engine/LJ/lua/common/props/propsTemplates/huge.santp");
    std::error_code makeDirectoryError;
    std::filesystem::create_directories(std::filesystem::path(huge).parent_path(), makeDirectoryError);
    { std::ofstream(huge, std::ios::binary).close(); }
    std::error_code resizeError;
    std::filesystem::resize_file(huge, 70ull * 1024 * 1024, resizeError);   // sparse size claim, no real write
    Check(!resizeError, "oversize fixture: file resized to a 70MB size claim");

    const Io::TemplateSourceScanReport report = Io::ScanTemplateSources(root);
    Check(report.skippedOversizeFileCount == 1, "oversize: exactly one file skipped for size");
    Check(FindFileEndingWith(report, "huge.santp") == nullptr, "oversize: never appended to files");
}

void TestUnreadableFileIncrementsCounter() {
    const std::string root = ScratchFolderPath("SanGenTemplateScanTest_Unreadable");
    // A directory masquerading with a .santp name: matches the extension filter but is not a
    // regular file, so it can never be opened as one -- the platform-portable "unreadable" fixture.
    const std::string fakeDirectory = Io::JoinExportPath(root, "engine/LJ/lua/common/markers/markerTemplates/weird.santp");
    std::error_code makeDirectoryError;
    std::filesystem::create_directories(fakeDirectory, makeDirectoryError);
    Check(!makeDirectoryError, "unreadable fixture: directory named *.santp created");

    const Io::TemplateSourceScanReport report = Io::ScanTemplateSources(root);
    Check(report.skippedUnreadableFileCount == 1, "unreadable: exactly one file counted");
    Check(FindFileEndingWith(report, "weird.santp") == nullptr, "unreadable: never appended to files");
}

} // namespace

int main() {
    TestEmptyRootReturnsDefaultReport();
    TestMixedExtensionFilteringAndContentHash();
    TestMissingGamedataIsPartialIngestNotFailure();
    TestUnzippedPreferredOverSanpackNeverBoth();
    TestSanpackOnlyFallback();
    TestOversizeFileNeverOpened();
    TestUnreadableFileIncrementsCounter();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
