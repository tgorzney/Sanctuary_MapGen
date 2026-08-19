// MapExporter_UnknownImportMerge_IO.h — the Unknown-Import re-merge step (STEP24_
// ImportNeverRefuses_IO ruling 6). Split out of `MapExporter_Recipe_IO.cpp` (ARCH §1.5's
// `Type_Aspect_LAYER` split-file pattern) so this ticket's addition does not grow that file's
// existing size further than the one call site it needs; header-only/`inline`, same
// header-only-pure-function precedent as `JsonPrimitives_IO.h`.
#pragma once
#include "UnknownImportBag_IO.h"
#include <nlohmann/json.hpp>

namespace SanmapGen {
namespace Io {

// Called LAST in `MapExporter::BuildSanmapJsonText`, immediately before `document.dump()` — after
// every other `document[...] =` write. Writes the WHOLE bag as one nested object under the reserved
// `UnknownImport` key (IO_MIGRATION_SPEC.md §6 — a single filterable/deletable container, not
// merged flat into the document's own top level; corrects STEP24_ImportNeverRefuses_IO's flat
// shape, STEP28_UnknownImportNesting_IO). `unknownData` nullable — a no-op when absent, as is an
// empty bag (§ acceptance item 4: no unrecognized data means no `UnknownImport` key at all, not an
// empty object). `document` is `nlohmann::ordered_json` (the exporter's own document type,
// MapExporter_Recipe_IO.cpp) — the bag's `unknownTopLevelKeys` is a plain `nlohmann::json`, which
// converts cleanly on assignment into an `ordered_json` value.
inline void MergeUnknownImportKeys(nlohmann::ordered_json& document,
                                   const UnknownImportBag* unknownData) {
    if (unknownData == nullptr) return;
    if (!unknownData->unknownTopLevelKeys.empty())
        document["UnknownImport"] = unknownData->unknownTopLevelKeys;
}

} // namespace Io
} // namespace SanmapGen
