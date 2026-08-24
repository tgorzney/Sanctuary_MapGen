// TemplateIngest_Fold_IO.h — private ARCH §1.5 aspect split off TemplateIngest_IO.cpp: the
// sequential, priority-sorted record-fold (STEP89's implementation contract, step 5) that both the
// cache-hit and cache-miss paths converge on. NOT part of this domain's public surface —
// TemplateIngest_IO.h is; only TemplateIngest_IO.cpp includes this (mirrors
// TemplateIngestCache_Record_IO.h's own "private aspect" posture, ticket 88).
#pragma once
#include "TemplateIngest_IO.h"
#include <vector>

namespace SanmapGen {
namespace Io {

// records is taken BY VALUE and stable-sorted internally by sourcePriorityRank ascending (Q7's
// first-write-wins resolution, ticket 87) -- the caller's own copy (used separately for
// Io::DetectTpIdCollisions, which reports in original discovery order) is left untouched. Walks the
// sorted copy once, classifying each record into report's counters and the footprint/tags maps
// exactly per STEP89 §2 step 5.
void FoldRecordsIntoReport(std::vector<TemplateRecord> records, TemplateIngestReport& report);

} // namespace Io
} // namespace SanmapGen
