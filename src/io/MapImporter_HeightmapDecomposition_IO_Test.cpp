// MapImporter_HeightmapDecomposition_IO_Test.cpp — STEP101 acceptance: importing a genuine
// externally-authored `.sanmap` (recipe.layerStack empty, exactly the reported bug's scenario)
// decomposes its baked heightmap + one-stratum mask into per-stratum baked Params::Layers, and
// running the full GenerationAssembler pipeline once reproduces the ORIGINAL imported heightfield —
// the actual proof the reported bug is fixed. Its own binary (not folded into MapImporter_IO_Test's
// three-TU group) since it is the first IO acceptance test that reaches into Pipeline to run a real
// GenerationAssembler, not just parse/round-trip a document.
#include "MapImporter_IO.h"
#include "MapExporter_IO.h"
#include "../data/BakedLayerImage_DATA.h"
#include "../data/MapFields_DATA.h"
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

// The assembler.Run() proof: NoiseBlend (STEP100) reads the decomposed baked layers. Thermal's
// relaxation is disabled to isolate THIS mechanism from an unrelated sim stage that mutates the
// heightfield on its own defaults (Erosion is already a no-op by default — ErosionLayerSettings::
// bEnabled starts false). `tolerance` distinguishes the ticket's own two acceptance bars: the FIRST
// import's reproduction is exact up to float rounding (the stratum-1 mask quantizes to a byte and
// back); the round-tripped re-import additionally passes through the Mask stage's own transform in
// between, so it earns a looser (still tight) tolerance.
bool AssembledHeightfieldMatchesOriginal(const Params::MapRecipe& recipe,
                                         const std::vector<Data::BakedLayerImage>& bakedLayerImages,
                                         const Data::FloatField& originalHeightfield, float tolerance) {
    Pipeline::GenerationAssembler assembler(recipe);
    assembler.BakedLayerImages() = bakedLayerImages;
    assembler.Thermal().Constants().iterationCount = 0;
    assembler.Run();
    const Data::FloatField& assembled = assembler.Fields().heightfield;
    for (int y = 0; y < originalHeightfield.Height(); ++y)
        for (int x = 0; x < originalHeightfield.Width(); ++x)
            if (!NearlyEqual(assembled.Get(x, y), originalHeightfield.Get(x, y), tolerance)) return false;
    return true;
}

void CheckFreshImportDecomposesAndReproduces(const std::string& folderPath) {
    Params::MapRecipe loadedRecipe;
    Data::MapFields loadedFields;
    std::vector<Data::BakedLayerImage> bakedLayerImages;
    const Io::MapImportResult result = Io::MapImporter::LoadSanmap(
        folderPath, loadedRecipe, &loadedFields, Io::MapImportOptions(), nullptr, nullptr, &bakedLayerImages);
    Check(result.bSucceeded && result.bBakedFieldsLoaded, "the synthetic map loads");

    Check(loadedRecipe.layerStack.geoLayers.size() == 1,
          "the empty-recipe gate synthesizes exactly one new GeoLayer");
    Check(loadedRecipe.layerStack.TotalLayerCount() == 2,
          "stratum 0 (mandatory) and stratum 1 (covering) survive; strata 2-8 are skipped");

    bool bStratum0Baked = false, bStratum1Baked = false;
    int identifier0 = -1, identifier1 = -1;
    for (const Params::GeoLayer& group : loadedRecipe.layerStack.geoLayers)
        for (const Params::Layer& layer : group.layers) {
            Check(layer.bBaked, "every decomposed layer is bBaked");
            if (layer.stratumIndex == 0) { bStratum0Baked = true; identifier0 = layer.layerIdentifier; }
            if (layer.stratumIndex == 1) { bStratum1Baked = true; identifier1 = layer.layerIdentifier; }
        }
    Check(bStratum0Baked && bStratum1Baked, "stratum 0 and stratum 1 both got a decomposed layer");
    Check(identifier0 != identifier1, "the two layers carry distinct layerIdentifiers");

    Check(bakedLayerImages.size() == 2, "assembler.BakedLayerImages() has 2 matching entries");
    Check(Data::FindBakedLayerImage(bakedLayerImages, identifier0) != nullptr
          && Data::FindBakedLayerImage(bakedLayerImages, identifier1) != nullptr,
          "both layerIdentifiers resolve to a stored image");

    Check(NearlyEqual(loadedFields.materialProportions[1].Get(0, 0), 1.0f, 1.0e-4f),
          "materialProportions[1] matches the TGA's fully-covering mask");
    Check(NearlyEqual(loadedFields.surfaceStratumWeights[1].Get(0, 0), 0.0f, 1.0e-6f),
          "surfaceStratumWeights stays untouched by import (zero-filled until the Mask stage runs)");

    Check(AssembledHeightfieldMatchesOriginal(loadedRecipe, bakedLayerImages, loadedFields.heightfield, 1.0e-4f),
          "assembler.Run() reproduces the ORIGINAL imported heightfield -- the reported bug's fix");
}

void CheckReimportAfterRoundTripDoesNotDuplicate(const std::string& firstFolderPath) {
    Params::MapRecipe firstRecipe;
    Data::MapFields firstFields;
    std::vector<Data::BakedLayerImage> firstBakedLayerImages;
    Io::MapImporter::LoadSanmap(firstFolderPath, firstRecipe, &firstFields, Io::MapImportOptions(),
                                nullptr, nullptr, &firstBakedLayerImages);

    // Run once so surfaceStratumWeights (what the EXPORTER reads for the masks) is populated by the
    // real Mask stage -- otherwise a re-export would write back an all-zero mask.
    Pipeline::GenerationAssembler assembler(firstRecipe);
    assembler.BakedLayerImages() = firstBakedLayerImages;
    assembler.Thermal().Constants().iterationCount = 0;
    assembler.Run();

    const std::string secondFolderPath = ScratchFolderPath("SanGenHeightmapDecompositionRoundTrip");
    Io::MapExportOptions exportOptions;
    const Io::MapExportResult exportResult =
        Io::MapExporter::ExportAll(secondFolderPath, firstRecipe, assembler.Fields(), exportOptions);
    Check(exportResult.bSucceeded, "the decomposed recipe round-trips back out through export");

    Params::MapRecipe secondRecipe;
    Data::MapFields secondFields;
    std::vector<Data::BakedLayerImage> secondBakedLayerImages;
    const Io::MapImportResult secondResult = Io::MapImporter::LoadSanmap(
        secondFolderPath, secondRecipe, &secondFields, Io::MapImportOptions(), nullptr, nullptr,
        &secondBakedLayerImages);
    Check(secondResult.bSucceeded, "the re-exported map re-imports");

    Check(secondRecipe.layerStack.geoLayers.size() == firstRecipe.layerStack.geoLayers.size()
          && secondRecipe.layerStack.TotalLayerCount() == firstRecipe.layerStack.TotalLayerCount(),
          "re-importing after an export round trip does not duplicate layers");

    Check(AssembledHeightfieldMatchesOriginal(secondRecipe, secondBakedLayerImages,
                                              assembler.Fields().heightfield, 2.0e-3f),
          "the re-imported recipe reproduces the same (already-decomposed-once) heightfield");
}

} // namespace

int main() {
    const std::string folderPath = ScratchFolderPath("SanGenHeightmapDecompositionTest");
    WriteSyntheticExternalMap(folderPath);
    CheckFreshImportDecomposesAndReproduces(folderPath);
    CheckReimportAfterRoundTripDoesNotDuplicate(folderPath);
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
