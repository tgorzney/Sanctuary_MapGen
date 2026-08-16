// AssetAtlasCache_Fingerprint_IO.cpp — "is the cached atlas still the truth?" (ASSET_LOADING_SPEC
// "Disk cache"). Path + byte size + modification time is the cheap default; the content hash is
// opt-in because hashing a ~2 GB sanpack costs more than the extraction the cache is avoiding.
// Also derives the cache file names: the user picks the cache FOLDER (owner decision), so two
// different sanpacks with the same file name must not collide inside it — the name carries a
// digest of the full source path.
#include "AssetAtlasCache_IO.h"
#include <cstdio>
#include <filesystem>
#include <vector>

namespace SanmapGen {
namespace Io {

namespace {

std::uint64_t HashBytes(std::uint64_t seed, const unsigned char* bytes, std::size_t byteSize) {
    std::uint64_t digest = seed;
    for (std::size_t index = 0; index < byteSize; ++index) {
        digest ^= bytes[index];
        digest *= 1099511628211ull;      // FNV-1a
    }
    return digest;
}

std::uint64_t HashText(const std::string& text) {
    return HashBytes(14695981039346656037ull, reinterpret_cast<const unsigned char*>(text.data()), text.size());
}

// Streamed so the optional content hash never needs the archive resident in RAM.
std::uint64_t HashFileContents(const std::string& filePath) {
    std::FILE* file = std::fopen(filePath.c_str(), "rb");
    if (file == nullptr) return 0;
    std::vector<unsigned char> readBuffer(1u << 16);
    std::uint64_t digest = 14695981039346656037ull;
    for (;;) {
        const std::size_t readCount = std::fread(readBuffer.data(), 1, readBuffer.size(), file);
        if (readCount == 0) break;
        digest = HashBytes(digest, readBuffer.data(), readCount);
    }
    std::fclose(file);
    return digest;
}

std::string CacheFileStem(const std::string& sourcePath) {
    const std::filesystem::path source(sourcePath);
    char digestText[17] = {};
    std::snprintf(digestText, sizeof(digestText), "%016llx",
                  static_cast<unsigned long long>(HashText(sourcePath)));
    return source.stem().string() + "_" + digestText;
}

} // namespace

SourceFingerprint FingerprintOfFile(const std::string& filePath, bool bIncludeContentHash) {
    SourceFingerprint fingerprint;
    std::error_code errorCode;
    const std::filesystem::path path(filePath);
    const std::uintmax_t byteSize = std::filesystem::file_size(path, errorCode);
    if (errorCode) return fingerprint;                     // missing/unreadable: never matches
    fingerprint.sourcePath = std::filesystem::absolute(path, errorCode).string();
    if (errorCode) fingerprint.sourcePath = filePath;
    fingerprint.byteSize = static_cast<std::uint64_t>(byteSize);
    const std::filesystem::file_time_type modifiedTime = std::filesystem::last_write_time(path, errorCode);
    if (!errorCode)
        fingerprint.modifiedTime = static_cast<std::uint64_t>(modifiedTime.time_since_epoch().count());
    if (bIncludeContentHash) fingerprint.contentHash = HashFileContents(filePath);
    return fingerprint;
}

std::string AssetAtlasCache::ManifestPathFor(const std::string& cacheDirectory, const std::string& sourcePath) {
    return (std::filesystem::path(cacheDirectory) / (CacheFileStem(sourcePath) + ".sanatlasmanifest")).string();
}

std::string AssetAtlasCache::PageBlobPathFor(const std::string& cacheDirectory, const std::string& sourcePath) {
    return (std::filesystem::path(cacheDirectory) / (CacheFileStem(sourcePath) + ".sanatlaspages")).string();
}

} // namespace Io
} // namespace SanmapGen
