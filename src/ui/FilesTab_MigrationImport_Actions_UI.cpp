// FilesTab_MigrationImport_Actions_UI.cpp — the two headless actions behind STEP26B's "Check for
// Migrations..." reconciliation dialog: building the preview (RunCheckForMigrations) and applying a
// selection (RunSelectiveMigrationImport). Layer: UI. Split out of FilesTab_Actions_UI.cpp under
// the ARCH §1.5 file-size ceilings (FilesTab_Browse_UI.h's own precedent for this tab) — a THIRD,
// independent read/parse of state.sanmapPath is what these two share with each other, not what the
// tab's eight FilesTabAction buttons already do.
//
// No imgui frame, no window, no GL context: same headless posture as FilesTab_Actions_UI.cpp's own
// RunFilesTabAction, so both actions here are drivable from a test with no imgui frame.
#include "FilesTab_UI.h"
#include "MigrationReconciliationDialog_UI.h"
#include "../data/MapFields_DATA.h"
#include "../io/FilesystemPrimitives_IO.h"
#include "../io/Sanmap_MigrationPreview_IO.h"
#include "../io/UnknownImportBag_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include <nlohmann/json.hpp>

namespace SanmapGen {
namespace Ui {
namespace {

// Resolves + reads + size-caps state.sanmapPath's raw document text (Constitution §6) — the same
// cap RunOpenSanmap's own read already enforced (FilesTab_Actions_UI.cpp); a second, independent
// read must not silently skip it just because Open already succeeded once.
bool ReadSanmapDocumentText(FilesTabState& state, const Io::MapImportOptions& options,
                            std::string& outFolderPath, std::string& outDocumentText,
                            Io::MapImportResult& result) {
    std::string documentPath;
    if (!Io::ResolveSanmapDocumentPath(state.sanmapPath, documentPath, outFolderPath)) {
        result.Log("No .sanmap document was found at that path.");
        return false;
    }
    if (!Io::ReadTextFileBytes(documentPath, outDocumentText)) {
        result.Log("Could not read " + documentPath);
        return false;
    }
    if (outDocumentText.size() > options.safetyLimits.maximumDocumentByteSize) {
        result.Log(documentPath + " is larger than the safety limit.");
        return false;
    }
    return true;
}

// Parses `documentText`, false (logged) on a parse error or a non-object document — the same two
// refusal reasons `MapImporter::ParseSanmapJsonText` itself guards against.
bool ParseSanmapDocumentJson(const std::string& documentText, nlohmann::json& outDocument,
                             Io::MapImportResult& result) {
    try {
        outDocument = nlohmann::json::parse(documentText);
    } catch (const std::exception& parseError) {
        result.Log(std::string("JSON parse error: ") + parseError.what());
        return false;
    }
    if (!outDocument.is_object()) { result.Log("The document is not a JSON object."); return false; }
    return true;
}

} // namespace

// The "Check for Migrations..." button's own action (ruling 1): a fresh, read-only re-derive of
// state.sanmapPath into a Io::MigrationPreviewReport, copied into the dialog's own state
// (ResetMigrationDialogFromReport, MigrationReconciliationDialog_UI.h) with `bOpenRequested` set —
// never mutates the document, never touches `recipe`/`fields` (this is a preview, not a load).
bool RunCheckForMigrations(FilesTabState& state) {
    Io::MapImportResult result;
    result.Log("Checking " + state.sanmapPath + " for a migration walk.");
    std::string folderPath;
    std::string documentText;
    if (!ReadSanmapDocumentText(state, state.importOptions, folderPath, documentText, result)) {
        AppendFilesTabLog(state, result.debugLog);
        return false;
    }
    nlohmann::json document;
    if (!ParseSanmapDocumentJson(documentText, document, result)) {
        AppendFilesTabLog(state, result.debugLog);
        return false;
    }
    const Io::MigrationPreviewReport report = Io::PreviewSanmapMigrationWalk(document);
    ResetMigrationDialogFromReport(state.migrationDialogState, report);
    state.migrationDialogState.bOpenRequested = true;
    result.Log("Migration preview built; the reconciliation dialog is ready.");
    AppendFilesTabLog(state, result.debugLog);
    return true;
}

// Ruling 3's "Apply Selected" action. A FRESH, independent read/parse/apply/re-parse over
// state.sanmapPath — never touches whatever recipe/fields/unknownImportData the original direct-read
// Open already produced until the very end, when this function's own result REPLACES them entirely
// (never merged).
bool RunSelectiveMigrationImport(FilesTabState& state, Params::MapRecipe& recipe,
                                 Data::MapFields* fields,
                                 const std::vector<std::string>& selectedMigrationNames) {
    if (state.sanmapPath.empty()) {
        AppendFilesTabLog(state, "Selective migration apply refused: no .sanmap file or map folder "
                                 "is set.");
        return false;
    }
    Io::MapImportOptions options = state.importOptions;
    options.bLoadBakedFields = state.bLoadBakedFieldsOnImport;

    Io::MapImportResult result;
    std::string folderPath;
    std::string documentText;
    if (!ReadSanmapDocumentText(state, options, folderPath, documentText, result)) {
        AppendFilesTabLog(state, result.debugLog);
        return false;
    }
    nlohmann::json document;
    if (!ParseSanmapDocumentJson(documentText, document, result)) {
        AppendFilesTabLog(state, result.debugLog);
        return false;
    }

    Io::ApplySelectedSanmapMigrations(document, selectedMigrationNames, result);

    // STEP24_ImportNeverRefuses_IO ruling 4: a fresh, independent parse replaces whatever the prior
    // session's bag held, exactly RunOpenSanmap's own posture on a fresh read (nullable, caller-owned
    // — nothing to reset when no caller bound one).
    if (state.unknownImportData != nullptr) *state.unknownImportData = Io::UnknownImportBag();
    const bool bRecipeLoaded = Io::MapImporter::ParseSanmapJsonText(document.dump(), recipe, options,
                                                                    result, state.unknownImportData);
    if (bRecipeLoaded && options.bLoadBakedFields && fields != nullptr)
        Io::MapImporter::LoadBakedFields(folderPath, recipe, *fields, options, result);

    AppendFilesTabLog(state, result.debugLog);
    return bRecipeLoaded;
}

} // namespace Ui
} // namespace SanmapGen
