// SanpackReader_IO_Test.cpp — M5-4 acceptance, part 1: single-pass ingestion.
// Asserts the two properties ASSET_LOADING_SPEC actually cares about — the central directory is
// parsed EXACTLY ONCE per open no matter how often it is asked for, and the extraction pass
// visits entries in FILE-OFFSET order (not directory order, not name order) — plus the
// Constitution §6 path: a CRC-damaged entry is reported invalid instead of crashing or
// returning half-inflated bytes. argv[1] is a writable scratch directory (the shared
// add_sangen_test contract); the synthetic sanpack is written there.
#include "AssetPipeline_TestSupport_IO.h"
#include "SanpackReader_IO.h"
#include <algorithm>
#include <filesystem>

using namespace SanmapGen::Io;
using namespace AssetPipelineTest;

namespace {

const SanpackPayload* FindPayload(const std::vector<SanpackPayload>& payloads, const std::string& name) {
    for (const SanpackPayload& payload : payloads)
        if (payload.name == name) return &payload;
    return nullptr;
}

} // namespace

namespace AssetPipelineTest {

void RunSanpackReaderChecks(const SyntheticSanpack& layout) {
    SanpackReader reader;
    Check(reader.Open(layout.sanpackPath), "synthetic sanpack memory-maps");
    Check(reader.MappedByteSize() > 0, "mapped view has a size");

    // 1. The central directory is read once and only once, however often it is requested.
    for (int request = 0; request < 9; ++request) reader.ReadCentralDirectoryOnce();
    Check(reader.Statistics().centralDirectoryReadCount == 1,
          "central directory parsed exactly once across 9 requests");
    Check(reader.Statistics().directoryEntryCount == 7, "all 7 synthetic entries were catalogued");

    SanpackEntryFilter filter;
    filter.extensions = { ".dds", ".sanmodel" };
    SanpackSafetyLimits limits;
    std::vector<SanpackPayload> payloads;
    Check(reader.ExtractFiltered(filter, limits, payloads), "single-pass extraction runs");
    Check(reader.Statistics().centralDirectoryReadCount == 1,
          "extraction reused the parsed directory — still one central-directory read");
    Check(reader.Statistics().filteredEntryCount == 6, "the .sanbank entry was filtered out");
    Check(payloads.size() == 6, "one payload per accepted entry");

    // 2. Offset order, verified against an independent sort of the directory records. The
    //    fixture's central directory is deliberately reversed, so "directory order" and "offset
    //    order" disagree and this check fails for a reader that skips the sort.
    Check(reader.Statistics().bExtractedInFileOffsetOrder, "pass never walked an offset backwards");
    std::vector<const SanpackEntry*> directoryOrder;
    for (const SanpackEntry& entry : reader.DirectoryEntries())
        if (filter.Accepts(entry.name)) directoryOrder.push_back(&entry);
    std::vector<const SanpackEntry*> offsetOrder = directoryOrder;
    std::stable_sort(offsetOrder.begin(), offsetOrder.end(),
                     [](const SanpackEntry* left, const SanpackEntry* right) {
                         return left->localHeaderOffset < right->localHeaderOffset;
                     });
    bool bDirectoryOrderDiffers = false;
    for (std::size_t index = 0; index < offsetOrder.size(); ++index)
        if (directoryOrder[index] != offsetOrder[index]) bDirectoryOrderDiffers = true;
    Check(bDirectoryOrderDiffers, "fixture check: the central directory is NOT in offset order");
    bool bOrderMatchesOffsets = offsetOrder.size() == payloads.size();
    bool bOrderMatchesDirectory = directoryOrder.size() == payloads.size();
    for (std::size_t index = 0; index < payloads.size(); ++index) {
        if (index >= offsetOrder.size() || offsetOrder[index]->name != payloads[index].name)
            bOrderMatchesOffsets = false;
        if (index >= directoryOrder.size() || directoryOrder[index]->name != payloads[index].name)
            bOrderMatchesDirectory = false;
    }
    Check(bOrderMatchesOffsets, "payloads came back in ascending file-offset order");
    Check(!bOrderMatchesDirectory, "the pass sorted by offset — it did not just walk the directory");

    // 3. Validation: the byte-flipped stored entry fails, the intact ones do not.
    const SanpackPayload* damaged = FindPayload(payloads, layout.crcDamagedIconName);
    Check(damaged != nullptr && !damaged->bValid, "CRC-damaged entry is reported invalid");
    Check(damaged != nullptr && damaged->bytes.empty(), "an invalid entry hands back no bytes at all");
    Check(reader.Statistics().invalidEntryCount == 1, "exactly one entry failed archive-level validation");
    const SanpackPayload* valid = FindPayload(payloads, layout.validIconName);
    Check(valid != nullptr && valid->bValid, "the deflated icon inflated and matched its CRC");
    Check(valid != nullptr && valid->bytes.size() == 128u + 4096u, "inflated to the declared length");
    Check(FindPayload(payloads, layout.ignoredName) == nullptr, "the filtered-out entry was never touched");

    // 4. A prefix filter narrows the same single pass without a second directory read.
    SanpackEntryFilter unitIconFilter;
    unitIconFilter.pathPrefixes = { "UI/UI/Sprites/Icons/Units/" };
    unitIconFilter.extensions = { ".dds" };
    std::vector<SanpackPayload> unitIcons;
    Check(reader.ExtractFiltered(unitIconFilter, limits, unitIcons), "prefix-filtered pass runs");
    Check(unitIcons.size() == 4, "prefix filter kept only the unit icons");
    Check(reader.Statistics().centralDirectoryReadCount == 1, "still exactly one central-directory read");
    reader.Close();
}

} // namespace AssetPipelineTest

int main(int argc, char** argv) {
    std::filesystem::path scratchDirectory =
        std::filesystem::path(argc > 1 ? argv[1] : ".") / "assetPipelineTest";
    std::error_code errorCode;
    std::filesystem::create_directories(scratchDirectory, errorCode);
    SyntheticSanpack layout;
    layout.sanpackPath = (scratchDirectory / "synthetic.sanpack").string();
    Check(WriteSyntheticSanpack(layout), "synthetic sanpack written with the miniz writer");
    if (FailureCount() == 0) {
        RunSanpackReaderChecks(layout);
        RunAtlasCacheChecks(layout, scratchDirectory.string());
    }
    if (FailureCount() == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", FailureCount());
    return 1;
}
