// AssetAtlasCache_DiskFormat_IO.h — the on-disk layout of the cached atlas, private to the
// atlas cache (ARCH §1.5 aspect split). Two files, deliberately (ASSET_LOADING_SPEC "Disk
// cache"): a small MANIFEST (fingerprint + page dimensions + `name -> {page, rect}`) and the
// PAGE BLOB (raw RGBA8 pages). Splitting them is the whole point of the fingerprint check —
// a launch that finds a stale cache reads a few kilobytes of manifest and never touches the
// multi-megabyte blob. Little-endian fixed-width fields, written and read through these
// helpers so the two sides cannot drift.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace SanmapGen {
namespace Io {
namespace DiskFormat {

constexpr std::uint32_t manifestMagic = 0x4d414e53u;   // 'SNAM'
constexpr std::uint32_t pageBlobMagic = 0x47504153u;   // 'SAPG'
constexpr std::uint32_t formatVersion = 1;

inline void AppendUnsigned32(std::vector<unsigned char>& bytes, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8) bytes.push_back(static_cast<unsigned char>(value >> shift));
}
inline void AppendUnsigned64(std::vector<unsigned char>& bytes, std::uint64_t value) {
    for (int shift = 0; shift < 64; shift += 8) bytes.push_back(static_cast<unsigned char>(value >> shift));
}
inline void AppendText(std::vector<unsigned char>& bytes, const std::string& text) {
    AppendUnsigned32(bytes, static_cast<std::uint32_t>(text.size()));
    bytes.insert(bytes.end(), text.begin(), text.end());
}

// A cursor that refuses to read past the end of the buffer: a truncated or hostile cache file
// fails the load and triggers a rebuild instead of reading out of bounds (Constitution §6).
class ByteCursor {
public:
    ByteCursor(const unsigned char* data, std::size_t byteSize) : data(data), byteSize(byteSize) {}
    bool IsGood() const { return bGood; }
    std::size_t Remaining() const { return bGood ? byteSize - position : 0; }
    std::uint32_t ReadUnsigned32() {
        if (!Require(4)) return 0;
        std::uint32_t value = 0;
        for (int index = 0; index < 4; ++index)
            value |= static_cast<std::uint32_t>(data[position + index]) << (8 * index);
        position += 4;
        return value;
    }
    std::uint64_t ReadUnsigned64() {
        const std::uint64_t low = ReadUnsigned32();
        const std::uint64_t high = ReadUnsigned32();
        return low | (high << 32);
    }
    std::string ReadText() {
        const std::uint32_t length = ReadUnsigned32();
        if (!Require(length)) return std::string();
        std::string text(reinterpret_cast<const char*>(data + position), length);
        position += length;
        return text;
    }
    const unsigned char* ReadBlock(std::size_t blockByteSize) {
        if (!Require(blockByteSize)) return nullptr;
        const unsigned char* block = data + position;
        position += blockByteSize;
        return block;
    }

private:
    bool Require(std::size_t byteCount) {
        if (!bGood || byteCount > byteSize - position) { bGood = false; return false; }
        return true;
    }
    const unsigned char* data = nullptr;
    std::size_t byteSize = 0;
    std::size_t position = 0;
    bool bGood = true;
};

} // namespace DiskFormat
} // namespace Io
} // namespace SanmapGen
