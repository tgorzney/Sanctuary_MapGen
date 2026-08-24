// MapImporter_IO.cpp — resolve, read, LoadSanmap. Layer: IO.
// The order is deliberate and is the whole of Constitution §6 for this module: resolve the path
// before touching a byte, cap the file size before reading it, then hand the text to
// `ParseSanmapJsonText` (MapImporter_ParseDocument_IO.cpp, STEP35_ImporterParseDocumentSplit_IO —
// the in-memory JSON-assembly orchestrator lives there now; this file keeps only the disk/path
// aspect: resolve, read, and the top-level LoadSanmap action that sequences the two).
#include "MapImporter_IO.h"
#include "FootprintBakeStaleness_IO.h"
#include "TemplateIngest_IO.h"
#include "../data/BakedLayerImage_DATA.h"
#include "../params/MapRecipe_PARAMS.h"
#include <filesystem>
#include <fstream>
#include <iterator>

namespace SanmapGen {
namespace Io {
namespace {

constexpr const char* sanmapExtension       = ".sanmap";
constexpr const char* defaultSanmapFileName = "mapdef.sanmap";

// The whole file as text, or false when it is missing, unreadable, or past the cap.
bool ReadDocumentText(const std::string& documentPath, std::uint64_t maximumByteSize,
                      std::string& outText, MapImportResult& result) {
    std::error_code sizeError;
    const std::uintmax_t byteSize = std::filesystem::file_size(std::filesystem::path(documentPath), sizeError);
    if (sizeError) { result.Log("Could not size " + documentPath); return false; }
    if (static_cast<std::uint64_t>(byteSize) > maximumByteSize) {
        result.Log("Refused " + documentPath + ": it is larger than the safety limit.");
        return false;
    }
    std::ifstream inputStream(documentPath, std::ios::binary);
    if (!inputStream) { result.Log("Could not open " + documentPath); return false; }
    outText.assign(std::istreambuf_iterator<char>(inputStream), std::istreambuf_iterator<char>());
    return true;
}

} // namespace

bool ResolveSanmapDocumentPath(const std::string& pathOrFolder, std::string& outDocumentPath,
                               std::string& outFolderPath) {
    if (pathOrFolder.empty()) return false;
    std::error_code pathError;
    const std::filesystem::path candidate(pathOrFolder);
    if (std::filesystem::is_regular_file(candidate, pathError)) {
        outDocumentPath = candidate.string();
        outFolderPath   = candidate.parent_path().string();
        return true;
    }
    if (!std::filesystem::is_directory(candidate, pathError)) return false;
    outFolderPath = candidate.string();
    const std::filesystem::path defaultDocument = candidate / defaultSanmapFileName;
    if (std::filesystem::is_regular_file(defaultDocument, pathError)) {
        outDocumentPath = defaultDocument.string();
        return true;
    }
    for (const std::filesystem::directory_entry& entry :
         std::filesystem::directory_iterator(candidate, pathError)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension().string() != sanmapExtension) continue;
        outDocumentPath = entry.path().string();
        return true;
    }
    return false;
}

MapImportResult MapImporter::LoadSanmap(const std::string& pathOrFolder, Params::MapRecipe& outRecipe,
                                        Data::MapFields* outFields, const MapImportOptions& options,
                                        UnknownImportBag* outUnknownData,
                                        const TemplateIngestReport* currentTemplateIngestReport,
                                        std::vector<Data::BakedLayerImage>* outBakedLayerImages) {
    MapImportResult result;
    result.Log("Loading " + pathOrFolder);
    if (!ResolveSanmapDocumentPath(pathOrFolder, result.resolvedDocumentPath, result.resolvedFolderPath)) {
        result.Log("No .sanmap document was found at that path.");
        return result;
    }
    result.Log("Resolved document: " + result.resolvedDocumentPath);
    std::string documentText;
    if (!ReadDocumentText(result.resolvedDocumentPath, options.safetyLimits.maximumDocumentByteSize,
                          documentText, result))
        return result;

    result.bRecipeLoaded = ParseSanmapJsonText(documentText, outRecipe, options, result, outUnknownData);
    if (!result.bRecipeLoaded) return result;
    result.bSucceeded = true;
    result.Log("Recipe loaded: map size " + std::to_string(outRecipe.geometry.mapSize)
               + ", " + std::to_string(outRecipe.layerStack.TotalLayerCount()) + " layer(s).");

    // STEP96 §3.1 call site 1: a sibling pre-flight, same tier as STEP82's export-time spawn-marker
    // check. Never forces an ingest -- skipped entirely when the caller has none resident.
    if (currentTemplateIngestReport != nullptr) {
        const FootprintBakeStalenessReport stalenessReport =
            CheckFootprintBakeStaleness(outRecipe, *currentTemplateIngestReport);
        if (!stalenessReport.AllFresh()) result.Warn(stalenessReport.SummaryText());
    }

    if (options.bLoadBakedFields && outFields != nullptr) {
        // outBakedLayerImages is nullable (caller-owned, `outFields`'s own precedent) -- with
        // nothing bound, LoadBakedFields still runs and still injects the decomposed layers into
        // outRecipe.layerStack; only their pixels have nowhere to land, so this scratch vector
        // simply falls away once the call returns.
        std::vector<Data::BakedLayerImage> scratchBakedLayerImages;
        std::vector<Data::BakedLayerImage>& bakedLayerImages =
            outBakedLayerImages != nullptr ? *outBakedLayerImages : scratchBakedLayerImages;
        result.bBakedFieldsLoaded = LoadBakedFields(result.resolvedFolderPath, outRecipe, *outFields,
                                                    options, result, bakedLayerImages);
    }
    return result;
}

} // namespace Io
} // namespace SanmapGen
