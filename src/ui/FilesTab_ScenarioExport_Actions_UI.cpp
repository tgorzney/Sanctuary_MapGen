// FilesTab_ScenarioExport_Actions_UI.cpp — STEP77's FilesTabAction::ExportScenarioScript. Layer:
// UI. Headless: one call into Io::ExportMapScenario (STEP71), refused with a logged reason (never
// half-done) when no valid game install root is configured (Constitution §6).
#include "FilesTab_ScenarioExport_Actions_UI.h"
#include "FilesTab_UI.h"
#include "ScenariosTab_UI.h"
#include "../io/GameInstallLocation_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Ui {

bool RunScenarioScriptExport(FilesTabState& state, const Params::MapRecipe& recipe) {
    if (state.gameInstallRoot == nullptr
        || !Io::ValidateGameInstallRoot(*state.gameInstallRoot).bValid) {
        AppendFilesTabLog(state, "Scenario export needs a valid game install root — set one first.");
        return false;
    }
    const std::string overridePath = (state.scenarioRuntimeOverridePath != nullptr)
        ? *state.scenarioRuntimeOverridePath : std::string();
    state.lastScenarioExportResult = Io::ExportMapScenario(*state.gameInstallRoot, recipe,
        state.scenarioRuntimeResourceDirectory, overridePath);
    AppendFilesTabLog(state, state.lastScenarioExportResult.debugLog);
    return state.lastScenarioExportResult.bDataLuaWritten
        || state.lastScenarioExportResult.bRuntimeCopied;
}

std::vector<std::string> ScenariosNeedingSpawnsAcknowledgment(const Params::Scenarios& scenarios) {
    std::vector<std::string> affected;
    for (const Params::PatternScenario& scenario : scenarios.patternScenarios)
        if (ScenarioNeedsSpawnsAcknowledgment(scenario.body))
            affected.push_back(ScenarioRowLabel(scenario.body));
    for (const Params::CountScenario& scenario : scenarios.countScenarios)
        if (ScenarioNeedsSpawnsAcknowledgment(scenario.body))
            affected.push_back(ScenarioRowLabel(scenario.body));
    return affected;
}

} // namespace Ui
} // namespace SanmapGen
