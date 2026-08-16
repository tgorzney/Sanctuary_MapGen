// MapImporter_Fields_IO_Test.cpp — the on-disk half of the "Load Sanmap" acceptance test:
// resolving what the user picked, and reading the `Textures/` payload back into a caller-owned
// Data::MapFields. Runs against a REAL map written by MapExporter into a scratch folder, so the
// 16-bit RAW and the two BGRA TGAs are exercised byte for byte rather than through a stub.
#include "MapFormat_TestSupport_IO.h"
#include "MapImporter_IO.h"
#include "MapExporter_IO.h"
#include "../data/MapFields_DATA.h"

namespace SanmapGen {
namespace MapFormatTest {
namespace {

// An 8-cell map with one full-height vertex and one saturated weight in each mask slice.
Params::MapRecipe WriteFixtureMap(const std::string& folderPath) {
    Params::MapRecipe recipe = BuildPopulatedRecipe();
    recipe.geometry.mapSize = 8;
    Data::MapFields fields;
    fields.Resize(recipe.geometry.VertexSize(), 0.0f);
    fields.heightfield.Set(3, 2, 1.0f);
    fields.surfaceStratumWeights[0].Set(1, 1, 1.0f);
    fields.surfaceStratumWeights[5].Set(2, 3, 1.0f);
    Io::MapExportOptions exportOptions;
    exportOptions.mapName = "mapdef";
    Check(Io::MapExporter::ExportAll(folderPath, recipe, fields, exportOptions).bSucceeded,
          "the fixture map exports");
    return recipe;
}

void CheckPathResolution(const std::string& folderPath) {
    std::string documentPath;
    std::string resolvedFolderPath;
    Check(Io::ResolveSanmapDocumentPath(folderPath, documentPath, resolvedFolderPath),
          "a FOLDER resolves to the mapdef.sanmap inside it");
    std::string documentPathFromFile;
    Check(Io::ResolveSanmapDocumentPath(documentPath, documentPathFromFile, resolvedFolderPath)
          && documentPathFromFile == documentPath, "and the document itself resolves to itself");
}

void CheckBakedFieldsComeBack(const std::string& folderPath, const Params::MapRecipe& written) {
    Params::MapRecipe loadedRecipe;
    Data::MapFields loadedFields;
    const Io::MapImportResult result =
        Io::MapImporter::LoadSanmap(folderPath, loadedRecipe, &loadedFields);
    Check(result.bSucceeded && result.bRecipeLoaded, "the map loads from its folder");
    Check(result.bBakedFieldsLoaded, "and the Textures payload comes back with it");
    Check(loadedFields.VertexSize() == written.geometry.VertexSize(),
          "the fields are sized from the RECIPE, not from the file");
    Check(NearlyEqual(loadedFields.heightfield.Get(3, 2), 1.0f),
          "the heightmap sample round-trips through the 16-bit RAW");
    Check(NearlyEqual(loadedFields.heightfield.Get(0, 0), 0.0f),
          "and an untouched vertex stays at the floor");
    Check(NearlyEqual(loadedFields.surfaceStratumWeights[0].Get(1, 1), 1.0f),
          "a low-slice stratum weight round-trips through its TGA");
    Check(NearlyEqual(loadedFields.surfaceStratumWeights[5].Get(2, 3), 1.0f),
          "and so does a high-slice one");
}

void CheckTheFieldDestinationIsOptional(const std::string& folderPath) {
    Params::MapRecipe recipe;
    const Io::MapImportResult result = Io::MapImporter::LoadSanmap(folderPath, recipe, nullptr);
    Check(result.bSucceeded && !result.bBakedFieldsLoaded,
          "with no field destination bound the recipe still loads and the textures are skipped");

    Io::MapImportOptions options;
    options.bLoadBakedFields = false;
    Data::MapFields untouchedFields;
    const Io::MapImportResult skipped =
        Io::MapImporter::LoadSanmap(folderPath, recipe, &untouchedFields, options);
    Check(skipped.bSucceeded && !untouchedFields.IsSized(),
          "and asking not to load them leaves the caller's fields alone");
}

} // namespace

void RunBakedFieldTests() {
    const std::string folderPath = ScratchFolderPath("SanGenMapImporterTest");
    const Params::MapRecipe written = WriteFixtureMap(folderPath);
    CheckPathResolution(folderPath);
    CheckBakedFieldsComeBack(folderPath, written);
    CheckTheFieldDestinationIsOptional(folderPath);
}

} // namespace MapFormatTest
} // namespace SanmapGen
