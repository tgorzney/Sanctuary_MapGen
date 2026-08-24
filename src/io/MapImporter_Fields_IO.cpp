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
#include "../data/StratumArt_DATA.h"
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

// One uncompressed BGRA TGA into four consecutive surface-weight fields, AND (STEP105) the same
// four `Data::StratumArt::importedMask` fields at the TGA's own NATIVE resolution -- that field's
// own contract is "any resolution, the Mask stage resamples it bilinearly" (MASKING_SPEC §1.8), so
// unlike `materialProportions` (vertexSize-clipped, the generated grid's own resolution) this write
// is never cropped to `sampleSize`.
bool LoadStratumMaskTga(const std::string& filePath, int firstWeightIndex, int sampleSize,
                        Data::MapFields& outFields, std::vector<Data::StratumArt>& outStratumArt,
                        const MapImportOptions& options, MapImportResult& result) {
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
    if (outStratumArt.size() < static_cast<std::size_t>(Data::MapFields::stratumCount))
        outStratumArt.resize(static_cast<std::size_t>(Data::MapFields::stratumCount));
    // Size each of this call's four destination fields once, before the pixel loop below (the loop
    // only ever calls Set(), never Resize()).
    for (int channel = 0; channel < 4; ++channel) {
        const int weightIndex = firstWeightIndex + bgraWeightOrder[channel];
        if (weightIndex < Data::MapFields::stratumCount)
            outStratumArt[weightIndex].importedMask.Resize(fileWidth, fileHeight, 0.0f);
    }
    // Widened to the TGA's own fileWidth/fileHeight (not sampleSize) so the native-resolution write
    // below is never cropped; the existing vertexSize-clipped materialProportions write stays gated
    // on column/row < sampleSize exactly as before.
    for (int row = 0; row < fileHeight; ++row) {
        // The file's first row is the image's BOTTOM row.
        const std::size_t fileRow = static_cast<std::size_t>(fileHeight - 1 - row);
        for (int column = 0; column < fileWidth; ++column) {
            const std::size_t pixelStart =
                tgaHeaderByteSize + (fileRow * fileWidth + static_cast<std::size_t>(column)) * 4u;
            for (int channel = 0; channel < 4; ++channel) {
                const int weightIndex = firstWeightIndex + bgraWeightOrder[channel];
                if (weightIndex >= Data::MapFields::stratumCount) continue;
                const float value = static_cast<float>(bytes[pixelStart + channel]) * (1.0f / 255.0f);
                // MASKING_SPEC §1.5/§1.6: IO seeds the PHYSICAL field (materialProportions) from an
                // imported mask — a physical-field approximation, never surfaceStratumWeights (the
                // Mask stage's own exclusive, VISIBLE-field output; ARCH §3.4 single-writer rule,
                // STEP101 gap 1). Unchanged, still vertexSize-clipped.
                if (column < sampleSize && row < sampleSize)
                    outFields.materialProportions[weightIndex].Set(column, row, value);
                // STEP105: the parallel, native-resolution destination the Mask stage's
                // ImportedMaskMode path actually consumes (Mask_Prepare_PROC.cpp/Mask_Merge_PROC.h).
                outStratumArt[weightIndex].importedMask.Set(column, row, value);
            }
        }
    }
    result.Log("Loaded " + filePath + " (" + std::to_string(fileWidth) + " x "
               + std::to_string(fileHeight) + ").");
    return true;
}

} // namespace

// A validated 16-bit little-endian RAW heightmap into a caller-supplied field (STEP102's public
// primitive) — see MapImporter_IO.h for the full contract. `outField` is resized here, so a
// payload that disagrees with `vertexSize` is clipped/flat-padded, never trusted (Constitution §6).
bool MapImporter::LoadRawHeightmapIntoField(const std::string& filePath, int vertexSize,
                                            Data::FloatField& outField, const MapImportOptions& options,
                                            MapImportResult& result) {
    outField.Resize(vertexSize, vertexSize, 0.0f);
    std::vector<unsigned char> bytes;
    if (!ReadWholeFile(filePath, options.safetyLimits.maximumTextureByteSize, bytes)) return false;
    const std::size_t expectedSampleCount = static_cast<std::size_t>(vertexSize) * vertexSize;
    if (bytes.size() < expectedSampleCount * 2u) {
        result.Warn("heightmap RAW is smaller than the map's grid; the tail was left flat.");
    }
    const std::size_t readableSamples = bytes.size() / 2u < expectedSampleCount
        ? bytes.size() / 2u : expectedSampleCount;
    for (std::size_t sampleIndex = 0; sampleIndex < readableSamples; ++sampleIndex) {
        const unsigned int lowByte  = bytes[sampleIndex * 2u];
        const unsigned int highByte = bytes[sampleIndex * 2u + 1u];
        const unsigned int sample = lowByte | (highByte << 8);
        const int column = static_cast<int>(sampleIndex % static_cast<std::size_t>(vertexSize));
        const int row    = static_cast<int>(sampleIndex / static_cast<std::size_t>(vertexSize));
        outField.Set(column, row, static_cast<float>(sample) * (1.0f / 65535.0f));
    }
    result.Log("Loaded heightmap RAW: " + std::to_string(readableSamples) + " sample(s).");
    return readableSamples > 0;
}

