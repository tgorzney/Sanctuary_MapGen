// MapExporter_IO.cpp — the two export ACTIONS the Files tab offers, and nothing else.
// Layer: IO. Both create the destination folder, write the document, and report every file they
// produced so the tab's log panel can show the result rather than a silent success.
#include "MapExporter_IO.h"
#include "FilesystemPrimitives_IO.h"
#include "MapExporter_ArmySpawnMarkerValidation_IO.h"
#include "MapExporter_BlueprintValidation_IO.h"
#include "MapExporter_ScenarioAreaNameValidation_IO.h"
#include "MapExporter_Recipe_IO.h"
#include "UnknownImportBag_IO.h"
#include "../data/MapFields_DATA.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

// STEP209 -- warn, never block. A stale ScenarioBody::areaName is a legal, tolerated state (the named
// Area was renamed/deleted after the scenario picked it) -- ARCH_15_05_ParamsScenariosType.md §15.5
// AMENDED 2026-08-28 rules "never crash, never emit garbage", exporting the scenario's own last-known
// rectangle instead. This function only reports. Defined ahead of WriteSanmapDocument (its one caller
// inside this translation unit) since it is called from there.
void ReportScenarioAreaNameReferences(const Params::MapRecipe& recipe, MapExportResult& result) {
    const ScenarioAreaNameValidationReport report = ValidateScenarioAreaNameReferences(recipe);
    if (report.AllReferencesResolve()) return;
    result.Warn(report.SummaryText());
}

bool WriteSanmapDocument(const std::string& folderPath, const Params::MapRecipe& recipe,
                         const MapExportOptions& options, MapExportResult& result,
                         const UnknownImportBag* unknownData) {
    // STEP76_ArmyIdentityNaming_IO §3c option (ii): verify-and-warn, not rewrite (BuildArmiesJson
    // stays const, never a const_cast). This is the one place both public export actions
    // (ExportSanmapOnly/ExportAll) share, so it is the natural home for the backstop — it fires
    // before the document is even assembled, and `result` is already the sink the Files tab's log
    // panel reads.
    CheckArmyIdentitiesWellFormed(recipe.armies, result);
    ReportScenarioAreaNameReferences(recipe, result);

    // The output file name matches the document's own `mapName` (STEP25_MapNameCredits_IO moved
    // this off `MapExportOptions` onto the recipe — it is real, importable document content, not an
    // export-run-only option).
    const std::string documentPath = JoinExportPath(folderPath, recipe.mapName + ".sanmap");
    const std::string documentText = MapExporter::BuildSanmapJsonText(recipe, options, unknownData);
    // R10 (STEP84_SanmapExportFormattingContract_IO): `BuildSanmapJsonText` returns an empty string
    // when `dump()` hit invalid UTF-8 (caught in MapExporter_Recipe_IO.cpp) — a hard failure, not a
    // legitimately-empty document (the envelope alone is never empty). Refuse to write a truncated
    // `.sanmap` rather than silently succeeding on 0 bytes.
    if (documentText.empty()) {
        result.Log("Failed to write " + documentPath + ": the document text is empty (build failed).");
        return false;
    }
    // STEP84_SanmapExportFormattingContract_IO / R3: WriteBinaryFileBytes goes through
    // std::ios::binary specifically so nlohmann's `\n` line endings are NOT translated to `\r\n` by
    // Windows text mode. Shipped `.sanmap` files are LF-only (measured against
    // "Pandemonium Isthmus.sanmap"). Dropping `std::ios::binary` here silently reproduces the exact
    // defect that damaged that live reference map on 2026-08-21 (every `\n` became `\r\n`) — do not
    // "simplify" this to a text-mode write.
    if (!WriteBinaryFileBytes(documentPath, documentText.data(), documentText.size())) {
        result.Log("Failed to write " + documentPath);
        return false;
    }
    result.Log("Wrote " + documentPath);
    result.RecordWrittenFile(documentPath);
    return true;
}

