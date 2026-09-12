// ScenarioScript_RuntimeResource_IO_Test.cpp — acceptance test for the bundled/override Map
// Scenario runtime Lua resolver (STEP72). Scratch-directory pattern per MapExporter_IO_Test.cpp:25-31.
// argv[1] is the staged shader directory (unused here); argv[2] is the CMake-staged
// SANGEN_V2_LUA_RESOURCE_DIRECTORY holding the real bundled SanGenScenarioRuntime.lua.
#include "ScenarioScript_RuntimeResource_IO.h"
#include "FilesystemPrimitives_IO.h"
#include "../sys/LuaSyntaxCheck_SYS.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

using namespace SanmapGen;

static int failureCount = 0;

static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

static std::string ScratchFolderPath(const char* suffix) {
    std::error_code pathError;
    const std::filesystem::path folder =
        std::filesystem::temp_directory_path(pathError) /
        (std::string("SanGenScenarioRuntimeResourceTest_") + suffix);
    std::filesystem::remove_all(folder, pathError);
    std::filesystem::create_directories(folder, pathError);
    return folder.string();
}

static void WriteTextFile(const std::string& filePath, const std::string& contents) {
    std::ofstream outputStream(filePath, std::ios::binary | std::ios::trunc);
    outputStream << contents;
}

// 1. Override path set and readable.
static void TestOverridePathReadableWinsCleanly() {
    const std::string scratchFolder = ScratchFolderPath("Override");
    const std::string overridePath = Io::JoinExportPath(scratchFolder, "MyOverride.lua");
    WriteTextFile(overridePath, "-- override content\nScenario = {}\n");

    const Io::ScenarioRuntimeResourceResult result =
        Io::LoadScenarioRuntimeText("D:/never/read/this", overridePath);

    Check(result.bSucceeded, "an override path that exists and is readable succeeds");
    Check(result.sourceDescription == "override", "the source is reported as override");
    Check(result.errorMessage.empty(), "a clean override resolution carries no diagnostic");
    Check(result.runtimeLuaText == "-- override content\nScenario = {}\n",
          "the returned text matches the seeded override content exactly");
}

// 2. Override empty, valid bundled directory.
static void TestEmptyOverrideFallsThroughToBundled() {
    const std::string scratchFolder = ScratchFolderPath("Bundled");
    const std::string bundledPath = Io::JoinExportPath(scratchFolder, "SanGenScenarioRuntime.lua");
    WriteTextFile(bundledPath, "-- bundled content\nScenario = {}\n");

    const Io::ScenarioRuntimeResourceResult result = Io::LoadScenarioRuntimeText(scratchFolder, "");

    Check(result.bSucceeded, "an empty override resolves against a valid bundled directory");
    Check(result.sourceDescription == "bundled", "the source is reported as bundled");
}

// 3. Loud degrade proof.
static void TestUnreadableOverrideDegradesLoudlyToBundled() {
    const std::string scratchFolder = ScratchFolderPath("Degrade");
    const std::string bundledPath = Io::JoinExportPath(scratchFolder, "SanGenScenarioRuntime.lua");
    WriteTextFile(bundledPath, "-- bundled content\nScenario = {}\n");
    const std::string nonexistentOverridePath = Io::JoinExportPath(scratchFolder, "DoesNotExist.lua");

    const Io::ScenarioRuntimeResourceResult result =
        Io::LoadScenarioRuntimeText(scratchFolder, nonexistentOverridePath);

    Check(result.bSucceeded, "a bad override degrades to bundled rather than hard-failing");
    Check(result.sourceDescription == "bundled", "the degrade actually used the bundled text");
    Check(!result.errorMessage.empty(), "the degrade is never silent -- errorMessage is populated");
    Check(result.errorMessage.find(nonexistentOverridePath) != std::string::npos,
          "the diagnostic names the override path that failed");
}