bool MapImporter::LoadBakedFields(const std::string& folderPath, const std::string& documentPath,
                                  Params::MapRecipe& recipe,
                                  Data::MapFields& outFields, const MapImportOptions& options,
                                  MapImportResult& result,
                                  std::vector<Data::BakedLayerImage>& outBakedLayerImages,
                                  std::vector<Data::StratumArt>& outStratumArt) {
    // STEP109: computed once here (never duplicated into DecomposeBakedHeightmapIntoLayers) — the
    // fresh-synthesis GeoLayer's own name source, never the document's own JSON "name" field.
    const std::string sourceFileName = std::filesystem::path(documentPath).stem().string();
    const int vertexSize = recipe.geometry.VertexSize();
    if (vertexSize < 2) { result.Warn("The recipe's geometry is too small to size the fields."); return false; }
    outFields.Resize(vertexSize, 0.0f);
    const std::string texturesFolder = JoinExportPath(folderPath, options.texturesFolderName);
    const bool bHeightmapLoaded = LoadRawHeightmapIntoField(
        JoinExportPath(texturesFolder, options.heightmapRawName), vertexSize, outFields.heightfield,
        options, result);
    if (!bHeightmapLoaded) result.Log("No heightmap.raw beside the document; the heightfield is flat.");
    const bool bLowLoaded = LoadStratumMaskTga(
        JoinExportPath(texturesFolder, options.stratumMaskLowName), 0, vertexSize - 1, outFields,
        outStratumArt, options, result);
    const bool bHighLoaded = LoadStratumMaskTga(
        JoinExportPath(texturesFolder, options.stratumMaskHighName), 4, vertexSize - 1, outFields,
        outStratumArt, options, result);
    // STEP105: only once the heightmap itself is present -- with none, DecomposeBakedHeightmapIntoLayers
    // would synthesize/re-derive an all-zero bake, which is strictly worse than leaving the recipe alone.
    if (bHeightmapLoaded)
        DecomposeBakedHeightmapIntoLayers(recipe, outFields, outBakedLayerImages, result, sourceFileName);

    // STEP105 §3: default a stratum's importedMaskMode to StaticOverride ONLY when it was never
    // explicitly configured by the imported document.
    //
    // DEVIATION FROM THE TICKET'S OWN TEXT (verified live, not assumed): the ticket's proposed test
    // was "recipe.strata.size() <= stratumIndex" (MapRecipe_PARAMS.h's own "strata past the end run
    // on their defaults"). That signal cannot fire against ANY document written by SanGen's own
    // exporter: `BuildStratumLayersJson` (MapExporter_StratumLayers_IO.cpp) unconditionally writes
    // all `sanmapStratumCount` (9) `stratumLayers[]` slots, defaulting an unconfigured stratum from
    // `Params::Stratum()` rather than omitting it -- so `ReadStratumLayersJson`
    // (MapImporter_StratumLayers_IO.cpp) always grows `recipe.strata` to 9 on import, regardless of
    // whether a human ever touched that slot. Confirmed live: this ticket's OWN acceptance fixture
    // (WriteSyntheticExternalMap, which exports through `Io::MapExporter::ExportAll`) already hits
    // this — `recipe.strata.size() == 9` before this loop even runs, so the size-only check never
    // grows/defaults anything.
    //
    // The signal actually available after JSON parsing is whether the resolved importedMaskMode is
    // still sitting at its own class default (`Disabled`, `Params::Stratum`'s own field default) --
    // so a stratum is treated as "unconfigured" when EITHER it has no slot yet (a genuinely
    // externally-authored document with no `stratumLayers` section at all) OR its slot exists but
    // still reads Disabled. This is what makes `CheckReimportAfterRoundTripDoesNotDuplicate` work:
    // the first import's own StaticOverride default gets written back out explicitly on re-export,
    // so the second import reads back something other than Disabled and this loop leaves it alone.
    // Known imperfection (reported, not silently papered over): a document that explicitly chose
    // Disabled on purpose, whose TGA still happens to carry leftover non-zero pixels for that
    // channel, is indistinguishable from "never configured" and will also be bumped to
    // StaticOverride. Fixing that needs the importer to track per-key JSON presence, out of scope
    // here -- route to the IO Architecture Expert if it ever surfaces as a real complaint.
    for (std::size_t stratumIndex = 0; stratumIndex < outStratumArt.size(); ++stratumIndex) {
        if (!outStratumArt[stratumIndex].HasImportedMask()) continue;
        if (recipe.strata.size() <= stratumIndex) recipe.strata.resize(stratumIndex + 1);
        if (recipe.strata[stratumIndex].importedMaskMode == Params::ImportedMaskMode::Disabled)
            recipe.strata[stratumIndex].importedMaskMode = Params::ImportedMaskMode::StaticOverride;
    }
    return bHeightmapLoaded || bLowLoaded || bHighLoaded;
}

} // namespace Io
} // namespace SanmapGen
