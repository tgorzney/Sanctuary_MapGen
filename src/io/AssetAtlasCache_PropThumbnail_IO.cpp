// AssetAtlasCache_PropThumbnail_IO.cpp — the two "no valid source image" producers.
//  * MakePlaceholderImage is the Constitution §6 fallback: a corrupt/rejected entry still gets
//    an atlas slot, so the UI shows an obviously-missing tile instead of crashing or blanking.
//  * RenderPropThumbnail is the ASSET_LOADING_SPEC prop path: prop folders ship heavy 3D assets
//    and NO stored preview, so SanGen must produce one on demand and cache it into the same
//    disk atlas. Implemented here as a deterministic flat-shaded stand-in derived from the model
//    bytes — the PLUMBING (render -> pack -> cache -> reload) is what M5-4 owns; thumbnail
//    render QUALITY is explicitly out of scope for this work-order and a mesh rasterizer is not
//    smuggled in ahead of its own order (ARCH §8.4).
#include "AssetAtlasCache_Decode_IO.h"
#include <cstdint>

namespace SanmapGen {
namespace Io {
namespace Decode {

namespace {

void WriteTexel(AtlasImage& image, int pixelX, int pixelY, unsigned char red, unsigned char green,
                unsigned char blue, unsigned char alpha) {
    unsigned char* destination = image.rgbaPixels.data() +
        (static_cast<std::size_t>(pixelY) * image.width + pixelX) * AtlasImage::bytesPerPixel;
    destination[0] = red;
    destination[1] = green;
    destination[2] = blue;
    destination[3] = alpha;
}

// A cheap order-independent digest of the model bytes: the thumbnail of a given prop is the
// same image on every machine and every run, which is what makes it cacheable.
std::uint32_t DigestOfBytes(const unsigned char* bytes, std::size_t byteSize) {
    std::uint32_t digest = 2166136261u;
    for (std::size_t index = 0; index < byteSize; ++index) {
        digest ^= bytes[index];
        digest *= 16777619u;
    }
    return digest;
}

} // namespace

AtlasImage MakePlaceholderImage(int width, int height) {
    AtlasImage image;
    image.width = width > 0 ? width : 1;
    image.height = height > 0 ? height : 1;
    image.rgbaPixels.assign(image.ExpectedByteSize(), 0);
    const int checkerSize = 4;
    for (int pixelY = 0; pixelY < image.height; ++pixelY) {
        for (int pixelX = 0; pixelX < image.width; ++pixelX) {
            const bool bMagentaCell = ((pixelX / checkerSize) + (pixelY / checkerSize)) % 2 == 0;
            WriteTexel(image, pixelX, pixelY, bMagentaCell ? 255 : 0, 0, bMagentaCell ? 255 : 0, 255);
        }
    }
    return image;
}

AtlasImage RenderPropThumbnail(const unsigned char* modelBytes, std::size_t byteSize,
                               int width, int height) {
    if (modelBytes == nullptr || byteSize == 0) return MakePlaceholderImage(width, height);
    AtlasImage image;
    image.width = width > 0 ? width : 1;
    image.height = height > 0 ? height : 1;
    image.rgbaPixels.assign(image.ExpectedByteSize(), 0);

    const std::uint32_t digest = DigestOfBytes(modelBytes, byteSize);
    const unsigned char baseRed = static_cast<unsigned char>(64 + (digest & 0x7fu));
    const unsigned char baseGreen = static_cast<unsigned char>(64 + ((digest >> 8) & 0x7fu));
    const unsigned char baseBlue = static_cast<unsigned char>(64 + ((digest >> 16) & 0x7fu));
    const float halfWidth = 0.5f * static_cast<float>(image.width);
    const float halfHeight = 0.5f * static_cast<float>(image.height);
    const float radius = (halfWidth < halfHeight ? halfWidth : halfHeight) - 1.0f;
    const float radiusReciprocal = radius > 0.0f ? 1.0f / radius : 0.0f;
    for (int pixelY = 0; pixelY < image.height; ++pixelY) {
        const float offsetY = static_cast<float>(pixelY) + 0.5f - halfHeight;
        for (int pixelX = 0; pixelX < image.width; ++pixelX) {
            const float offsetX = static_cast<float>(pixelX) + 0.5f - halfWidth;
            const float distance = (offsetX * offsetX + offsetY * offsetY) * radiusReciprocal * radiusReciprocal;
            if (distance > 1.0f) continue;                       // transparent outside the silhouette
            const float shade = 1.0f - 0.5f * distance;          // flat lambert-ish falloff
            WriteTexel(image, pixelX, pixelY, static_cast<unsigned char>(baseRed * shade),
                       static_cast<unsigned char>(baseGreen * shade),
                       static_cast<unsigned char>(baseBlue * shade), 255);
        }
    }
    return image;
}

} // namespace Decode
} // namespace Io
} // namespace SanmapGen