// 4. Neither readable.
static void TestNeitherReadableFails() {
    const std::string scratchFolder = ScratchFolderPath("NeitherReadable");
    // scratchFolder deliberately has no SanGenScenarioRuntime.lua in it.
    const std::string nonexistentOverridePath = Io::JoinExportPath(scratchFolder, "DoesNotExist.lua");

    const Io::ScenarioRuntimeResourceResult result =
        Io::LoadScenarioRuntimeText(scratchFolder, nonexistentOverridePath);

    Check(!result.bSucceeded, "neither an override nor a bundled default readable fails outright");
    const std::string expectedBundledPath = Io::JoinExportPath(scratchFolder, "SanGenScenarioRuntime.lua");
    Check(result.errorMessage.find(expectedBundledPath) != std::string::npos,
          "the failure names the bundled path that was attempted");
    Check(result.runtimeLuaText.empty(), "no partial text is returned on a hard failure");
}

// 5. Readable override short-circuits before the bundled directory is ever required to exist.
static void TestReadableOverrideShortCircuitsBeforeTouchingBundledDirectory() {
    const std::string scratchFolder = ScratchFolderPath("ShortCircuit");
    const std::string overridePath = Io::JoinExportPath(scratchFolder, "MyOverride.lua");
    WriteTextFile(overridePath, "-- override content\n");
    const std::string garbageBundledDirectory =
        Io::JoinExportPath(scratchFolder, "no_such_subdirectory_at_all/deeper_still");

    const Io::ScenarioRuntimeResourceResult result =
        Io::LoadScenarioRuntimeText(garbageBundledDirectory, overridePath);

    Check(result.bSucceeded, "a readable override succeeds even with a garbage bundled directory");
    Check(result.sourceDescription == "override", "and the bundled directory was never consulted");
}

