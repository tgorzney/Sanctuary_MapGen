// AssetAtlasCache_DiskSave_IO.cpp — persist the built atlas so the next launch skips extraction
// entirely (ASSET_LOADING_SPEC "the be-done-with-it"). The manifest is assembled in memory (it
// is kilobytes) and the page blob is streamed page by page, so writing the cache never needs a
// second copy of the atlas in RAM.
#include "AssetAtlasCache_IO.h"
#include "AssetAtlasCache_DiskFormat_IO.h"
#include <cstdio>
#include <filesystem>
#include <iostream>

namespace SanmapGen {
namespace Io {

namespace {

std::vector<unsigned char> BuildManifestBytes(const AssetAtlas& atlas, const SourceFingerprint& fingerprint) {
    std::vector<unsigned char> bytes;
    DiskFormat::AppendUnsigned32(bytes, DiskFormat::manifestMagic);
    DiskFormat::AppendUnsigned32(bytes, DiskFormat::formatVersion);
    DiskFormat::AppendText(bytes, fingerprint.sourcePath);
    DiskFormat::AppendUnsigned64(bytes, fingerprint.byteSize);
    DiskFormat::AppendUnsigned64(bytes, fingerprint.modifiedTime);
    DiskFormat::AppendUnsigned64(bytes, fingerprint.contentHash);
    DiskFormat::AppendUnsigned32(bytes, static_cast<std::uint32_t>(atlas.PageCount()));
    for (const AtlasImage& page : atlas.Pages()) {
        DiskFormat::AppendUnsigned32(bytes, static_cast<std::uint32_t>(page.width));
        DiskFormat::AppendUnsigned32(bytes, static_cast<std::uint32_t>(page.height));
    }
    DiskFormat::AppendUnsigned32(bytes, static_cast<std::uint32_t>(atlas.EntryCount()));
    for (const AtlasEntry& entry : atlas.Entries()) {
        DiskFormat::AppendText(bytes, entry.name);
        DiskFormat::AppendUnsigned32(bytes, static_cast<std::uint32_t>(entry.pageIndex));
        DiskFormat::AppendUnsigned32(bytes, static_cast<std::uint32_t>(entry.pixelX));
        DiskFormat::AppendUnsigned32(bytes, static_cast<std::uint32_t>(entry.pixelY));
        DiskFormat::AppendUnsigned32(bytes, static_cast<std::uint32_t>(entry.width));
        DiskFormat::AppendUnsigned32(bytes, static_cast<std::uint32_t>(entry.height));
        DiskFormat::AppendUnsigned32(bytes, entry.bPlaceholder ? 1u : 0u);
    }
    return bytes;
}

bool WriteWholeFile(const std::string& filePath, const void* data, std::size_t byteSize) {
    std::FILE* file = std::fopen(filePath.c_str(), "wb");
    if (file == nullptr) return false;
    const bool bWritten = byteSize == 0 || std::fwrite(data, 1, byteSize, file) == byteSize;
    std::fclose(file);
    return bWritten;
}

bool WritePageBlob(const std::string& filePath, const AssetAtlas& atlas) {
    std::FILE* file = std::fopen(filePath.c_str(), "wb");
    if (file == nullptr) return false;
    std::vector<unsigned char> header;
    DiskFormat::AppendUnsigned32(header, DiskFormat::pageBlobMagic);
    DiskFormat::AppendUnsigned32(header, DiskFormat::formatVersion);
    DiskFormat::AppendUnsigned32(header, static_cast<std::uint32_t>(atlas.PageCount()));
    bool bWritten = std::fwrite(header.data(), 1, header.size(), file) == header.size();
    for (const AtlasImage& page : atlas.Pages()) {
        std::vector<unsigned char> pageHeader;
        DiskFormat::AppendUnsigned32(pageHeader, static_cast<std::uint32_t>(page.width));
        DiskFormat::AppendUnsigned32(pageHeader, static_cast<std::uint32_t>(page.height));
        DiskFormat::AppendUnsigned64(pageHeader, static_cast<std::uint64_t>(page.rgbaPixels.size()));
        bWritten = bWritten && std::fwrite(pageHeader.data(), 1, pageHeader.size(), file) == pageHeader.size();
        bWritten = bWritten && std::fwrite(page.rgbaPixels.data(), 1, page.rgbaPixels.size(), file) ==
                                   page.rgbaPixels.size();
    }
    std::fclose(file);
    return bWritten;
}

} // namespace

bool AssetAtlasCache::SaveToDisk(const std::string& cacheDirectory, const SourceFingerprint& fingerprint) const {
    if (!fingerprint.IsValid() || atlas.IsEmpty() || cacheDirectory.empty()) return false;
    std::error_code errorCode;
    std::filesystem::create_directories(cacheDirectory, errorCode);
    if (errorCode) {
        std::cerr << "AssetAtlasCache: cannot create cache directory '" << cacheDirectory << "'.\n";
        return false;
    }
    // The blob goes down first: a manifest that exists without its pages would be a cache hit
    // pointing at nothing, and the loader would have to unwind it.
    if (!WritePageBlob(PageBlobPathFor(cacheDirectory, fingerprint.sourcePath), atlas)) {
        std::cerr << "AssetAtlasCache: failed to write the atlas page blob.\n";
        return false;
    }
    const std::vector<unsigned char> manifestBytes = BuildManifestBytes(atlas, fingerprint);
    if (!WriteWholeFile(ManifestPathFor(cacheDirectory, fingerprint.sourcePath),
                        manifestBytes.data(), manifestBytes.size())) {
        std::cerr << "AssetAtlasCache: failed to write the atlas manifest.\n";
        return false;
    }
    return true;
}

} // namespace Io
} // namespace SanmapGen
