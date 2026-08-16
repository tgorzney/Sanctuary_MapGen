// AtlasResidency_SYS.h — the RESIDENT icon atlas the UI samples (M5-4 / ASSET_LOADING_SPEC).
// Layer: SYS, because GPU residency is SYS's job and a GL handle may live nowhere else
// (Constitution §1, ARCH §3.2). It owns nothing but a page-indexed list of opaque
// GpuTextureHandles and drives them through GpuResourceManager's public API — there is no GL
// call in this file, and none in the IO layer that produced the pixels.
//
// Deliberately takes RAW pixel spans, not an Io type: SYS must not depend on IO (ARCH §3.1), so
// the seam is `(width, height, bytes)`. The caller — the app shell / UI, which legally sees both
// layers — walks Io::AssetAtlas::Pages() and uploads each page here, then samples it with the
// uv-rect from the same atlas's manifest. Uploading is one-way: nothing reads back through here.
#pragma once
#include "GpuResource_SYS.h"
#include <cstddef>
#include <string>
#include <vector>

namespace SanmapGen {
namespace Sys {

class AtlasResidency {
public:
    explicit AtlasResidency(std::string atlasName = "assetAtlas") : atlasName(std::move(atlasName)) {}

    // Uploads one tightly packed RGBA8 page. The texture is keyed by atlas name + page index, so
    // a rebuilt atlas of the same shape re-uses the same texture instead of leaking a new one.
    // Rejects a null/short buffer or a non-positive size rather than uploading garbage.
    bool UploadPage(GpuResourceManager& manager, int pageIndex, int width, int height,
                    const unsigned char* rgbaPixels, std::size_t byteSize);

    // Invalid handle for a page that was never uploaded — callers check IsValid().
    GpuTextureHandle PageTexture(int pageIndex) const;
    int PageCount() const { return static_cast<int>(pageTextures.size()); }

    // What the UI actually calls per draw: bind the page a manifest entry named, then sample it
    // with that entry's uv-rect. False when the page is not resident (draw the placeholder).
    bool BindPage(GpuResourceManager& manager, int pageIndex, unsigned textureUnit) const;

    // Drops the handle list. The textures themselves stay owned by GpuResourceManager, which is
    // the single owner of every GL object (ARCH §3.3).
    void Clear() { pageTextures.clear(); }

    std::string PageTextureName(int pageIndex) const;

private:
    std::string atlasName;
    std::vector<GpuTextureHandle> pageTextures;
};

} // namespace Sys
} // namespace SanmapGen
