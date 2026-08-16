// AssetAtlasCache_IO_Test.cpp — M5-4 acceptance, parts 2-4: the atlas manifest resolves a known
// icon to the right page + uv, the disk cache round-trips (a second load with a matching
// fingerprint skips extraction entirely), and every corrupt entry becomes a placeholder without
// taking the build down. Pages are deliberately small here (the shipped default is 4096²) so
// the test writes kilobytes, not megabytes.
#include "AssetPipeline_TestSupport_IO.h"
#include "AssetAtlasCache_IO.h"
#include "../sys/ThreadPool_SYS.h"
#include <cmath>
#include <filesystem>

using namespace SanmapGen;
using namespace SanmapGen::Io;
using namespace AssetPipelineTest;

namespace {

AtlasBuildSettings TestSettings() {
    AtlasBuildSettings settings;
    settings.pageWidth = 256;
    settings.pageHeight = 256;
    settings.propThumbnailWidth = 32;
    settings.propThumbnailHeight = 32;
    return settings;
}

SanpackEntryFilter TestFilter() {
    SanpackEntryFilter filter;
    filter.extensions = { ".dds", ".sanmodel" };
    return filter;
}

bool TexelEquals(const AssetAtlas& atlas, const AtlasEntry& entry, int offsetX, int offsetY,
                 unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha) {
    if (entry.pageIndex < 0 || entry.pageIndex >= atlas.PageCount()) return false;
    const AtlasImage& page = atlas.Pages()[static_cast<std::size_t>(entry.pageIndex)];
    const std::size_t texel = (static_cast<std::size_t>(entry.pixelY + offsetY) * page.width +
                               entry.pixelX + offsetX) * AtlasImage::bytesPerPixel;
    if (texel + 3 >= page.rgbaPixels.size()) return false;
    return page.rgbaPixels[texel] == red && page.rgbaPixels[texel + 1] == green &&
           page.rgbaPixels[texel + 2] == blue && page.rgbaPixels[texel + 3] == alpha;
}

bool UvMatchesPixelRect(const AssetAtlas& atlas, const AtlasEntry& entry) {
    const AtlasImage& page = atlas.Pages()[static_cast<std::size_t>(entry.pageIndex)];
    const float tolerance = 1.0e-6f;
    return std::fabs(entry.uvMinimumX - static_cast<float>(entry.pixelX) / page.width) < tolerance &&
           std::fabs(entry.uvMinimumY - static_cast<float>(entry.pixelY) / page.height) < tolerance &&
           std::fabs(entry.uvMaximumX - static_cast<float>(entry.pixelX + entry.width) / page.width) < tolerance &&
           std::fabs(entry.uvMaximumY - static_cast<float>(entry.pixelY + entry.height) / page.height) < tolerance;
}

void CheckPlaceholder(const AssetAtlas& atlas, const std::string& name, const char* label) {
    const AtlasEntry* entry = atlas.Find(name);
    Check(entry != nullptr && entry->bPlaceholder, label);
}

} // namespace

