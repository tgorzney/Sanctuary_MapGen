// FilesTab_ScenarioFullDataImport_Actions_UI.cpp — see the header for the contract. Headless: no
// imgui frame, no window, no GL context, same posture as every other Files-tab action.
#include "FilesTab_ScenarioFullDataImport_Actions_UI.h"
#include "FilesTab_UI.h"
#include "FilesTab_ScenarioImportReview_UI.h"
#include "../io/ScenarioScript_FullDataImport_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Ui {
namespace {

// STEP266 §3 — one line per shape, never a single pass/fail bool, extending
// ScenarioScript_AreaImport_IO's own per-flag banner style (FilesTab_ScenarioExportRow_Draw_UI.cpp's
// precedent) to the three shapes the IO layer's own `debugLog` does not summarize (it only ever logs
// area collisions/near-misses — see ScenarioScript_FullDataImport_IO.cpp's own header comment). Every
// near-miss's `reason` string is appended verbatim, never swallowed (§3's own explicit requirement).
std::string BuildScenarioFullDataImportBanner(const Io::ScenarioFullDataImportResult& result) {
    std::string banner;
    auto appendLine = [&banner](const std::string& line) { banner += line; banner += '\n'; };

    appendLine("Areas: " + std::to_string(result.writtenAreaNames.size()) + " imported, "
              + std::to_string(result.skippedCollisionAreaNames.size()) + " collisions, "
              + std::to_string(result.areas.nearMisses.size()) + " near-misses.");

    appendLine("Match conditions: " + std::to_string(result.matchConditions.conditionSets.size())
              + " candidate set(s) extracted, " + std::to_string(result.matchConditions.nearMisses.size())
              + " near-miss(es).");
    for (const auto& nearMiss : result.matchConditions.nearMisses)
        appendLine("  match-condition near-miss '" + nearMiss.identifier + "': " + nearMiss.reason);

    appendLine("Slot patterns: " + std::to_string(result.slotPatterns.entries.size())
              + " candidate(s) extracted, " + std::to_string(result.slotPatterns.nearMisses.size())
              + " near-miss(es).");
    for (const auto& nearMiss : result.slotPatterns.nearMisses)
        appendLine("  slot-pattern near-miss '" + nearMiss.identifier + "': " + nearMiss.reason);

    appendLine("Unit placements: " + std::to_string(result.unitPlacements.placements.size())
              + " candidate(s) extracted, " + std::to_string(result.unitPlacements.nearMisses.size())
              + " near-miss(es).");
    for (const auto& nearMiss : result.unitPlacements.nearMisses)
        appendLine("  unit-placement near-miss '" + nearMiss.armyName + "': " + nearMiss.reason);

    return banner;
}

} // namespace

bool RunImportScenarioFullData(FilesTabState& state, Params::MapRecipe& recipe) {
    if (state.scenarioFullDataImportPath.empty()) {
        AppendFilesTabLog(state, "Import refused: no scenario script .lua path is set.");
        return false;
    }
    const Io::ScenarioFullDataImportResult result = Io::ImportFullScenarioDataFromScenarioScriptFile(
        state.scenarioFullDataImportPath, recipe.geometry.mapSize, recipe);
    AppendFilesTabLog(state, result.debugLog);
    const bool bRefused = result.bRefusedAsSanGenOwnedFile || result.bRefusedUnreadableFile
        || result.bRefusedOversizedFile;
    // Mirrors ScenarioScript_FullDataImport_IO's own "left completely untouched on every refusal
    // path" posture (its header comment): a refused click must not clobber whatever the review panel
    // is showing from a PREVIOUS successful import.
    if (!bRefused) {
        AppendFilesTabLog(state, BuildScenarioFullDataImportBanner(result));
        ResetScenarioImportReviewState(state.scenarioImportReview, result.matchConditions.conditionSets,
                                       result.slotPatterns.entries, result.unitPlacements.placements);
    }
    return !bRefused;
}

} // namespace Ui
} // namespace SanmapGen
