// FilesTab_ScenarioFullDataImport_UI_Test.cpp — STEP266 acceptance: the "Import Scenario Data
// (Conditions/Patterns/Placements)" button (RunFilesTabAction(FilesTabAction::ImportScenarioFullData,
// ...)). Headless: no imgui frame, no window, no GL context, same posture as
// FilesTab_ScenarioAreaImport_UI_Test.cpp, whose fixture-writing helpers this file mirrors.
#include "FilesTab_TestSupport_UI.h"
#include "FilesTab_UI.h"
#include "../io/FilesystemPrimitives_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include <fstream>

namespace SanmapGen {
namespace FilesTabTest {
namespace {

// One of each of the four ratified Part-B shapes, PLUS one deliberate near-miss per one of the
// three non-area shapes (an area near-miss is not exercised here — ScenarioScript_AreaRectangleExtract_IO_Test.cpp
// already covers that grammar in isolation):
//   - match conditions: a valid Template-A chain, plus an `or`-containing body (an automatic
//     near-miss for the whole field, ScenarioScript_MatchConditionExtract_IO.h's own doc comment).
//   - slot patterns: a valid {name, pattern} entry, plus a `pattern`-only entry with no sibling
//     `name` (a near-miss, ScenarioScript_SlotPatternExtract_IO.h's own doc comment).
//   - unit placements: a valid armyName-keyed row, plus an armyIndex-keyed row (REJECTED as a
//     near-miss naming exactly that ambiguity, ScenarioScript_UnitPlacementExtract_IO.h's own
//     "Binding rule").
const char* kOneOfEachShapePlusNearMissesSource =
    "local AREA_356 = { x = 846, y = 846, width = 356, height = 356 }\n"
    "\n"
    "local COUNT_SCENARIOS = {\n"
    "    { name = \"1v1\", match = function(t, h, a, pattern) return t == 2 end, area = AREA_356 },\n"
    "    { name = \"2v2\", match = function(t, h, a, pattern) return t == 4 or h == 2 end, area = AREA_356 },\n"
    "}\n"
    "\n"
    "local PATTERN_SCENARIOS = {\n"
    "    { name = \"4human-slots5-8\", pattern = \"----hhhh--------\" },\n"
    "    { pattern = \"hhhh------------\" },\n"
    "}\n"
    "\n"
    "local UNIT_PLACEMENTS = {\n"
    "    { armyName = \"ARMY_01\", templateIdentifier = \"ucn3001\", x = 10, y = 0, z = 20 },\n"
    "    { armyIndex = 1, templateIdentifier = \"ucn3002\", x = 5, y = 0, z = 5 },\n"
    "}\n";

std::string WriteScratchFile(const std::string& folder, const char* fileName, const std::string& contents) {
    const std::string filePath = Io::JoinExportPath(folder, fileName);
    std::ofstream outputStream(filePath, std::ios::binary | std::ios::trunc);
    outputStream << contents;
    outputStream.close();
    return filePath;
}

const Params::MapArea* FindAreaByName(const Params::MapRecipe& recipe, const std::string& name) {
    for (const Params::MapArea& area : recipe.areas) if (area.name == name) return &area;
    return nullptr;
}

bool LogContains(const std::string& log, const char* substring) {
    return log.find(substring) != std::string::npos;
}

// Work-order test 1 + test 5: one call populates all three review candidate lists AND recipe.areas
// (the unchanged area-import behavior), and the banner reports per-shape counts plus every
// near-miss's own reason, never swallowed.
void CheckSuccessfulClickPopulatesCandidatesAreasAndBanner() {
    const std::string folder = ScratchFolderPath("SanGenFilesTabScenarioFullDataImportSuccess");
    std::error_code createError;
    std::filesystem::create_directories(folder, createError);
    const std::string filePath =
        WriteScratchFile(folder, "ForeignMap_Scenarios_Script.lua", kOneOfEachShapePlusNearMissesSource);

    Ui::FilesTabState state;
    state.scenarioFullDataImportPath = filePath;
    Params::MapRecipe recipe;
    Check(Ui::RunFilesTabAction(Ui::FilesTabAction::ImportScenarioFullData, state, recipe, nullptr),
          "a valid foreign scenario .lua path succeeds");

    // recipe.areas: the ONE unchanged, auto-reconciled behavior.
    Check(recipe.areas.size() == 1, "the area rectangle landed in recipe.areas");
    const Params::MapArea* area = FindAreaByName(recipe, "AREA_356");
    Check(area != nullptr && area->width == 356.0f, "AREA_356's field values came through correctly");

    // The three non-auto-attached candidate lists, stashed for the review panel.
    Check(state.scenarioImportReview.matchConditionCandidates.size() == 1,
          "exactly the one valid match-condition candidate set was stashed");
    Check(state.scenarioImportReview.slotPatternCandidates.size() == 1,
          "exactly the one valid slot-pattern candidate was stashed");
    Check(state.scenarioImportReview.unitPlacementCandidates.size() == 1,
          "exactly the one valid unit-placement candidate was stashed");
    Check(state.scenarioImportReview.matchConditionTargetComboIndex.size() == 1
              && state.scenarioImportReview.matchConditionTargetComboIndex[0] == -1,
          "the new candidate's target picker starts unpicked (-1) -- never auto-selected");

    // The per-shape banner: counts plus every near-miss reason, verbatim, never swallowed.
    Check(LogContains(state.debugLog, "Match conditions: 1 candidate set(s) extracted, 1 near-miss(es)."),
          "the match-condition count line reports one accepted, one near-miss");
    Check(LogContains(state.debugLog, "Slot patterns: 1 candidate(s) extracted, 1 near-miss(es)."),
          "the slot-pattern count line reports one accepted, one near-miss");
    Check(LogContains(state.debugLog, "Unit placements: 1 candidate(s) extracted, 1 near-miss(es)."),
          "the unit-placement count line reports one accepted, one near-miss");
    Check(LogContains(state.debugLog, "match-condition near-miss"),
          "the match-condition near-miss's own reason is appended to the log, not swallowed");
    Check(LogContains(state.debugLog, "slot-pattern near-miss"),
          "the slot-pattern near-miss's own reason is appended to the log, not swallowed");
    Check(LogContains(state.debugLog, "unit-placement near-miss"),
          "the unit-placement near-miss's own reason is appended to the log, not swallowed");
}

// An empty path is refused with a logged reason, touches neither recipe nor the review state.
void CheckEmptyPathIsRefusedAndTouchesNothing() {
    Ui::FilesTabState state;   // scenarioFullDataImportPath left empty
    Params::MapRecipe recipe;
    Check(!Ui::RunFilesTabAction(Ui::FilesTabAction::ImportScenarioFullData, state, recipe, nullptr),
          "an empty scenarioFullDataImportPath is refused");
    Check(!state.debugLog.empty(), "with the reason logged");
    Check(recipe.areas.empty(), "recipe.areas is untouched by the refusal");
    Check(state.scenarioImportReview.matchConditionCandidates.empty()
              && state.scenarioImportReview.slotPatternCandidates.empty()
              && state.scenarioImportReview.unitPlacementCandidates.empty(),
          "the review panel's candidate lists are untouched by the refusal");
}

// The action's label is non-empty (uniqueness across all eleven actions is FilesTab_UI_Test.cpp's
// own CheckEveryActionIsLabelledAndClassified job already).
void CheckLabelIsNonEmpty() {
    const std::string label = Ui::FilesTabActionLabel(Ui::FilesTabAction::ImportScenarioFullData);
    Check(!label.empty(), "ImportScenarioFullData carries a non-empty caption");
}

} // namespace

void RunScenarioFullDataImportTests() {
    CheckSuccessfulClickPopulatesCandidatesAreasAndBanner();
    CheckEmptyPathIsRefusedAndTouchesNothing();
    CheckLabelIsNonEmpty();
}

} // namespace FilesTabTest
} // namespace SanmapGen
