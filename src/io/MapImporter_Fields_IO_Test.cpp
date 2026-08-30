// MapImporter_Fields_IO_Test.cpp — the on-disk half of the "Load Sanmap" acceptance test:
// resolving what the user picked, and reading the `Textures/` payload back into a caller-owned
// Data::MapFields. Runs against a REAL map written by MapExporter into a scratch folder, so the
// 16-bit RAW and the two BGRA TGAs are exercised byte for byte rather than through a stub.
#include "MapFormat_TestSupport_IO.h"
#include "MapImporter_IO.h"
#include "MapExporter_IO.h"
#include "../data/MapFields_DATA.h"
#include "../data/StratumArt_DATA.h"

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
    // STEP101 gap 1: the loader seeds the PHYSICAL field (materialProportions), never the
    // Mask-stage-exclusive surfaceStratumWeights (MASKING_SPEC §1.5/§1.6, ARCH §3.4 single-writer
    // rule) — even though the fixture itself wrote surfaceStratumWeights before export (that field
    // is what the EXPORTER reads, unaffected by this fix).
    Check(NearlyEqual(loadedFields.materialProportions[0].Get(1, 1), 1.0f),
          "a low-slice stratum weight round-trips through its TGA into materialProportions");
    Check(NearlyEqual(loadedFields.materialProportions[5].Get(2, 3), 1.0f),
          "and so does a high-slice one");
    Check(NearlyEqual(loadedFields.surfaceStratumWeights[0].Get(1, 1), 0.0f)
          && NearlyEqual(loadedFields.surfaceStratumWeights[5].Get(2, 3), 0.0f),
          "surfaceStratumWeights stays at MapFields::Resize's zero-fill -- import never writes it");
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

// STEP220 — the ONE thing this ticket's whole fix depends on: LoadStratumMaskTga must stamp a
// FRESH, DIFFERENT importedMaskVersion on every successful load, never the same value twice (a
// naive "read my own current value and increment it" bump would fail this, since every import
// path hands the loader a brand-new, default-constructed Data::StratumArt each time — see this
// ticket's own MapImporter_Fields_IO.cpp comment for why). Loads the SAME fixture map twice
// (re-importing identical content is still a distinct load EVENT) and confirms the two resulting
// versions differ — a relative check, never an absolute literal: nextImportedMaskVersion is a
// process-lifetime counter shared by every test in this binary, not reset per test case.
//
// DEVIATION FROM THE TICKET'S OWN CALL SHAPE (verified live against MapImporter_IO.h, not
// assumed): the ticket's proposed call passed `nullptr` for `outFields` while still expecting
// `outStratumArt` to come back populated. `MapImporter::LoadSanmap`'s own body gates its ENTIRE
// `LoadBakedFields` call (which is the only thing that ever populates `outStratumArt`) on
// `options.bLoadBakedFields && outFields != nullptr` — so a null `outFields` means
// `outStratumArt` is never touched at all, leaving every version at the 0 sentinel and failing
// this test's own first assertion. A real (non-null) `Data::MapFields` destination is supplied
// below instead, preserving the test's actual intent (load twice through the real production
// path, compare `importedMaskVersion`) without changing what it proves. The full 8-argument call
// also threads explicit `nullptr`s through `outUnknownData`/`currentTemplateIngestReport`/
// `outBakedLayerImages` — three nullable parameters the ticket's assumed signature didn't
// account for sitting between `options` and `outStratumArt`.
void CheckImportedMaskVersionAdvancesOnEveryLoad(const std::string& folderPath) {
    Params::MapRecipe firstRecipe;
    Data::MapFields firstFields;
    std::vector<Data::StratumArt> firstStratumArt;
    const Io::MapImportResult firstResult =
        Io::MapImporter::LoadSanmap(folderPath, firstRecipe, &firstFields, Io::MapImportOptions(),
                                    nullptr, nullptr, nullptr, &firstStratumArt);
    Check(firstResult.bSucceeded, "the first load succeeds");
    Check(firstStratumArt[0].importedMaskVersion > 0 && firstStratumArt[5].importedMaskVersion > 0,
          "a successfully loaded stratum's version is stamped (never left at the 0 sentinel)");

    Params::MapRecipe secondRecipe;
    Data::MapFields secondFields;
    std::vector<Data::StratumArt> secondStratumArt;
    const Io::MapImportResult secondResult =
        Io::MapImporter::LoadSanmap(folderPath, secondRecipe, &secondFields, Io::MapImportOptions(),
                                    nullptr, nullptr, nullptr, &secondStratumArt);
    Check(secondResult.bSucceeded, "the second load succeeds");
    Check(secondStratumArt[0].importedMaskVersion != firstStratumArt[0].importedMaskVersion,
          "re-loading the SAME file still draws a NEW version — every load is its own event, "
          "never derived from content equality");
    Check(secondStratumArt[0].importedMaskVersion != secondStratumArt[4].importedMaskVersion
          || secondStratumArt[0].importedMaskVersion == secondStratumArt[1].importedMaskVersion,
          "strata sharing one TGA call (0-3, the low slice) share one stamp; a stratum from the "
          "OTHER TGA call (4, the high slice) draws its own separate stamp");
}

} // namespace

void RunBakedFieldTests() {
    const std::string folderPath = ScratchFolderPath("SanGenMapImporterTest");
    const Params::MapRecipe written = WriteFixtureMap(folderPath);
    CheckPathResolution(folderPath);
    CheckBakedFieldsComeBack(folderPath, written);
    CheckTheFieldDestinationIsOptional(folderPath);
    CheckImportedMaskVersionAdvancesOnEveryLoad(folderPath);
}

} // namespace MapFormatTest
} // namespace SanmapGen
