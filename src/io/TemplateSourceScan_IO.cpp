// TemplateSourceScan_IO.cpp — walks the .santp/.sanprop template source roots off a validated game
// install and returns raw file text with zero interpretation (Constitution §1 — dialect/JSON
// sniffing is TemplateDialect_IO's job, ticket 87). Reuses Io::SanpackReader/ReadTextFileBytes/
// JoinExportPath verbatim rather than duplicating any zip or read logic
// (ARCH_18_SantpFootprintIngestion.md).
#include "TemplateSourceScan_IO.h"
#include "FilesystemPrimitives_IO.h"
#include "SanpackReader_IO.h"
#include <cctype>
#include <filesystem>

namespace SanmapGen {
namespace Io {

namespace {

// The same five-line FNV-1a AssetAtlasCache_Fingerprint_IO.cpp's own private HashBytes already
// implements — duplicated locally rather than exported cross-file (a standard hash this small is
// not worth a cross-file dependency for).
std::uint64_t HashBytes(std::uint64_t seed, const unsigned char* bytes, std::size_t byteSize) {
    std::uint64_t digest = seed;
    for (std::size_t index = 0; index < byteSize; ++index) {
        digest ^= bytes[index];
        digest *= 1099511628211ull;      // FNV-1a
    }
    return digest;
}

std::uint64_t HashText(const std::string& text) {
    return HashBytes(14695981039346656037ull, reinterpret_cast<const unsigned char*>(text.data()),
                      text.size());
}

bool IsTemplateExtension(const std::filesystem::path& entryPath) {
    std::string extension = entryPath.extension().string();
    for (char& character : extension)
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    return extension == ".santp" || extension == ".sanprop";
}

// Matches SanpackSafetyLimits::maximumEntryByteSize's own default -- reused as a VALUE, not a
// shared type (these files are observed at 35.8 KB max; this is purely a defensive sanity cap).
constexpr std::uint64_t kMaximumSourceFileByteSize = 64ull * 1024 * 1024;

// One loose/unzipped subtree, walked recursively -- shared by every loose LJ/lua and Gamedata
// source AND the unzipped Environment tree ("walk it exactly as step 4").
void ScanLooseSubtree(const std::filesystem::path& subtreePath, TemplateSourceRank sourceRank,
                      TemplateSourceScanReport& report) {
    std::error_code walkError;
    if (!std::filesystem::is_directory(subtreePath, walkError)) return;
    for (const std::filesystem::directory_entry& entry :
         std::filesystem::recursive_directory_iterator(subtreePath, walkError)) {
        if (!IsTemplateExtension(entry.path())) continue;
        std::error_code entryError;
        if (!entry.is_regular_file(entryError) || entryError) {
            ++report.skippedUnreadableFileCount;
            continue;
        }
        const std::uintmax_t byteSize = std::filesystem::file_size(entry.path(), entryError);
        if (entryError) { ++report.skippedUnreadableFileCount; continue; }
        if (byteSize > kMaximumSourceFileByteSize) { ++report.skippedOversizeFileCount; continue; }

        std::string sourceText;
        if (!ReadTextFileBytes(entry.path().string(), sourceText)) {
            ++report.skippedUnreadableFileCount;
            continue;
        }

        TemplateSourceFile file;
        file.logicalPath = entry.path().string();
        file.sourceText  = sourceText;
        file.sourceRank  = sourceRank;
        file.byteSize    = static_cast<std::uint64_t>(byteSize);
        std::error_code timeError;
        const std::filesystem::file_time_type modifiedTime =
            std::filesystem::last_write_time(entry.path(), timeError);
        if (!timeError)
            file.modifiedTime = static_cast<std::uint64_t>(modifiedTime.time_since_epoch().count());
        file.contentHash = HashText(sourceText);
        report.files.push_back(std::move(file));
    }
}

// Environment.sanpack fallback -- reads via the real SanpackReader, never re-implemented here.
void ScanEnvironmentSanpack(const std::string& sanpackPath, TemplateSourceScanReport& report) {
    SanpackReader reader;
    if (!reader.Open(sanpackPath) || !reader.ReadCentralDirectoryOnce()) return;
    SanpackEntryFilter filter;
    filter.extensions = { ".santp", ".sanprop" };
    SanpackSafetyLimits limits;
    std::vector<SanpackPayload> payloads;
    if (!reader.ExtractFiltered(filter, limits, payloads)) return;
    for (const SanpackPayload& payload : payloads) {
        if (!payload.bValid) { ++report.skippedUnreadableFileCount; continue; }
        TemplateSourceFile file;
        file.logicalPath = sanpackPath + "!" + payload.name;
        file.sourceText.assign(reinterpret_cast<const char*>(payload.bytes.data()), payload.bytes.size());
        file.sourceRank   = TemplateSourceRank::CompressedSanpack;
        file.byteSize     = static_cast<std::uint64_t>(payload.bytes.size());
        file.modifiedTime = 0;
        file.contentHash  = HashText(file.sourceText);
        report.files.push_back(std::move(file));
    }
}

} // namespace

TemplateSourceScanReport ScanTemplateSources(const std::string& gameInstallRoot) {
    TemplateSourceScanReport report;
    if (gameInstallRoot.empty()) return report;

    const std::string luaCommonRoot = JoinExportPath(gameInstallRoot, "engine/LJ/lua/common");
    ScanLooseSubtree(JoinExportPath(luaCommonRoot, "units/unitsTemplates"), TemplateSourceRank::LooseFile, report);
    ScanLooseSubtree(JoinExportPath(luaCommonRoot, "props/propsTemplates"), TemplateSourceRank::LooseFile, report);
    ScanLooseSubtree(JoinExportPath(luaCommonRoot, "markers/markerTemplates"), TemplateSourceRank::LooseFile, report);
    ScanLooseSubtree(JoinExportPath(luaCommonRoot, "projectiles/projectilesTemplates"), TemplateSourceRank::LooseFile, report);

    const std::string gamedataRoot = JoinExportPath(gameInstallRoot, "engine/Sanctuary_Data/Gamedata");
    std::error_code gamedataError;
    report.bGamedataRootPresent = std::filesystem::is_directory(gamedataRoot, gamedataError);
    if (!report.bGamedataRootPresent) return report;   // partial ingest: LJ/lua already populated above

    ScanLooseSubtree(JoinExportPath(gamedataRoot, "Props"), TemplateSourceRank::LooseFile, report);
    ScanLooseSubtree(JoinExportPath(gamedataRoot, "Pandemonium"), TemplateSourceRank::LooseFile, report);

    const std::string unzippedEnvironmentPath =
        JoinExportPath(gamedataRoot, "Environment.sanpack.unzipped/Environment");
    std::error_code unzippedError;
    if (std::filesystem::is_directory(unzippedEnvironmentPath, unzippedError)) {
        report.bUnzippedEnvironmentTreePresent = true;
        ScanLooseSubtree(unzippedEnvironmentPath, TemplateSourceRank::UnzippedPackTree, report);
        return report;   // NEVER also read the sanpack -- §3.3's collision-storm warning
    }

    const std::string environmentSanpackPath = JoinExportPath(gamedataRoot, "Environment.sanpack");
    std::error_code sanpackError;
    if (std::filesystem::is_regular_file(environmentSanpackPath, sanpackError)) {
        report.bEnvironmentSanpackPresent = true;
        ScanEnvironmentSanpack(environmentSanpackPath, report);
    }
    return report;
}

} // namespace Io
} // namespace SanmapGen
