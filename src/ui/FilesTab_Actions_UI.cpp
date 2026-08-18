// FilesTab_Actions_UI.cpp — the eight Files-tab actions, headless. Layer: UI.
// No imgui frame, no window, no GL context: an action is a path plus one call into IO, so the
// whole of the tab's behavior is drivable from a test (FilesTab_UI_Test.cpp).
//
// Every action is REFUSED with a logged reason rather than half-done: a missing path, an unbound
// SupCom seam and an ungenerated field set each answer false and say why (Constitution §6). The
// tab writes no byte and creates no folder itself — `Io::EnsureExportFolderExists` is the door.
#include "FilesTab_UI.h"
#include "../data/MapFields_DATA.h"
#include "../params/MapRecipe_PARAMS.h"

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

bool RunOpenSanmap(FilesTabState& state, Params::MapRecipe& recipe, Data::MapFields* fields) {
    if (state.sanmapPath.empty()) {
        AppendFilesTabLog(state, "Open refused: no .sanmap file or map folder is set.");
        return false;
    }
    Io::MapImportOptions options = state.importOptions;
    options.bLoadBakedFields = state.bLoadBakedFieldsOnImport;
    const Io::MapImportResult result =
        Io::MapImporter::LoadSanmap(state.sanmapPath, recipe, fields, options);
    AppendFilesTabLog(state, result.debugLog);
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

bool RunRecipeExport(FilesTabAction action, FilesTabState& state, const Params::MapRecipe& recipe,
                     const Data::MapFields* fields) {
    // `state.assetPack` — the internal warn-not-block safety net (MapExporter_IO.cpp); the UI's own
    // pre-check/confirm-dialog gate already ran, in FilesTab_Draw_UI.cpp, before this was called.
    const Io::MapExportResult result = (action == FilesTabAction::ExportAll && fields != nullptr)
        ? Io::MapExporter::ExportAll(state.exportFolderPath, recipe, *fields, state.exportOptions,
                                     state.assetPack)
        : Io::MapExporter::ExportSanmapOnly(state.exportFolderPath, recipe, state.exportOptions,
                                            state.assetPack);
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
    case FilesTabAction::ExportSanmapOnly:   return "Export Sanmap Only";
    case FilesTabAction::ExportAll:          return "Export All (Project+Textures)";
    case FilesTabAction::ExportHeightmapRaw: return "Export Heightmap RAW";
    case FilesTabAction::ExportSlopeImage:   return "Export Slope PNG";
    case FilesTabAction::ExportFlowImage:    return "Export Flow PNG";
    case FilesTabAction::ExportStratumMasks: return "Export Stratums TGA";
    }
    return "";
}

bool RunFilesTabAction(FilesTabAction action, FilesTabState& state, Params::MapRecipe& recipe,
                       Data::MapFields* fields) {
    if (FilesTabActionNeedsBakedFields(action) && (fields == nullptr || !fields->IsSized())) {
        AppendFilesTabLog(state, std::string(FilesTabActionLabel(action))
                          + " refused: nothing is generated yet, so there is no baked field to write.");
        return false;
    }
    if (action == FilesTabAction::OpenSanmap)      return RunOpenSanmap(state, recipe, fields);
    if (action == FilesTabAction::ImportSupComLua) return RunImportSupComLua(state, recipe);
    if (action == FilesTabAction::ExportSanmapOnly || action == FilesTabAction::ExportAll)
        return RunRecipeExport(action, state, recipe, fields);
    return RunTextureExport(action, state, *fields);
}

} // namespace Ui
} // namespace SanmapGen
