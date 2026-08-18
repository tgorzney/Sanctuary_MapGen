// MapImporter_IO.cpp — resolve, read, parse. Layer: IO.
// The order is deliberate and is the whole of Constitution §6 for this module: resolve the path
// before touching a byte, cap the file size before reading it, parse inside a try, and only then
// let the block readers move the recipe off its defaults.
#include "MapImporter_IO.h"
#include "MapImporter_Recipe_IO.h"
#include "Sanmap_MigrationRunner_IO.h"
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

bool MapImporter::ParseSanmapJsonText(const std::string& documentText, Params::MapRecipe& outRecipe,
                                      const MapImportOptions& options, MapImportResult& result) {
    nlohmann::json document;
    try {
        document = nlohmann::json::parse(documentText);
    } catch (const std::exception& parseError) {
        result.Log(std::string("JSON parse error: ") + parseError.what());
        return false;
    }
    if (!document.is_object()) { result.Log("The document is not a JSON object."); return false; }

    // The migration runner is the LITERAL FIRST thing that touches the document — before even the
    // `height`/`width` reads below — so a refused document (newer than this build, or carrying no
    // version marker at all) can never leave outRecipe partially mutated (Constitution §6 atomicity;
    // IO_MIGRATION_SPEC.md §4.4: runs before any domain block reader).
    if (!RunSanmapMigrations(document, result)) return false;

    // The format's own `height` is the terrain's vertical extent (SANMAP_FORMAT_SPEC): it is the
    // authority when there is no generator block, and mapGeneratorData overrides it below.
    ReadJsonFloat(document, "height", outRecipe.geometry.terrainMaxHeight);
    int mapWidth = outRecipe.geometry.mapSize;
    if (ReadJsonInteger(document, "width", mapWidth)
        && mapWidth >= options.safetyLimits.minimumMapSize
        && mapWidth <= options.safetyLimits.maximumMapSize)
        outRecipe.geometry.mapSize = mapWidth;

    // `areas`/`armies`/`markers`/`chains`/`props`/`decals`/`PropGroups`/`DecalGroups` are top-level
    // `.sanmap` keys, SIBLINGS of `mapGeneratorData`, not nested inside it — read unconditionally
    // and BEFORE the mapGeneratorData-presence gate below, so a hand-authored file with real entity
    // content but no mapGeneratorData block still keeps it (Critical wiring correction,
    // STEP2_ArmiesAreas_IO, applied directly to markers/chains per STEP3_MarkersChains_IO and to
    // props/decals per STEP5_PropsDecalsValidation_UI). outRecipe.geometry.mapSize is already
    // populated from the top-level `width` key above, so the positionZ flip-inverse is correct here.
    // *Groups readers run BEFORE the instance readers: the layerIndex clamp (ARCH §12) validates
    // against propLayers/decalLayers, which only the *Groups readers populate.
    ReadAreasJson(document, outRecipe);
    ReadArmiesJson(document, outRecipe);
    ReadMarkersJson(document, outRecipe);
    ReadChainsJson(document, outRecipe);
    ReadPropGroupsJson(document, outRecipe);
    ReadPropsJson(document, outRecipe, result);
    ReadDecalGroupsJson(document, outRecipe);
    ReadDecalsJson(document, outRecipe, result);
    // ATMOSPHERE_PARAMS_SPEC: ~49 flat top-level keys, same tier/ordering rule as the entity
    // domains above — unconditional, before the mapGeneratorData gate below.
    ReadAtmosphereJson(document, outRecipe, result);
    // STEP10_SlopeDefaults_Mechanism: one flat top-level object, same tier/ordering rule as the
    // entity domains above — unconditional, before the mapGeneratorData gate below.
    ReadSlopeDefaultsJson(document, outRecipe);
    // SANMAP_FORMAT_SPEC Correction 13: `stratumLayers[9]`, same tier/ordering rule as the entity
    // domains above — unconditional, before the mapGeneratorData gate below. NOT alongside
    // `ReadStrataSettingsJson` below, which stays gated: that one reads the separate, legacy
    // `mapGeneratorData.Stratums` blob and MERGES onto whatever this call already wrote (see
    // MapImporter_Recipe_IO.cpp's own header comment on that reader).
    ReadStratumLayersJson(document, outRecipe, result);

    if (!document.contains("mapGeneratorData") || !document["mapGeneratorData"].is_object()) {
        result.Warn("No mapGeneratorData block: only the map's own dimensions were recovered.");
        return true;
    }
    const nlohmann::json& generatorData = document["mapGeneratorData"];
    ReadGeometryJson(generatorData, options, outRecipe, result);
    ReadWaterJson(generatorData, outRecipe);
    ReadLayerStackJson(generatorData, outRecipe.layerStack);
    ReadStrataSettingsJson(generatorData, outRecipe);
    ReadPlacementRulesJson(generatorData, outRecipe);
    return true;
}

MapImportResult MapImporter::LoadSanmap(const std::string& pathOrFolder, Params::MapRecipe& outRecipe,
                                        Data::MapFields* outFields, const MapImportOptions& options) {
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

    result.bRecipeLoaded = ParseSanmapJsonText(documentText, outRecipe, options, result);
    if (!result.bRecipeLoaded) return result;
    result.bSucceeded = true;
    result.Log("Recipe loaded: map size " + std::to_string(outRecipe.geometry.mapSize)
               + ", " + std::to_string(outRecipe.layerStack.TotalLayerCount()) + " layer(s).");

    if (options.bLoadBakedFields && outFields != nullptr)
        result.bBakedFieldsLoaded = LoadBakedFields(result.resolvedFolderPath, outRecipe, *outFields,
                                                    options, result);
    return result;
}

} // namespace Io
} // namespace SanmapGen