// 6. Real bundled resource self-check, driven off the CMake-staged directory (argv[2]).
// Correction 2026-08-22: item (a) of the ticket's original acceptance test depended on STEP70
// (Io::kScenarioGeneratedFileBannerLine), which did not exist in this worktree yet -- replaced with a
// hardcoded literal comparison, per the ticket's correction. Item (d) depended on STEP65
// (Sys::CheckLuaSyntax) and was skipped outright at the time; STEP251 closes that TODO below now
// that STEP65 has shipped.
static void TestRealBundledResourceSelfCheck(const std::string& luaResourceDirectory) {
    if (luaResourceDirectory.empty()) {
        std::printf("SKIP TestRealBundledResourceSelfCheck: no lua resource directory given (argv[2])\n");
        return;
    }
    const Io::ScenarioRuntimeResourceResult result = Io::LoadScenarioRuntimeText(luaResourceDirectory, "");
    Check(result.bSucceeded, "the real staged bundled resource resolves");
    Check(result.sourceDescription == "bundled", "and it is reported as bundled");

    const std::string& text = result.runtimeLuaText;

    // (a) TODO(after STEP65+STEP70 land): replace this literal with Io::kScenarioGeneratedFileBannerLine
    // ReadTextFileBytes reads verbatim (Constitution §6 byte fidelity, pinned by this file's own
    // TestReadTextFileBytesCoverage), and on a Windows checkout with core.autocrlf on, the real
    // on-disk resource legitimately carries CRLF even though the git blob is LF-only -- strip a
    // trailing '\r' before the literal compare so this check pins the banner TEXT, not the host's
    // line-ending convention.
    std::string firstLine = text.substr(0, text.find('\n'));
    if (!firstLine.empty() && firstLine.back() == '\r') firstLine.pop_back();
    Check(firstLine == "-- GENERATED BY SANGEN -- DO NOT HAND-EDIT (regenerated on "
                        "every export)",
          "the bundled resource opens with the generated-file banner literal");

    Check(text.find("Scenario = {}") != std::string::npos,
          "Scenario stays a GLOBAL table declaration");
    // "local Scenario" alone is NOT the right substring: it false-positive-matches the unrelated
    // "local ScenarioData = Import(...)" declaration a few lines below (ScenarioData begins with
    // "Scenario"). The real regression this guards against is the DECLARATION itself becoming
    // `local Scenario = {}` (Part 1's own comment: "MUST stay a GLOBAL table ... never `local`").
    Check(text.find("local Scenario = {}") == std::string::npos,
          "and is never accidentally localized (Import() would silently yield nothing)");
    Check(text.find("function Scenario.ResolveAndApply") != std::string::npos,
          "ResolveAndApply is defined");
    Check(text.find("function Scenario.SpawnUnits") != std::string::npos,
          "SpawnUnits (the generic executor replacing SpawnNavalFleets) is defined");
    // STEP204 (naval retirement correction): negative assertions prove the dead naval machinery
    // was actually removed, not merely supplemented.
    Check(text.find("SpawnNavalFleets") == std::string::npos,
          "the retired SpawnNavalFleets function name is gone");
    Check(text.find("NAVAL_") == std::string::npos,
          "no NAVAL_* tuning constant survives");
    Check(text.find("spawnsUnits") != std::string::npos,
          "the renamed spawnsUnits field is read by ResolveAndApply");
    Check(text.find("matchedScenario.navy") == std::string::npos,
          "the retired matchedScenario.navy read is gone");

    // (d) STEP251 -- closes this file's own pre-existing TODO: Sys::CheckLuaSyntax (STEP65) now
    // exists in this worktree (it did not when the TODO above was written). Incidental fix, flagged
    // here rather than silently bundled -- this ticket's own dependency on Sys::CheckLuaSyntax
    // happens to unblock it.
    Check(Sys::CheckLuaSyntax(text).bSucceeded, "the real bundled resource is syntactically valid Lua");

    // STEP251 -- the generic per-scenario dispatcher (ARCH_15_04_ThreeFileOnDiskShape.md
    // "AMENDED 2026-09-03") is defined, and the ARCH_15_05 OPEN-item placeholder comment it replaces
    // is gone, not merely supplemented.
    Check(text.find("function Scenario.SpawnMatchedScenarioUnits") != std::string::npos,
          "the generic per-scenario dispatcher is defined");
    Check(text.find("is deliberately NOT added here") == std::string::npos,
          "the ARCH_15_05 OPEN-item placeholder comment was replaced, not merely supplemented");

    // 7b. Missing-alloy-roster guard is present and loud (text assertions on the bundled resource).
    Check(text.find("bAlloyRosterAvailable") != std::string::npos,
          "the fail-loud alloy-roster guard is present");
    Check(text.find("pairs(ARMY_ID_TO_NAME or {})") == std::string::npos,
          "the roster iteration was not regressed to a silent-fallback form");
    Check(text.find("KNOWN_ALLOY_MARKERS or {}") == std::string::npos,
          "neither was the marker lookup");
    // Part 1's own Warn() call wraps this sentence across two adjacent Lua string literals
    // ("...WILL BE ".."REMOVED: ...") to stay under the line-length convention, so the raw file
    // text never contains "NO ALLOY MARKERS WILL BE REMOVED" as one contiguous run (only the
    // Lua-evaluated, concatenated string would). Pin both literal fragments instead -- this still
    // catches the sentence being quietly softened or dropped, without depending on how it happens
    // to be line-wrapped in source.
    Check(text.find("NO ALLOY MARKERS WILL BE") != std::string::npos,
          "the warning text itself is pinned so it cannot be quietly softened (fragment 1)");
    Check(text.find("REMOVED: every alloy on the map will remain visible for every composition.") !=
              std::string::npos,
          "the warning text itself is pinned so it cannot be quietly softened (fragment 2)");

    // STEP253 (ARCH_15_05_ParamsScenariosType.md §15.5 AMENDED 2026-09-04): SlotRangeOccupiedCount
    // widens EvaluateScenarioCondition/EvaluateScenarioConditions by a 5th `slotPattern` parameter,
    // forwarded through the one existing FindMatchingScenario Tier-2 call site.
    Check(text.find("condition.field == \"SlotRangeOccupiedCount\"") != std::string::npos,
          "the SlotRangeOccupiedCount branch is present in EvaluateScenarioCondition");
    Check(text.find("function EvaluateScenarioCondition(condition, total, humanCount, aiCount, slotPattern)")
              != std::string::npos || text.find("EvaluateScenarioCondition(condition, total, humanCount, aiCount, slotPattern)")
              != std::string::npos,
          "EvaluateScenarioCondition widened to a 5th slotPattern parameter");
    Check(text.find("EvaluateScenarioConditions(conditions, total, humanCount, aiCount, slotPattern)")
              != std::string::npos,
          "EvaluateScenarioConditions widened to a 5th slotPattern parameter");
    Check(text.find("pcall(EvaluateScenarioConditions, scenario.conditions, total, humanCount, aiCount, slotPattern)")
              != std::string::npos,
          "the Tier-2 FindMatchingScenario call site forwards slotPattern through");

    // STEP263 (ARCH_15_14_ForeignScenarioFullDataImportAndUnitPlacement.md §15.14 Part A): the full
    // matched-scenario table is retained, not just its name.
    Check(text.find("local currentMatchedScenario = nil") != std::string::npos,
          "the currentMatchedScenario upvalue is declared");
    Check(text.find("currentMatchedScenario = matchedScenario") != std::string::npos,
          "Scenario.ResolveAndApply assigns the full matched scenario table to currentMatchedScenario");

    // Scenario.SpawnUnits rotation forwarding, nil-safely -- the regression guard: an instruction
    // shaped like existing category-4 generator output (no rotationW field) must still fall back to
    // IDENTITY_ROTATION unchanged.
    Check(text.find("local rotation = (instr.rotationW ~= nil)") != std::string::npos,
          "Scenario.SpawnUnits computes rotation with a nil-safe rotationW guard");
    Check(text.find("or IDENTITY_ROTATION") != std::string::npos,
          "Scenario.SpawnUnits falls back to IDENTITY_ROTATION when rotationW is nil -- the "
          "regression case for existing category-4 generator output");
    // Two separate single-line finds, not one embedded-newline literal spanning both source lines --
    // a real on-disk checkout can legitimately carry CRLF line endings for an LF-only git blob (see
    // this file's own banner-line comment above), which a bare "\n" literal would not match. Position
    // ordering proves the two fragments are the SAME statement, adjacent, not two unrelated hits.
    {
        const std::size_t pcallPosition = text.find("pcall(CreateUnit, instr.armyIndex, instr.templateIdentifier,");
        const std::size_t float3Position = text.find("EngineClasses.float3(instr.x, instr.y, instr.z), rotation)");
        Check(pcallPosition != std::string::npos && float3Position != std::string::npos,
              "Scenario.SpawnUnits' pcall(CreateUnit, ...) call and its EngineClasses.float3(...), "
              "rotation) continuation both appear in the output");
        Check(float3Position != std::string::npos && pcallPosition != std::string::npos
              && float3Position > pcallPosition && (float3Position - pcallPosition) < 100,
              "Scenario.SpawnUnits forwards the computed rotation as CreateUnit's 4th argument "
              "(the continuation line immediately follows the pcall(CreateUnit, ...) line)");
    }

    // Scenario.SpawnBakedUnitPlacements -- resolves armyName against the live pairs(Armies) handle,
    // Warn()s and skips an unresolvable army, never guessing/hardcoding an armyIndex.
    Check(text.find("function Scenario.SpawnBakedUnitPlacements(scenario)") != std::string::npos,
          "Scenario.SpawnBakedUnitPlacements is defined");
    Check(text.find("local function ResolveArmyIndexByName(armyName)") != std::string::npos,
          "ResolveArmyIndexByName is defined");
    Check(text.find("if not scenario.unitPlacements then return end") != std::string::npos,
          "Scenario.SpawnBakedUnitPlacements is a no-op, no error, when unitPlacements is nil "
          "(every pre-STEP263 scenario table)");
    Check(text.find("scenario unit placement named unknown army") != std::string::npos,
          "an unresolvable armyName is Warn()ed, not silently dropped");

    // Wiring: SpawnMatchedScenarioUnits calls SpawnBakedUnitPlacements unconditionally, BEFORE the
    // early-out that only governs the category-4 Import() path -- baked placements are independent
    // of the spawnsUnits opt-in flag.
    const std::size_t spawnMatchedScenarioUnitsPosition = text.find("function Scenario.SpawnMatchedScenarioUnits(area)");
    const std::size_t spawnBakedCallPosition = text.find("Scenario.SpawnBakedUnitPlacements(currentMatchedScenario)");
    const std::size_t earlyOutPosition = text.find("if not currentMatchedScenarioName then return end");
    Check(spawnMatchedScenarioUnitsPosition != std::string::npos && spawnBakedCallPosition != std::string::npos
          && earlyOutPosition != std::string::npos,
          "SpawnMatchedScenarioUnits, the SpawnBakedUnitPlacements call, and the category-4 early-out "
          "all appear in the output");
    Check(spawnMatchedScenarioUnitsPosition < spawnBakedCallPosition && spawnBakedCallPosition < earlyOutPosition,
          "Scenario.SpawnBakedUnitPlacements(currentMatchedScenario) is called before the "
          "category-4-only early-out -- baked placements fire even when spawnsUnits == false and no "
          "generator file exists");
}

