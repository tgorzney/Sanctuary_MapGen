// MapImporter_HeightmapDecomposition_IO_Test.cpp — STEP105 acceptance (revises STEP101's own),
// extended by STEP109: a genuinely externally-authored `.sanmap` (recipe.layerStack empty, exactly
// the reported bug's scenario) synthesizes exactly ONE flattened baked height layer, and its
// stratum-1 mask art feeds `Data::StratumArt::importedMask` / `Params::Stratum::importedMaskMode`
// rather than splitting the height. Running the full GenerationAssembler pipeline once reproduces
// the ORIGINAL imported heightfield AND, via `ImportedMaskMode::StaticOverride`, the original
// imported mask -- the real end-to-end proof this ticket's second half works. Its own binary (not
// folded into MapImporter_IO_Test's three-TU group) since it is the first IO acceptance test that
// reaches into Pipeline to run a real GenerationAssembler, not just parse/round-trip a document.
// STEP109 adds: the fresh-synthesis GeoLayer's name derives from the document's own FILENAME stem
// (underscores -> spaces, trimmed, "Imported Bake" fallback), never the JSON "Name"/mapName field,
// and a hand-renamed group survives a re-import round trip untouched.
#include "MapImporter_IO.h"
#include "MapExporter_IO.h"
#include "../data/BakedLayerImage_DATA.h"
#include "../data/MapFields_DATA.h"
#include "../data/StratumArt_DATA.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../pipeline/GenerationAssembler_PIPELINE.h"
#include <cmath>
#include <cstdio>
#include <filesystem>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

bool NearlyEqual(float left, float right, float tolerance) { return std::fabs(left - right) <= tolerance; }

// A private, EMPTY folder under the platform temp directory — MapFormatTest::ScratchFolderPath's
// own precedent (MapFormat_TestSupport_IO.h), reproduced here rather than shared since this test
// deliberately does not join that group's three-TU binary (see the header comment above).
std::string ScratchFolderPath(const char* folderName) {
    std::error_code pathError;
    const std::filesystem::path folder = std::filesystem::temp_directory_path(pathError) / folderName;
    std::filesystem::remove_all(folder, pathError);
    return folder.string();
}

// A small, non-trivial heightfield (no two cells share a value) plus a single, fully-covering
// stratum-1 mask — the ticket's own acceptance fixture ("all-white ... = stratum 1 fully covering").
// `recipe` is deliberately FRESH: an empty layerStack, exactly a genuine externally-authored
// `.sanmap` with no SanGen HeightmapStack section (the reported bug's exact scenario).
Params::MapRecipe WriteSyntheticExternalMap(const std::string& folderPath) {
    Params::MapRecipe recipe;
    recipe.geometry.mapSize = 8;
    const int vertexSize = recipe.geometry.VertexSize();

    Data::MapFields fields;
    fields.Resize(vertexSize, 0.0f);
    for (int y = 0; y < vertexSize; ++y)
        for (int x = 0; x < vertexSize; ++x)
            fields.heightfield.Set(x, y, static_cast<float>((x * 5 + y * 3) % 11) / 10.0f);
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
        fields.surfaceStratumWeights[stratum].Fill(stratum == 1 ? 1.0f : 0.0f);

    Io::MapExportOptions exportOptions;
    const Io::MapExportResult exportResult =
        Io::MapExporter::ExportAll(folderPath, recipe, fields, exportOptions);
    Check(exportResult.bSucceeded, "the synthetic external map exports");
    return recipe;
}

// The assembler.Run() proof: NoiseBlend (STEP100) reads the decomposed baked layer, and the Mask
// stage's ImportedMaskMode::StaticOverride path (fed by `stratumArt`) reproduces the imported mask.
// Thermal's relaxation is disabled to isolate THIS mechanism from an unrelated sim stage that
// mutates the heightfield on its own defaults (Erosion is already a no-op by default -- ErosionLayerSettings::
// bEnabled starts false).
void RunAssembler(Pipeline::GenerationAssembler& assembler,
                  const std::vector<Data::BakedLayerImage>& bakedLayerImages,
                  const std::vector<Data::StratumArt>& stratumArt) {
    assembler.BakedLayerImages() = bakedLayerImages;
    assembler.StratumArt() = stratumArt;
    assembler.Thermal().Constants().iterationCount = 0;
    assembler.Run();
}

