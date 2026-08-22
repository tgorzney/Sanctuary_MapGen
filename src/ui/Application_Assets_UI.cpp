// Application_Assets_UI.cpp — the shell's asset bridge: run the M5-4 sanpack -> atlas -> disk-cache
// pipeline (IO), make its pages resident through Sys::AtlasResidency (SYS), and publish the result
// as the Ui::IconAtlasManifest the M5-3 icon grid consumes (UI). Layer: UI.
//
// THE GAP M5-6 FLAGGED, CLOSED HERE. `Io::AtlasEntry` is keyed by `name` — an archive path string;
// `Ui::IconAtlasEntry` carries an integer `iconId`. The shell is the ONE unit that legally sees IO
// and UI at once (ARCH §3.1), so it assigns each atlas entry its index in `Entries()` as that id
// and keeps the id -> template-identifier side table below. A tab may not invent an atlas source
// and IO may not learn a UI id, so neither of them could have built this (ARCH §8.4).
//
// The identifier an icon names is the entry's FILE STEM: ASSET_LOADING_SPEC pins unit thumbnails
// at `UI/Sprites/Icons/Units/<tpId>.dds`, so for that set the stem IS the `tpId`. For any other
// asset family the stem is the best available name and is reported as such; nothing here invents a
// mapping the spec does not state.
#include "Application_UI.h"
#include <cstring>

