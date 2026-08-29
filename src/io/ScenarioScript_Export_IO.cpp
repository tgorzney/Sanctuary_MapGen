// ScenarioScript_Export_IO.cpp -- see ScenarioScript_Export_IO.h for the file-level contract.
// Implements the 8-step sequence of STEP71 §3: validate root -> ensure the map script folder ->
// existence-only orchestrator check -> render Data.lua -> syntax pre-check -> banner-gated write
// for Data.lua -> resolve+syntax-check+banner-gated write for Runtime.lua -> return. Every failure
// path degrades gracefully (Constitution §6) -- never a crash, never a partial/corrupt write, never
// one file's refusal blocking the other's write.
#include "ScenarioScript_Export_IO.h"
#include "FilesystemPrimitives_IO.h"
#include "GameInstallLocation_IO.h"
#include "MapExporter_ScenarioAreaNameValidation_IO.h"
#include "ScenarioScript_DataLua_IO.h"
#include "ScenarioScript_RuntimeResource_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../sys/LuaSyntaxCheck_SYS.h"
#include <filesystem>

namespace SanmapGen {
namespace Io {
namespace {

bool StartsWithBanner(const std::string& text) {
    const std::string banner(kScenarioGeneratedFileBannerLine);
    return text.compare(0, banner.size(), banner) == 0;
}

// Shared banner-gated read/overwrite/collision logic for one SanGen-owned scenario file
// (`MAP_SCENARIO_SPEC.md` §2.1 point 3, `ARCH_15_04_ThreeFileOnDiskShape.md` §15.4). Never touches
// the original file when it is occupied by unrecognized content -- writes the freshly-rendered text
// to a `.sangen-pending.lua` sibling instead.
void WriteWithOverwriteSafety(const std::string& targetPath, const std::string& pendingPath,
                              const std::string& renderedText, bool& bWrittenFlag,
                              bool& bCollisionFlag, ScenarioExportResult& result) {
    std::string existingText;
    const bool bExists = ReadTextFileBytes(targetPath, existingText);
    if (!bExists || StartsWithBanner(existingText)) {
        WriteBinaryFileBytes(targetPath, renderedText.data(), renderedText.size());
        bWrittenFlag = true;
        result.writtenFilePaths.push_back(targetPath);
        return;
    }
    WriteBinaryFileBytes(pendingPath, renderedText.data(), renderedText.size());
    bCollisionFlag = true;
    result.Log("unrecognized content already occupies " + targetPath +
              " -- the freshly-rendered text was written to " + pendingPath +
              " instead; the original was left untouched.");
}

} // namespace

ScenarioExportResult ExportMapScenario(const std::string& gameInstallRoot,
                                       const Params::MapRecipe& recipe,
                                       const std::string& runtimeResourceDirectory,
                                       const std::string& runtimeOverridePathOrEmpty) {
    ScenarioExportResult result;

    // 1. Validate the game install root (STEP64).
    const GameInstallRootValidation rootValidation = ValidateGameInstallRoot(gameInstallRoot);
    if (!rootValidation.bValid) {
        result.Log(rootValidation.reason);
        return result;
    }

    // 2. Ensure the map script directory exists.
    const std::string mapScriptDirectory =
        JoinExportPath(JoinExportPath(JoinExportPath(gameInstallRoot, "engine"), "LJ/lua/maps"),
                       recipe.mapName);
    std::string folderErrorMessage;
    if (!EnsureFolderExists(mapScriptDirectory, folderErrorMessage)) {
        result.Log(folderErrorMessage);
        return result;
    }

    // 3. Existence-only check for the hand-authored orchestrator -- never opened/read/written.
    std::error_code existenceError;
    result.bOrchestratorPresent = std::filesystem::exists(
        std::filesystem::path(JoinExportPath(mapScriptDirectory, recipe.mapName + "_data.lua")),
        existenceError);
    if (!result.bOrchestratorPresent) {
        result.Log("scenario files written but inert until " + recipe.mapName + "_data.lua exists");
    }

    // 4. Render the Data.lua text (STEP70).
    const std::string dataLuaText = BuildScenarioDataLuaText(recipe);

    // STEP209 -- warn, never block, on the same tier as the JSON leg's ReportScenarioAreaNameReferences.
    // ScenarioExportResult has no Warn/warningCount (only Log/debugLog) -- use Log here, matching this
    // type's own existing convention (every other finding in this file uses result.Log).
    const ScenarioAreaNameValidationReport areaNameReport = ValidateScenarioAreaNameReferences(recipe);
    if (!areaNameReport.AllReferencesResolve()) result.Log(areaNameReport.SummaryText());

    // 5. Syntax pre-check on SanGen's own render -- refuse rather than write known-broken Lua.
    const Sys::LuaSyntaxCheckResult dataSyntax = Sys::CheckLuaSyntax(dataLuaText);
    if (!dataSyntax.bSucceeded) {
        result.bDataLuaSyntaxCheckFailed = true;
        result.Log("Data.lua syntax check failed at line " + std::to_string(dataSyntax.lineNumber) +
                  ": " + dataSyntax.message);
    } else {
        // 6. Write-target safety for <MapName>_Scenarios_Data.lua.
        const std::string dataLuaPath =
            JoinExportPath(mapScriptDirectory, recipe.mapName + "_Scenarios_Data.lua");
        const std::string dataLuaPendingPath =
            JoinExportPath(mapScriptDirectory, recipe.mapName + "_Scenarios_Data.sangen-pending.lua");
        WriteWithOverwriteSafety(dataLuaPath, dataLuaPendingPath, dataLuaText,
                                 result.bDataLuaWritten, result.bDataLuaCollisionDetected, result);
    }

    // 7. Runtime resolution + write, symmetric to steps 5-6.
    const ScenarioRuntimeResourceResult runtimeResult =
        LoadScenarioRuntimeText(runtimeResourceDirectory, runtimeOverridePathOrEmpty);
    // Amendment (STEP72): errorMessage is diagnostic/advisory, non-empty on a successful
    // degrade-to-bundled as well as a hard failure -- log it whenever non-empty, never gated on
    // !bSucceeded alone.
    if (!runtimeResult.errorMessage.empty()) {
        result.Log(runtimeResult.errorMessage);
    }
    if (runtimeResult.bSucceeded) {
        const Sys::LuaSyntaxCheckResult runtimeSyntax = Sys::CheckLuaSyntax(runtimeResult.runtimeLuaText);
        if (!runtimeSyntax.bSucceeded) {
            result.bRuntimeSyntaxCheckFailed = true;
            result.Log("Runtime.lua syntax check failed at line " +
                      std::to_string(runtimeSyntax.lineNumber) + ": " + runtimeSyntax.message);
        } else {
            const std::string runtimeLuaPath =
                JoinExportPath(mapScriptDirectory, recipe.mapName + "_Scenarios_Runtime.lua");
            const std::string runtimeLuaPendingPath = JoinExportPath(
                mapScriptDirectory, recipe.mapName + "_Scenarios_Runtime.sangen-pending.lua");
            WriteWithOverwriteSafety(runtimeLuaPath, runtimeLuaPendingPath, runtimeResult.runtimeLuaText,
                                     result.bRuntimeCopied, result.bRuntimeCollisionDetected, result);
            result.Log("runtime resolved from source: " + runtimeResult.sourceDescription);
        }
    }
    // !bSucceeded -> bRuntimeCopied stays false; a missing/unreadable runtime never blocks the
    // Data.lua write already completed above, and never crashes the export.

    // 8. Return.
    return result;
}

} // namespace Io
} // namespace SanmapGen
