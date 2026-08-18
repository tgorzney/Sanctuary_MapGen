// MapExporter_IO.cpp — the two export ACTIONS the Files tab offers, and nothing else.
// Layer: IO. Both create the destination folder, write the document, and report every file they
// produced so the tab's log panel can show the result rather than a silent success.
#include "MapExporter_IO.h"
#include "../data/MapFields_DATA.h"
#include "../params/MapRecipe_PARAMS.h"
#include <filesystem>

namespace SanmapGen {
namespace Io {
namespace {

bool WriteSanmapDocument(const std::string& folderPath, const Params::MapRecipe& recipe,
                         const MapExportOptions& options, MapExportResult& result) {
    const std::string documentPath = JoinExportPath(folderPath, options.mapName + ".sanmap");
    const std::string documentText = MapExporter::BuildSanmapJsonText(recipe, options);
    if (!WriteBinaryFileBytes(documentPath, documentText.data(), documentText.size())) {
        result.Log("Failed to write " + documentPath);
        return false;
    }
    result.Log("Wrote " + documentPath);
    result.RecordWrittenFile(documentPath);
    return true;
}

// The blueprintPath safety net (finalized IO plumbing item 4): warn-not-block, for a caller that
// passes a non-null assetPack without going through the Files-tab dialog at all — the SAME
// validation the UI pre-check runs, just logged instead of gating the button. `result.bSucceeded`
// is NEVER touched here.
void LogBlueprintValidationFindings(const Params::MapRecipe& recipe, const SanpackReader* assetPack,
                                    MapExportResult& result) {
    if (assetPack == nullptr) return;
    const BlueprintValidationReport report = ValidatePropAndDecalBlueprintPaths(recipe, *assetPack);
    if (!report.AllResolved()) result.Log(report.SummaryText());
}

} // namespace

bool EnsureExportFolderExists(const std::string& folderPath, MapExportResult& result) {
    if (folderPath.empty()) { result.Log("Export failed: no destination folder was given."); return false; }
    std::error_code folderError;
    const std::filesystem::path folder(folderPath);
    std::filesystem::create_directories(folder, folderError);
    if (folderError && !std::filesystem::exists(folder)) {
        result.Log("Export failed: could not create " + folderPath);
        return false;
    }
    return true;
}

MapExportResult MapExporter::ExportSanmapOnly(const std::string& folderPath,
                                              const Params::MapRecipe& recipe,
                                              const MapExportOptions& options,
                                              const SanpackReader* assetPack) {
    MapExportResult result;
    if (!recipe.IsValid()) { result.Log("Export refused: the recipe's geometry is not valid."); return result; }
    LogBlueprintValidationFindings(recipe, assetPack, result);
    if (!EnsureExportFolderExists(folderPath, result)) return result;
    result.bSucceeded = WriteSanmapDocument(folderPath, recipe, options, result);
    return result;
}

MapExportResult MapExporter::ExportAll(const std::string& folderPath, const Params::MapRecipe& recipe,
                                       const Data::MapFields& fields, const MapExportOptions& options,
                                       const SanpackReader* assetPack) {
    MapExportResult result;
    if (!recipe.IsValid()) { result.Log("Export refused: the recipe's geometry is not valid."); return result; }
    LogBlueprintValidationFindings(recipe, assetPack, result);
    if (!EnsureExportFolderExists(folderPath, result)) return result;
    if (!WriteSanmapDocument(folderPath, recipe, options, result)) return result;

    const std::string texturesFolder = JoinExportPath(folderPath, options.fileNames.texturesFolderName);
    if (!EnsureExportFolderExists(texturesFolder, result)) return result;
    if (!fields.IsSized()) {
        result.Log("No baked fields: the .sanmap was written without its Textures payload.");
        result.bSucceeded = true;
        return result;
    }

    bool bAllWritten = true;
    if (options.bWriteHeightmapRaw)
        bAllWritten &= WriteHeightmapRaw(JoinExportPath(texturesFolder, options.fileNames.heightmapRawName),
                                         fields, result);
    if (options.bWriteStratumMasks)
        bAllWritten &= WriteStratumMaskImages(texturesFolder, fields, options, result);
    if (options.bWriteSlopeImage)
        bAllWritten &= WriteSlopeImage(JoinExportPath(texturesFolder, options.fileNames.slopeImageName),
                                       fields, options, result);
    if (options.bWriteFlowImage)
        bAllWritten &= WriteFlowImage(JoinExportPath(texturesFolder, options.fileNames.flowImageName),
                                      fields, options, result);
    result.bSucceeded = bAllWritten;
    result.Log("Export All finished: " + std::to_string(result.WrittenFileCount()) + " file(s).");
    return result;
}

} // namespace Io
} // namespace SanmapGen
