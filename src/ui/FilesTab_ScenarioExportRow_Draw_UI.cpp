// FilesTab_ScenarioExportRow_Draw_UI.cpp — STEP77 Fix §4/§5's draw surface. Layer: UI.
// Machine-local settings (game install root, runtime script override) live HERE, on the Files tab,
// not in Scenarios — matching ArmiesTabState::gamedataDirectory's precedent (Fix §5's own
// reasoning: this is machine-local configuration, not recipe content).
#include "FilesTab_ScenarioExportRow_Draw_UI.h"
#include "FilesTab_Browse_UI.h"
#include "FilesTab_ExportGate_UI.h"
#include "FilesTab_UI.h"
#include "Section_UI.h"
#include "../data/MapFields_DATA.h"
#include "../io/GameInstallLocation_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

const ImVec4 kWarningTextColor(0.95f, 0.75f, 0.15f, 1.0f);
const ImVec4 kErrorTextColor(0.95f, 0.35f, 0.35f, 1.0f);

// Fix §4's context-sensitive export row: unset -> a redirect (never a dead-end disabled button);
// set-but-invalid -> the reason, verbatim, export disabled; valid -> the real gated export button.
void DrawScenarioScriptExportRow(FilesTabState& state, Params::MapRecipe& recipe, Data::MapFields* fields) {
    if (state.gameInstallRoot == nullptr) {
        ImGui::TextUnformatted("Scenario export: no game install root is configured for this build.");
        return;
    }
    if (state.gameInstallRoot->empty()) {
        DrawFilesTabPathRow("Locate Game Install...", FilesTabBrowseKind::GameInstallRoot,
                            *state.gameInstallRoot);
        return;
    }
    const Io::GameInstallRootValidation validation = Io::ValidateGameInstallRoot(*state.gameInstallRoot);
    if (!validation.bValid) {
        ImGui::TextColored(kErrorTextColor, "%s", validation.reason.c_str());
        return;
    }
    DrawGatedExportButton(FilesTabAction::ExportScenarioScript, state, recipe, fields);
}

// Fix §4's result banner: every flag gets its own line — the exact paths already live in
// state.debugLog (RunScenarioScriptExport's own AppendFilesTabLog call), which this points at
// rather than re-deriving Io::ExportMapScenario's own path-construction rule (out-of-scope, ticket
// §"Explicit out-of-scope").
void DrawScenarioExportResultBanner(const Io::ScenarioExportResult& result) {
    if (result.bDataLuaCollisionDetected)
        ImGui::TextColored(kWarningTextColor, "Data.lua: unrecognized content occupies the target "
                           "path — the freshly-rendered text was written to its .sangen-pending.lua "
                           "sibling instead. See the log below for both paths; reconcile manually.");
    if (result.bRuntimeCollisionDetected)
        ImGui::TextColored(kWarningTextColor, "Runtime.lua: unrecognized content occupies the target "
                           "path — the resolved runtime text was written to its .sangen-pending.lua "
                           "sibling instead. See the log below for both paths; reconcile manually.");
    if (result.bDataLuaSyntaxCheckFailed)
        ImGui::TextColored(kErrorTextColor,
                           "Data.lua: the rendered text failed Sys::CheckLuaSyntax — write refused.");
    if (result.bRuntimeSyntaxCheckFailed)
        ImGui::TextColored(kErrorTextColor,
                           "Runtime.lua: the resolved runtime text failed Sys::CheckLuaSyntax — write refused.");
}

} // namespace

void DrawScenarioScriptExportSection(FilesTabState& state, Params::MapRecipe& recipe,
                                     Data::MapFields* fields) {
    if (!DrawSectionBegin("Export Scenario Script", state.scenarioExportSection)) return;
    // The "Game Install Root" settings row only appears ONCE ONE IS SET — the export row below is
    // the sole redirect while it is empty (Fix §4), so the two never duplicate the same affordance.
    if (state.gameInstallRoot != nullptr && !state.gameInstallRoot->empty())
        DrawFilesTabPathRow("Game Install Root", FilesTabBrowseKind::GameInstallRoot,
                            *state.gameInstallRoot);
    if (state.scenarioRuntimeOverridePath != nullptr)
        DrawFilesTabPathRow("Runtime Script Override (optional)",
                            FilesTabBrowseKind::ScenarioRuntimeOverrideLua,
                            *state.scenarioRuntimeOverridePath);
    ImGui::Separator();
    DrawScenarioScriptExportRow(state, recipe, fields);
    DrawScenarioExportResultBanner(state.lastScenarioExportResult);
    DrawSectionEnd();
}

} // namespace Ui
} // namespace SanmapGen
