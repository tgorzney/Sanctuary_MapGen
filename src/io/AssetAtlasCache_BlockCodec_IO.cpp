// AssetAtlasCache_BlockCodec_IO.cpp — DXT1/DXT5 4x4 block decode (GAMEDATA_LAYOUT_SPEC: the
// stored unit thumbnails are 64² DXT5, the strategic icons 112² DDS). Pure CPU pixel math for
// the atlas cache, kept in its own translation unit for the ARCH §1.5 ceiling. The required
// payload length is computed from the block grid and checked BEFORE any block is touched, so a
// truncated surface is rejected rather than read out of bounds (Constitution §6).
#include "AssetAtlasCache_Decode_IO.h"
#include <cstdint>

namespace SanmapGen {
namespace Io {
namespace Decode {

namespace {

struct ColorRgba { unsigned char red, green, blue, alpha; };

ColorRgba ExpandRgb565(std::uint16_t packed) {
    const int red = (packed >> 11) & 0x1f;
    const int green = (packed >> 5) & 0x3f;
    const int blue = packed & 0x1f;
    return ColorRgba{ static_cast<unsigned char>((red * 255 + 15) / 31),
                      static_cast<unsigned char>((green * 255 + 31) / 63),
                      static_cast<unsigned char>((blue * 255 + 15) / 31), 255 };
}

ColorRgba MixColors(ColorRgba first, ColorRgba second, int firstWeight, int secondWeight, int divisor) {
    return ColorRgba{
        static_cast<unsigned char>((first.red * firstWeight + second.red * secondWeight) / divisor),
        static_cast<unsigned char>((first.green * firstWeight + second.green * secondWeight) / divisor),
        static_cast<unsigned char>((first.blue * firstWeight + second.blue * secondWeight) / divisor),
        255 };
}

// The 4 colors a DXT color block addresses. DXT5 always uses the 4-color (opaque) mode.
void BuildColorTable(const unsigned char* block, bool bFourColorModeOnly, ColorRgba* outColors) {
    const std::uint16_t packedFirst = static_cast<std::uint16_t>(block[0] | (block[1] << 8));
    const std::uint16_t packedSecond = static_cast<std::uint16_t>(block[2] | (block[3] << 8));
    outColors[0] = ExpandRgb565(packedFirst);
    outColors[1] = ExpandRgb565(packedSecond);
    if (bFourColorModeOnly || packedFirst > packedSecond) {
        outColors[2] = MixColors(outColors[0], outColors[1], 2, 1, 3);
        outColors[3] = MixColors(outColors[0], outColors[1], 1, 2, 3);
    } else {
        outColors[2] = MixColors(outColors[0], outColors[1], 1, 1, 2);
        outColors[3] = ColorRgba{ 0, 0, 0, 0 };   // 1-bit alpha: the punch-through texel
    }
}

// The 8 alpha values a DXT5 alpha block addresses.
void BuildAlphaTable(const unsigned char* block, unsigned char* outAlpha) {
    outAlpha[0] = block[0];
    outAlpha[1] = block[1];
    if (outAlpha[0] > outAlpha[1]) {
        for (int step = 1; step < 7; ++step)
            outAlpha[step + 1] = static_cast<unsigned char>(((7 - step) * outAlpha[0] + step * outAlpha[1]) / 7);
    } else {
        for (int step = 1; step < 5; ++step)
            outAlpha[step + 1] = static_cast<unsigned char>(((5 - step) * outAlpha[0] + step * outAlpha[1]) / 5);
        outAlpha[6] = 0;
        outAlpha[7] = 255;
    }
}

void WriteBlockTexels(const unsigned char* colorBlock, const ColorRgba* colors, const unsigned char* alphaTable,
                      std::uint64_t alphaIndexBits, int blockX, int blockY, AtlasImage& outImage) {
    const std::uint32_t colorIndices = static_cast<std::uint32_t>(colorBlock[4]) |
        (static_cast<std::uint32_t>(colorBlock[5]) << 8) | (static_cast<std::uint32_t>(colorBlock[6]) << 16) |
        (static_cast<std::uint32_t>(colorBlock[7]) << 24);
    for (int texelY = 0; texelY < 4; ++texelY) {
        const int pixelY = blockY + texelY;
        if (pixelY >= outImage.height) break;
        for (int texelX = 0; texelX < 4; ++texelX) {
            const int pixelX = blockX + texelX;
            if (pixelX >= outImage.width) break;
            const int texel = texelY * 4 + texelX;
            ColorRgba color = colors[(colorIndices >> (2 * texel)) & 0x3u];
            if (alphaTable != nullptr)
                color.alpha = alphaTable[(alphaIndexBits >> (3 * texel)) & 0x7u];
            unsigned char* destination = outImage.rgbaPixels.data() +
                (static_cast<std::size_t>(pixelY) * outImage.width + pixelX) * AtlasImage::bytesPerPixel;
            destination[0] = color.red;
            destination[1] = color.green;
            destination[2] = color.blue;
            destination[3] = color.alpha;
        }
    }
}

} // namespace

bool DecodeBlockCompressedSurface(const unsigned char* blocks, std::size_t byteSize,
                                  int width, int height, bool bHasAlphaBlock, AtlasImage& outImage) {
    const int blockColumnCount = (width + 3) / 4;
    const int blockRowCount = (height + 3) / 4;
    const std::size_t blockByteSize = bHasAlphaBlock ? 16u : 8u;
    if (blocks == nullptr ||
        byteSize < static_cast<std::size_t>(blockColumnCount) * blockRowCount * blockByteSize) return false;
    outImage.width = width;
    outImage.height = height;
    outImage.rgbaPixels.assign(outImage.ExpectedByteSize(), 0);
    unsigned char alphaTable[8] = {};
    for (int blockRow = 0; blockRow < blockRowCount; ++blockRow) {
        for (int blockColumn = 0; blockColumn < blockColumnCount; ++blockColumn) {
            const unsigned char* block =
                blocks + (static_cast<std::size_t>(blockRow) * blockColumnCount + blockColumn) * blockByteSize;
            std::uint64_t alphaIndexBits = 0;
            if (bHasAlphaBlock) {
                BuildAlphaTable(block, alphaTable);
                for (int byteIndex = 0; byteIndex < 6; ++byteIndex)
                    alphaIndexBits |= static_cast<std::uint64_t>(block[2 + byteIndex]) << (8 * byteIndex);
            }
            const unsigned char* colorBlock = block + (bHasAlphaBlock ? 8 : 0);
            ColorRgba colors[4];
            BuildColorTable(colorBlock, bHasAlphaBlock, colors);
            WriteBlockTexels(colorBlock, colors, bHasAlphaBlock ? alphaTable : nullptr, alphaIndexBits,
                             blockColumn * 4, blockRow * 4, outImage);
        }
    }
    return true;
}

} // namespace Decode
} // namespace Io
} // namespace SanmapGen
