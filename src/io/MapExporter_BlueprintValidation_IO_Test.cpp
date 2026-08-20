// MapExporter_BlueprintValidation_IO_Test.cpp — M5-4 harness acceptance, part 5:
// STEP5_PropsDecalsValidation_UI's `SanpackReader::HasEntry` and
// `Io::ValidatePropAndDecalBlueprintPaths`, plus the export-side refuse-by-default gate
// (`MapExporter::ExportSanmapOnly`'s `assetPack`/`bBlueprintValidationAcknowledged` parameters,
// STEP39_BlueprintValidationGate_IO), all against the SAME synthetic sanpack
// `SanpackReader_IO_Test.cpp` already built (reuses `AssetPipeline_TestSupport_IO.h` rather than
// hand-rolling a second archive).
#include "AssetPipeline_TestSupport_IO.h"
#include "MapExporter_BlueprintValidation_IO.h"
#include "MapExporter_IO.h"
#include "SanpackReader_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include <filesystem>

using namespace SanmapGen;
using namespace SanmapGen::Io;
using namespace AssetPipelineTest;

namespace {

// One prop group + one decal group, both blueprintPaths controlled by the caller — lets the same
// builder produce the "everything resolves" and "one unresolved" fixtures.
Params::MapRecipe BuildFixtureRecipe(const std::string& propBlueprintPath,
                                     const std::string& decalBlueprintPath) {
    Params::MapRecipe recipe;
    Params::PropInstanceGroup propGroup;
    propGroup.blueprintPath = propBlueprintPath;
    recipe.props.push_back(propGroup);
    Params::DecalInstanceGroup decalGroup;
    decalGroup.blueprintPath = decalBlueprintPath;
    recipe.decals.push_back(decalGroup);
    return recipe;
}

} // namespace

namespace AssetPipelineTest {

void RunBlueprintValidationChecks(const SyntheticSanpack& layout, const std::string& scratchDirectory) {
    SanpackReader reader;
    Check(reader.Open(layout.sanpackPath), "blueprintPath reader memory-maps the synthetic sanpack");
    Check(reader.ReadCentralDirectoryOnce(), "and parses its central directory");

    // 1. HasEntry — exact, case-sensitive, no fuzzy matching.
    Check(reader.HasEntry(layout.propModelName), "HasEntry finds a real, exact archive path");
    Check(!reader.HasEntry("Environment/Environment/03_Desert/Props/edbm01/NOPE.sanmodel"),
          "and reports false for a path that is not in the archive");
    std::string upperCasedName = layout.propModelName;
    for (char& character : upperCasedName)
        if (character >= 'a' && character <= 'z') character = static_cast<char>(character - 'a' + 'A');
    Check(!reader.HasEntry(upperCasedName), "the lookup is case-sensitive — no fuzzy match");

    SanpackReader unopenedReader;
    Check(!unopenedReader.HasEntry(layout.propModelName),
          "an unopened/unparsed reader answers false, never asserts (Constitution §6)");

    // 2. ValidatePropAndDecalBlueprintPaths — all-resolved is silent.
    const Params::MapRecipe resolvedRecipe = BuildFixtureRecipe(layout.propModelName, layout.propModelName);
    const BlueprintValidationReport resolvedReport =
        ValidatePropAndDecalBlueprintPaths(resolvedRecipe, reader);
    Check(resolvedReport.AllResolved(), "two real blueprintPaths resolve cleanly");
    Check(resolvedReport.SummaryText().empty(), "and an all-resolved report has nothing to say");

    // 3. One unresolved path — reported, props-then-decals order, never dropped.
    const std::string unresolvedPath = "Props/Nowhere/Missing.santp";
    const Params::MapRecipe dirtyRecipe = BuildFixtureRecipe(layout.propModelName, unresolvedPath);
    const BlueprintValidationReport dirtyReport = ValidatePropAndDecalBlueprintPaths(dirtyRecipe, reader);
    Check(!dirtyReport.AllResolved(), "one unresolved decal blueprintPath is caught");
    Check(dirtyReport.unresolvedBlueprintPaths.size() == 1, "and only the bad one is reported");
    Check(!dirtyReport.unresolvedBlueprintPaths.empty()
          && dirtyReport.unresolvedBlueprintPaths[0] == unresolvedPath,
          "the literal unresolved path is preserved verbatim");
    Check(dirtyReport.SummaryText().find(unresolvedPath) != std::string::npos,
          "the summary names the offending path");

    // 4. The export-side safety net (STEP39_BlueprintValidationGate_IO): assetPack non-null and an
    // unresolved path REFUSES the write by default — a structural gate now, not just one button's
    // discipline — unless the caller explicitly acknowledges it.
    std::error_code pathError;
    const std::filesystem::path exportFolder =
        std::filesystem::path(scratchDirectory) / "blueprintValidationExport";
    std::filesystem::remove_all(exportFolder, pathError);
    const std::string documentPath = (exportFolder / (dirtyRecipe.mapName + ".sanmap")).string();

    // 4a. Acceptance item 1: no acknowledgment -> refused, nothing written, as a headless/batch
    // caller that bypasses the UI dialog entirely would hit.
    const MapExportResult unacknowledgedResult =
        MapExporter::ExportSanmapOnly(exportFolder.string(), dirtyRecipe, MapExportOptions(), &reader);
    Check(!unacknowledgedResult.bSucceeded,
          "an unresolved blueprintPath with no acknowledgment refuses the write");
    Check(unacknowledgedResult.WrittenFileCount() == 0, "and records nothing written");
    Check(!std::filesystem::exists(documentPath, pathError), "no .sanmap file is produced");
    Check(unacknowledgedResult.debugLog.find(unresolvedPath) != std::string::npos,
          "the finding still reaches the export's own debugLog");
    Check(unacknowledgedResult.debugLog.find("refused") != std::string::npos,
          "and the refusal itself is logged");

    // 4b. Acceptance item 2: the SAME call WITH acknowledgment succeeds and writes exactly as
    // before this ticket.
    const MapExportResult acknowledgedResult =
        MapExporter::ExportSanmapOnly(exportFolder.string(), dirtyRecipe, MapExportOptions(), &reader,
                                      /*unknownData=*/nullptr,
                                      /*bBlueprintValidationAcknowledged=*/true);
    Check(acknowledgedResult.bSucceeded, "an acknowledged unresolved blueprintPath still exports");
    Check(std::filesystem::exists(documentPath, pathError), "and this time the .sanmap IS produced");
    Check(acknowledgedResult.debugLog.find(unresolvedPath) != std::string::npos,
          "the finding is still logged even when acknowledged");

    // 5. assetPack == nullptr — the pre-ticket contract: no validation attempted, nothing logged,
    // no acknowledgment required.
    const MapExportResult skippedResult =
        MapExporter::ExportSanmapOnly(exportFolder.string(), dirtyRecipe, MapExportOptions(), nullptr);
    Check(skippedResult.bSucceeded, "with no assetPack the same dirty recipe still exports cleanly");
    Check(skippedResult.debugLog.find(unresolvedPath) == std::string::npos,
          "and nothing about blueprintPaths is logged: validation was skipped entirely");

    // 6. Acceptance item 4: zero unresolved paths succeeds without requiring any acknowledgment.
    const MapExportResult cleanResult =
        MapExporter::ExportSanmapOnly(exportFolder.string(), resolvedRecipe, MapExportOptions(), &reader);
    Check(cleanResult.bSucceeded, "an all-resolved recipe exports with no acknowledgment at all");

    reader.Close();
}

} // namespace AssetPipelineTest
