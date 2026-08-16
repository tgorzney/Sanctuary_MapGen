// AssetAtlasCache_Pack_IO.cpp — shelf packing of the decoded images into large RGBA8 pages
// (ASSET_LOADING_SPEC: "thousands of small icons collapse into a handful of atlas pages").
// Images are placed tallest-first so shelves stay tight, and ties break on the name so the same
// input always produces the same atlas — a packing that wobbled between runs would invalidate
// its own disk cache. The page count cap is a low-VRAM safety valve, not an expected path.
#include "AssetAtlasCache_IO.h"
#include <algorithm>
#include <iostream>
#include <numeric>

namespace SanmapGen {
namespace Io {

namespace {

struct ShelfCursor {
    int pageIndex = -1;
    int cursorX = 0;
    int shelfY = 0;
    int shelfHeight = 0;
};

void BlitImage(const AtlasImage& source, AtlasImage& page, int destinationX, int destinationY) {
    const std::size_t rowByteSize = static_cast<std::size_t>(source.width) * AtlasImage::bytesPerPixel;
    for (int row = 0; row < source.height; ++row) {
        const unsigned char* sourceRow = source.rgbaPixels.data() + static_cast<std::size_t>(row) * rowByteSize;
        unsigned char* destinationRow = page.rgbaPixels.data() +
            (static_cast<std::size_t>(destinationY + row) * page.width + destinationX) * AtlasImage::bytesPerPixel;
        std::copy(sourceRow, sourceRow + rowByteSize, destinationRow);
    }
}

AtlasImage MakeEmptyPage(int width, int height) {
    AtlasImage page;
    page.width = width;
    page.height = height;
    page.rgbaPixels.assign(page.ExpectedByteSize(), 0);
    return page;
}

} // namespace

bool AssetAtlasCache::PackImages(const std::vector<AtlasImage>& images, const std::vector<std::string>& names,
                                 const std::vector<unsigned char>& placeholderFlags,
                                 const AtlasBuildSettings& settings, AtlasBuildReport& outReport) {
    atlas.Clear();
    if (settings.pageWidth <= 0 || settings.pageHeight <= 0) return false;
    std::vector<int> order(images.size());
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](int left, int right) {
        const AtlasImage& first = images[static_cast<std::size_t>(left)];
        const AtlasImage& second = images[static_cast<std::size_t>(right)];
        if (first.height != second.height) return first.height > second.height;
        if (first.width != second.width) return first.width > second.width;
        return names[static_cast<std::size_t>(left)] < names[static_cast<std::size_t>(right)];
    });

    ShelfCursor cursor;
    const int padding = settings.entryPaddingPixels > 0 ? settings.entryPaddingPixels : 0;
    for (const int index : order) {
        const AtlasImage& image = images[static_cast<std::size_t>(index)];
        if (!image.IsValid() || image.width > settings.pageWidth || image.height > settings.pageHeight) {
            std::cerr << "AssetAtlasCache: '" << names[static_cast<std::size_t>(index)]
                      << "' does not fit an atlas page and was skipped.\n";
            continue;
        }
        const int steppedWidth = image.width + padding;
        const int steppedHeight = image.height + padding;
        if (cursor.pageIndex < 0 || cursor.cursorX + steppedWidth > settings.pageWidth) {
            cursor.cursorX = 0;                                     // next shelf
            cursor.shelfY += cursor.shelfHeight;
            cursor.shelfHeight = 0;
        }
        if (cursor.pageIndex < 0 || cursor.shelfY + steppedHeight > settings.pageHeight) {
            if (atlas.PageCount() >= settings.maximumPageCount) {
                std::cerr << "AssetAtlasCache: page cap (" << settings.maximumPageCount
                          << ") reached; remaining icons are not atlased.\n";
                break;
            }
            atlas.MutablePages().push_back(MakeEmptyPage(settings.pageWidth, settings.pageHeight));
            cursor = ShelfCursor{ atlas.PageCount() - 1, 0, 0, 0 };
        }
        AtlasImage& page = atlas.MutablePages()[static_cast<std::size_t>(cursor.pageIndex)];
        BlitImage(image, page, cursor.cursorX, cursor.shelfY);
        AtlasEntry entry;
        entry.name = names[static_cast<std::size_t>(index)];
        entry.pageIndex = cursor.pageIndex;
        entry.pixelX = cursor.cursorX;
        entry.pixelY = cursor.shelfY;
        entry.width = image.width;
        entry.height = image.height;
        entry.bPlaceholder = placeholderFlags[static_cast<std::size_t>(index)] != 0;
        atlas.AddEntryWithDerivedUv(entry, page.width, page.height);
        cursor.cursorX += steppedWidth;
        if (steppedHeight > cursor.shelfHeight) cursor.shelfHeight = steppedHeight;
    }
    outReport.packedEntryCount = atlas.EntryCount();
    return true;
}

} // namespace Io
} // namespace SanmapGen
