// FilesTab_ExportGate_UI.cpp — the confirm-dialog pre-check/deferred-commit gate itself. Layer:
// UI. STEP5_PropsDecalsValidation_UI's blueprintPath warning and STEP77's mandatory-spawns warning
// share this SAME confirmDialogState/pendingConfirmAction/bConfirmActionPending machinery — a
// different rule, the same stash-and-defer shape (Fix §3).
#include "FilesTab_ExportGate_UI.h"
#include "FilesTab_ScenarioExport_Actions_UI.h"
#include "FilesTab_UI.h"
#include "ConfirmDialog_UI.h"
#include "../data/MapFields_DATA.h"
#include "../io/MapExporter_BlueprintValidation_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// Joins a name list with ", " — the mandatory-spawns dialog body's own formatting.
std::string JoinedNameList(const std::vector<std::string>& names) {
    std::string joined;
    for (std::size_t index = 0; index < names.size(); ++index) {
        if (index > 0) joined += ", ";
        joined += names[index];
    }
    return joined;
}

} // namespace

bool PreCheckGatedExport(FilesTabAction action, FilesTabState& state, const Params::MapRecipe& recipe) {
    if (action == FilesTabAction::ExportScenarioScript) {
        const std::vector<std::string> affected =
            ScenariosNeedingSpawnsAcknowledgment(recipe.scenarios);
        if (affected.empty()) return true;
        state.pendingConfirmAction  = action;
        state.bConfirmActionPending = true;
        state.confirmDialogBodyText =
            "These scenarios have no explicit spawns and no authoringNote: " + JoinedNameList(affected)
            + ". The 2h1ai regression (MAP_SCENARIO_SPEC.md \xC2\xA7" "6) applies unless that is intentional.";
        state.confirmDialogState.bOpenRequested = true;
        return false;
    }
    // ExportSanmapOnly/ExportAll's own blueprintPath check (STEP5). `state.assetPack == nullptr`
    // skips validation entirely (Files-tab flow item 1).
    if (state.assetPack == nullptr) return true;
    const Io::BlueprintValidationReport report =
        Io::ValidatePropAndDecalBlueprintPaths(recipe, *state.assetPack);
    if (report.AllResolved()) return true;
    state.pendingConfirmAction              = action;
    state.bConfirmActionPending             = true;
    state.confirmDialogBodyText             = report.SummaryText();
    state.confirmDialogState.bOpenRequested = true;
    return false;
}

void DrawGatedExportButton(FilesTabAction action, FilesTabState& state, Params::MapRecipe& recipe,
                           Data::MapFields* fields) {
    if (!ImGui::Button(FilesTabActionLabel(action))) return;
    if (PreCheckGatedExport(action, state, recipe)) RunFilesTabAction(action, state, recipe, fields);
}

// `bBlueprintValidationAcknowledged = true` is the `Export Anyway` choice, threaded straight
// through to RunFilesTabAction — required by ExportSanmapOnly/ExportAll's own IO-side refuse-by-
// default gate (STEP39_BlueprintValidationGate_IO); ExportScenarioScript ignores the flag, since
// Io::ExportMapScenario carries no equivalent gate of its own. The title reflects WHICH pre-check
// stashed the pending action (PreCheckGatedExport above).
void DrawPendingExportWarningDialog(FilesTabState& state, Params::MapRecipe& recipe,
                                    Data::MapFields* fields) {
    const bool bScenarioSpawnsGate =
        state.pendingConfirmAction == FilesTabAction::ExportScenarioScript;
    ConfirmDialogOptions options;
    options.title                = bScenarioSpawnsGate ? "Scenarios Missing Spawns" : "Unresolved blueprintPath";
    options.bodyText             = state.confirmDialogBodyText;
    options.primaryButtonLabel   = "Export Anyway";
    options.secondaryButtonLabel = "Cancel";
    const ConfirmDialogChange change =
        DrawConfirmDialog("filesTabExportWarning", state.confirmDialogState, options);
    if (change.bPrimaryClicked) {
        RunFilesTabAction(state.pendingConfirmAction, state, recipe, fields,
                          /*bBlueprintValidationAcknowledged=*/true);
        state.bConfirmActionPending = false;
    } else if (change.bSecondaryClicked) {
        state.bConfirmActionPending = false;
    }
}

} // namespace Ui
} // namespace SanmapGen
