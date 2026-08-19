// MapExporter_IO_Test.cpp — acceptance test for the `.sanmap` + `Textures/` writer (section D).
// Drives the real exporter against a scratch folder under the platform temp directory: the
// quantizers, the path join, the document's own top-level fields, and the exact byte layout of
// the heightmap RAW and the stratum TGAs (which is what the Sanctuary editor reads).
#include "MapExporter_IO.h"
#include "FilesystemPrimitives_IO.h"
#include "MapExporter_SampleQuantize_IO.h"
#include "../data/MapFields_DATA.h"
#include "../params/MapRecipe_PARAMS.h"
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <vector>

using namespace SanmapGen;

static int failureCount = 0;

static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

static std::string ScratchFolderPath() {
    std::error_code pathError;
    const std::filesystem::path folder =
        std::filesystem::temp_directory_path(pathError) / "SanGenMapExporterTest";
    std::filesystem::remove_all(folder, pathError);
    return folder.string();
}

static std::vector<unsigned char> ReadFileBytes(const std::string& filePath) {
    std::ifstream inputStream(filePath, std::ios::binary);
    return std::vector<unsigned char>(std::istreambuf_iterator<char>(inputStream),
                                      std::istreambuf_iterator<char>());
}

static void TestQuantizersClampInsteadOfWrapping() {
    Check(Io::QuantizeNormalizedHeightSample(-1.0f) == 0u, "a negative height quantizes to 0");
    Check(Io::QuantizeNormalizedHeightSample(2.0f) == 65535u, "an over-range height saturates");
    Check(Io::QuantizeNormalizedHeightSample(1.0f) == 65535u, "exactly 1 is the top sample");
    Check(Io::QuantizeNormalizedWeightSample(-0.5f) == 0u, "a negative weight quantizes to 0");
    Check(Io::QuantizeNormalizedWeightSample(1.5f) == 255u, "an over-range weight saturates");
    Check(Io::QuantizeNormalizedWeightSample(0.0f) == 0u, "and zero is zero");
}

static void TestPathJoinNeverDoublesASeparator() {
    Check(Io::JoinExportPath("", "Textures") == "Textures", "no folder yields the segment alone");
    Check(Io::JoinExportPath("D:/maps", "Textures") == "D:/maps/Textures", "a plain folder is joined");
    Check(Io::JoinExportPath("D:/maps/", "Textures") == "D:/maps/Textures", "a trailing / is not doubled");
    Check(Io::JoinExportPath("D:\\maps\\", "Textures") == "D:\\maps\\Textures",
          "and neither is a trailing backslash");
}

static void TestDocumentCarriesTheFormatsOwnFields() {
    Params::MapRecipe recipe;
    recipe.geometry.mapSize = 512;
    recipe.geometry.terrainMaxHeight = 200.0f;
    recipe.water.bEnabled = true;
    recipe.water.waterLevelMaximum = 42.5f;
    recipe.mapName = "scratch";
    recipe.mapCredits = "A Test Cartographer";
    const std::string documentText = Io::MapExporter::BuildSanmapJsonText(recipe);

    Check(documentText.find("\"fileVersion\"") != std::string::npos, "the document names its version");
    Check(documentText.find("\"name\": \"scratch\"") != std::string::npos,
          "the document's own name comes from the recipe, not a write-only export option "
          "(STEP25_MapNameCredits_IO)");
    Check(documentText.find("\"credits\": \"A Test Cartographer\"") != std::string::npos,
          "and so does credits");
    Check(documentText.find("\"mapGeneratorData\"") != std::string::npos,
          "and carries the generator-state block that makes the sanmap the source of truth");
    Check(documentText.find("\"heightmapResolution\": 513") != std::string::npos,
          "heightmapResolution is the VERTEX count, not the cell count");
    Check(documentText.find("\"stratumLayers\"") != std::string::npos,
          "the format's fixed texture layer array is present");
    Check(documentText.find("\"props\"") != std::string::npos,
          "and props/decals are written valid and empty when the recipe has none, never omitted — "
          "STEP4_PropsDecals_IO gave them real Params:: types and pure builders, and "
          "STEP5_PropsDecalsValidation_UI live-wired those builders into BuildSanmapJsonText "
          "(SCOPE NOTE 1); areas/armies/markers/chains/props/decals are all covered end to end by "
          "MapImporter_IO_Test's CheckArmiesAndAreas/CheckMarkersAndChains/CheckPropsAndDecals");
}

