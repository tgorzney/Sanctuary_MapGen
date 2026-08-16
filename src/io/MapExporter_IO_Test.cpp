// MapExporter_IO_Test.cpp — acceptance test for the `.sanmap` + `Textures/` writer (section D).
// Drives the real exporter against a scratch folder under the platform temp directory: the
// quantizers, the path join, the document's own top-level fields, and the exact byte layout of
// the heightmap RAW and the stratum TGAs (which is what the Sanctuary editor reads).
#include "MapExporter_IO.h"
#include "../data/MapFields_DATA.h"
#include "../params/MapRecipe_PARAMS.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
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
    Io::MapExportOptions options;
    options.mapName = "scratch";
    const std::string documentText = Io::MapExporter::BuildSanmapJsonText(recipe, options);

    Check(documentText.find("\"fileVersion\"") != std::string::npos, "the document names its version");
    Check(documentText.find("\"mapGeneratorData\"") != std::string::npos,
          "and carries the generator-state block that makes the sanmap the source of truth");
    Check(documentText.find("\"heightmapResolution\": 513") != std::string::npos,
          "heightmapResolution is the VERTEX count, not the cell count");
    Check(documentText.find("\"stratumLayers\"") != std::string::npos,
          "the format's fixed texture layer array is present");
    Check(documentText.find("\"props\"") != std::string::npos,
          "and the entity domains are written empty and valid, never omitted (SCOPE NOTE 1)");
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
    Data::MapFields fields;
    fields.Resize(recipe.geometry.VertexSize(), 0.0f);
    fields.heightfield.Set(0, 0, 1.0f);
    fields.surfaceStratumWeights[0].Set(0, 0, 1.0f);
    fields.slope.Set(1, 1, 0.5f);
    fields.flow.Set(1, 1, 0.25f);

    Io::MapExportOptions options;
    options.mapName = "scratch";
    const Io::MapExportResult result = Io::MapExporter::ExportAll(scratchFolder, recipe, fields, options);
    Check(result.bSucceeded, "Export All succeeds against a sized field set");
    Check(result.WrittenFileCount() == 6,
          "and writes the document plus heightmap, two masks, slope and flow");

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
    TestAnInvalidRecipeIsRefusedBeforeAnythingIsWritten();
    TestExportAllWritesEveryPayload();
    TestFolderPreparationIsTheOneDoorAboveIo();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