// The blueprintPath safety net (STEP39_BlueprintValidationGate_IO): structural now, not just one
// button's discipline — the SAME validation the UI pre-check runs, enforced here so every present
// and future caller of ExportSanmapOnly/ExportAll gets it for free. `assetPack == nullptr` skips
// validation entirely (pre-STEP39 contract). All-resolved -> proceed silently. Unresolved -> the
// finding is always logged, and the write is refused (false, nothing created) UNLESS the caller
// already passed `bBlueprintValidationAcknowledged = true` — the same choice the Files tab's
// confirm-dialog "Export Anyway" click represents (FilesTab_Draw_UI.cpp).
bool CheckBlueprintValidationGate(const Params::MapRecipe& recipe, const SanpackReader* assetPack,
                                  bool bBlueprintValidationAcknowledged, MapExportResult& result) {
    if (assetPack == nullptr) return true;
    const BlueprintValidationReport report = ValidatePropAndDecalBlueprintPaths(recipe, *assetPack);
    if (report.AllResolved()) return true;
    result.Log(report.SummaryText());
    if (bBlueprintValidationAcknowledged) return true;
    result.Log("Export refused: unresolved blueprintPath(s) were not acknowledged.");
    return false;
}

// STEP82: warn-only, never gates. Returns void BY DESIGN -- an orphaned army is a legal, tolerated
// state (ARCH_16_08_SpawnArmyShrink.md §16.8) and must never influence whether an export proceeds.
// If a future edit is tempted to make this bool, that is the ARCH ruling it would be breaking.
void ReportArmiesWithoutSpawnMarkers(const Params::MapRecipe& recipe, MapExportResult& result) {
    const ArmySpawnMarkerValidationReport report = ValidateArmiesHaveSpawnMarkers(recipe);
    if (report.AllArmiesHaveSpawnMarkers()) return;
    result.Warn(report.SummaryText());
}

} // namespace

bool EnsureExportFolderExists(const std::string& folderPath, MapExportResult& result) {
    std::string errorMessage;
    if (!EnsureFolderExists(folderPath, errorMessage)) { result.Log("Export failed: " + errorMessage); return false; }
    return true;
}

MapExportResult MapExporter::ExportSanmapOnly(const std::string& folderPath,
                                              const Params::MapRecipe& recipe,
                                              const MapExportOptions& options,
                                              const SanpackReader* assetPack,
                                              const UnknownImportBag* unknownData,
                                              bool bBlueprintValidationAcknowledged) {
    MapExportResult result;
    if (!recipe.IsValid()) { result.Log("Export refused: the recipe's geometry is not valid."); return result; }
    ReportArmiesWithoutSpawnMarkers(recipe, result);
    if (!CheckBlueprintValidationGate(recipe, assetPack, bBlueprintValidationAcknowledged, result))
        return result;
    if (!EnsureExportFolderExists(folderPath, result)) return result;
    result.bSucceeded = WriteSanmapDocument(folderPath, recipe, options, result, unknownData);
    return result;
}

MapExportResult MapExporter::ExportAll(const std::string& folderPath, const Params::MapRecipe& recipe,
                                       const Data::MapFields& fields, const MapExportOptions& options,
                                       const SanpackReader* assetPack,
                                       const UnknownImportBag* unknownData,
                                       bool bBlueprintValidationAcknowledged) {
    MapExportResult result;
    if (!recipe.IsValid()) { result.Log("Export refused: the recipe's geometry is not valid."); return result; }
    ReportArmiesWithoutSpawnMarkers(recipe, result);
    if (!CheckBlueprintValidationGate(recipe, assetPack, bBlueprintValidationAcknowledged, result))
        return result;
    if (!EnsureExportFolderExists(folderPath, result)) return result;
    if (!WriteSanmapDocument(folderPath, recipe, options, result, unknownData)) return result;

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