// STEP1_ShippingBugFixes: `maskRemapMin`/`maskRemapMax` are real Vector4 objects (ARCH §7.2
// item 10), not bare scalars, and `height` is always a whole number (SanMap.cs:24 types it int)
// even when a designer's `terrainMaxHeight` is fractional.
static void TestStratumLayersWriteVector4RemapAndIntegerHeight() {
    Params::MapRecipe recipe;
    recipe.geometry.mapSize = 4;
    recipe.strata.resize(1);
    recipe.strata[0].maskRemapMinimum[0] = 0.1f;
    recipe.strata[0].maskRemapMinimum[1] = 0.2f;
    recipe.strata[0].maskRemapMinimum[2] = 0.3f;
    recipe.strata[0].maskRemapMinimum[3] = 0.4f;
    recipe.strata[0].maskRemapMaximum[0] = 0.5f;
    recipe.strata[0].maskRemapMaximum[1] = 0.6f;
    recipe.strata[0].maskRemapMaximum[2] = 0.7f;
    recipe.strata[0].maskRemapMaximum[3] = 0.9f;
    recipe.geometry.terrainMaxHeight = 127.6f;   // a legal fractional Params::Geometry value

    Io::MapExportOptions options;
    const std::string documentText = Io::MapExporter::BuildSanmapJsonText(recipe, options);
    const nlohmann::json document = nlohmann::json::parse(documentText);
    const nlohmann::json& stratumLayer = document.at("stratumLayers").at(0);

    auto checkVector4 = [](const nlohmann::json& vector, float x, float y, float z, float w,
                           const char* label) {
        Check(vector.is_object(), (std::string(label) + " is an object, not a bare number").c_str());
        const bool bHasAllComponents = vector.contains("x") && vector.contains("y")
                                     && vector.contains("z") && vector.contains("w");
        Check(bHasAllComponents, (std::string(label) + " carries all four Vector4 keys").c_str());
        if (!bHasAllComponents) return;
        const bool bComponentsMatch =
            std::abs(vector.at("x").get<float>() - x) < 1e-5f
            && std::abs(vector.at("y").get<float>() - y) < 1e-5f
            && std::abs(vector.at("z").get<float>() - z) < 1e-5f
            && std::abs(vector.at("w").get<float>() - w) < 1e-5f;
        Check(bComponentsMatch,
              (std::string(label) + " round-trips the stratum's own 4 components").c_str());
    };
    checkVector4(stratumLayer.at("maskRemapMin"), 0.1f, 0.2f, 0.3f, 0.4f, "maskRemapMin");
    checkVector4(stratumLayer.at("maskRemapMax"), 0.5f, 0.6f, 0.7f, 0.9f, "maskRemapMax");

    Check(document.at("height").is_number_integer(),
          "a fractional terrainMaxHeight still writes height as a whole number");
    Check(document.at("height").get<int>() == 128,
          "127.6 rounds up to 128 (rounds, does not truncate)");

    recipe.geometry.terrainMaxHeight = 200.0f;   // already whole
    const nlohmann::json wholeHeightDocument =
        nlohmann::json::parse(Io::MapExporter::BuildSanmapJsonText(recipe, options));
    Check(wholeHeightDocument.at("height").is_number_integer(),
          "an already-integer terrainMaxHeight also writes height as a whole number");
    Check(wholeHeightDocument.at("height").get<int>() == 200,
          "and its value round-trips exactly");
}

// STEP30_LegacyBlobFieldHoming_IO: the exporter's own half of the 4 new field homes — each key
// present, at the right JSON location, carrying the recipe's value at full precision (not just the
// legacy `mapGeneratorData` blob's copy, which already covered these values before this ticket).
static void TestNewFieldHomesAreWritten() {
    Params::MapRecipe recipe;
    recipe.geometry.mapSize = 4;
    recipe.geometry.terrainMaxHeight = 142.375f;   // a non-round float, full precision matters
    recipe.strata.resize(1);
    recipe.strata[0].importedMaskMode = Params::ImportedMaskMode::StaticOverride;
    recipe.strata[0].bEnabled = false;
    recipe.water.deepWaterDepthMinimum = 7.5f;

    const std::string documentText = Io::MapExporter::BuildSanmapJsonText(recipe);
    const nlohmann::json document = nlohmann::json::parse(documentText);

    Check(document.contains("GeneralMapSettings") && document["GeneralMapSettings"].contains("TerrainMaxHeight")
          && std::abs(document["GeneralMapSettings"]["TerrainMaxHeight"].get<float>() - 142.375f) < 1e-4f,
          "GeneralMapSettings.TerrainMaxHeight is written at full float precision, sibling of "
          "TerrainMinHeight");

    const nlohmann::json& stratumLayer = document.at("stratumLayers").at(0);
    Check(stratumLayer.contains("ImportedMaskMode")
          && stratumLayer.at("ImportedMaskMode").get<int>()
             == static_cast<int>(Params::ImportedMaskMode::StaticOverride),
          "stratumLayers[0].ImportedMaskMode is written as the enum ordinal");
    Check(stratumLayer.contains("Enabled") && stratumLayer.at("Enabled").get<bool>() == false,
          "stratumLayers[0].Enabled is written");

    Check(document.contains("deepWaterDepthMin")
          && std::abs(document["deepWaterDepthMin"].get<float>() - 7.5f) < 1e-4f,
          "the top-level deepWaterDepthMin key is written, sibling of hasWater/waterLevel/waterDepth");
}