// 7. Empty directory and empty override never crashes.
static void TestEmptyDirectoryAndEmptyOverrideNeverCrashes() {
    const Io::ScenarioRuntimeResourceResult result = Io::LoadScenarioRuntimeText("", "");
    Check(!result.bSucceeded, "an empty directory and an empty override cannot succeed");
    Check(!result.errorMessage.empty(), "but it fails with an actionable message, never a crash");
}

// 8. ReadTextFileBytes coverage.
static void TestReadTextFileBytesCoverage() {
    const std::string scratchFolder = ScratchFolderPath("ReadTextFileBytes");
    const std::string missingPath = Io::JoinExportPath(scratchFolder, "Missing.txt");
    std::string outText = "unchanged";
    Check(!Io::ReadTextFileBytes(missingPath, outText), "a missing file reads as false");
    Check(outText == "unchanged", "and leaves outText untouched on failure");

    const std::string existingPath = Io::JoinExportPath(scratchFolder, "Existing.txt");
    const std::string expectedBytes = std::string("before\0after", 12);
    {
        std::ofstream outputStream(existingPath, std::ios::binary | std::ios::trunc);
        outputStream.write(expectedBytes.data(), static_cast<std::streamsize>(expectedBytes.size()));
    }
    std::string readBackText;
    Check(Io::ReadTextFileBytes(existingPath, readBackText), "an existing file reads as true");
    Check(readBackText == expectedBytes, "and the bytes match exactly, including an embedded \\0");
}

int main(int argumentCount, char** arguments) {
    const std::string luaResourceDirectory = (argumentCount > 2) ? arguments[2] : "";

    TestOverridePathReadableWinsCleanly();
    TestEmptyOverrideFallsThroughToBundled();
    TestUnreadableOverrideDegradesLoudlyToBundled();
    TestNeitherReadableFails();
    TestReadableOverrideShortCircuitsBeforeTouchingBundledDirectory();
    TestRealBundledResourceSelfCheck(luaResourceDirectory);
    TestEmptyDirectoryAndEmptyOverrideNeverCrashes();
    TestReadTextFileBytesCoverage();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
