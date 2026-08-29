// ScenarioScript_AreaImport_IO.cpp -- see the header for the full contract. Order of guards,
// cheapest-first: filename (zero I/O) -> file existence/size stat (no content read) -> byte cap ->
// read -> banner-line -> pure extraction -> additive reconciliation.
#include "ScenarioScript_AreaImport_IO.h"
#include "FilesystemPrimitives_IO.h"
#include "ScenarioScript_DataLua_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include <algorithm>
#include <filesystem>

namespace SanmapGen {
namespace Io {
namespace {

// Independent copy of ScenarioScript_Export_IO.cpp's own private StartsWithBanner helper --
// deliberately NOT promoted into ScenarioScript_DataLua_IO.h by this ticket (see the work-order's
// "Interpretation calls made" section for the explicit scope-cut rationale); the IO Architecture
// Expert's own ruling left that promotion optional.
bool StartsWithBanner(const std::string& text) {
    const std::string banner(kScenarioGeneratedFileBannerLine);
    return text.compare(0, banner.size(), banner) == 0;
}

// ARCH §15.11 item 1's second, independent half of the refusal guard: refuse the two SanGen-owned
// filenames by name, regardless of content. Suffix match on the filename only (path-separator
// -agnostic) -- "<MapName>" is not known to this function and is not needed for a suffix check.
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

ScenarioAreaImportResult ImportAreaRectanglesFromScenarioScriptFile(const std::string& sourceFilePath,
                                                                    Params::MapRecipe& recipe) {
    ScenarioAreaImportResult result;

    // Item 1, half A -- filename refusal, checked before the file is even opened.
    if (HasSanGenOwnedScenarioFilenameSuffix(sourceFilePath)) {
        result.bRefusedGeneratedFile = true;
        result.Log("refused " + sourceFilePath +
                   " -- a SanGen-owned scenario filename is never read back (ARCH §15.11 item 1)");
        return result;
    }

    // Item 10 -- byte cap enforced via a filesystem stat, BEFORE any content is read into memory.
    std::error_code statError;
    const std::filesystem::path sourcePath(sourceFilePath);
    const std::uintmax_t fileSizeBytes = std::filesystem::file_size(sourcePath, statError);
    if (statError) {
        result.bRefusedUnreadableFile = true;
        result.Log("refused " + sourceFilePath + " -- file missing or unreadable (" + statError.message() + ")");
        return result;
    }
    if (fileSizeBytes > kMaxScenarioAreaImportSourceBytes) {
        result.bRefusedOversizedFile = true;
        result.Log("refused " + sourceFilePath + " -- source text exceeds the " +
                   std::to_string(kMaxScenarioAreaImportSourceBytes) + "-byte import cap (Constitution §6)");
        return result;
    }

    std::string sourceText;
    if (!ReadTextFileBytes(sourceFilePath, sourceText)) {
        result.bRefusedUnreadableFile = true;
        result.Log("refused " + sourceFilePath + " -- file could not be opened for read");
        return result;
    }

    // Item 1, half B -- banner-line refusal, checked BEFORE ever calling the pure extractor. This
    // is the load-bearing guard: it makes "SanGen never reads back what SanGen wrote" a checked
    // property rather than a convention (§15.11's own wording).
    if (StartsWithBanner(sourceText)) {
        result.bRefusedGeneratedFile = true;
        result.Log("refused " + sourceFilePath +
                   " -- first line matches Io::kScenarioGeneratedFileBannerLine (ARCH §15.11 item 1): "
                   "SanGen never reads back a file it wrote");
        return result;
    }

    // Items 2-7, 10 -- the pure, filename-agnostic extraction.
    const ScenarioAreaExtractionResult extraction = ExtractAreaRectanglesFromScenarioScriptText(sourceText);
    result.bRectangleCountCapExceeded = extraction.bRectangleCountCapExceeded;
    result.collisionIdentifiers       = extraction.collisionIdentifiers;
    result.nearMisses                 = extraction.nearMisses;
    for (const std::string& collisionIdentifier : extraction.collisionIdentifiers) {
        result.Log("in-file collision on '" + collisionIdentifier + "' -- last assignment wins (ARCH §15.11 item 7)");
    }
    for (const ScenarioAreaExtractionNearMiss& nearMiss : extraction.nearMisses) {
        result.Log("near-miss '" + nearMiss.identifier + "': " + nearMiss.reason);
    }
    if (extraction.bRectangleCountCapExceeded) {
        result.Log("rectangle-count cap reached -- extraction stopped early (Constitution §6)");
    }

    // Item 9 -- additive-never-destructive reconciliation into recipe.areas: a name collision is
    // skipped and reported, never a silent overwrite. Every non-colliding rectangle in the same file
    // is still imported (not all-or-nothing).
    for (const Params::MapArea& candidateArea : extraction.areas) {
        const bool bCollides = std::any_of(recipe.areas.begin(), recipe.areas.end(),
            [&candidateArea](const Params::MapArea& existingArea) { return existingArea.name == candidateArea.name; });
        if (bCollides) {
            result.skippedCollisionNames.push_back(candidateArea.name);
            result.Log("skipped '" + candidateArea.name +
                      "' -- an area with that name already exists in recipe.areas (ARCH §15.11 item 9)");
            continue;
        }
        recipe.areas.push_back(candidateArea);
        result.writtenNames.push_back(candidateArea.name);
    }

    return result;
}

} // namespace Io
} // namespace SanmapGen