static void TestAnInvalidRecipeIsRefusedBeforeAnythingIsWritten() {
    const std::string scratchFolder = ScratchFolderPath();
    Params::MapRecipe recipe;
    recipe.geometry.terrainMaxHeight = 0.0f;          // fails Geometry::IsValid()
    const Io::MapExportResult result = Io::MapExporter::ExportSanmapOnly(scratchFolder, recipe);
    Check(!result.bSucceeded, "an invalid recipe is refused");
    Check(result.WrittenFileCount() == 0, "and nothing was written");
    Check(!result.debugLog.empty(), "with the reason logged for the tab's panel");
    std::error_code pathError;
    Check(!std::filesystem::exists(std::filesystem::path(scratchFolder), pathError),
          "the destination folder was not even created");
}

static void TestExportAllWritesEveryPayload() {
    const std::string scratchFolder = ScratchFolderPath();
    Params::MapRecipe recipe;
    recipe.geometry.mapSize = 8;
    recipe.mapName = "scratch";
    Data::MapFields fields;
    fields.Resize(recipe.geometry.VertexSize(), 0.0f);
    fields.heightfield.Set(0, 0, 1.0f);
    fields.surfaceStratumWeights[0].Set(0, 0, 1.0f);
    fields.slope.Set(1, 1, 0.5f);
    fields.flow.Set(1, 1, 0.25f);

    Io::MapExportOptions options;
    const Io::MapExportResult result = Io::MapExporter::ExportAll(scratchFolder, recipe, fields, options);
    Check(result.bSucceeded, "Export All succeeds against a sized field set");
    Check(result.WrittenFileCount() == 6,
          "and writes the document plus heightmap, two masks, slope and flow");
    std::error_code pathError;
    Check(std::filesystem::exists(std::filesystem::path(Io::JoinExportPath(scratchFolder, "scratch.sanmap")),
                                  pathError),
          "the document's file name comes from recipe.mapName, not a write-only export option "
          "(STEP25_MapNameCredits_IO)");

    const std::string texturesFolder = Io::JoinExportPath(scratchFolder, "Textures");
    const std::vector<unsigned char> heightBytes =
        ReadFileBytes(Io::JoinExportPath(texturesFolder, "heightmap.raw"));
    Check(heightBytes.size() == static_cast<std::size_t>(9 * 9 * 2),
          "the RAW is (mapSize+1)^2 sixteen-bit samples");
    Check(heightBytes.size() > 1 && heightBytes[0] == 0xFFu && heightBytes[1] == 0xFFu,
          "stored little-endian: a full-height first vertex is FF FF");

    const std::vector<unsigned char> maskBytes =
        ReadFileBytes(Io::JoinExportPath(texturesFolder, "stratums_1_4.tga"));
    Check(maskBytes.size() == 18u + static_cast<std::size_t>(8 * 8 * 4),
          "the mask TGA is CELL-sized with an 18-byte header");
    Check(maskBytes.size() > 17 && maskBytes[2] == 2u && maskBytes[16] == 32u,
          "uncompressed 32-bit, never RLE (the editor rejects RLE)");
    // Bottom-row-first: row 0 of the field is the LAST row of the file. Weight 0 rides channel 2.
    const std::size_t lastRowStart = 18u + static_cast<std::size_t>(7 * 8 * 4);
    Check(maskBytes.size() > lastRowStart + 2 && maskBytes[lastRowStart + 2] == 255u,
          "and weight 0 of cell (0,0) lands on the B channel of the file's last row");
}

static void TestFolderPreparationIsTheOneDoorAboveIo() {
    Io::MapExportResult result;
    Check(!Io::EnsureExportFolderExists(std::string(), result), "an empty destination is refused");
    Check(!result.debugLog.empty(), "with the reason logged");
    const std::string scratchFolder = Io::JoinExportPath(ScratchFolderPath(), "Nested/Deeper");
    Check(Io::EnsureExportFolderExists(scratchFolder, result), "a nested destination is created");
    Check(Io::EnsureExportFolderExists(scratchFolder, result), "and creating it twice is fine");
}

int main() {
    TestQuantizersClampInsteadOfWrapping();
    TestPathJoinNeverDoublesASeparator();
    TestDocumentCarriesTheFormatsOwnFields();
    TestStratumLayersWriteVector4RemapAndIntegerHeight();
    TestNewFieldHomesAreWritten();
    TestAnInvalidRecipeIsRefusedBeforeAnythingIsWritten();
    TestExportAllWritesEveryPayload();
    TestFolderPreparationIsTheOneDoorAboveIo();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
