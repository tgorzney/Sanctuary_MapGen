// SanpackReader_Extract_IO.cpp — the single sequential pass (ASSET_LOADING_SPEC step 3).
// Filters the cached directory to the entries the app asked for, sorts them by local-header
// offset, and walks them forward exactly once so the OS pages the archive in one direction and
// the disk never seeks backwards. The pass records whether it stayed ordered
// (bExtractedInFileOffsetOrder) — the instrumentation the acceptance test asserts on.
// A rejected entry does NOT abort the pass: it is counted, logged and returned invalid so the
// atlas can drop a placeholder in its place (Constitution §6).
#include "SanpackReader_IO.h"
#include <algorithm>
#include <iostream>

namespace SanmapGen {
namespace Io {

bool SanpackReader::ExtractFiltered(const SanpackEntryFilter& filter, const SanpackSafetyLimits& limits,
                                    std::vector<SanpackPayload>& outPayloads) {
    if (!ReadCentralDirectoryOnce()) return false;

    std::vector<const SanpackEntry*> neededEntries;
    neededEntries.reserve(directoryEntries.size());
    for (const SanpackEntry& entry : directoryEntries)
        if (filter.Accepts(entry.name)) neededEntries.push_back(&entry);

    std::sort(neededEntries.begin(), neededEntries.end(),
              [](const SanpackEntry* left, const SanpackEntry* right) {
                  return left->localHeaderOffset < right->localHeaderOffset;
              });
    statistics.filteredEntryCount = static_cast<int>(neededEntries.size());

    std::uint64_t previousOffset = 0;
    std::uint64_t runningTotalByteSize = 0;
    outPayloads.reserve(outPayloads.size() + neededEntries.size());
    for (const SanpackEntry* entry : neededEntries) {
        if (entry->localHeaderOffset < previousOffset) statistics.bExtractedInFileOffsetOrder = false;
        previousOffset = entry->localHeaderOffset;
        SanpackPayload payload;
        payload.name = entry->name;
        payload.bValid = ReadEntryPayload(*entry, limits, runningTotalByteSize, payload);
        if (!payload.bValid) {
            payload.bytes.clear();      // never hand back a partially validated buffer
            ++statistics.invalidEntryCount;
            std::cerr << "SanpackReader: rejected '" << payload.name << "' (" << payload.rejectionReason
                      << ") — a placeholder is used instead.\n";
        }
        ++statistics.extractedEntryCount;
        outPayloads.push_back(std::move(payload));
    }
    return true;
}

} // namespace Io
} // namespace SanmapGen
