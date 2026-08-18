// MapImporter_Validation_IO_Test.cpp — the Constitution §6 half of the "Load Sanmap" acceptance
// test: a corrupt, partial or hostile document must leave the recipe on its own defaults, log the
// reason, and come out VALID — never crash and never hand the pipeline a recipe it would refuse.
#include "MapFormat_TestSupport_IO.h"
#include "MapImporter_IO.h"

namespace SanmapGen {
namespace MapFormatTest {
namespace {

void CheckUnusableDocumentsAreRefused() {
    const Io::MapImportOptions options;
    Params::MapRecipe recipe;
    Io::MapImportResult result;
    Check(!Io::MapImporter::ParseSanmapJsonText("{ this is not json", recipe, options, result),
          "unparseable text is refused");
    Check(!result.debugLog.empty(), "with the parse error logged for the tab's panel");
    Check(!Io::MapImporter::ParseSanmapJsonText("[1,2,3]", recipe, options, result),
          "a non-object document is refused");
}

void CheckAPartialDocumentRecoversWhatItHas() {
    const Io::MapImportOptions options;
    Params::MapRecipe recipe;
    Io::MapImportResult result;
    // "SanGenVersion":2 satisfies the migration runner's version gate (STEP6_MigrationSubsystem_IO
    // — the runner is now the literal first thing ParseSanmapJsonText does, and a document with no
    // version marker at all is refused, never guessed). This fixture is deliberately partial BELOW
    // that gate: it still has no `mapGeneratorData` block, which is what this check actually tests.
    Check(Io::MapImporter::ParseSanmapJsonText("{\"SanGenVersion\": 2, \"width\": 128}", recipe,
                                               options, result),
          "a document with no generator block still loads what it has");
    Check(recipe.geometry.mapSize == 128, "recovering the map's own dimensions");
    Check(result.warningCount == 1, "and warning that the generator settings were not there");
}

// A wrong-typed key, an out-of-cap size, a negative ceiling and an out-of-range enum, in one go.
void CheckHostileValuesFallBackToDefaults() {
    const Io::MapImportOptions options;
    Params::MapRecipe recipe;
    Io::MapImportResult result;
    // "SanGenVersion":2 — see CheckAPartialDocumentRecoversWhatItHas's note above; the hostility
    // under test here is entirely inside mapGeneratorData, below the migration runner's gate.
    const char* hostileText = "{\"SanGenVersion\":2,\"mapGeneratorData\":{\"MapSize\":99999,\"Seed\":\"nope\","
                              "\"TerrainMaxHeight\":-4.0,\"WorldUnitsPerCell\":0.0,"
                              "\"GeoLayers\":[{\"Mode\":77}]}}";
    Check(Io::MapImporter::ParseSanmapJsonText(hostileText, recipe, options, result),
          "a hostile document parses rather than throwing");
    Check(recipe.geometry.mapSize == Params::Geometry().mapSize,
          "an out-of-cap MapSize is refused and the default kept");
    Check(recipe.geometry.seed == Params::Geometry().seed, "a wrong-typed Seed is ignored");
    Check(recipe.geometry.terrainMaxHeight >= 1.0f,
          "a negative ceiling is raised so Geometry::IsValid() can never fail on import");
    Check(recipe.geometry.worldUnitsPerCell > 0.0f, "and a zero cell size is restored");
    Check(recipe.IsValid(), "so the whole recipe comes out valid");
    Check(recipe.layerStack.geoLayers.size() == 1
          && recipe.layerStack.geoLayers[0].mode == Params::GeoLayer().mode,
          "an out-of-range enum is fenced to its default rather than cast wild");
    Check(result.warningCount > 0, "with every fallback warned about");
}

void CheckAnUnresolvablePathIsRefused() {
    std::string documentPath;
    std::string folderPath;
    Check(!Io::ResolveSanmapDocumentPath(std::string(), documentPath, folderPath),
          "an empty path resolves to nothing");
    Check(!Io::ResolveSanmapDocumentPath("D:/no/such/place", documentPath, folderPath),
          "and so does a path that is neither a file nor a folder");
    Params::MapRecipe recipe;
    const Io::MapImportResult result = Io::MapImporter::LoadSanmap("D:/no/such/place", recipe, nullptr);
    Check(!result.bSucceeded && !result.debugLog.empty(),
          "loading it fails with the reason in the panel");
}

} // namespace

void RunValidationTests() {
    CheckUnusableDocumentsAreRefused();
    CheckAPartialDocumentRecoversWhatItHas();
    CheckHostileValuesFallBackToDefaults();
    CheckAnUnresolvablePathIsRefused();
}

} // namespace MapFormatTest
} // namespace SanmapGen