bool FieldMatches(const Data::FloatField& assembled, const Data::FloatField& original, float tolerance) {
    for (int y = 0; y < original.Height(); ++y)
        for (int x = 0; x < original.Width(); ++x)
            if (!NearlyEqual(assembled.Get(x, y), original.Get(x, y), tolerance)) return false;
    return true;
}

// STEP109: finds the single `.sanmap` document WriteSyntheticExternalMap already wrote into
// `folderPath` and renames it so its STEM becomes `newStem` -- lets these tests control the file
// stem independent of the recipe's own `mapName`/JSON `"Name"` field (the ticket's own explicit
// contrast: derived from the FILENAME, never `general.name`). Returns the new document path.
std::string RenameSanmapDocumentStem(const std::string& folderPath, const std::string& newStem) {
    std::error_code walkError;
    std::string oldDocumentPath;
    for (const std::filesystem::directory_entry& entry :
         std::filesystem::directory_iterator(folderPath, walkError)) {
        if (entry.path().extension().string() == ".sanmap") { oldDocumentPath = entry.path().string(); break; }
    }
    if (oldDocumentPath.empty()) return std::string();
    const std::filesystem::path newDocumentPath =
        std::filesystem::path(folderPath) / (newStem + ".sanmap");
    std::error_code renameError;
    std::filesystem::rename(oldDocumentPath, newDocumentPath, renameError);
    return renameError ? std::string() : newDocumentPath.string();
}

void CheckFreshImportSynthesizesOneLayerAndFeedsMask(const std::string& folderPath) {
    Params::MapRecipe loadedRecipe;
    Data::MapFields loadedFields;
    std::vector<Data::BakedLayerImage> bakedLayerImages;
    std::vector<Data::StratumArt> stratumArt;
    const Io::MapImportResult result = Io::MapImporter::LoadSanmap(
        folderPath, loadedRecipe, &loadedFields, Io::MapImportOptions(), nullptr, nullptr,
        &bakedLayerImages, &stratumArt);
    Check(result.bSucceeded && result.bBakedFieldsLoaded, "the synthetic map loads");

    // --- Height: exactly ONE baked layer, at stratumIndex 0, holding the heightfield verbatim.
    Check(loadedRecipe.layerStack.geoLayers.size() == 1,
          "the empty-recipe gate synthesizes exactly one new GeoLayer");
    Check(loadedRecipe.layerStack.TotalLayerCount() == 1,
          "exactly ONE baked layer -- STEP105 collapses the old per-stratum split");

    int bakedLayerIdentifier = -1;
    for (const Params::GeoLayer& group : loadedRecipe.layerStack.geoLayers)
        for (const Params::Layer& layer : group.layers) {
            Check(layer.bBaked, "the one decomposed layer is bBaked");
            Check(layer.stratumIndex == 0, "the one decomposed layer sits at the default stratumIndex 0");
            bakedLayerIdentifier = layer.layerIdentifier;
        }
    Check(bakedLayerIdentifier != -1, "the one decomposed layer was found");

    Check(bakedLayerImages.size() == 1, "assembler.BakedLayerImages() has exactly 1 entry");
    const Data::FloatField* bakedImage = Data::FindBakedLayerImage(bakedLayerImages, bakedLayerIdentifier);
    Check(bakedImage != nullptr, "the layerIdentifier resolves to a stored image");
    Check(bakedImage != nullptr && FieldMatches(*bakedImage, loadedFields.heightfield, 1.0e-6f),
          "the baked image equals loadedFields.heightfield cell-for-cell -- verbatim, no per-stratum mask");

    // --- Stratum mask: materialProportions unchanged (still the physical-field write, Mask_Apply_PROC's
    // own procedural-weight input); the NEW parallel write lands in StratumArt at NATIVE resolution.
    Check(NearlyEqual(loadedFields.materialProportions[1].Get(0, 0), 1.0f, 1.0e-4f),
          "materialProportions[1] matches the TGA's fully-covering mask -- unchanged write path");
    Check(NearlyEqual(loadedFields.surfaceStratumWeights[1].Get(0, 0), 0.0f, 1.0e-6f),
          "surfaceStratumWeights stays untouched by import (zero-filled until the Mask stage runs)");

    // The TGA's own native resolution is mapSize x mapSize (WriteStratumMaskTga: "CELL-sized, not
    // vertex-sized" -- MapExporter_Textures_IO.cpp), i.e. VertexSize() - 1: unclipped, so this is
    // the real proof the write is NOT vertexSize-clipped -- it lands at the TGA's own dimensions.
    const int expectedNativeSize = loadedRecipe.geometry.VertexSize() - 1;
    Check(stratumArt.size() > 1, "the imported mask landed on a real stratum slot");
    Check(stratumArt[1].HasImportedMask(), "StratumArt()[1].HasImportedMask() is true after import");
    Check(stratumArt[1].importedMask.Width() == expectedNativeSize
          && stratumArt[1].importedMask.Height() == expectedNativeSize,
          "importedMask dimensions equal the TGA's own native fileWidth/fileHeight");

    // --- The default-mode rule: a genuinely unconfigured stratum defaults to StaticOverride.
    Check(loadedRecipe.strata.size() > 1, "recipe.strata grew to cover the imported stratum");
    Check(loadedRecipe.strata.size() > 1
          && loadedRecipe.strata[1].importedMaskMode == Params::ImportedMaskMode::StaticOverride,
          "an unconfigured stratum's importedMaskMode defaults to StaticOverride");

    // --- The end-to-end proof: assembler.Run() reproduces BOTH the original heightfield (the height
    // half) AND the original imported mask, via ImportedMaskMode::StaticOverride actually consuming
    // what import fed it (the stratum-mask half).
    Pipeline::GenerationAssembler assembler(loadedRecipe);
    RunAssembler(assembler, bakedLayerImages, stratumArt);
    Check(FieldMatches(assembler.Fields().heightfield, loadedFields.heightfield, 1.0e-4f),
          "assembler.Run() reproduces the ORIGINAL imported heightfield -- the reported bug's fix");
    Check(NearlyEqual(assembler.Fields().surfaceStratumWeights[1].Get(0, 0), 1.0f, 1.0e-2f),
          "after Run(), surfaceStratumWeights[1] reproduces the imported mask -- StaticOverride "
          "actually consumes what import fed it (the real proof this ticket's 2nd half works)");
}

