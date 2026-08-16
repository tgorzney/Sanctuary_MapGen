// SanpackReader_CentralDirectory_IO.cpp — the ONE central-directory read (ASSET_LOADING_SPEC).
// Locates the end-of-central-directory record, upgrades it through the zip64 locator when the
// archive carries one, and parses every file header into SanpackEntry records that the
// extraction pass then sorts by offset. Runs exactly once per Open: the parsed vector is the
// cache, and statistics.centralDirectoryReadCount is incremented only by an actual scan, which
// is what the acceptance test asserts. Every field read is bounds-checked first (Constitution §6).
#include "SanpackReader_IO.h"
#include "SanpackReader_Format_IO.h"
#include <iostream>

namespace SanmapGen {
namespace Io {

namespace {

using namespace ZipFormat;

// The record sits at the end, behind an archive comment of at most 64 KiB; scan backwards and
// take the first signature whose declared comment length actually reaches the file end.
bool LocateEndOfCentralDirectory(const unsigned char* data, std::uint64_t mappedByteSize,
                                 std::uint64_t& outRecordOffset) {
    const std::uint64_t window = maximumArchiveCommentByteSize + endOfCentralDirectoryByteSize;
    const std::uint64_t lowestOffset = mappedByteSize > window ? mappedByteSize - window : 0;
    std::uint64_t offset = mappedByteSize - endOfCentralDirectoryByteSize;
    for (;;) {
        const std::uint64_t commentLength = ReadUnsigned32(data + offset) == endOfCentralDirectorySignature
                                          ? ReadUnsigned16(data + offset + 20) : mappedByteSize;
        if (offset + endOfCentralDirectoryByteSize + commentLength == mappedByteSize) {
            outRecordOffset = offset;
            return true;
        }
        if (offset == lowestOffset) return false;
        --offset;
    }
}

// >64 K entries or >4 GB offsets live in the zip64 record the locator points at.
void ApplyZip64Upgrade(const unsigned char* data, std::uint64_t mappedByteSize,
                       std::uint64_t recordOffset, std::uint64_t& entryCount,
                       std::uint64_t& directoryOffset) {
    if (recordOffset < zip64LocatorByteSize) return;
    const std::uint64_t locatorOffset = recordOffset - zip64LocatorByteSize;
    if (ReadUnsigned32(data + locatorOffset) != zip64LocatorSignature) return;
    const std::uint64_t zip64Offset = ReadUnsigned64(data + locatorOffset + 8);
    if (!FitsInsideMapping(zip64Offset, zip64DirectoryByteSize, mappedByteSize)) return;
    if (ReadUnsigned32(data + zip64Offset) != zip64EndOfCentralDirectorySignature) return;
    entryCount = ReadUnsigned64(data + zip64Offset + 32);
    directoryOffset = ReadUnsigned64(data + zip64Offset + 48);
}

} // namespace

bool SanpackReader::FindEndOfCentralDirectory(std::uint64_t& outEntryCount,
                                              std::uint64_t& outDirectoryOffset) const {
    std::uint64_t recordOffset = 0;
    if (!LocateEndOfCentralDirectory(mappedData, mappedByteSize, recordOffset)) return false;
    outEntryCount = ZipFormat::ReadUnsigned16(mappedData + recordOffset + 10);
    outDirectoryOffset = ZipFormat::ReadUnsigned32(mappedData + recordOffset + 16);
    ApplyZip64Upgrade(mappedData, mappedByteSize, recordOffset, outEntryCount, outDirectoryOffset);
    return ZipFormat::FitsInsideMapping(outDirectoryOffset, ZipFormat::centralFileHeaderByteSize,
                                        mappedByteSize) || outEntryCount == 0;
}

bool SanpackReader::ParseDirectoryRecords(std::uint64_t entryCount, std::uint64_t directoryOffset) {
    using namespace ZipFormat;
    directoryEntries.clear();
    directoryEntries.reserve(static_cast<std::size_t>(entryCount));
    std::uint64_t cursor = directoryOffset;
    for (std::uint64_t index = 0; index < entryCount; ++index) {
        if (!FitsInsideMapping(cursor, centralFileHeaderByteSize, mappedByteSize)) return false;
        const unsigned char* header = mappedData + cursor;
        if (ReadUnsigned32(header) != centralFileHeaderSignature) return false;
        const std::uint64_t nameLength = ReadUnsigned16(header + 28);
        const std::uint64_t extraLength = ReadUnsigned16(header + 30);
        const std::uint64_t commentLength = ReadUnsigned16(header + 32);
        const std::uint64_t recordSize = centralFileHeaderByteSize + nameLength + extraLength + commentLength;
        if (!FitsInsideMapping(cursor, recordSize, mappedByteSize)) return false;
        SanpackEntry entry;
        entry.name.assign(reinterpret_cast<const char*>(header + centralFileHeaderByteSize),
                          static_cast<std::size_t>(nameLength));
        entry.compressionMethod = ReadUnsigned16(header + 10);
        entry.expectedCrc32 = ReadUnsigned32(header + 16);
        entry.compressedByteSize = ReadUnsigned32(header + 20);
        entry.uncompressedByteSize = ReadUnsigned32(header + 24);
        entry.localHeaderOffset = ReadUnsigned32(header + 42);
        directoryEntries.push_back(std::move(entry));
        cursor += recordSize;
    }
    return true;
}

bool SanpackReader::ReadCentralDirectoryOnce() {
    if (bDirectoryParsed) return !directoryEntries.empty();   // cached — no second scan, ever
    if (mappedData == nullptr) return false;
    bDirectoryParsed = true;
    ++statistics.centralDirectoryReadCount;
    std::uint64_t entryCount = 0;
    std::uint64_t directoryOffset = 0;
    if (!FindEndOfCentralDirectory(entryCount, directoryOffset)) {
        std::cerr << "SanpackReader: no readable end-of-central-directory record.\n";
        return false;
    }
    if (!ParseDirectoryRecords(entryCount, directoryOffset)) {
        std::cerr << "SanpackReader: central directory is truncated or corrupt.\n";
        directoryEntries.clear();
        return false;
    }
    statistics.directoryEntryCount = static_cast<int>(directoryEntries.size());
    return !directoryEntries.empty();   // same answer the cached path gives on every later call
}

} // namespace Io
} // namespace SanmapGen
