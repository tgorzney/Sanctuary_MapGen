// SanpackReader_Inflate_IO.cpp — validate-then-inflate for ONE entry (Constitution §6).
// The only file that talks to miniz. Nothing here trusts the archive: the local header is
// re-read and re-checked against the mapped view, the declared sizes are capped before a byte
// is allocated, the inflate must produce exactly the declared length, and the CRC-32 must
// match. Any failure returns false with a reason — the caller substitutes a placeholder.
#include "SanpackReader_IO.h"
#include "SanpackReader_Format_IO.h"
#include <algorithm>
#include <miniz.h>

namespace SanmapGen {
namespace Io {

namespace {

using namespace ZipFormat;

// The local header repeats the name and carries its OWN extra-field length, so the payload
// offset can only be resolved from the local record — never from the central one.
bool ResolvePayloadOffset(const unsigned char* mappedData, std::uint64_t mappedByteSize,
                          const SanpackEntry& entry, std::uint64_t& outPayloadOffset,
                          std::string& outRejectionReason) {
    if (!FitsInsideMapping(entry.localHeaderOffset, localFileHeaderByteSize, mappedByteSize)) {
        outRejectionReason = "local header outside the archive";
        return false;
    }
    const unsigned char* header = mappedData + entry.localHeaderOffset;
    if (ReadUnsigned32(header) != localFileHeaderSignature) {
        outRejectionReason = "local header signature mismatch";
        return false;
    }
    const std::uint64_t headerSize = localFileHeaderByteSize + ReadUnsigned16(header + 26) +
                                     ReadUnsigned16(header + 28);
    if (!FitsInsideMapping(entry.localHeaderOffset, headerSize + entry.compressedByteSize, mappedByteSize)) {
        outRejectionReason = "payload runs past the end of the archive";
        return false;
    }
    outPayloadOffset = entry.localHeaderOffset + headerSize;
    return true;
}

bool IsWithinLimits(const SanpackEntry& entry, const SanpackSafetyLimits& limits,
                    std::uint64_t runningTotalByteSize, std::string& outRejectionReason) {
    if (entry.uncompressedByteSize > limits.maximumEntryByteSize) {
        outRejectionReason = "entry exceeds the per-entry byte cap";
        return false;
    }
    if (runningTotalByteSize + entry.uncompressedByteSize > limits.maximumTotalByteSize) {
        outRejectionReason = "ingest exceeds the total byte cap";
        return false;
    }
    if (entry.compressionMethod != compressionMethodStored &&
        entry.compressionMethod != compressionMethodDeflate) {
        outRejectionReason = "unsupported compression method";
        return false;
    }
    return true;
}

} // namespace

bool SanpackReader::ReadEntryPayload(const SanpackEntry& entry, const SanpackSafetyLimits& limits,
                                     std::uint64_t& runningTotalByteSize, SanpackPayload& outPayload) const {
    std::uint64_t payloadOffset = 0;
    if (!IsWithinLimits(entry, limits, runningTotalByteSize, outPayload.rejectionReason)) return false;
    if (!ResolvePayloadOffset(mappedData, mappedByteSize, entry, payloadOffset, outPayload.rejectionReason))
        return false;

    const unsigned char* source = mappedData + payloadOffset;
    const std::size_t declaredSize = static_cast<std::size_t>(entry.uncompressedByteSize);
    outPayload.bytes.resize(declaredSize);
    if (entry.compressionMethod == ZipFormat::compressionMethodStored) {
        if (entry.compressedByteSize != entry.uncompressedByteSize) {
            outPayload.rejectionReason = "stored entry size mismatch";
            return false;
        }
        if (declaredSize > 0) std::copy(source, source + declaredSize, outPayload.bytes.begin());
    } else {
        const std::size_t inflatedSize = tinfl_decompress_mem_to_mem(
            outPayload.bytes.data(), declaredSize, source,
            static_cast<std::size_t>(entry.compressedByteSize), 0);
        if (inflatedSize != declaredSize) {
            outPayload.rejectionReason = "inflate failed or produced the wrong length";
            return false;
        }
    }
    if (limits.bVerifyCrc32) {
        const mz_ulong actualCrc32 = mz_crc32(MZ_CRC32_INIT, outPayload.bytes.data(), declaredSize);
        if (static_cast<std::uint32_t>(actualCrc32) != entry.expectedCrc32) {
            outPayload.rejectionReason = "CRC-32 mismatch";
            return false;
        }
    }
    runningTotalByteSize += entry.uncompressedByteSize;
    return true;
}

} // namespace Io
} // namespace SanmapGen
