// SanpackReader_IO.h — single-pass sanpack (zip) ingestion. Layer: IO — it LOADS, it never
// simulates and it never touches GPU state (Constitution §1, ARCH §3.1/§3.2).
// ASSET_LOADING_SPEC: memory-map the archive (never copy ~2 GB into RAM), read the central
// directory EXACTLY ONCE, filter to the entries the app actually needs, sort them by file
// offset, and inflate them in ONE forward pass so the disk never seeks backwards. Every entry
// is validated on the way (Constitution §6) — a bad record is reported as an invalid payload
// for the caller to replace with a placeholder, and never crashes or is trusted into RAM.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace SanmapGen {
namespace Io {

// One kept central-directory record. Sizes are the archive's claims, not yet trusted.
struct SanpackEntry {
    std::string   name;
    std::uint64_t localHeaderOffset = 0;
    std::uint64_t compressedByteSize = 0;
    std::uint64_t uncompressedByteSize = 0;
    std::uint32_t expectedCrc32 = 0;
    std::uint16_t compressionMethod = 0;
};

// Which entries the app needs. Empty list = "accept everything on that axis"; extensions are
// matched case-insensitively and include the dot (".dds").
struct SanpackEntryFilter {
    std::vector<std::string> pathPrefixes;
    std::vector<std::string> extensions;
    bool Accepts(const std::string& entryName) const;
};

// Validation caps (Constitution §6) — settings with sane defaults, never literals at a use site.
struct SanpackSafetyLimits {
    std::uint64_t maximumEntryByteSize = 64ull * 1024 * 1024;
    std::uint64_t maximumTotalByteSize = 1024ull * 1024 * 1024;
    bool bVerifyCrc32 = true;
};

// One extracted payload. bValid == false means the record failed validation: the bytes are
// empty and the caller substitutes a placeholder (never a partial or unverified buffer).
struct SanpackPayload {
    std::string name;
    std::vector<unsigned char> bytes;
    std::string rejectionReason;
    bool bValid = false;
};

// Instrumentation the acceptance test reads: the central directory must be parsed exactly once
// and the extraction pass must walk file offsets forward.
struct SanpackIngestStatistics {
    int  centralDirectoryReadCount = 0;
    int  directoryEntryCount = 0;
    int  filteredEntryCount = 0;
    int  extractedEntryCount = 0;
    int  invalidEntryCount = 0;
    bool bExtractedInFileOffsetOrder = true;
};

class SanpackReader {
public:
    SanpackReader() = default;
    ~SanpackReader() { Close(); }
    SanpackReader(const SanpackReader&) = delete;
    SanpackReader& operator=(const SanpackReader&) = delete;

    // Maps the archive read-only. No archive bytes are copied here.
    bool Open(const std::string& sanpackPath);
    void Close();
    bool IsOpen() const { return mappedData != nullptr; }
    std::uint64_t MappedByteSize() const { return mappedByteSize; }

    // Parses the central directory on the first call and caches the records; every later call
    // reuses them and does NOT re-scan (centralDirectoryReadCount stays at 1 per Open).
    bool ReadCentralDirectoryOnce();

    // Filter -> sort by local-header offset -> one sequential inflate pass, appending a payload
    // per accepted entry (valid or not, so the caller can place a placeholder by name).
    bool ExtractFiltered(const SanpackEntryFilter& filter, const SanpackSafetyLimits& limits,
                         std::vector<SanpackPayload>& outPayloads);

    const std::vector<SanpackEntry>& DirectoryEntries() const { return directoryEntries; }
    const SanpackIngestStatistics& Statistics() const { return statistics; }

private:
    bool MapFile(const std::string& sanpackPath);
    void UnmapFile();
    bool FindEndOfCentralDirectory(std::uint64_t& outEntryCount, std::uint64_t& outDirectoryOffset) const;
    bool ParseDirectoryRecords(std::uint64_t entryCount, std::uint64_t directoryOffset);
    // Resolves the payload of one entry: local header -> bounds -> inflate -> CRC. Never throws.
    bool ReadEntryPayload(const SanpackEntry& entry, const SanpackSafetyLimits& limits,
                          std::uint64_t& runningTotalByteSize, SanpackPayload& outPayload) const;

    const unsigned char* mappedData = nullptr;
    std::uint64_t mappedByteSize = 0;
    void* platformFileHandle = nullptr;      // opaque: Win32 HANDLE / POSIX descriptor
    void* platformMappingHandle = nullptr;   // opaque: Win32 file-mapping object
    std::vector<SanpackEntry> directoryEntries;
    SanpackIngestStatistics statistics;
    bool bDirectoryParsed = false;
};

} // namespace Io
} // namespace SanmapGen
