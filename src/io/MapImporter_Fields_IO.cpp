// MapImporter_Fields_IO.cpp — the `Textures/` payload back into a caller-owned `Data::MapFields`.
// Layer: IO. The exact inverse of MapExporter_Textures_IO.cpp: a 16-bit little-endian heightmap
// and two uncompressed 32-bit BGRA TGAs, read bottom-row-first with the same channel swizzle.
//
// The fields are sized from the RECIPE, never from the file: a payload that disagrees with the
// document is clipped to the recipe's grid and the mismatch is logged (Constitution §6). A missing
// texture is not an error — a .sanmap exported "sanmap only" simply has none.
#include "MapImporter_IO.h"
#include "FilesystemPrimitives_IO.h"
#include "MapImporter_HeightmapDecomposition_IO.h"
#include "../data/BakedLayerImage_DATA.h"
#include "../data/MapFields_DATA.h"
#include "../params/MapRecipe_PARAMS.h"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

namespace SanmapGen {
namespace Io {
namespace {

constexpr std::size_t tgaHeaderByteSize = 18u;
constexpr int bgraWeightOrder[4] = { 2, 1, 0, 3 };   // matches the exporter's swizzle

bool ReadWholeFile(const std::string& filePath, std::uint64_t maximumByteSize,
                   std::vector<unsigned char>& outBytes) {
    std::error_code sizeError;
    const std::filesystem::path path(filePath);
    if (!std::filesystem::is_regular_file(path, sizeError)) return false;
    const std::uintmax_t byteSize = std::filesystem::file_size(path, sizeError);
    if (sizeError || static_cast<std::uint64_t>(byteSize) > maximumByteSize) return false;
    std::ifstream inputStream(filePath, std::ios::binary);
    if (!inputStream) return false;
    outBytes.resize(static_cast<std::size_t>(byteSize));
    if (!outBytes.empty())
        inputStream.read(reinterpret_cast<char*>(outBytes.data()),
                         static_cast<std::streamsize>(outBytes.size()));
    return static_cast<bool>(inputStream);
}

bool LoadHeightmapRaw(const std::string& filePath, int vertexSize, Data::MapFields& outFields,
                      const MapImportOptions& options, MapImportResult& result) {
    std::vector<unsigned char> bytes;
    if (!ReadWholeFile(filePath, options.safetyLimits.maximumTextureByteSize, bytes)) return false;
    const std::size_t expectedSampleCount = static_cast<std::size_t>(vertexSize) * vertexSize;
    if (bytes.size() < expectedSampleCount * 2u) {
        result.Warn("heightmap.raw is smaller than the map's grid; the tail was left flat.");
    }
    const std::size_t readableSamples = bytes.size() / 2u < expectedSampleCount
        ? bytes.size() / 2u : expectedSampleCount;
    for (std::size_t sampleIndex = 0; sampleIndex < readableSamples; ++sampleIndex) {
        const unsigned int lowByte  = bytes[sampleIndex * 2u];
        const unsigned int highByte = bytes[sampleIndex * 2u + 1u];
        const unsigned int sample = lowByte | (highByte << 8);
        const int column = static_cast<int>(sampleIndex % static_cast<std::size_t>(vertexSize));
        const int row    = static_cast<int>(sampleIndex / static_cast<std::size_t>(vertexSize));
        outFields.heightfield.Set(column, row, static_cast<float>(sample) * (1.0f / 65535.0f));
    }
    result.Log("Loaded heightmap RAW: " + std::to_string(readableSamples) + " sample(s).");
    return readableSamples > 0;
}

// One uncompressed BGRA TGA into four consecutive surface-weight fields.
bool LoadStratumMaskTga(const std::string& filePath, int firstWeightIndex, int sampleSize,
                        Data::MapFields& outFields, const MapImportOptions& options,
                        MapImportResult& result) {
    std::vector<unsigned char> bytes;
    if (!ReadWholeFile(filePath, options.safetyLimits.maximumTextureByteSize, bytes)) return false;
    if (bytes.size() < tgaHeaderByteSize) { result.Warn(filePath + " is too short to be a TGA."); return false; }
    const int fileWidth  = bytes[12] | (bytes[13] << 8);
    const int fileHeight = bytes[14] | (bytes[15] << 8);
    if (bytes[2] != 2 || bytes[16] != 32) {
        result.Warn(filePath + " is not an uncompressed 32-bit TGA; it was skipped.");
        return false;
    }
    const std::size_t pixelCount = static_cast<std::size_t>(fileWidth) * fileHeight;
    if (fileWidth < 1 || fileHeight < 1 || bytes.size() < tgaHeaderByteSize + pixelCount * 4u) {
        result.Warn(filePath + " is truncated; it was skipped.");
        return false;
    }
    const int copySize = fileWidth < sampleSize ? fileWidth : sampleSize;
    for (int row = 0; row < copySize; ++row) {
        // The file's first row is the image's BOTTOM row.
        const std::size_t fileRow = static_cast<std::size_t>(fileHeight - 1 - row);
        for (int column = 0; column < copySize; ++column) {
            const std::size_t pixelStart =
                tgaHeaderByteSize + (fileRow * fileWidth + static_cast<std::size_t>(column)) * 4u;
            for (int channel = 0; channel < 4; ++channel) {
                const int weightIndex = firstWeightIndex + bgraWeightOrder[channel];
                if (weightIndex >= Data::MapFields::stratumCount) continue;
                // MASKING_SPEC §1.5/§1.6: IO seeds the PHYSICAL field (materialProportions) from an
                // imported mask — a physical-field approximation, never surfaceStratumWeights (the
                // Mask stage's own exclusive, VISIBLE-field output; ARCH §3.4 single-writer rule,
                // STEP101 gap 1).
                outFields.materialProportions[weightIndex].Set(
                    column, row, static_cast<float>(bytes[pixelStart + channel]) * (1.0f / 255.0f));
            }
        }
    }
    result.Log("Loaded " + filePath + " (" + std::to_string(fileWidth) + " x "
               + std::to_string(fileHeight) + ").");
    return true;
}

} // namespace

bool MapImporter::LoadBakedFields(const std::string& folderPath, Params::MapRecipe& recipe,
                                  Data::MapFields& outFields, const MapImportOptions& options,
                                  MapImportResult& result,
                                  std::vector<Data::BakedLayerImage>& outBakedLayerImages) {
    const int vertexSize = recipe.geometry.VertexSize();
    if (vertexSize < 2) { result.Warn("The recipe's geometry is too small to size the fields."); return false; }
    outFields.Resize(vertexSize, 0.0f);
    const std::string texturesFolder = JoinExportPath(folderPath, options.texturesFolderName);
    const bool bHeightmapLoaded = LoadHeightmapRaw(
        JoinExportPath(texturesFolder, options.heightmapRawName), vertexSize, outFields, options, result);
    if (!bHeightmapLoaded) result.Log("No heightmap.raw beside the document; the heightfield is flat.");
    const bool bLowLoaded = LoadStratumMaskTga(
        JoinExportPath(texturesFolder, options.stratumMaskLowName), 0, vertexSize - 1, outFields,
        options, result);
    const bool bHighLoaded = LoadStratumMaskTga(
        JoinExportPath(texturesFolder, options.stratumMaskHighName), 4, vertexSize - 1, outFields,
        options, result);
    // STEP101: only once the heightmap itself is present -- with none, DecomposeBakedHeightmapIntoLayers
    // would synthesize/re-derive an all-zero bake, which is strictly worse than leaving the recipe alone.
    if (bHeightmapLoaded) DecomposeBakedHeightmapIntoLayers(recipe, outFields, outBakedLayerImages);
    return bHeightmapLoaded || bLowLoaded || bHighLoaded;
}

} // namespace Io
} // namespace SanmapGen
