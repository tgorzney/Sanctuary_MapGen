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
    return document.dump(indent);
}

} // namespace Io
} // namespace SanmapGen
