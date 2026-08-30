// FilesTab_ScenarioAreaImport_UI_Test.cpp — STEP224 acceptance: the "Import Areas from Scenario
// Script..." button (RunFilesTabAction(FilesTabAction::ImportScenarioAreas, ...)). Headless: no
// imgui frame, no window, no GL context, same posture as every other Files-tab test unit. Reuses
// the minimal foreign-scenario fixture shape ScenarioScript_AreaImport_IO_Test.cpp already
// exercises (a plain `local NAME = { x=, y=, width=, height= }` block under a non-SanGen-owned
// filename) rather than inventing a new one.
#include "FilesTab_TestSupport_UI.h"
#include "FilesTab_UI.h"
#include "../io/FilesystemPrimitives_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include <fstream>

namespace SanmapGen {
namespace FilesTabTest {
namespace {

// Same shape as ScenarioScript_AreaImport_IO_Test.cpp's own `kRealNamedKeyBlock` fixture.
const char* kRealNamedKeyBlock =
    "local AREA_356 = { x = 846, y = 846, width = 356, height = 356 }\n"
    "local AREA_169 = { x = 668.4444444444445, y = 824, width = 711.1111111111111, height = 400 }\n";

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

// (a) A successful click adds the expected rectangles into recipe.areas by name.
void CheckSuccessfulClickAddsExpectedAreasByName() {
    const std::string folder = ScratchFolderPath("SanGenFilesTabScenarioAreaImportSuccess");
    std::error_code createError;
    std::filesystem::create_directories(folder, createError);
    const std::string filePath =
        WriteScratchFile(folder, "ForeignMap_Scenarios_Script.lua", kRealNamedKeyBlock);

    Ui::FilesTabState state;
    state.scenarioAreaImportPath = filePath;
    Params::MapRecipe recipe;
    Check(Ui::RunFilesTabAction(Ui::FilesTabAction::ImportScenarioAreas, state, recipe, nullptr),
          "a valid foreign scenario .lua path succeeds");
    Check(recipe.areas.size() == 2, "both rectangles landed in recipe.areas");
    const Params::MapArea* area356 = FindAreaByName(recipe, "AREA_356");
    Check(area356 != nullptr && area356->width == 356.0f,
          "AREA_356's field values came through correctly");
    const Params::MapArea* area169 = FindAreaByName(recipe, "AREA_169");
    Check(area169 != nullptr, "AREA_169 landed too");
    // NOTE: a clean import with no collisions/near-misses leaves `result.debugLog` empty
    // (ScenarioScript_AreaImport_IO.cpp only calls Log() on a refusal/collision/near-miss/cap path),
    // so AppendFilesTabLog's own no-op-on-empty-text guard means the panel stays untouched here --
    // not a bug, matching the IO layer's own acceptance test (which asserts no such thing either).
}

// (b) An empty path is refused with a logged reason and touches nothing.
void CheckEmptyPathIsRefusedAndTouchesNothing() {
    Ui::FilesTabState state;   // scenarioAreaImportPath left empty
    Params::MapRecipe recipe;
    Check(!Ui::RunFilesTabAction(Ui::FilesTabAction::ImportScenarioAreas, state, recipe, nullptr),
          "an empty scenarioAreaImportPath is refused");
    Check(!state.debugLog.empty(), "with the reason logged");
    Check(recipe.areas.empty(), "recipe.areas is untouched by the refusal");
}

// (c) The action's label is non-empty (and distinct — FilesTab_UI_Test.cpp's own
// CheckEveryActionIsLabelledAndClassified already asserts uniqueness across all ten actions).
void CheckLabelIsNonEmpty() {
    const std::string label = Ui::FilesTabActionLabel(Ui::FilesTabAction::ImportScenarioAreas);
    Check(!label.empty(), "ImportScenarioAreas carries a non-empty caption");
}

} // namespace

void RunScenarioAreaImportTests() {
    CheckSuccessfulClickAddsExpectedAreasByName();
    CheckEmptyPathIsRefusedAndTouchesNothing();
    CheckLabelIsNonEmpty();
}

} // namespace FilesTabTest
} // namespace SanmapGen