void CheckReimportAfterRoundTripDoesNotDuplicate(const std::string& firstFolderPath) {
    Params::MapRecipe firstRecipe;
    Data::MapFields firstFields;
    std::vector<Data::BakedLayerImage> firstBakedLayerImages;
    std::vector<Data::StratumArt> firstStratumArt;
    Io::MapImporter::LoadSanmap(firstFolderPath, firstRecipe, &firstFields, Io::MapImportOptions(),
                                nullptr, nullptr, &firstBakedLayerImages, &firstStratumArt);

    // Run once so surfaceStratumWeights (what the EXPORTER reads for the masks) is populated by the
    // real Mask stage -- otherwise a re-export would write back an all-zero mask.
    Pipeline::GenerationAssembler assembler(firstRecipe);
    RunAssembler(assembler, firstBakedLayerImages, firstStratumArt);

    const std::string secondFolderPath = ScratchFolderPath("SanGenHeightmapDecompositionRoundTrip");
    Io::MapExportOptions exportOptions;
    const Io::MapExportResult exportResult =
        Io::MapExporter::ExportAll(secondFolderPath, firstRecipe, assembler.Fields(), exportOptions);
    Check(exportResult.bSucceeded, "the decomposed recipe round-trips back out through export");

    Params::MapRecipe secondRecipe;
    Data::MapFields secondFields;
    std::vector<Data::BakedLayerImage> secondBakedLayerImages;
    std::vector<Data::StratumArt> secondStratumArt;
    const Io::MapImportResult secondResult = Io::MapImporter::LoadSanmap(
        secondFolderPath, secondRecipe, &secondFields, Io::MapImportOptions(), nullptr, nullptr,
        &secondBakedLayerImages, &secondStratumArt);
    Check(secondResult.bSucceeded, "the re-exported map re-imports");

    Check(secondRecipe.layerStack.geoLayers.size() == firstRecipe.layerStack.geoLayers.size()
          && secondRecipe.layerStack.TotalLayerCount() == firstRecipe.layerStack.TotalLayerCount()
          && secondRecipe.layerStack.TotalLayerCount() == 1,
          "re-importing after an export round trip does not duplicate layers -- still exactly 1");

    // The round-tripped document's own StratumGenerationSettings/stratumLayers section now carries an
    // EXPLICIT importedMaskMode (it was written by the first import's own default -- Constitution §6:
    // once set, it round-trips like any other recipe value). The second import must READ that value
    // back, never re-default it a second time.
    Check(firstRecipe.strata.size() > 1
          && firstRecipe.strata[1].importedMaskMode == Params::ImportedMaskMode::StaticOverride,
          "the first import's own default is StaticOverride (sanity, re-asserted before round trip)");
    Check(secondRecipe.strata.size() > 1
          && secondRecipe.strata[1].importedMaskMode == Params::ImportedMaskMode::StaticOverride,
          "the second import's strata[1].importedMaskMode still reads StaticOverride -- the "
          "round-tripped EXPLICIT document value is read back, not re-defaulted a second time");

    Pipeline::GenerationAssembler secondAssembler(secondRecipe);
    RunAssembler(secondAssembler, secondBakedLayerImages, secondStratumArt);
    Check(FieldMatches(secondAssembler.Fields().heightfield, assembler.Fields().heightfield, 2.0e-3f),
          "the re-imported recipe reproduces the same (already-decomposed-once) heightfield");
}

