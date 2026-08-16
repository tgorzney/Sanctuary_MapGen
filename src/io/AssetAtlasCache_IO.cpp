// AssetAtlasCache_IO.cpp — the one entry point and the decode fan-out. Order of business
// (ASSET_LOADING_SPEC): fingerprint -> try the disk cache -> only on a miss open the sanpack,
// run the single-pass ingest, decode, pack, and write the cache back. On a cache hit the
// sanpack is never opened, which is exactly what bExtractionRan reports.
#include "AssetAtlasCache_IO.h"
#include "AssetAtlasCache_Decode_IO.h"
#include "../sys/ThreadPool_SYS.h"
#include <iostream>

namespace SanmapGen {
namespace Io {

namespace {

// Per-item outcome, tallied serially afterwards so the parallel decode touches no shared counter.
enum class DecodeOutcome : unsigned char { DecodedIcon, RenderedPropThumbnail, Placeholder };

bool HasExtension(const std::string& name, const std::string& extension) {
    if (extension.empty() || extension.size() > name.size()) return false;
    return name.compare(name.size() - extension.size(), extension.size(), extension) == 0;
}

} // namespace

bool AssetAtlasCache::BuildOrLoad(const std::string& sanpackPath, const std::string& cacheDirectory,
                                  const SanpackEntryFilter& filter, const AtlasBuildSettings& settings,
                                  AtlasBuildReport& outReport, Sys::ThreadPool* workerPool) {
    outReport = AtlasBuildReport{};
    atlas.Clear();
    const SourceFingerprint fingerprint =
        FingerprintOfFile(sanpackPath, settings.bIncludeContentHashInFingerprint);
    if (LoadFromDisk(cacheDirectory, fingerprint)) {
        outReport.bLoadedFromDiskCache = true;
        outReport.packedEntryCount = atlas.EntryCount();
        return true;
    }
    if (!BuildFromSanpack(sanpackPath, filter, settings, outReport, workerPool)) return false;
    outReport.bWroteDiskCache = SaveToDisk(cacheDirectory, fingerprint);
    return true;
}

bool AssetAtlasCache::BuildFromSanpack(const std::string& sanpackPath, const SanpackEntryFilter& filter,
                                       const AtlasBuildSettings& settings, AtlasBuildReport& outReport,
                                       Sys::ThreadPool* workerPool) {
    SanpackReader reader;
    if (!reader.Open(sanpackPath)) return false;
    std::vector<SanpackPayload> payloads;
    outReport.bExtractionRan = true;
    const bool bExtracted = reader.ExtractFiltered(filter, settings.sanpackLimits, payloads);
    outReport.sanpackStatistics = reader.Statistics();
    reader.Close();                       // the archive is never reopened for icons
    if (!bExtracted) return false;

    const int payloadCount = static_cast<int>(payloads.size());
    std::vector<AtlasImage> images(payloads.size());
    std::vector<std::string> names(payloads.size());
    std::vector<unsigned char> placeholderFlags(payloads.size(), 0);
    std::vector<DecodeOutcome> outcomes(payloads.size(), DecodeOutcome::Placeholder);
    // Each index writes only its own slot, so the fan-out needs no synchronization.
    const auto decodeOne = [&](int index) {
        const SanpackPayload& payload = payloads[static_cast<std::size_t>(index)];
        const std::size_t slot = static_cast<std::size_t>(index);
        names[slot] = payload.name;
        std::string rejectionReason;
        if (payload.bValid && HasExtension(payload.name, settings.propModelExtension)) {
            images[slot] = Decode::RenderPropThumbnail(payload.bytes.data(), payload.bytes.size(),
                                                       settings.propThumbnailWidth, settings.propThumbnailHeight);
            outcomes[slot] = DecodeOutcome::RenderedPropThumbnail;
            return;
        }
        if (payload.bValid &&
            Decode::DecodeDirectDrawSurface(payload.bytes.data(), payload.bytes.size(),
                                            settings.maximumIconWidth, settings.maximumIconHeight,
                                            images[slot], rejectionReason)) {
            outcomes[slot] = DecodeOutcome::DecodedIcon;
            return;
        }
        images[slot] = Decode::MakePlaceholderImage(settings.placeholderWidth, settings.placeholderHeight);
        placeholderFlags[slot] = 1;
        outcomes[slot] = DecodeOutcome::Placeholder;
    };
    if (workerPool != nullptr) workerPool->ParallelFor(0, payloadCount, decodeOne);
    else for (int index = 0; index < payloadCount; ++index) decodeOne(index);

    for (const DecodeOutcome outcome : outcomes) {
        if (outcome == DecodeOutcome::DecodedIcon) ++outReport.decodedIconCount;
        else if (outcome == DecodeOutcome::RenderedPropThumbnail) ++outReport.renderedPropThumbnailCount;
        else ++outReport.placeholderCount;
    }
    if (outReport.placeholderCount > 0)
        std::cerr << "AssetAtlasCache: " << outReport.placeholderCount
                  << " entry(ies) failed validation and were packed as placeholders.\n";
    return PackImages(images, names, placeholderFlags, settings, outReport);
}

} // namespace Io
} // namespace SanmapGen
