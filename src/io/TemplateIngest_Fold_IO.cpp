// TemplateIngest_Fold_IO.cpp — see TemplateIngest_Fold_IO.h's own header comment.
#include "TemplateIngest_Fold_IO.h"
#include <algorithm>

namespace SanmapGen {
namespace Io {
namespace {

// Tags follow the SAME first-write-wins discipline as the footprint map, but are tracked
// independently (their own map, their own presence check) -- STEP89 §2 step 5's "tags and footprint
// are independent signals, a record can usefully carry one without the other." An empty tags[] on
// the winning-priority record never "claims" the slot, so a later (lower-priority) record's real
// tags can still surface -- the reading that best serves the map's own purpose (never silently
// blocking real tag data behind an empty placeholder).
void FoldTagsIfFirstWrite(const TemplateRecord& record, TemplateIngestReport& report) {
    if (record.tags.empty()) return;
    if (report.tagsByTemplateIdentifier.find(record.templateIdentifier) !=
        report.tagsByTemplateIdentifier.end())
        return;
    report.tagsByTemplateIdentifier.emplace(record.templateIdentifier, record.tags);
}

} // namespace

void FoldRecordsIntoReport(std::vector<TemplateRecord> records, TemplateIngestReport& report) {
    std::stable_sort(records.begin(), records.end(),
                      [](const TemplateRecord& left, const TemplateRecord& right) {
                          return left.sourcePriorityRank < right.sourcePriorityRank;
                      });

    for (const TemplateRecord& record : records) {
        // Structurally unreachable in a bRecognized-true record (TemplateDialect_IO.cpp only ever
        // sets one of the four real dialect kinds once bRecognized is true) -- kept as a defensive
        // no-op rather than assumed away, per Constitution §6.
        if (record.dialectKind == TemplateDialectKind::Unrecognized) continue;

        if (record.dialectKind == TemplateDialectKind::ProjectileTemplate ||
            record.dialectKind == TemplateDialectKind::MarkerTemplate) {
            ++report.skippedProjectileOrMarkerCount;
            continue;
        }

        if (!record.bHasFootprint) {
            ++report.skippedMissingFootprintCount;
            FoldTagsIfFirstWrite(record, report);
            continue;
        }

        const bool bAlreadyHasFootprint =
            report.footprintByTemplateIdentifier.find(record.templateIdentifier) !=
            report.footprintByTemplateIdentifier.end();
        if (!bAlreadyHasFootprint) {
            TemplateFootprintRecord footprintRecord;
            footprintRecord.baseFootprintWidth = record.baseFootprintWidth;
            footprintRecord.baseFootprintDepth = record.baseFootprintDepth;
            footprintRecord.sourceFingerprint = SourceFingerprint{
                record.sourceLogicalPath, record.sourceByteSize, record.sourceModifiedTime,
                record.sourceContentHash};
            report.footprintByTemplateIdentifier.emplace(record.templateIdentifier, footprintRecord);
            ++report.ingestedFootprintRecordCount;
        }
        FoldTagsIfFirstWrite(record, report);
    }
}

} // namespace Io
} // namespace SanmapGen
