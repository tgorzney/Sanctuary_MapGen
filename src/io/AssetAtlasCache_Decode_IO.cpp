// AssetAtlasCache_Decode_IO.cpp — the .dds header gate and the uncompressed path. The block
// codecs live in AssetAtlasCache_BlockDecode_IO.cpp and the placeholder/prop fallbacks in
// AssetAtlasCache_PropThumbnail_IO.cpp (ARCH §1.5). Validate-then-default-then-log
// (Constitution §6): every rejection names its reason and yields no pixels at all.
#include "AssetAtlasCache_Decode_IO.h"
#include <cstdint>

namespace SanmapGen {
namespace Io {
namespace Decode {

namespace {

constexpr std::uint32_t directDrawSurfaceMagic = 0x20534444u;   // 'DDS '
constexpr std::uint32_t fourCharacterCodeDxt1  = 0x31545844u;   // 'DXT1'
constexpr std::uint32_t fourCharacterCodeDxt5  = 0x35545844u;   // 'DXT5'
constexpr std::size_t   surfaceHeaderByteSize  = 128;           // magic + 124-byte DDS_HEADER
constexpr std::uint32_t pixelFormatFlagFourCharacterCode = 0x4u;
constexpr std::uint32_t pixelFormatFlagRgb = 0x40u;

std::uint32_t ReadUnsigned32(const unsigned char* at) {
    return static_cast<std::uint32_t>(at[0]) | (static_cast<std::uint32_t>(at[1]) << 8) |
           (static_cast<std::uint32_t>(at[2]) << 16) | (static_cast<std::uint32_t>(at[3]) << 24);
}

// 32-bit uncompressed surfaces: the channel order comes from the masks, so both the common
// A8R8G8B8 (BGRA in memory) and R8G8B8A8 layouts decode correctly instead of being guessed.
bool DecodeUncompressedSurface(const unsigned char* pixels, std::size_t byteSize, int width, int height,
                               std::uint32_t redMask, std::uint32_t blueMask, AtlasImage& outImage) {
    const std::size_t texelCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    if (byteSize < texelCount * 4) return false;
    const bool bBlueFirst = (blueMask & 0xffu) != 0 && (redMask & 0x00ff0000u) != 0;
    outImage.rgbaPixels.resize(texelCount * AtlasImage::bytesPerPixel);
    for (std::size_t texel = 0; texel < texelCount; ++texel) {
        const unsigned char* source = pixels + texel * 4;
        unsigned char* destination = outImage.rgbaPixels.data() + texel * 4;
        destination[0] = bBlueFirst ? source[2] : source[0];
        destination[1] = source[1];
        destination[2] = bBlueFirst ? source[0] : source[2];
        destination[3] = source[3];
    }
    return true;
}

} // namespace

bool DecodeDirectDrawSurface(const unsigned char* bytes, std::size_t byteSize,
                             int maximumWidth, int maximumHeight,
                             AtlasImage& outImage, std::string& outRejectionReason) {
    outImage = AtlasImage{};
    if (bytes == nullptr || byteSize < surfaceHeaderByteSize) {
        outRejectionReason = "shorter than a .dds header";
        return false;
    }
    if (ReadUnsigned32(bytes) != directDrawSurfaceMagic || ReadUnsigned32(bytes + 4) != 124) {
        outRejectionReason = "not a .dds surface";
        return false;
    }
    const std::uint32_t height = ReadUnsigned32(bytes + 12);
    const std::uint32_t width = ReadUnsigned32(bytes + 16);
    if (width == 0 || height == 0 || width > static_cast<std::uint32_t>(maximumWidth) ||
        height > static_cast<std::uint32_t>(maximumHeight)) {
        outRejectionReason = "dimensions are zero or past the cap";
        return false;
    }
    outImage.width = static_cast<int>(width);
    outImage.height = static_cast<int>(height);

    const std::uint32_t pixelFormatFlags = ReadUnsigned32(bytes + 80);
    const std::uint32_t fourCharacterCode = ReadUnsigned32(bytes + 84);
    const unsigned char* payload = bytes + surfaceHeaderByteSize;
    const std::size_t payloadByteSize = byteSize - surfaceHeaderByteSize;
    bool bDecoded = false;
    if ((pixelFormatFlags & pixelFormatFlagFourCharacterCode) != 0 &&
        (fourCharacterCode == fourCharacterCodeDxt1 || fourCharacterCode == fourCharacterCodeDxt5)) {
        bDecoded = DecodeBlockCompressedSurface(payload, payloadByteSize, outImage.width, outImage.height,
                                                fourCharacterCode == fourCharacterCodeDxt5, outImage);
    } else if ((pixelFormatFlags & pixelFormatFlagRgb) != 0 && ReadUnsigned32(bytes + 88) == 32) {
        bDecoded = DecodeUncompressedSurface(payload, payloadByteSize, outImage.width, outImage.height,
                                             ReadUnsigned32(bytes + 92), ReadUnsigned32(bytes + 100), outImage);
    } else {
        outRejectionReason = "unsupported .dds pixel format";
        outImage = AtlasImage{};
        return false;
    }
    if (!bDecoded || !outImage.IsValid()) {
        outRejectionReason = "surface payload is truncated";
        outImage = AtlasImage{};
        return false;
    }
    return true;
}

} // namespace Decode
} // namespace Io
} // namespace SanmapGen