// STEP109: the fresh-synthesis GeoLayer's name is derived from the document's own FILENAME stem
// (underscores -> spaces), never the JSON "Name"/mapName field -- WriteSyntheticExternalMap's
// recipe.mapName stays at its own default ("mapdef"), so a match here can only be explained by the
// renamed FILE stem, not the document's own name field.
void CheckFreshImportDerivesNameFromFilenameStem() {
    const std::string folderPath = ScratchFolderPath("SanGenHeightmapDecompositionStemTest");
    WriteSyntheticExternalMap(folderPath);
    Check(!RenameSanmapDocumentStem(folderPath, "Test_Map_One").empty(),
          "the exported document was found and renamed to Test_Map_One.sanmap");

    Params::MapRecipe loadedRecipe;
    Data::MapFields loadedFields;
    std::vector<Data::BakedLayerImage> bakedLayerImages;
    std::vector<Data::StratumArt> stratumArt;
    const Io::MapImportResult result = Io::MapImporter::LoadSanmap(
        folderPath, loadedRecipe, &loadedFields, Io::MapImportOptions(), nullptr, nullptr,
        &bakedLayerImages, &stratumArt);
    Check(result.bSucceeded, "the renamed document loads");
    Check(loadedRecipe.layerStack.geoLayers.size() == 1, "exactly one fresh-synthesis GeoLayer");
    Check(!loadedRecipe.layerStack.geoLayers.empty()
          && loadedRecipe.layerStack.geoLayers[0].name == "Test Map One",
          "\"Test_Map_One\" -> \"Test Map One\" -- derived from the FILENAME stem, not mapName "
          "(still \"mapdef\")");
}

void CheckLeadingUnderscoreIsTrimmed() {
    const std::string folderPath = ScratchFolderPath("SanGenHeightmapDecompositionLeadingUnderscoreTest");
    WriteSyntheticExternalMap(folderPath);
    Check(!RenameSanmapDocumentStem(folderPath, "_leading").empty(),
          "the exported document was found and renamed to _leading.sanmap");

    Params::MapRecipe loadedRecipe;
    Data::MapFields loadedFields;
    std::vector<Data::BakedLayerImage> bakedLayerImages;
    std::vector<Data::StratumArt> stratumArt;
    Io::MapImporter::LoadSanmap(folderPath, loadedRecipe, &loadedFields, Io::MapImportOptions(),
                                nullptr, nullptr, &bakedLayerImages, &stratumArt);
    Check(!loadedRecipe.layerStack.geoLayers.empty()
          && loadedRecipe.layerStack.geoLayers[0].name == "leading",
          "a leading underscore is trimmed after the underscore->space swap (\"_leading\" -> \"leading\")");
}

