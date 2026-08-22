// MapExporter_Recipe_IO.cpp — the `.sanmap` document's own top-level assembly: `BuildSanmapJsonText`
// alone. Layer: IO. A pure orchestrator (STEP31_ExporterRecipeOrchestrator_IO) — zero real logic;
// it only sequences calls into MapExporter_DocumentAssembly_IO's 4 helpers, exactly mirroring how
// MapImporter_IO.cpp already includes MapImporter_Recipe_IO.h for its own cross-file calls.
// STEP36_LegacyBlobDeletion_IO: the legacy `mapGeneratorData` blob (`BuildMapGeneratorDataJson`,
// MapExporter_MapGeneratorData_IO.cpp) is no longer written here — every field it carried now has a
// confirmed top-level duplicate (STEP11/STEP27/STEP30). The IMPORT side's gated legacy readers
// (`ReadGeometryJson`/`ReadWaterJson`/`ReadStrataSettingsJson`, MapImporter_Recipe_IO.cpp) are
// untouched — real old files still carry this blob and still need to import correctly.
#include "MapExporter_Recipe_IO.h"
#include "MapExporter_DocumentAssembly_IO.h"
#include "MapExporter_IO.h"
#include "MapExporter_UnknownImportMerge_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {

// STEP84_SanmapExportFormattingContract_IO — the ratified `.sanmap` emission contract. Every export
// through this function guarantees:
//  R1  4-space indentation, one level per nesting depth        — free (nlohmann `dump(4)`).
//  R2  `": "` key/value separator                               — free (nlohmann pretty-print).
//  R3  LF line endings, never CRLF, on every platform           — enforced at the WRITE site
//      (`WriteBinaryFileBytes`, MapExporter_IO.cpp) via `std::ios::binary`; nlohmann itself only
//      ever emits `\n`.
//  R4  No terminating newline after the closing `}`             — free (`dump()` appends none).
//  R5  No trailing whitespace on any line                       — free (nlohmann never emits any).
//  R6  One array element / one object member per line; `[]`/`{}` for empties — free (pretty-print).
//  R7  Every floating-point C++ field carries a decimal point or exponent — free (Grisu2 appends
//      `.0` to integral values, `json.hpp:17906-17909`).
//  R8  Every format-integer field is a JSON integer, no decimal point — SanGen enforces this at each
//      write site that stores a format integer in a C++ `float` (e.g. `height`, `waterLevel` —
//      `MapExporter_DocumentAssembly_IO.cpp`, `std::lround` then `static_cast<int>`).
//  R9  Two exports of the same recipe (+ bag), same or different process, are byte-identical —
//      SanGen enforces this by construction: no unordered container, no pointer-derived text, no
//      wall-clock content anywhere on this path.
//  R10 A string field holding invalid UTF-8 produces a logged, reported failure, never an uncaught
//      exception — enforced immediately below: `dump()` runs with the strict `error_handler`
//      (accurate bytes over silently-mangled ones), so its `type_error` is caught here and turned
//      into an empty return, which `WriteSanmapDocument`'s existing failure path already reports.
//      `ensure_ascii = false` (raw UTF-8, not `\uXXXX`) is kept — matches the reference in every
//      observable way (it is pure ASCII, where both settings agree); the `\uXXXX` alternative is
//      untested against the game's own parser and is a deliberate open question, not an oversight.
std::string MapExporter::BuildSanmapJsonText(const Params::MapRecipe& recipe,
                                             const MapExportOptions& options,
                                             const UnknownImportBag* unknownData) {
    nlohmann::ordered_json document;
    BuildDocumentEnvelopeJson(recipe, document);
    AppendEntityDomainsJson(recipe, document);
    AppendStackDomainsJson(recipe, document);
    AppendSimulationDomainsJson(recipe, document);

    // STEP24_ImportNeverRefuses_IO ruling 6: the Unknown-Import re-merge, LAST, immediately before
    // dump() — after every other `document[...] =` write above.
    MergeUnknownImportKeys(document, unknownData);

    const int indent = options.jsonIndentSpaceCount > 0 ? options.jsonIndentSpaceCount : -1;
    try {
        return document.dump(indent);
    } catch (const nlohmann::json::exception&) {
        // R10: `dump()`'s default `error_handler_t::strict` throws `type_error` (316) on a
        // std::string holding invalid UTF-8 (e.g. an ANSI-encoded Windows path landing in a name/
        // credits/blueprintPath field). Report, don't mangle: an empty string is a hard-failure
        // signal `WriteSanmapDocument` (MapExporter_IO.cpp) already treats as "Failed to write ...".
        return std::string();
    }
}

} // namespace Io
} // namespace SanmapGen
