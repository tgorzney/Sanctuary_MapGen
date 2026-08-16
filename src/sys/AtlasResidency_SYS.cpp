// AtlasResidency_SYS.cpp — page uploads for the resident icon atlas. Every GPU touch goes
// through GpuResourceManager (EnsureTexture / UploadTexture / BindTextureSampler), so this file
// contains no GL symbol at all and the atlas inherits the manager's allocate-once behaviour:
// re-uploading the same page shape reallocates nothing.
#include "AtlasResidency_SYS.h"
#include <iostream>

namespace SanmapGen {
namespace Sys {

std::string AtlasResidency::PageTextureName(int pageIndex) const {
    return atlasName + "_page" + std::to_string(pageIndex);
}

bool AtlasResidency::UploadPage(GpuResourceManager& manager, int pageIndex, int width, int height,
                                const unsigned char* rgbaPixels, std::size_t byteSize) {
    const std::size_t requiredByteSize = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4;
    if (pageIndex < 0 || width <= 0 || height <= 0 || rgbaPixels == nullptr || byteSize < requiredByteSize) {
        std::cerr << "AtlasResidency: page " << pageIndex << " has no valid RGBA8 surface to upload.\n";
        return false;
    }
    const GpuTextureHandle texture =
        manager.EnsureTexture(PageTextureName(pageIndex), width, height, GpuTextureFormat::Rgba8);
    if (!texture.IsValid()) return false;
    manager.UploadTexture(texture, rgbaPixels, byteSize);
    if (static_cast<std::size_t>(pageIndex) >= pageTextures.size())
        pageTextures.resize(static_cast<std::size_t>(pageIndex) + 1);
    pageTextures[static_cast<std::size_t>(pageIndex)] = texture;
    return true;
}

GpuTextureHandle AtlasResidency::PageTexture(int pageIndex) const {
    if (pageIndex < 0 || static_cast<std::size_t>(pageIndex) >= pageTextures.size()) return GpuTextureHandle{};
    return pageTextures[static_cast<std::size_t>(pageIndex)];
}

bool AtlasResidency::BindPage(GpuResourceManager& manager, int pageIndex, unsigned textureUnit) const {
    const GpuTextureHandle texture = PageTexture(pageIndex);
    if (!texture.IsValid()) return false;
    manager.BindTextureSampler(texture, textureUnit);
    return true;
}

} // namespace Sys
} // namespace SanmapGen
