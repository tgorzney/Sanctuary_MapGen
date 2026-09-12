// ScenarioScript_FullDataImport_IO.cpp -- see the header for the full contract. Guard order,
// cheapest-first, identical to ScenarioScript_AreaImport_IO.cpp: filename (zero I/O) -> file
// existence/size stat (no content read) -> byte cap -> read -> banner-line -> pure extraction (all
// four, over the SAME text) -> additive reconciliation (areas only).
#include "ScenarioScript_FullDataImport_IO.h"
#include "FilesystemPrimitives_IO.h"
#include "ScenarioScript_DataLua_IO.h"
#include "../params/MapArea_PARAMS.h"
#include "../params/MapRecipe_PARAMS.h"
#include <algorithm>
#include <filesystem>

namespace SanmapGen {
namespace Io {
namespace {

// Independent copy of ScenarioScript_AreaImport_IO.cpp's own private StartsWithBanner/
// HasSanGenOwnedScenarioFilenameSuffix helpers -- same "each file owns its own copy" precedent that
// file's own top-of-file note already establishes for this guard machinery.
bool StartsWithBanner(const std::string& text) {
    const std::string banner(kScenarioGeneratedFileBannerLine);
    return text.compare(0, banner.size(), banner) == 0;
}

bool HasSanGenOwnedScenarioFilenameSuffix(const std::string& filePath) {
    const std::size_t lastSlashIndex = filePath.find_last_of("/\\");
    const std::string fileName = (lastSlashIndex == std::string::npos) ? filePath : filePath.substr(lastSlashIndex + 1);
    static const char* const kOwnedSuffixes[] = { "_Scenarios_Runtime.lua", "_Scenarios_Data.lua" };
    for (const char* suffix : kOwnedSuffixes) {
        const std::string suffixText(suffix);
        if (fileName.size() >= suffixText.size()
            && fileName.compare(fileName.size() - suffixText.size(), suffixText.size(), suffixText) == 0) {
            return true;
        }
    }
    return false;
}

} // namespace

ScenarioFullDataImportResult ImportFullScenarioDataFromScenarioScriptFile(
        const std::string& sourceFilePath, int mapSize, Params::MapRecipe& recipe) {
    ScenarioFullDataImportResult result;

    // Item 1, half A -- filename refusal, checked before the file is even opened.
    if (HasSanGenOwnedScenarioFilenameSuffix(sourceFilePath)) {
        result.bRefusedAsSanGenOwnedFile = true;
        result.Log("refused " + sourceFilePath +
                   " -- a SanGen-owned scenario filename is never read back (ARCH §15.11 item 1)");
        return result;
    }

    // Byte cap enforced via a filesystem stat, BEFORE any content is read into memory.
    std::error_code statError;
    const std::filesystem::path sourcePath(sourceFilePath);
    const std::uintmax_t fileSizeBytes = std::filesystem::file_size(sourcePath, statError);
    if (statError) {
        result.bRefusedUnreadableFile = true;
        result.Log("refused " + sourceFilePath + " -- file missing or unreadable (" + statError.message() + ")");
        return result;
    }
    if (fileSizeBytes > kMaxScenarioFullDataImportSourceBytes) {
        result.bRefusedOversizedFile = true;
        result.Log("refused " + sourceFilePath + " -- source text exceeds the " +
                   std::to_string(kMaxScenarioFullDataImportSourceBytes) + "-byte import cap (Constitution §6)");
        return result;
    }

    std::string sourceText;
    if (!ReadTextFileBytes(sourceFilePath, sourceText)) {
        result.bRefusedUnreadableFile = true;
        result.Log("refused " + sourceFilePath + " -- file could not be opened for read");
        return result;
    }

    // Item 1, half B -- banner-line refusal, checked BEFORE ever calling any pure extractor.
    if (StartsWithBanner(sourceText)) {
        result.bRefusedAsSanGenOwnedFile = true;
        result.Log("refused " + sourceFilePath +
                   " -- first line matches Io::kScenarioGeneratedFileBannerLine (ARCH §15.11 item 1): "
                   "SanGen never reads back a file it wrote");
        return result;
    }

    // ONE read, all four pure, filename-agnostic extractions over the SAME text.
    result.areas           = ExtractAreaRectanglesFromScenarioScriptText(sourceText);
    result.matchConditions = ExtractMatchConditionsFromScenarioScriptText(sourceText);
    result.slotPatterns    = ExtractSlotPatternsFromScenarioScriptText(sourceText);
    result.unitPlacements  = ExtractUnitPlacementsFromScenarioScriptText(sourceText, mapSize);

    for (const std::string& collisionIdentifier : result.areas.collisionIdentifiers) {
        result.Log("in-file area collision on '" + collisionIdentifier + "' -- last assignment wins (ARCH §15.11 item 7)");
    }
    for (const ScenarioAreaExtractionNearMiss& nearMiss : result.areas.nearMisses) {
        result.Log("area near-miss '" + nearMiss.identifier + "': " + nearMiss.reason);
    }

    // Item 9 -- additive-never-destructive reconciliation into recipe.areas, EXACTLY
    // ScenarioScript_AreaImport_IO.cpp's own policy: a name collision is skipped and reported, never a
    // silent overwrite; every non-colliding rectangle in the same file is still imported. This is the
    // ONLY one of the four extraction results this orchestrator ever writes into `recipe` --
    // matchConditions/slotPatterns/unitPlacements are surfaced on the result struct only, never
    // written into recipe.scenarios or any Params::ScenarioBody.
    for (const Params::MapArea& candidateArea : result.areas.areas) {
        const bool bCollides = std::any_of(recipe.areas.begin(), recipe.areas.end(),
            [&candidateArea](const Params::MapArea& existingArea) { return existingArea.name == candidateArea.name; });
        if (bCollides) {
            result.skippedCollisionAreaNames.push_back(candidateArea.name);
            result.Log("skipped '" + candidateArea.name +
                      "' -- an area with that name already exists in recipe.areas (ARCH §15.11 item 9)");
            continue;
        }
        Params::InsertMapAreaSortedBySize(recipe.areas, candidateArea);
        result.writtenAreaNames.push_back(candidateArea.name);
    }

    return result;
}

} // namespace Io
} // namespace SanmapGen