namespace AssetPipelineTest {

void RunAtlasCacheChecks(const SyntheticSanpack& layout, const std::string& scratchDirectory) {
    const std::string cacheDirectory = (std::filesystem::path(scratchDirectory) / "atlasCache").string();
    std::error_code errorCode;
    std::filesystem::remove_all(cacheDirectory, errorCode);
    const AtlasBuildSettings settings = TestSettings();
    Sys::ThreadPool workerPool(4);

    // --- cold build -------------------------------------------------------------------------
    AssetAtlasCache coldCache;
    AtlasBuildReport coldReport;
    Check(coldCache.BuildOrLoad(layout.sanpackPath, cacheDirectory, TestFilter(), settings, coldReport,
                                &workerPool), "cold build succeeds");
    Check(coldReport.bExtractionRan && !coldReport.bLoadedFromDiskCache, "cold build ran the extraction");
    Check(coldReport.bWroteDiskCache, "cold build wrote the disk cache");
    Check(coldReport.sanpackStatistics.centralDirectoryReadCount == 1, "one central-directory read");
    Check(coldReport.decodedIconCount == 2, "both intact DXT5 icons decoded");
    Check(coldReport.renderedPropThumbnailCount == 1, "the prop model got a rendered thumbnail");
    Check(coldReport.placeholderCount == 3, "CRC-damaged, non-dds and oversize entries became placeholders");
    Check(coldReport.packedEntryCount == 6, "every accepted entry has an atlas slot");

    // --- the manifest resolves a known icon to the right page + uv ---------------------------
    const AtlasEntry* icon = coldCache.Atlas().Find(layout.validIconName);
    Check(icon != nullptr, "manifest resolves the known unit icon by name");
    if (icon != nullptr) {
        Check(icon->width == layout.iconWidth && icon->height == layout.iconHeight, "icon keeps its size");
        Check(icon->pageIndex >= 0 && icon->pageIndex < coldCache.Atlas().PageCount(), "icon page is real");
        Check(!icon->bPlaceholder, "the intact icon is not a placeholder");
        Check(UvMatchesPixelRect(coldCache.Atlas(), *icon), "uv-rect matches the packed pixel rect");
        Check(TexelEquals(coldCache.Atlas(), *icon, 1, 1, 255, 0, 0, 255),
              "the icon's pixels really are at that page+uv (flat red DXT5 decoded)");
        Check(TexelEquals(coldCache.Atlas(), *icon, layout.iconWidth - 1, layout.iconHeight - 1,
                          255, 0, 0, 255), "the icon's far corner landed inside its rect too");
    }
    const AtlasEntry* secondIcon = coldCache.Atlas().Find(layout.secondIconName);
    Check(secondIcon != nullptr && TexelEquals(coldCache.Atlas(), *secondIcon, 1, 1, 0, 0, 255, 255),
          "the second icon decoded to its own colour in its own rect");
    const AtlasEntry* propThumbnail = coldCache.Atlas().Find(layout.propModelName);
    Check(propThumbnail != nullptr && propThumbnail->width == settings.propThumbnailWidth,
          "the prop thumbnail was packed at the configured size");

    // --- corrupt entries degrade to placeholders, no crash ------------------------------------
    CheckPlaceholder(coldCache.Atlas(), layout.crcDamagedIconName, "CRC-damaged entry became a placeholder");
    CheckPlaceholder(coldCache.Atlas(), layout.badHeaderIconName, "non-.dds entry became a placeholder");
    CheckPlaceholder(coldCache.Atlas(), layout.oversizeIconName, "oversize surface became a placeholder");

    // --- warm load: matching fingerprint, extraction must NOT run -----------------------------
    AssetAtlasCache warmCache;
    AtlasBuildReport warmReport;
    Check(warmCache.BuildOrLoad(layout.sanpackPath, cacheDirectory, TestFilter(), settings, warmReport,
                                &workerPool), "warm load succeeds");
    Check(warmReport.bLoadedFromDiskCache, "warm load came from the disk cache");
    Check(!warmReport.bExtractionRan, "warm load skipped extraction entirely");
    Check(warmReport.sanpackStatistics.centralDirectoryReadCount == 0, "the sanpack was never even opened");
    Check(warmCache.Atlas().EntryCount() == coldCache.Atlas().EntryCount(), "same entry count round-tripped");
    Check(warmCache.Atlas().PageCount() == coldCache.Atlas().PageCount(), "same page count round-tripped");
    const AtlasEntry* warmIcon = warmCache.Atlas().Find(layout.validIconName);
    Check(warmIcon != nullptr && icon != nullptr && warmIcon->pageIndex == icon->pageIndex &&
          warmIcon->pixelX == icon->pixelX && warmIcon->pixelY == icon->pixelY,
          "the reloaded manifest resolves the icon to the same page and rect");
    Check(warmIcon != nullptr && UvMatchesPixelRect(warmCache.Atlas(), *warmIcon), "reloaded uv-rect holds");
    Check(warmIcon != nullptr && TexelEquals(warmCache.Atlas(), *warmIcon, 1, 1, 255, 0, 0, 255),
          "the reloaded page blob carries the same pixels");
    CheckPlaceholder(warmCache.Atlas(), layout.crcDamagedIconName, "placeholder flag survived the round trip");

    // --- a changed sanpack must invalidate the fingerprint and rebuild -------------------------
    SyntheticSanpack changedLayout = layout;
    changedLayout.iconWidth = 32;
    changedLayout.iconHeight = 32;
    Check(WriteSyntheticSanpack(changedLayout), "sanpack rewritten with different content");
    AssetAtlasCache rebuiltCache;
    AtlasBuildReport rebuiltReport;
    Check(rebuiltCache.BuildOrLoad(changedLayout.sanpackPath, cacheDirectory, TestFilter(), settings,
                                   rebuiltReport, &workerPool), "rebuild after a source change succeeds");
    Check(rebuiltReport.bExtractionRan && !rebuiltReport.bLoadedFromDiskCache,
          "a changed fingerprint forces a real rebuild, never a stale cache hit");
    const AtlasEntry* rebuiltIcon = rebuiltCache.Atlas().Find(changedLayout.validIconName);
    Check(rebuiltIcon != nullptr && rebuiltIcon->width == 32, "the rebuilt atlas reflects the new source");
}

} // namespace AssetPipelineTest
