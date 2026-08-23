// FilesTab_UI_Test.cpp — acceptance test for the Files / Save tab (section D). This unit holds the
// binary's main(), the action catalogue checks, the bounded log panel, and the refusal contract.
// Everything here is headless: an action is a path plus one call into IO, so the tab's whole
// behavior is drivable with no imgui frame, no window and no GL context. The three sections'
// pixels are a by-eye check against a live frame; nothing here asserts on them.
#include "FilesTab_TestSupport_UI.h"
#include "FilesTab_UI.h"
#include "../data/MapFields_DATA.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace FilesTabTest {
namespace {

void CheckEveryActionIsLabelledAndClassified() {
    Check(Ui::filesTabActionCount == 9, "STEP77 adds ExportScenarioScript — nine actions total");
    int bakedFieldActionCount = 0;
    std::vector<std::string> labels;
    for (int actionIndex = 0; actionIndex < Ui::filesTabActionCount; ++actionIndex) {
        const Ui::FilesTabAction action = static_cast<Ui::FilesTabAction>(actionIndex);
        const char* label = Ui::FilesTabActionLabel(action);
        Check(label != nullptr && *label != '\0', "every action carries a caption");
        labels.push_back(label);
        if (Ui::FilesTabActionNeedsBakedFields(action)) ++bakedFieldActionCount;
    }
    Check(bakedFieldActionCount == 5,
          "the five texture-writing actions are the ones that need baked fields");
    Check(!Ui::FilesTabActionNeedsBakedFields(Ui::FilesTabAction::ExportSanmapOnly),
          "and 'sanmap only' is not one of them — the recipe alone is enough");
    Check(!Ui::FilesTabActionNeedsBakedFields(Ui::FilesTabAction::ExportScenarioScript),
          "ExportScenarioScript reads only recipe.scenarios/recipe.armies — no baked field needed");
    const std::string scenarioLabel = Ui::FilesTabActionLabel(Ui::FilesTabAction::ExportScenarioScript);
    Check(!scenarioLabel.empty(), "ExportScenarioScript's label is non-empty");
    int matchCount = 0;
    for (const std::string& label : labels) if (label == scenarioLabel) ++matchCount;
    Check(matchCount == 1, "ExportScenarioScript's label is unique among all nine actions");
}

void CheckTheLogPanelIsBoundedAndDropsWholeLines() {
    Ui::FilesTabState state;
    Ui::AppendFilesTabLog(state, std::string());
    Check(state.debugLog.empty(), "appending nothing logs nothing");
    Ui::AppendFilesTabLog(state, "first line");
    Check(state.debugLog == "first line\n", "a line with no newline of its own gets one");
    Ui::AppendFilesTabLog(state, "second line\n");
    Check(state.debugLog == "first line\nsecond line\n", "and a line that has one is not doubled");

    state.maximumDebugLogCharacterCount = 20;
    Ui::AppendFilesTabLog(state, "third line");
    Check(static_cast<int>(state.debugLog.size()) <= state.maximumDebugLogCharacterCount,
          "the panel is held inside its budget");
    Check(state.debugLog == "third line\n",
          "by dropping WHOLE lines off the front: newest output kept, never a half line");
}

void CheckActionsAreRefusedWithAReasonRatherThanHalfDone() {
    Ui::FilesTabState state;
    Params::MapRecipe recipe;
    Check(!Ui::RunFilesTabAction(Ui::FilesTabAction::OpenSanmap, state, recipe, nullptr),
          "opening with no path set is refused");
    Check(!state.debugLog.empty(), "with the reason in the panel");

    state.debugLog.clear();
    Check(!Ui::RunFilesTabAction(Ui::FilesTabAction::ImportSupComLua, state, recipe, nullptr),
          "the SupCom action with no importer bound is refused (SCOPE NOTE 1)");
    Check(!state.debugLog.empty(), "and says so instead of pretending to work");

    state.debugLog.clear();
    Check(!Ui::RunFilesTabAction(Ui::FilesTabAction::ExportHeightmapRaw, state, recipe, nullptr),
          "a texture export with no fields bound is refused");
    Data::MapFields emptyFields;
    Check(!Ui::RunFilesTabAction(Ui::FilesTabAction::ExportAll, state, recipe, &emptyFields),
          "and so is one whose fields have never been sized");
}

void CheckAMissingDestinationIsRefusedEvenWithFields() {
    Ui::FilesTabState state;
    Params::MapRecipe recipe;
    Data::MapFields fields;
    fields.Resize(recipe.geometry.VertexSize(), 0.0f);
    Check(!Ui::RunFilesTabAction(Ui::FilesTabAction::ExportSlopeImage, state, recipe, &fields),
          "with fields but no destination folder a texture export is refused");
    Check(!Ui::RunFilesTabAction(Ui::FilesTabAction::ExportSanmapOnly, state, recipe, &fields),
          "and so is the document export");
    Check(!state.debugLog.empty(), "with the reason logged both times");
}

} // namespace

void RunTabStateTests() {
    CheckEveryActionIsLabelledAndClassified();
    CheckTheLogPanelIsBoundedAndDropsWholeLines();
    CheckActionsAreRefusedWithAReasonRatherThanHalfDone();
    CheckAMissingDestinationIsRefusedEvenWithFields();
}

} // namespace FilesTabTest
} // namespace SanmapGen

int main() {
    SanmapGen::FilesTabTest::RunTabStateTests();
    SanmapGen::FilesTabTest::RunRoundTripTests();
    SanmapGen::FilesTabTest::RunScenarioExportTests();
    if (SanmapGen::FilesTabTest::FailureCount() == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", SanmapGen::FilesTabTest::FailureCount());
    return 1;
}
