// ResolvePackRelativeAsset_IO.cpp — see the header for the contract and the resolution order.
#include "ResolvePackRelativeAsset_IO.h"
#include "FilesystemPrimitives_IO.h"
#include "SanpackReader_IO.h"
#include <filesystem>

namespace SanmapGen {
namespace Io {
namespace {

// `.../engine/Sanctuary_Data/Gamedata` — the SAME root TemplateSourceScan_IO.cpp resolves its own
// unzipped-tree/sanpack paths against; not re-exported from that file (private to it, ARCH §1.5),
// so this is deliberately re-derived rather than shared, mirroring that file's own comment that
// this literal is duplicated wherever it's needed rather than pulled cross-file for one segment.
std::string GamedataRoot(const std::string& gameInstallRoot) {
    return JoinExportPath(JoinExportPath(gameInstallRoot, "engine"), "Sanctuary_Data/Gamedata");
}

bool TryUnzippedTree(const std::string& gamedataRoot, const std::string& packRelativePath,
                     PackRelativeAssetResult& result) {
    const std::string unzippedRoot = JoinExportPath(gamedataRoot, "Environment.sanpack.unzipped");
    const std::string candidatePath = JoinExportPath(unzippedRoot, packRelativePath);
    std::error_code existsError;
    if (!std::filesystem::is_regular_file(candidatePath, existsError)) return false;
    if (!ReadBinaryFileBytes(candidatePath, result.bytes)) {
        result.errorMessage = "ResolvePackRelativeAsset: found '" + candidatePath + "' but could not read it.";
        return true;   // resolved but failed -- caller must not also try the sanpack fallback
    }
    result.bSucceeded = true;
    return true;
}

bool TrySanpack(const std::string& gamedataRoot, const std::string& packRelativePath,
               PackRelativeAssetResult& result) {
    const std::string sanpackPath = JoinExportPath(gamedataRoot, "Environment.sanpack");
    std::error_code existsError;
    if (!std::filesystem::is_regular_file(sanpackPath, existsError)) {
        result.errorMessage = "ResolvePackRelativeAsset: neither the unzipped Environment tree nor '"
                             + sanpackPath + "' is present.";
        return false;
    }
    SanpackReader reader;
    if (!reader.Open(sanpackPath) || !reader.ReadCentralDirectoryOnce()) {
        result.errorMessage = "ResolvePackRelativeAsset: could not open/read '" + sanpackPath + "'.";
        return false;
    }
    SanpackEntryFilter filter;
    filter.pathPrefixes = { packRelativePath };
    SanpackSafetyLimits limits;
    std::vector<SanpackPayload> payloads;
    if (!reader.ExtractFiltered(filter, limits, payloads)) {
        result.errorMessage = "ResolvePackRelativeAsset: extraction from '" + sanpackPath + "' failed.";
        return false;
    }
    for (const SanpackPayload& payload : payloads) {
        if (payload.name != packRelativePath) continue;   // the prefix filter can over-match
        if (!payload.bValid) {
            result.errorMessage = "ResolvePackRelativeAsset: '" + packRelativePath
                                 + "' failed validation inside " + sanpackPath + ": " + payload.rejectionReason;
            return false;
        }
        result.bytes = payload.bytes;
        result.bSucceeded = true;
        return true;
    }
    result.errorMessage = "ResolvePackRelativeAsset: '" + packRelativePath + "' not found in " + sanpackPath + ".";
    return false;
}

} // namespace

PackRelativeAssetResult ResolvePackRelativeAssetBytes(const std::string& gameInstallRoot,
                                                       const std::string& packRelativePath) {
    PackRelativeAssetResult result;
    if (gameInstallRoot.empty() || packRelativePath.empty()) {
        result.errorMessage = "ResolvePackRelativeAsset: empty gameInstallRoot or packRelativePath.";
        return result;
    }
    const std::string gamedataRoot = GamedataRoot(gameInstallRoot);
    if (TryUnzippedTree(gamedataRoot, packRelativePath, result)) return result;
    TrySanpack(gamedataRoot, packRelativePath, result);
    return result;
}

} // namespace Io
} // namespace SanmapGen
