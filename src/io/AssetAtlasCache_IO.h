// AssetAtlasCache_IO.h — build the icon atlas once, persist it, and reload it (ASSET_LOADING_SPEC
// "Atlas build" + "Disk cache"). Layer: IO — it loads, decodes and writes files; it never
// simulates and holds no GPU state (ARCH §3.1/§3.2).
// The contract: fingerprint the sanpack (path + size + mtime, optional content hash); a matching
// manifest in the cache directory means the atlas is reloaded from the blob and the sanpack is
// NEVER opened or extracted (AtlasBuildReport::bExtractionRan stays false). Otherwise the
// single-pass SanpackReader ingest runs, entries are decoded/validated (a bad one becomes a
// placeholder), everything is packed into pages, and the result is written back to the cache.
#pragma once
#include "AssetAtlasCache_Atlas_IO.h"
#include "SanpackReader_IO.h"
#include <cstdint>
#include <string>

namespace SanmapGen {
namespace Sys { class ThreadPool; }
namespace Io {

// Every atlas constant is a setting with a sane default (Constitution §8) — no literal at a
// use site. The dimension caps are the Constitution §6 validation gate on decoded images.
struct AtlasBuildSettings {
    int pageWidth = 4096;
    int pageHeight = 4096;
    int entryPaddingPixels = 1;
    int maximumPageCount = 8;
    int maximumIconWidth = 1024;
    int maximumIconHeight = 1024;
    int placeholderWidth = 16;
    int placeholderHeight = 16;
    int propThumbnailWidth = 64;
    int propThumbnailHeight = 64;
    bool bIncludeContentHashInFingerprint = false;   // path+size+mtime is the cheap default
    std::string propModelExtension = ".sanmodel";    // entries that need a rendered thumbnail
    SanpackSafetyLimits sanpackLimits;               // the ingest caps travel with the settings
};

// What makes a cached atlas still valid for its source archive.
struct SourceFingerprint {
    std::string   sourcePath;
    std::uint64_t byteSize = 0;
    std::uint64_t modifiedTime = 0;
    std::uint64_t contentHash = 0;
    bool IsValid() const { return !sourcePath.empty() && byteSize > 0; }
    bool Matches(const SourceFingerprint& other) const {
        return IsValid() && sourcePath == other.sourcePath && byteSize == other.byteSize &&
               modifiedTime == other.modifiedTime && contentHash == other.contentHash;
    }
};
SourceFingerprint FingerprintOfFile(const std::string& filePath, bool bIncludeContentHash);

// Instrumentation the acceptance test reads: which path the build took and what it produced.
struct AtlasBuildReport {
    bool bLoadedFromDiskCache = false;
    bool bExtractionRan = false;
    bool bWroteDiskCache = false;
    int  decodedIconCount = 0;
    int  renderedPropThumbnailCount = 0;
    int  placeholderCount = 0;
    int  packedEntryCount = 0;
    SanpackIngestStatistics sanpackStatistics;
};

class AssetAtlasCache {
public:
    // The one entry point. workerPool is optional: when supplied, decoding fans out across it
    // (Sys::ThreadPool, ARCH §3.3 runtime primitive); when null, decoding runs inline.
    bool BuildOrLoad(const std::string& sanpackPath, const std::string& cacheDirectory,
                     const SanpackEntryFilter& filter, const AtlasBuildSettings& settings,
                     AtlasBuildReport& outReport, Sys::ThreadPool* workerPool = nullptr);

    const AssetAtlas& Atlas() const { return atlas; }
    void Clear() { atlas.Clear(); }

    // Disk cache halves, public so a caller can pre-check a fingerprint without a build.
    bool LoadFromDisk(const std::string& cacheDirectory, const SourceFingerprint& expected);
    bool SaveToDisk(const std::string& cacheDirectory, const SourceFingerprint& fingerprint) const;
    static std::string ManifestPathFor(const std::string& cacheDirectory, const std::string& sourcePath);
    static std::string PageBlobPathFor(const std::string& cacheDirectory, const std::string& sourcePath);

private:
    bool BuildFromSanpack(const std::string& sanpackPath, const SanpackEntryFilter& filter,
                          const AtlasBuildSettings& settings, AtlasBuildReport& outReport,
                          Sys::ThreadPool* workerPool);
    // placeholderFlags is one byte per image (never std::vector<bool> — the decode fan-out
    // writes it from several workers, and a bit-packed vector is not element-wise safe).
    bool PackImages(const std::vector<AtlasImage>& images, const std::vector<std::string>& names,
                    const std::vector<unsigned char>& placeholderFlags,
                    const AtlasBuildSettings& settings, AtlasBuildReport& outReport);
    AssetAtlas atlas;
};

} // namespace Io
} // namespace SanmapGen
