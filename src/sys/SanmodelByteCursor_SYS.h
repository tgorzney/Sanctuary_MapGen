// SanmodelByteCursor_SYS.h — bounds-checked little-endian byte cursor, private ARCH §1.5 aspect
// split off SanmodelRead_SYS.cpp (kept its own file to stay under the file-size ceiling). Layer:
// SYS. Same "refuse to read past the end" shape as Io::DiskFormat::ByteCursor
// (AssetAtlasCache_DiskFormat_IO.h), reproduced here rather than shared because IO may depend on
// SYS but never the reverse (ARCH §3.1).
#pragma once
#include <cstdint>
#include <cstring>
#include <string>

namespace SanmapGen {
namespace Sys {

class SanmodelByteCursor {
public:
    SanmodelByteCursor(const unsigned char* data, std::size_t byteSize) : data(data), byteSize(byteSize) {}
    std::size_t Remaining() const { return bGood ? byteSize - position : 0; }

    // Raw little-endian int32 bit pattern from the next 4 bytes — NEVER round/cast a float, per
    // the `.sanmodel` format's binding correctness requirement.
    bool ReadInt32Bits(std::int32_t& outValue) {
        if (!Require(4)) return false;
        std::memcpy(&outValue, data + position, 4);
        position += 4;
        return true;
    }
    bool ReadFloat(float& outValue) {
        if (!Require(4)) return false;
        std::memcpy(&outValue, data + position, 4);
        position += 4;
        return true;
    }
    // NUL-terminated string, consuming the NUL. False if the terminator is never found before the
    // buffer ends (a truncated/malformed file).
    bool ReadNulTerminatedString(std::string& outText) {
        outText.clear();
        while (bGood && position < byteSize) {
            const unsigned char byte = data[position++];
            if (byte == 0) return true;
            outText.push_back(static_cast<char>(byte));
        }
        bGood = false;
        return false;
    }
    // Skips `byteCount` bytes without copying (segments a caller discards).
    bool Skip(std::uint64_t byteCount) {
        if (byteCount > Remaining()) { bGood = false; return false; }
        position += static_cast<std::size_t>(byteCount);
        return true;
    }

private:
    bool Require(std::size_t byteCount) {
        if (!bGood || byteCount > byteSize - position) { bGood = false; return false; }
        return true;
    }
    const unsigned char* data;
    std::size_t byteSize;
    std::size_t position = 0;
    bool bGood = true;
};

} // namespace Sys
} // namespace SanmapGen
