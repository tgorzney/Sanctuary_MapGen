// MapExporter_Image_IO.cpp — the two PNG visualizations of the baked fields: Slope and Flow.
// Layer: IO. Both are PICTURES, not data: they SAMPLE `Data::MapFields` and map the sample onto a
// grey level through the export options' display settings — nothing here re-derives a field
// (ARCH §3.2, the shadow-sim rule), which is why the slope image reads `fields.slope` (written by
// the Mask stage) instead of differencing the heightfield the way the v1 exporter did.
//
// PNG encoding uses miniz's own writer — the same vendored library `SanpackReader_Inflate_IO.cpp`
// already compiles into this library, so the image path adds no dependency and no build edit.
#include "MapExporter_IO.h"
#include "FilesystemPrimitives_IO.h"
#include "MapExporter_SampleQuantize_IO.h"
#include "../data/MapFields_DATA.h"
#include <miniz.h>
#include <cmath>

namespace SanmapGen {
namespace Io {
namespace {

// A greyscale 0..1 field rendered as an opaque RGBA image, cell-sized (see the note in
// MapExporter_Textures_IO.cpp: mapSize == VertexSize() - 1).
bool WriteGreyscaleFieldPng(const std::string& filePath, const Data::FloatField& field,
                            int sampleSize, float displayScale, MapExportResult& result) {
    if (sampleSize < 1 || field.IsEmpty()) {
        result.Log("Image skipped: the fields are not sized.");
        return false;
    }
    std::vector<unsigned char> pixels(static_cast<std::size_t>(sampleSize) * sampleSize * 4u, 0u);
    for (int row = 0; row < sampleSize; ++row) {
        for (int column = 0; column < sampleSize; ++column) {
            const unsigned char intensity =
                QuantizeNormalizedWeightSample(field.Get(column, row) * displayScale);
            const std::size_t pixelStart = (static_cast<std::size_t>(row) * sampleSize + column) * 4u;
            pixels[pixelStart + 0] = intensity;
            pixels[pixelStart + 1] = intensity;
            pixels[pixelStart + 2] = intensity;
            pixels[pixelStart + 3] = 255u;
        }
    }
    std::size_t encodedByteCount = 0;
    void* encodedBytes = tdefl_write_image_to_png_file_in_memory(
        pixels.data(), sampleSize, sampleSize, 4, &encodedByteCount);
    if (encodedBytes == nullptr) { result.Log("PNG encoding failed for " + filePath); return false; }
    const bool bWritten = WriteBinaryFileBytes(filePath, encodedBytes, encodedByteCount);
    mz_free(encodedBytes);
    if (!bWritten) { result.Log("Failed to write " + filePath); return false; }
    result.RecordWrittenFile(filePath);
    return true;
}

} // namespace

// The slope FIELD is a gradient magnitude (rise/run = tan of the angle, MASKING_SPEC 1.8), so the
// display scale is 1 / tan(the display maximum) — a degenerate or 90-degree maximum falls back to
// showing the raw gradient rather than dividing by zero (Constitution §6).
bool MapExporter::WriteSlopeImage(const std::string& filePath, const Data::MapFields& fields,
                                  const MapExportOptions& options, MapExportResult& result) {
    const float maximumDegrees = options.slopeDisplayMaximumDegrees;
    float displayScale = 1.0f;
    if (maximumDegrees > 0.0f && maximumDegrees < 89.9f) {
        const float maximumGradient = std::tan(maximumDegrees * 3.14159265358979f / 180.0f);
        if (maximumGradient > 1.0e-6f) displayScale = 1.0f / maximumGradient;
    }
    const bool bWritten = WriteGreyscaleFieldPng(filePath, fields.slope, fields.VertexSize() - 1,
                                                 displayScale, result);
    if (bWritten) result.Log("Wrote slope image: " + filePath);
    return bWritten;
}

bool MapExporter::WriteFlowImage(const std::string& filePath, const Data::MapFields& fields,
                                 const MapExportOptions& options, MapExportResult& result) {
    const float displayScale = options.flowDisplayMultiplier > 0.0f ? options.flowDisplayMultiplier
                                                                    : 1.0f;
    const bool bWritten = WriteGreyscaleFieldPng(filePath, fields.flow, fields.VertexSize() - 1,
                                                 displayScale, result);
    if (bWritten) result.Log("Wrote flow image: " + filePath);
    return bWritten;
}

} // namespace Io
} // namespace SanmapGen