namespace SanmapGen {
namespace Ui {
namespace {

// "UI/Sprites/Icons/Units/uel0001.dds" -> "uel0001".
std::string FileStemOfEntryName(const std::string& entryName) {
    const std::size_t lastSeparator = entryName.find_last_of("/\\");
    const std::size_t stemBegin = lastSeparator == std::string::npos ? 0 : lastSeparator + 1;
    const std::size_t lastDot = entryName.find_last_of('.');
    const std::size_t stemEnd =
        (lastDot == std::string::npos || lastDot < stemBegin) ? entryName.size() : lastDot;
    return entryName.substr(stemBegin, stemEnd - stemBegin);
}

} // namespace

void BuildIconAtlasManifest(const Io::AssetAtlas& atlas, const Sys::AtlasResidency& atlasResidency,
                            Sys::GpuResourceManager* gpuResourceManager,
                            IconAtlasManifest& outManifest,
                            std::vector<std::string>& outTemplateIdentifiers) {
    outManifest.entries.clear();
    outManifest.pageTextureIdentifiers.clear();
    outTemplateIdentifiers.clear();
    outManifest.entries.reserve(atlas.Entries().size());
    outTemplateIdentifiers.reserve(atlas.Entries().size());

    // The presentation identifier is an opaque toolkit VALUE, not a GL handle (GpuResource_SYS.h),
    // so forwarding it to the draw list keeps every GPU object behind the SYS seam. A page that is
    // not resident resolves to zero, which the grid reads as "nothing to draw".
    for (int pageIndex = 0; pageIndex < atlas.PageCount(); ++pageIndex) {
        const Sys::GpuTextureHandle pageTexture = atlasResidency.PageTexture(pageIndex);
        outManifest.pageTextureIdentifiers.push_back(
            gpuResourceManager != nullptr
                ? gpuResourceManager->TexturePresentationIdentifier(pageTexture)
                : std::uint64_t(0));
    }
    for (std::size_t entryIndex = 0; entryIndex < atlas.Entries().size(); ++entryIndex) {
        const Io::AtlasEntry& atlasEntry = atlas.Entries()[entryIndex];
        IconAtlasEntry iconEntry;
        iconEntry.iconId     = static_cast<int>(entryIndex);   // the stable id: its own index
        iconEntry.atlasPage  = atlasEntry.pageIndex;
        iconEntry.uvMinimumX = atlasEntry.uvMinimumX;
        iconEntry.uvMinimumY = atlasEntry.uvMinimumY;
        iconEntry.uvMaximumX = atlasEntry.uvMaximumX;
        iconEntry.uvMaximumY = atlasEntry.uvMaximumY;
        outManifest.entries.push_back(iconEntry);
        outTemplateIdentifiers.push_back(FileStemOfEntryName(atlasEntry.name));
    }
}

void Application::SetSanpackPath(const std::string& path) {
    const std::size_t copyCount =
        path.size() < sizeof(assetBridge.sanpackPath) - 1 ? path.size() : sizeof(assetBridge.sanpackPath) - 1;
    std::memcpy(assetBridge.sanpackPath, path.c_str(), copyCount);
    assetBridge.sanpackPath[copyCount] = '\0';
}

std::string Application::TemplateIdentifierOfIcon(int iconId) const {
    if (iconId < 0 || iconId >= static_cast<int>(assetBridge.iconTemplateIdentifiers.size()))
        return std::string();
    return assetBridge.iconTemplateIdentifiers[static_cast<std::size_t>(iconId)];
}

// The cache directory is the SystemTab's caller-owned buffer (its SCOPE NOTE 1: no PARAMS home
// exists for it, and inventing one needs its own work-order). The shell reads it at load time
// rather than mirroring it, so there is exactly one copy of that string in the process.
//
// STEP5_PropsDecalsValidation_UI, LOAD-BEARING: `tabState.files.assetPack` is fed the long-lived
// `assetPackReader` ONLY when its OWN Open()+ReadCentralDirectoryOnce() both succeed for the
// CURRENT sanpackPath — every other path through this function (no path set, a failed open, a
// failed directory parse) leaves it nullptr, never a pointer into an empty/unopened reader.
bool Application::LoadAssetAtlas() {
    assetBridge.iconManifest.entries.clear();
    assetBridge.iconManifest.pageTextureIdentifiers.clear();
    assetBridge.iconTemplateIdentifiers.clear();
    assetBridge.iconPairingLookup.Clear();
    assetBridge.atlasResidency.Clear();
    assetBridge.assetPackReader.Close();
    tabState.files.assetPack = nullptr;
    if (assetBridge.sanpackPath[0] == '\0') {
        assetBridge.assetStatusMessage = "No sanpack selected.";
        return false;
    }

    if (assetBridge.assetPackReader.Open(assetBridge.sanpackPath) &&
        assetBridge.assetPackReader.ReadCentralDirectoryOnce())
        tabState.files.assetPack = &assetBridge.assetPackReader;

    Io::AtlasBuildReport buildReport;
    const std::string cacheDirectory(tabState.system.assetCacheDirectory);
    if (!assetBridge.assetAtlasCache.BuildOrLoad(assetBridge.sanpackPath, cacheDirectory,
                                                 settings.assetEntryFilter,
                                                 settings.atlasBuildSettings, buildReport,
                                                 &threadPool)) {
        assetBridge.assetStatusMessage = "Atlas build failed; the icon pickers stay on typed template ids.";
        return false;
    }
    UploadAtlasPages();
    BuildIconAtlasManifest(assetBridge.assetAtlasCache.Atlas(), assetBridge.atlasResidency,
                           gpuResourceManager.get(), assetBridge.iconManifest,
                           assetBridge.iconTemplateIdentifiers);
    assetBridge.iconPairingLookup = BuildIconAtlasPairingLookup(assetBridge.iconTemplateIdentifiers);
    assetBridge.assetStatusMessage =
        "Atlas: " + std::to_string(assetBridge.assetAtlasCache.Atlas().EntryCount()) +
        " icons on " + std::to_string(assetBridge.assetAtlasCache.Atlas().PageCount()) +
        (buildReport.bLoadedFromDiskCache ? " page(s), from cache." : " page(s), built.");
    return true;
}

// SYS takes raw pixel spans, never an Io type (AtlasResidency_SYS.h), so the walk over the atlas's
// pages is the caller's job — this shell is that caller. With no GL seam nothing is resident and
// the grid degrades to zero page identifiers rather than failing the load.
bool Application::UploadAtlasPages() {
    if (gpuResourceManager == nullptr) return false;
    const std::vector<Io::AtlasImage>& pages = assetBridge.assetAtlasCache.Atlas().Pages();
    bool bEveryPageResident = true;
    for (std::size_t pageIndex = 0; pageIndex < pages.size(); ++pageIndex) {
        const Io::AtlasImage& page = pages[pageIndex];
        bEveryPageResident = assetBridge.atlasResidency.UploadPage(*gpuResourceManager,
                                                       static_cast<int>(pageIndex), page.width,
                                                       page.height, page.rgbaPixels.data(),
                                                       page.rgbaPixels.size())
                          && bEveryPageResident;
    }
    return bEveryPageResident;
}

} // namespace Ui
} // namespace SanmapGen
