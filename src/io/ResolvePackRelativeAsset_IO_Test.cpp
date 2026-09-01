// ResolvePackRelativeAsset_IO_Test.cpp — acceptance test for ResolvePackRelativeAssetBytes.
// Drives real scratch install layouts under the platform temp directory (never a real game
// install), mirroring TemplateSourceScan_IO_Test's own fixture posture: a loose unzipped-tree hit,
// a genuine miniz-written sanpack fallback, "unzipped wins when both exist," a missing-everywhere
// failure, and a binary payload (proving this path is byte-safe, not text-only like the sibling
// template scanner).
#include "ResolvePackRelativeAsset_IO.h"
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

void WriteBinaryFile(const std::string& filePath, const std::vector<unsigned char>& content) {
    std::error_code makeDirectoryError;
    std::filesystem::create_directories(std::filesystem::path(filePath).parent_path(), makeDirectoryError);
    std::ofstream outputStream(filePath, std::ios::binary | std::ios::trunc);
    outputStream.write(reinterpret_cast<const char*>(content.data()), static_cast<std::streamsize>(content.size()));
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

void TestEmptyInputsFailCleanly() {
    Check(!Io::ResolvePackRelativeAssetBytes("", "Environment/A/rock.santp").bSucceeded, "empty root fails");
    Check(!Io::ResolvePackRelativeAssetBytes("C:/fake/root", "").bSucceeded, "empty path fails");
}

void TestUnzippedTreeHit() {
    const std::string root = ScratchFolderPath("SanGenResolvePackAssetTest_Unzipped");
    const std::string gamedataRoot = Io::JoinExportPath(root, "engine/Sanctuary_Data/Gamedata");
    const std::vector<unsigned char> payload = { 0x01, 0x02, 0x03, 0xFF, 0x00, 0x7F };   // arbitrary binary
    WriteBinaryFile(Io::JoinExportPath(gamedataRoot,
                    "Environment.sanpack.unzipped/Environment/Desert/Props/rock_lod0.sanmodel"), payload);

    const Io::PackRelativeAssetResult result =
        Io::ResolvePackRelativeAssetBytes(root, "Environment/Desert/Props/rock_lod0.sanmodel");
    Check(result.bSucceeded, "unzipped hit: succeeds");
    Check(result.bytes == payload, "unzipped hit: bytes reproduced exactly, including 0x00/0xFF");
}

void TestSanpackFallbackWhenNoUnzippedTree() {
    const std::string root = ScratchFolderPath("SanGenResolvePackAssetTest_Sanpack");
    const std::string gamedataRoot = Io::JoinExportPath(root, "engine/Sanctuary_Data/Gamedata");
    std::error_code makeDirectoryError;
    std::filesystem::create_directories(gamedataRoot, makeDirectoryError);   // miniz never creates dirs
    const std::string sanpackPath = Io::JoinExportPath(gamedataRoot, "Environment.sanpack");
    const std::vector<std::pair<std::string, std::string>> entries = {
        { "Environment/Highlands/Props/edmm0101/edms0103_lod0.sanmodel", "lod0 bytes" },
        { "Environment/Highlands/Props/edmm0101/edms0103_lod1.sanmodel", "lod1 bytes" },
    };
    Check(WriteSyntheticZip(sanpackPath, entries), "synthetic Environment.sanpack written");

    const Io::PackRelativeAssetResult result = Io::ResolvePackRelativeAssetBytes(
        root, "Environment/Highlands/Props/edmm0101/edms0103_lod0.sanmodel");
    Check(result.bSucceeded, "sanpack fallback: succeeds");
    const std::string asText(result.bytes.begin(), result.bytes.end());
    Check(asText == "lod0 bytes", "sanpack fallback: exact entry selected, not lod1's sibling bytes");
}

void TestUnzippedTreeWinsWhenBothPresent() {
    const std::string root = ScratchFolderPath("SanGenResolvePackAssetTest_BothPresent");
    const std::string gamedataRoot = Io::JoinExportPath(root, "engine/Sanctuary_Data/Gamedata");
    const std::vector<unsigned char> unzippedPayload = { 0xAA, 0xBB };
    WriteBinaryFile(Io::JoinExportPath(gamedataRoot,
                    "Environment.sanpack.unzipped/Environment/A/only.sanmodel"), unzippedPayload);
    const std::string sanpackPath = Io::JoinExportPath(gamedataRoot, "Environment.sanpack");
    Check(WriteSyntheticZip(sanpackPath, { { "Environment/A/only.sanmodel", "stale sanpack copy" } }),
          "both-present fixture: sanpack also written");

    const Io::PackRelativeAssetResult result =
        Io::ResolvePackRelativeAssetBytes(root, "Environment/A/only.sanmodel");
    Check(result.bSucceeded, "both-present: succeeds");
    Check(result.bytes == unzippedPayload, "both-present: unzipped tree wins, sanpack never consulted");
}

void TestNotFoundAnywhereFailsCleanly() {
    const std::string root = ScratchFolderPath("SanGenResolvePackAssetTest_NotFound");
    std::error_code makeDirectoryError;
    std::filesystem::create_directories(Io::JoinExportPath(root, "engine/Sanctuary_Data/Gamedata"), makeDirectoryError);

    const Io::PackRelativeAssetResult result =
        Io::ResolvePackRelativeAssetBytes(root, "Environment/Nowhere/ghost.sanmodel");
    Check(!result.bSucceeded, "not found: bSucceeded == false");
    Check(!result.errorMessage.empty(), "not found: a diagnostic message is set");
}

} // namespace

int main() {
    TestEmptyInputsFailCleanly();
    TestUnzippedTreeHit();
    TestSanpackFallbackWhenNoUnzippedTree();
    TestUnzippedTreeWinsWhenBothPresent();
    TestNotFoundAnywhereFailsCleanly();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
