// AssetAtlasCache_Atlas_IO.h — the PRODUCT of the atlas cache: CPU-side RGBA8 page images plus
// the `name -> {page, uv-rect}` manifest (ASSET_LOADING_SPEC "Atlas build"). Split out of
// AssetAtlasCache_IO.h for the ARCH §1.5 ceiling; it is the cache's own result type, not an
// independently reachable one. Deliberately holds NO GPU handle: residency is SYS's job
// (ARCH §3.2) — a caller hands these pixels to Sys::AtlasResidency, which uploads them through
// GpuResource_SYS, and the UI then samples the page by the uv-rect recorded here.
#pragma once
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace SanmapGen {
namespace Io {

// One tightly packed RGBA8 surface: a decoded icon while building, an atlas page once packed.
struct AtlasImage {
    int width = 0;
    int height = 0;
    std::vector<unsigned char> rgbaPixels;
    static constexpr int bytesPerPixel = 4;
    std::size_t ExpectedByteSize() const {
        return static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * bytesPerPixel;
    }
    bool IsValid() const { return width > 0 && height > 0 && rgbaPixels.size() == ExpectedByteSize(); }
};

// Where one named asset ended up. The uv-rect is derived from the page size at insert time, so
// the sampler never repeats that division and a reloaded manifest cannot drift from its pages.
struct AtlasEntry {
    std::string name;
    int pageIndex = -1;
    int pixelX = 0;
    int pixelY = 0;
    int width = 0;
    int height = 0;
    float uvMinimumX = 0.0f;
    float uvMinimumY = 0.0f;
    float uvMaximumX = 0.0f;
    float uvMaximumY = 0.0f;
    bool bPlaceholder = false;   // the source entry failed validation (Constitution §6)
};

class AssetAtlas {
public:
    const AtlasEntry* Find(const std::string& name) const {
        const std::unordered_map<std::string, int>::const_iterator found = nameToEntryIndex.find(name);
        return found == nameToEntryIndex.end() ? nullptr : &entries[static_cast<std::size_t>(found->second)];
    }
    const std::vector<AtlasEntry>& Entries() const { return entries; }
    const std::vector<AtlasImage>& Pages() const { return pages; }
    int PageCount() const { return static_cast<int>(pages.size()); }
    int EntryCount() const { return static_cast<int>(entries.size()); }
    bool IsEmpty() const { return entries.empty(); }

    void Clear() { pages.clear(); entries.clear(); nameToEntryIndex.clear(); }
    // Build-time mutators, used by the packer and by the disk-cache loader only.
    std::vector<AtlasImage>& MutablePages() { return pages; }
    void AddEntry(const AtlasEntry& entry) {
        nameToEntryIndex[entry.name] = static_cast<int>(entries.size());
        entries.push_back(entry);
    }
    // The single uv derivation, so the packer and the loader cannot disagree about it.
    void AddEntryWithDerivedUv(AtlasEntry entry, int pageWidth, int pageHeight) {
        const float pageWidthReciprocal = pageWidth > 0 ? 1.0f / static_cast<float>(pageWidth) : 0.0f;
        const float pageHeightReciprocal = pageHeight > 0 ? 1.0f / static_cast<float>(pageHeight) : 0.0f;
        entry.uvMinimumX = static_cast<float>(entry.pixelX) * pageWidthReciprocal;
        entry.uvMinimumY = static_cast<float>(entry.pixelY) * pageHeightReciprocal;
        entry.uvMaximumX = static_cast<float>(entry.pixelX + entry.width) * pageWidthReciprocal;
        entry.uvMaximumY = static_cast<float>(entry.pixelY + entry.height) * pageHeightReciprocal;
        AddEntry(entry);
    }

private:
    std::vector<AtlasImage> pages;
    std::vector<AtlasEntry> entries;
    std::unordered_map<std::string, int> nameToEntryIndex;
};

} // namespace Io
} // namespace SanmapGen
