// TemplateIngestCache_IO.h — a fingerprinted disk cache for a whole ingestion run's TemplateRecord
// set, modelled on AssetAtlasCache's own disk-cache discipline (manifest-style path keying, a
// format-version stamp checked BEFORE the fingerprint, never-throwing bounds-safe reads that fall
// through to "rebuild") — NOT a byte-for-byte copy of its binary ByteCursor blob format; see this
// ticket's own "Deliberate design decision." Layer: IO.
#pragma once
#include "TemplateDialect_IO.h"
#include <cstdint>
#include <string>
#include <vector>

namespace SanmapGen {
namespace Io {

// Cheap validity check for the WHOLE scan — content-hashing every one of ~546 small files is "free"
// at this corpus size (unlike AssetAtlasCache's 1.75 GB sanpack case, which defaults content hashing
// off for exactly the opposite reason), so per-file fingerprints are folded into ONE combined digest
// rather than compared file-by-file — ANY changed, added, or removed source file changes this value,
// triggering a full rebuild (never a partial/incremental one — matches AssetAtlasCache's own
// all-or-nothing rebuild granularity, generalized from one source file to many).
struct TemplateIngestFingerprint {
    int           sourceFileCount     = 0;
    std::uint64_t combinedContentHash = 0;   // FNV-1a fold, in sourceLogicalPath-sorted order, of
                                              // every source file's own {byteSize, modifiedTime,
                                              // contentHash} — sorted so the fold is independent of
                                              // ThreadPool completion order (ticket 89's fan-out).
    bool Matches(const TemplateIngestFingerprint& other) const {
        return sourceFileCount == other.sourceFileCount &&
               combinedContentHash == other.combinedContentHash;
    }
};

struct CachedTemplateIngest {
    TemplateIngestFingerprint   fingerprint;
    std::vector<TemplateRecord> records;   // every bRecognized record from the run that produced
                                            // this cache, INCLUDING skipped-by-design Projectile/
                                            // Marker entries (ticket 89 still wants their counts).
};

// Bump whenever TemplateRecord's fields or TemplateDialect_IO's extraction logic changes — a
// mismatch invalidates the cache unconditionally, before the fingerprint is even compared
// (DESIGN_SantpFootprintIngestion_R1.md §4.2: "a reader-logic change invalidates every cache
// without touching source files").
inline constexpr std::uint32_t kTemplateIngestCacheFormatVersion = 1;

std::string TemplateIngestCachePathFor(const std::string& cacheDirectory, const std::string& gameInstallRoot);

// False + outCache left UNTOUCHED on ANY anomaly (missing file, malformed JSON, format-version
// mismatch, fingerprint mismatch, or a record whose fields fail type-checking) — a corrupt or stale
// cache is never fatal, it is simply not used (Constitution §6; caller falls through to a full
// rebuild, exactly AssetAtlasCache::LoadFromDisk's own contract).
bool LoadTemplateIngestCache(const std::string& cacheDirectory, const std::string& gameInstallRoot,
                              const TemplateIngestFingerprint& expected, CachedTemplateIngest& outCache);

// False (logged to std::cerr — this document sits outside the `.sanmap`/MapImportResult domain,
// same posture AppSettings_IO.cpp already uses) on a write failure — never a partial file
// (WriteBinaryFileBytes's own contract, reused rather than a second write path invented here).
bool SaveTemplateIngestCache(const std::string& cacheDirectory, const std::string& gameInstallRoot,
                              const CachedTemplateIngest& cache);

} // namespace Io
} // namespace SanmapGen
