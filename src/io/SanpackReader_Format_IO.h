// SanpackReader_Format_IO.h — the raw ZIP record layout SanpackReader parses out of the
// memory-mapped sanpack. Private detail of SanpackReader_IO (ARCH §1.5 aspect split); no
// other type and no other layer includes it. Pure field offsets and bounds arithmetic — it
// owns no memory and reads nothing on its own, so every accessor here is safe only after the
// caller has proven the record fits inside the mapped view with FitsInsideMapping
// (Constitution §6: validate before you dereference).
#pragma once
#include <cstddef>
#include <cstdint>

namespace SanmapGen {
namespace Io {
namespace ZipFormat {

constexpr std::uint32_t endOfCentralDirectorySignature       = 0x06054b50u;
constexpr std::uint32_t zip64LocatorSignature                = 0x07064b50u;
constexpr std::uint32_t zip64EndOfCentralDirectorySignature  = 0x06064b50u;
constexpr std::uint32_t centralFileHeaderSignature           = 0x02014b50u;
constexpr std::uint32_t localFileHeaderSignature             = 0x04034b50u;

constexpr std::uint64_t endOfCentralDirectoryByteSize = 22;
constexpr std::uint64_t zip64LocatorByteSize          = 20;
constexpr std::uint64_t zip64DirectoryByteSize        = 56;
constexpr std::uint64_t centralFileHeaderByteSize     = 46;
constexpr std::uint64_t localFileHeaderByteSize       = 30;

// The archive comment is a 16-bit length, so the record starts at most 64 KiB + 22 from the end.
constexpr std::uint64_t maximumArchiveCommentByteSize = 0xffffull;

constexpr std::uint16_t compressionMethodStored  = 0;
constexpr std::uint16_t compressionMethodDeflate = 8;

// A 32-bit field holding this value means "the real value is in the zip64 extra field".
constexpr std::uint32_t zip64FieldSentinel = 0xffffffffu;

inline std::uint16_t ReadUnsigned16(const unsigned char* at) {
    return static_cast<std::uint16_t>(at[0] | (at[1] << 8));
}
inline std::uint32_t ReadUnsigned32(const unsigned char* at) {
    return static_cast<std::uint32_t>(at[0]) | (static_cast<std::uint32_t>(at[1]) << 8) |
           (static_cast<std::uint32_t>(at[2]) << 16) | (static_cast<std::uint32_t>(at[3]) << 24);
}
inline std::uint64_t ReadUnsigned64(const unsigned char* at) {
    return static_cast<std::uint64_t>(ReadUnsigned32(at)) |
           (static_cast<std::uint64_t>(ReadUnsigned32(at + 4)) << 32);
}

// The one bounds test every parse step goes through. Written without an addition so a hostile
// offset + size pair cannot wrap around and report "fits".
inline bool FitsInsideMapping(std::uint64_t offset, std::uint64_t byteCount, std::uint64_t mappedByteSize) {
    return byteCount <= mappedByteSize && offset <= mappedByteSize - byteCount;
}

} // namespace ZipFormat
} // namespace Io
} // namespace SanmapGen
