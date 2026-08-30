// FilesTab_Actions_UI.cpp — the Files-tab actions, headless. Layer: UI. The scenario-script export
// leg (STEP77) is split out into FilesTab_ScenarioExport_Actions_UI.cpp for the §1.5 ceiling.
// No imgui frame, no window, no GL context: an action is a path plus one call into IO, so the
// whole of the tab's behavior is drivable from a test (FilesTab_UI_Test.cpp).
//
// Every action is REFUSED with a logged reason rather than half-done: a missing path, an unbound
// SupCom seam and an ungenerated field set each answer false and say why (Constitution §6). The
// tab writes no byte and creates no folder itself — `Io::EnsureExportFolderExists` is the door.
#include "FilesTab_UI.h"
#include "FilesTab_ScenarioExport_Actions_UI.h"
#include "../data/BakedLayerImage_DATA.h"
#include "../data/MapFields_DATA.h"
#include "../data/StratumArt_DATA.h"
#include "../io/FilesystemPrimitives_IO.h"
#include "../io/ScenarioScript_AreaImport_IO.h"
#include "../io/UnknownImportBag_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include <utility>

namespace SanmapGen {
namespace Ui {
namespace {

// `<exportFolder>/Textures`, created on demand. False (logged) when there is no destination.
bool ResolveTexturesFolderPath(FilesTabState& state, Io::MapExportResult& result,
                               std::string& outTexturesFolderPath) {
    if (state.exportFolderPath.empty()) {
        result.Log("Export refused: no destination map folder is set.");
        return false;
    }
    outTexturesFolderPath = Io::JoinExportPath(state.exportFolderPath,
                                               state.exportOptions.fileNames.texturesFolderName);
    return Io::EnsureExportFolderExists(outTexturesFolderPath, result);
}

// STEP103: an Open REPLACES the whole live recipe/fields/baked-image cache, never merges onto
// whatever a previous file/session left behind — `Application`'s live `recipe` starts seeded with
// two default procedural noise layers, and a real `.sanmap` with no `HeightmapStack` section left
// them in place, defeating STEP101's empty-stack fresh-synthesis gate (silent noise-over-terrain).
// `*outBakedLayerImages` is a persistent vector too; left uncleared it can bleed a stale entry with a
// colliding `layerIdentifier` from a previous Open into the new file (`FindBakedLayerImage`'s
// first-match scan). Fix: load onto FRESH scratch state (`Params::MapRecipe()`, not
// `MakeDefaultMapRecipe()`) and commit (move) it onto the live objects only when the load succeeds —
// a refused/failed Open must leave the live session untouched (Constitution §6).
bool RunOpenSanmap(FilesTabState& state, Params::MapRecipe& recipe, Data::MapFields* fields,
                   std::vector<Data::BakedLayerImage>* outBakedLayerImages,
                   std::vector<Data::StratumArt>* outStratumArt) {
    if (state.sanmapPath.empty()) {
        AppendFilesTabLog(state, "Open refused: no .sanmap file or map folder is set.");
        return false;
    }
    Io::MapImportOptions options = state.importOptions;
    options.bLoadBakedFields = state.bLoadBakedFieldsOnImport;
    // STEP24_ImportNeverRefuses_IO ruling 4: describes THIS document, not a merge of two unrelated
    // documents' unknown data. Left as an unconditional eager reset (unlike the scratch state below)
    // — its blast radius is a scratch JSON bag, not a designer's authored recipe.
    if (state.unknownImportData != nullptr) *state.unknownImportData = Io::UnknownImportBag();

    Params::MapRecipe scratchRecipe;                        // genuinely empty, no seeded layers
    Data::MapFields scratchFields;                          // fresh/unsized (IsSized() == false)
    std::vector<Data::BakedLayerImage> scratchBakedImages;  // fresh/empty, no stale identifiers
    std::vector<Data::StratumArt> scratchStratumArt;        // fresh/empty (STEP105)
    const Io::MapImportResult result = Io::MapImporter::LoadSanmap(
        state.sanmapPath, scratchRecipe, fields != nullptr ? &scratchFields : nullptr, options,
        state.unknownImportData, state.templateIngestReport, &scratchBakedImages, &scratchStratumArt);
    AppendFilesTabLog(state, result.debugLog);
    // STEP26B_MigrationReconciliationDialog_UI ruling 1: gates the "Check for Migrations..." button
    // — only a completed Open that found no version marker at all may offer the dialog.
    state.bLastOpenHadNoVersionMarker = result.bNoVersionMarkerFound;

    // `bSucceeded` is set the moment the recipe parse succeeds, before LoadBakedFields runs, so a
    // document that parses but has a missing/corrupt heightmap.raw still counts as success (logged
    // warning, not a refusal) — unchanged; only WHAT gets committed changes (a fresh baseline, never
    // the stale live objects), and nothing is touched at all on an actual refusal.
    if (result.bSucceeded) {
        recipe = std::move(scratchRecipe);
        if (fields != nullptr) *fields = std::move(scratchFields);
        if (outBakedLayerImages != nullptr) *outBakedLayerImages = std::move(scratchBakedImages);
        if (outStratumArt != nullptr) *outStratumArt = std::move(scratchStratumArt);
    }
    return result.bSucceeded;
}

// SCOPE NOTE 1 (FilesTab_UI.h): with nothing bound the row says so instead of pretending to work.
bool RunImportSupComLua(FilesTabState& state, Params::MapRecipe& recipe) {
    if (state.ImportSupComLua == nullptr) {
        AppendFilesTabLog(state, "No SupCom Lua importer is bound; the action does nothing.");
        return false;
    }
    if (state.supComLuaPath.empty()) {
        AppendFilesTabLog(state, "Import refused: no Supreme Commander _save.lua path is set.");
        return false;
    }
    std::string importLog;
    const bool bImported = state.ImportSupComLua(state.supComUserData, state.supComLuaPath.c_str(),
                                                 recipe, importLog);
    AppendFilesTabLog(state, importLog);
    return bImported;
}

// ARCH §15.11 — human-triggered, one-shot: this action exists ONLY as an explicit click. It must
// never be called from RunOpenSanmap or from anywhere else automatic (see this ticket's own
// header comment for why).
bool RunImportScenarioAreas(FilesTabState& state, Params::MapRecipe& recipe) {
    if (state.scenarioAreaImportPath.empty()) {
        AppendFilesTabLog(state, "Import refused: no scenario script .lua path is set.");
        return false;
    }
    const Io::ScenarioAreaImportResult result =
        Io::ImportAreaRectanglesFromScenarioScriptFile(state.scenarioAreaImportPath, recipe);
    AppendFilesTabLog(state, result.debugLog);
    return !result.bRefusedGeneratedFile && !result.bRefusedUnreadableFile
        && !result.bRefusedOversizedFile && !result.writtenNames.empty();
}

bool RunRecipeExport(FilesTabAction action, FilesTabState& state, const Params::MapRecipe& recipe,
                     const Data::MapFields* fields, bool bBlueprintValidationAcknowledged) {
    // `state.assetPack` feeds the IO layer's own refuse-by-default blueprintPath gate
    // (STEP39_BlueprintValidationGate_IO, MapExporter_IO.cpp) — the UI's pre-check/confirm-dialog
    // (FilesTab_Draw_UI.cpp) already ran before this was called; `bBlueprintValidationAcknowledged`
    // is that dialog's own "Export Anyway" choice, threaded straight through.
    const Io::MapExportResult result = (action == FilesTabAction::ExportAll && fields != nullptr)
        ? Io::MapExporter::ExportAll(state.exportFolderPath, recipe, *fields, state.exportOptions,
                                     state.assetPack, state.unknownImportData,
                                     bBlueprintValidationAcknowledged)
        : Io::MapExporter::ExportSanmapOnly(state.exportFolderPath, recipe, state.exportOptions,
                                            state.assetPack, state.unknownImportData,
                                            bBlueprintValidationAcknowledged);
    AppendFilesTabLog(state, result.debugLog);
    return result.bSucceeded;
}

bool RunTextureExport(FilesTabAction action, FilesTabState& state, const Data::MapFields& fields) {
    Io::MapExportResult result;
    std::string texturesFolderPath;
    bool bWritten = false;
    if (ResolveTexturesFolderPath(state, result, texturesFolderPath)) {
        const Io::MapExportFileNames& fileNames = state.exportOptions.fileNames;
        if (action == FilesTabAction::ExportHeightmapRaw)
            bWritten = Io::MapExporter::WriteHeightmapRaw(
                Io::JoinExportPath(texturesFolderPath, fileNames.heightmapRawName), fields, result);
        else if (action == FilesTabAction::ExportSlopeImage)
            bWritten = Io::MapExporter::WriteSlopeImage(
                Io::JoinExportPath(texturesFolderPath, fileNames.slopeImageName), fields,
                state.exportOptions, result);
        else if (action == FilesTabAction::ExportFlowImage)
            bWritten = Io::MapExporter::WriteFlowImage(
                Io::JoinExportPath(texturesFolderPath, fileNames.flowImageName), fields,
                state.exportOptions, result);
        else
            bWritten = Io::MapExporter::WriteStratumMaskImages(texturesFolderPath, fields,
                                                               state.exportOptions, result);
    }
    AppendFilesTabLog(state, result.debugLog);
    return bWritten;
}

} // namespace

const char* FilesTabActionLabel(FilesTabAction action) {
    switch (action) {
    case FilesTabAction::OpenSanmap:         return "Open Sanmap File";
    case FilesTabAction::ImportSupComLua:    return "Import SupCom Lua";
    case FilesTabAction::ImportScenarioAreas: return "Import Areas from Scenario Script";
    case FilesTabAction::ExportSanmapOnly:   return "Export Sanmap Only";
    case FilesTabAction::ExportAll:          return "Export All (Project+Textures)";
    case FilesTabAction::ExportHeightmapRaw: return "Export Heightmap RAW";
    case FilesTabAction::ExportSlopeImage:   return "Export Slope PNG";
    case FilesTabAction::ExportFlowImage:    return "Export Flow PNG";
    case FilesTabAction::ExportStratumMasks: return "Export Stratums TGA";
    case FilesTabAction::ExportScenarioScript: return "Export Scenario Script";
    }
    return "";
}

bool RunFilesTabAction(FilesTabAction action, FilesTabState& state, Params::MapRecipe& recipe,
                       Data::MapFields* fields, bool bBlueprintValidationAcknowledged,
                       std::vector<Data::BakedLayerImage>* outBakedLayerImages,
                       std::vector<Data::StratumArt>* outStratumArt) {
    if (FilesTabActionNeedsBakedFields(action) && (fields == nullptr || !fields->IsSized())) {
        AppendFilesTabLog(state, std::string(FilesTabActionLabel(action))
                          + " refused: nothing is generated yet, so there is no baked field to write.");
        return false;
    }
    if (action == FilesTabAction::OpenSanmap)
        return RunOpenSanmap(state, recipe, fields, outBakedLayerImages, outStratumArt);
    if (action == FilesTabAction::ImportSupComLua) return RunImportSupComLua(state, recipe);
    if (action == FilesTabAction::ImportScenarioAreas) return RunImportScenarioAreas(state, recipe);
    if (action == FilesTabAction::ExportSanmapOnly || action == FilesTabAction::ExportAll)
        return RunRecipeExport(action, state, recipe, fields, bBlueprintValidationAcknowledged);
    if (action == FilesTabAction::ExportScenarioScript)
        return RunScenarioScriptExport(state, recipe);
    return RunTextureExport(action, state, *fields);
}

} // namespace Ui
} // namespace SanmapGen
