// MapExporter_Textures_IO.cpp — the two raw-byte payloads of the `Textures/` folder: the 16-bit
// heightmap and the uncompressed stratum-mask TGAs. Layer: IO.
//
// Both formats are byte-compatible with what the Sanctuary editor already reads (the v1 exporter's
// output): the heightmap is (mapSize+1)^2 little-endian uint16 samples of the NORMALIZED height,
// and each TGA is an uncompressed 32-bit BGRA image written bottom-row-first with no RLE (RLE is
// deliberately off — the editor rejects it).
#include "MapExporter_IO.h"
#include "../data/MapFields_DATA.h"
#include <cstdint>
#include <fstream>

namespace SanmapGen {
namespace Io {
namespace {

// Channel c of a BGRA pixel maps onto surface weight index (2,1,0,3) — the same swizzle the v1
// exporter produced through stb_image_write, kept so an existing map's masks read back identically.
constexpr int bgraWeightOrder[4] = { 2, 1, 0, 3 };

void AppendTgaHeader(std::vector<unsigned char>& bytes, int width, int height) {
    const unsigned char header[12] = { 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    bytes.insert(bytes.end(), header, header + 12);
    bytes.push_back(static_cast<unsigned char>(width & 0xFF));
    bytes.push_back(static_cast<unsigned char>((width >> 8) & 0xFF));
    bytes.push_back(static_cast<unsigned char>(height & 0xFF));
    bytes.push_back(static_cast<unsigned char>((height >> 8) & 0xFF));
    bytes.push_back(32);   // bits per pixel
    bytes.push_back(8);    // 8 alpha bits, bottom-left origin
}

// One 4-weight slice of the surface weights, as an uncompressed BGRA TGA.
bool WriteStratumMaskTga(const std::string& filePath, const Data::MapFields& fields,
                         int firstWeightIndex, MapExportResult& result) {
    // The mask textures are CELL-sized (mapSize), not vertex-sized: the engine samples a splat
    // texture per cell, and mapSize+1 is not a power of two. mapSize == VertexSize() - 1.
    const int sampleSize = fields.VertexSize() - 1;
    if (sampleSize < 1) { result.Log("Stratum mask skipped: the fields are not sized."); return false; }
    std::vector<unsigned char> bytes;
    bytes.reserve(static_cast<std::size_t>(sampleSize) * sampleSize * 4u + 18u);
    AppendTgaHeader(bytes, sampleSize, sampleSize);
    for (int row = sampleSize - 1; row >= 0; --row) {
        for (int column = 0; column < sampleSize; ++column) {
            for (int channel = 0; channel < 4; ++channel) {
                const int weightIndex = firstWeightIndex + bgraWeightOrder[channel];
                const float weight = weightIndex < Data::MapFields::stratumCount
                    ? fields.surfaceStratumWeights[weightIndex].Get(column, row) : 0.0f;
                bytes.push_back(QuantizeNormalizedWeightSample(weight));
            }
        }
    }
    if (!WriteBinaryFileBytes(filePath, bytes.data(), bytes.size())) {
        result.Log("Failed to write " + filePath);
        return false;
    }
    result.RecordWrittenFile(filePath);
    return true;
}

} // namespace

std::string JoinExportPath(const std::string& folderPath, const std::string& segmentName) {
    if (folderPath.empty()) return segmentName;
    const char lastCharacter = folderPath[folderPath.size() - 1];
    if (lastCharacter == '/' || lastCharacter == '\\') return folderPath + segmentName;
    return folderPath + "/" + segmentName;
}

bool WriteBinaryFileBytes(const std::string& filePath, const void* bytes, std::size_t byteCount) {
    if (bytes == nullptr && byteCount > 0) return false;
    std::ofstream outputStream(filePath, std::ios::binary | std::ios::trunc);
    if (!outputStream) return false;
    if (byteCount > 0)
        outputStream.write(static_cast<const char*>(bytes), static_cast<std::streamsize>(byteCount));
    outputStream.flush();
    return static_cast<bool>(outputStream);
}

bool MapExporter::WriteHeightmapRaw(const std::string& filePath, const Data::MapFields& fields,
                                    MapExportResult& result) {
    const int vertexSize = fields.VertexSize();
    if (vertexSize < 1) { result.Log("Heightmap RAW skipped: the fields are not sized."); return false; }
    std::vector<std::uint16_t> samples(static_cast<std::size_t>(vertexSize) * vertexSize, 0u);
    for (int row = 0; row < vertexSize; ++row)
        for (int column = 0; column < vertexSize; ++column)
            samples[static_cast<std::size_t>(row) * vertexSize + column] =
                QuantizeNormalizedHeightSample(fields.heightfield.Get(column, row));
    if (!WriteBinaryFileBytes(filePath, samples.data(), samples.size() * sizeof(std::uint16_t))) {
        result.Log("Failed to write " + filePath);
        return false;
    }
    result.Log("Wrote heightmap RAW: " + std::to_string(vertexSize) + " x " + std::to_string(vertexSize));
    result.RecordWrittenFile(filePath);
    return true;
}

bool MapExporter::WriteStratumMaskImages(const std::string& folderPath, const Data::MapFields& fields,
                                         const MapExportOptions& options, MapExportResult& result) {
    const std::string lowPath  = JoinExportPath(folderPath, options.fileNames.stratumMaskLowName);
    const std::string highPath = JoinExportPath(folderPath, options.fileNames.stratumMaskHighName);
    const bool bLowWritten  = WriteStratumMaskTga(lowPath, fields, 0, result);
    const bool bHighWritten = WriteStratumMaskTga(highPath, fields, 4, result);
    // Weight index 8 is the implicit BASE stratum: the format's 9 layers are 8 blended weights over
    // a base, exactly as the v1 exporter wrote them, so it has no file of its own.
    return bLowWritten && bHighWritten;
}

} // namespace Io
} // namespace SanmapGen
