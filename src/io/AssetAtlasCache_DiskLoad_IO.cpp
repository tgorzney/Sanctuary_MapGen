// AssetAtlasCache_DiskLoad_IO.cpp — the cache HIT path: read the small manifest, compare the
// fingerprint, and only then pull the page blob (ASSET_LOADING_SPEC "cold start becomes a couple
// of texture uploads"). A mismatched fingerprint costs one small read and no decoding at all;
// a truncated or malformed cache file simply fails the load and the caller rebuilds
// (Constitution §6 — a cache file is external input like any other).
#include "AssetAtlasCache_IO.h"
#include "AssetAtlasCache_DiskFormat_IO.h"
#include <cstdio>

namespace SanmapGen {
namespace Io {

namespace {

bool ReadWholeFile(const std::string& filePath, std::vector<unsigned char>& outBytes) {
    std::FILE* file = std::fopen(filePath.c_str(), "rb");
    if (file == nullptr) return false;
    outBytes.clear();
    unsigned char readBuffer[1u << 16];
    for (;;) {
        const std::size_t readCount = std::fread(readBuffer, 1, sizeof(readBuffer), file);
        if (readCount == 0) break;
        outBytes.insert(outBytes.end(), readBuffer, readBuffer + readCount);
    }
    std::fclose(file);
    return !outBytes.empty();
}

// The cheap half: magic, version, and the fingerprint comparison that decides everything.
bool ReadManifestHeader(DiskFormat::ByteCursor& cursor, const SourceFingerprint& expected) {
    if (cursor.ReadUnsigned32() != DiskFormat::manifestMagic) return false;
    if (cursor.ReadUnsigned32() != DiskFormat::formatVersion) return false;
    SourceFingerprint stored;
    stored.sourcePath = cursor.ReadText();
    stored.byteSize = cursor.ReadUnsigned64();
    stored.modifiedTime = cursor.ReadUnsigned64();
    stored.contentHash = cursor.ReadUnsigned64();
    return cursor.IsGood() && stored.Matches(expected);
}

std::vector<AtlasImage> ReadPageDimensions(DiskFormat::ByteCursor& cursor) {
    const std::uint32_t pageCount = cursor.ReadUnsigned32();
    std::vector<AtlasImage> pages(cursor.IsGood() ? pageCount : 0);
    for (std::size_t pageIndex = 0; pageIndex < pages.size() && cursor.IsGood(); ++pageIndex) {
        pages[pageIndex].width = static_cast<int>(cursor.ReadUnsigned32());
        pages[pageIndex].height = static_cast<int>(cursor.ReadUnsigned32());
    }
    return pages;
}

// Each page's own header must agree with the manifest before a byte of it is trusted.
bool ReadPagePixels(const std::string& blobPath, std::vector<AtlasImage>& pages) {
    std::vector<unsigned char> blobBytes;
    if (!ReadWholeFile(blobPath, blobBytes)) return false;
    DiskFormat::ByteCursor blob(blobBytes.data(), blobBytes.size());
    if (blob.ReadUnsigned32() != DiskFormat::pageBlobMagic ||
        blob.ReadUnsigned32() != DiskFormat::formatVersion ||
        blob.ReadUnsigned32() != static_cast<std::uint32_t>(pages.size())) return false;
    for (AtlasImage& page : pages) {
        if (blob.ReadUnsigned32() != static_cast<std::uint32_t>(page.width) ||
            blob.ReadUnsigned32() != static_cast<std::uint32_t>(page.height)) return false;
        const std::uint64_t pageByteSize = blob.ReadUnsigned64();
        if (!blob.IsGood() || pageByteSize != page.ExpectedByteSize()) return false;
        const unsigned char* pixels = blob.ReadBlock(static_cast<std::size_t>(pageByteSize));
        if (pixels == nullptr) return false;
        page.rgbaPixels.assign(pixels, pixels + static_cast<std::size_t>(pageByteSize));
    }
    return true;
}

bool ReadEntries(DiskFormat::ByteCursor& cursor, AssetAtlas& atlas) {
    const std::uint32_t entryCount = cursor.ReadUnsigned32();
    for (std::uint32_t index = 0; index < entryCount; ++index) {
        AtlasEntry entry;
        entry.name = cursor.ReadText();
        entry.pageIndex = static_cast<int>(cursor.ReadUnsigned32());
        entry.pixelX = static_cast<int>(cursor.ReadUnsigned32());
        entry.pixelY = static_cast<int>(cursor.ReadUnsigned32());
        entry.width = static_cast<int>(cursor.ReadUnsigned32());
        entry.height = static_cast<int>(cursor.ReadUnsigned32());
        entry.bPlaceholder = cursor.ReadUnsigned32() != 0;
        if (!cursor.IsGood() || entry.pageIndex < 0 || entry.pageIndex >= atlas.PageCount()) return false;
        const AtlasImage& page = atlas.Pages()[static_cast<std::size_t>(entry.pageIndex)];
        atlas.AddEntryWithDerivedUv(entry, page.width, page.height);   // same derivation as the packer
    }
    return cursor.IsGood() && !atlas.IsEmpty();
}

} // namespace

bool AssetAtlasCache::LoadFromDisk(const std::string& cacheDirectory, const SourceFingerprint& expected) {
    atlas.Clear();
    if (cacheDirectory.empty() || !expected.IsValid()) return false;
    std::vector<unsigned char> manifestBytes;
    if (!ReadWholeFile(ManifestPathFor(cacheDirectory, expected.sourcePath), manifestBytes)) return false;
    DiskFormat::ByteCursor manifest(manifestBytes.data(), manifestBytes.size());
    if (!ReadManifestHeader(manifest, expected)) return false;
    std::vector<AtlasImage> pages = ReadPageDimensions(manifest);
    if (!manifest.IsGood() || pages.empty()) return false;
    if (!ReadPagePixels(PageBlobPathFor(cacheDirectory, expected.sourcePath), pages)) return false;
    atlas.MutablePages() = std::move(pages);
    if (!ReadEntries(manifest, atlas)) { atlas.Clear(); return false; }
    return true;
}

} // namespace Io
} // namespace SanmapGen