void CheckDegenerateStemFallsBackToImportedBake() {
    const std::string folderPath = ScratchFolderPath("SanGenHeightmapDecompositionDegenerateStemTest");
    WriteSyntheticExternalMap(folderPath);
    Check(!RenameSanmapDocumentStem(folderPath, "___").empty(),
          "the exported document was found and renamed to ___.sanmap");

    Params::MapRecipe loadedRecipe;
    Data::MapFields loadedFields;
    std::vector<Data::BakedLayerImage> bakedLayerImages;
    std::vector<Data::StratumArt> stratumArt;
    Io::MapImporter::LoadSanmap(folderPath, loadedRecipe, &loadedFields, Io::MapImportOptions(),
                                nullptr, nullptr, &bakedLayerImages, &stratumArt);
    Check(!loadedRecipe.layerStack.geoLayers.empty()
          && loadedRecipe.layerStack.geoLayers[0].name == "Imported Bake",
          "an all-underscore (whitespace-only after the swap) stem falls back to \"Imported Bake\"");
}

// The out-of-scope guard, restated as its own acceptance case: a designer's hand rename of the
// imported GeoLayer must survive a round trip through export + re-import -- the re-hydration branch
// (recipe.layerStack already populated) never re-derives an existing group's name.
void CheckReimportAfterHandRenameDoesNotRevertName() {
    const std::string firstFolderPath = ScratchFolderPath("SanGenHeightmapDecompositionHandRenameTest");
    WriteSyntheticExternalMap(firstFolderPath);

    Params::MapRecipe firstRecipe;
    Data::MapFields firstFields;
    std::vector<Data::BakedLayerImage> firstBakedLayerImages;
    std::vector<Data::StratumArt> firstStratumArt;
    Io::MapImporter::LoadSanmap(firstFolderPath, firstRecipe, &firstFields, Io::MapImportOptions(),
                                nullptr, nullptr, &firstBakedLayerImages, &firstStratumArt);
    Check(firstRecipe.layerStack.geoLayers.size() == 1, "the fresh import synthesized one GeoLayer");
    if (firstRecipe.layerStack.geoLayers.empty()) return;

    firstRecipe.layerStack.geoLayers[0].name = "Hand Renamed By Designer";   // simulated Layer Editor edit

    Pipeline::GenerationAssembler assembler(firstRecipe);
    RunAssembler(assembler, firstBakedLayerImages, firstStratumArt);

    const std::string secondFolderPath =
        ScratchFolderPath("SanGenHeightmapDecompositionHandRenameRoundTrip");
    Io::MapExportOptions exportOptions;
    const Io::MapExportResult exportResult =
        Io::MapExporter::ExportAll(secondFolderPath, firstRecipe, assembler.Fields(), exportOptions);
    Check(exportResult.bSucceeded, "the hand-renamed recipe round-trips back out through export");

    Params::MapRecipe secondRecipe;
    Data::MapFields secondFields;
    std::vector<Data::BakedLayerImage> secondBakedLayerImages;
    std::vector<Data::StratumArt> secondStratumArt;
    const Io::MapImportResult secondResult = Io::MapImporter::LoadSanmap(
        secondFolderPath, secondRecipe, &secondFields, Io::MapImportOptions(), nullptr, nullptr,
        &secondBakedLayerImages, &secondStratumArt);
    Check(secondResult.bSucceeded, "the re-exported, hand-renamed map re-imports");
    Check(!secondRecipe.layerStack.geoLayers.empty()
          && secondRecipe.layerStack.geoLayers[0].name == "Hand Renamed By Designer",
          "re-importing a .sanmap whose group was hand-renamed does NOT revert the name -- the "
          "re-hydration branch never re-derives an existing GeoLayer's name");
}

} // namespace

int main() {
    const std::string folderPath = ScratchFolderPath("SanGenHeightmapDecompositionTest");
    WriteSyntheticExternalMap(folderPath);
    CheckFreshImportSynthesizesOneLayerAndFeedsMask(folderPath);
    CheckReimportAfterRoundTripDoesNotDuplicate(folderPath);
    CheckFreshImportDerivesNameFromFilenameStem();
    CheckLeadingUnderscoreIsTrimmed();
    CheckDegenerateStemFallsBackToImportedBake();
    CheckReimportAfterHandRenameDoesNotRevertName();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
