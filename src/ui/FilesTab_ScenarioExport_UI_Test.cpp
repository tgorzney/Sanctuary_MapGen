// FilesTab_ScenarioExport_UI_Test.cpp — acceptance test for STEP77's
// FilesTabAction::ExportScenarioScript and its mandatory-spawns pre-check. Headless: no imgui
// frame, no window, no GL context — RunFilesTabAction is a path plus one call into IO
// (Io::ExportMapScenario, STEP71), exactly like every other Files-tab action.
//
// "Stubbed" here follows ScenarioScript_Export_IO_Test.cpp's own posture (its header comment): the
// REAL Io::ExportMapScenario runs against a scratch filesystem shaped like a valid game install
// root — never a hand-rolled production stub of the function itself.
#include "FilesTab_ScenarioExport_Actions_UI.h"
#include "FilesTab_TestSupport_UI.h"
#include "FilesTab_UI.h"
#include "../params/MapRecipe_PARAMS.h"

#include <filesystem>
#include <fstream>
#include <string>

namespace SanmapGen {
namespace FilesTabTest {
namespace {

void WriteTextFile(const std::string& filePath, const std::string& contents) {
    std::error_code folderError;
    std::filesystem::create_directories(std::filesystem::path(filePath).parent_path(), folderError);
    std::ofstream outputStream(filePath, std::ios::binary | std::ios::trunc);
    outputStream << contents;
}

// Both required subpaths, per GameInstallLocation_IO (STEP64) — mirrors
// ScenarioScript_Export_IO_Test.cpp's own MakeValidGameInstallRoot verbatim.
void MakeValidGameInstallRoot(const std::string& root) {
    std::error_code folderError;
    std::filesystem::create_directories(root + "/engine/LJ/lua", folderError);
    std::filesystem::create_directories(root + "/engine/Sanctuary_Data/Maps", folderError);
}

// 5. gameInstallRoot == nullptr -> RunFilesTabAction logs and returns false, no crash.
void TestNullGameInstallRootSafelyRefuses() {
    Ui::FilesTabState state;
    state.gameInstallRoot = nullptr;
    Params::MapRecipe recipe;
    recipe.mapName = "NullRootTest";
    Check(!Ui::RunFilesTabAction(Ui::FilesTabAction::ExportScenarioScript, state, recipe, nullptr),
          "null gameInstallRoot: RunFilesTabAction returns false");
    Check(!state.debugLog.empty(), "null gameInstallRoot: the reason is logged, not a silent no-op");
}

// 6. A valid root + fixture recipe -> the real Io::ExportMapScenario reports a file written;
// lastScenarioExportResult reflects it verbatim (no reinterpretation).
void TestValidRootExportSucceedsAndResultIsForwardedVerbatim() {
    const std::string root = ScratchFolderPath("STEP77_ScenarioExport_ValidRoot");
    MakeValidGameInstallRoot(root);
    const std::string bundledDirectory = ScratchFolderPath("STEP77_ScenarioExport_Bundled");
    WriteTextFile(bundledDirectory + "/SanGenScenarioRuntime.lua",
                 "-- SanGen-generated Map Scenario runtime script\nScenario = {}\n");

    std::string gameInstallRoot = root;
    Ui::FilesTabState state;
    state.gameInstallRoot = &gameInstallRoot;   // caller-owned pointer, same posture as Application's
    state.scenarioRuntimeResourceDirectory = bundledDirectory;
    Params::MapRecipe recipe;
    recipe.mapName = "ScenarioExportFixtureMap";
    recipe.geometry.mapSize = 4;

    const bool bSucceeded =
        Ui::RunFilesTabAction(Ui::FilesTabAction::ExportScenarioScript, state, recipe, nullptr);

    Check(state.lastScenarioExportResult.bDataLuaWritten || state.lastScenarioExportResult.bRuntimeCopied,
          "valid root: at least one file reports written");
    Check(bSucceeded == (state.lastScenarioExportResult.bDataLuaWritten
                        || state.lastScenarioExportResult.bRuntimeCopied),
          "valid root: RunFilesTabAction's return value matches the stub's own flags verbatim");
    Check(!state.debugLog.empty(), "valid root: the export's own debugLog reaches the panel");
}

// 7. The mandatory-spawns pre-check (pure, no imgui): a Tier 1/2 entry with empty spawns AND an
// empty authoringNote opens the gate, naming it; a fully-acknowledged roster does not.
void TestMandatorySpawnsPreCheck() {
    Params::Scenarios scenarios;
    Params::CountScenario needsAcknowledgment;
    needsAcknowledgment.body.name = "RiskyComposition";
    // spawns left empty, authoringNote left empty -> flagged (STEP74's own rule).
    scenarios.countScenarios.push_back(needsAcknowledgment);

    const std::vector<std::string> affectedWhenRisky =
        Ui::ScenariosNeedingSpawnsAcknowledgment(scenarios);
    Check(affectedWhenRisky.size() == 1, "risky roster: exactly one scenario flagged");
    Check(!affectedWhenRisky.empty() && affectedWhenRisky[0] == "RiskyComposition",
          "risky roster: the flagged scenario is named");

    Params::Scenarios acknowledged;
    Params::PatternScenario patternWithNote;
    patternWithNote.body.name = "DocumentedPattern";
    patternWithNote.body.authoringNote = "Intentional: spawns inherited from the baseline map.";
    acknowledged.patternScenarios.push_back(patternWithNote);
    Params::CountScenario countWithSpawns;
    countWithSpawns.body.name = "SpawnedComposition";
    countWithSpawns.body.spawns.push_back(Params::ScenarioSpawn());
    acknowledged.countScenarios.push_back(countWithSpawns);

    const std::vector<std::string> affectedWhenAcknowledged =
        Ui::ScenariosNeedingSpawnsAcknowledgment(acknowledged);
    Check(affectedWhenAcknowledged.empty(),
          "fully-acknowledged roster: the gate does not open — nothing flagged");
}

} // namespace

void RunScenarioExportTests() {
    TestNullGameInstallRootSafelyRefuses();
    TestValidRootExportSucceedsAndResultIsForwardedVerbatim();
    TestMandatorySpawnsPreCheck();
}

} // namespace FilesTabTest
} // namespace SanmapGen
