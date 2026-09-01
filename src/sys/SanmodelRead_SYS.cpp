// SanmodelRead_SYS.cpp — see SanmodelRead_SYS.h for the format and the binding correctness
// requirement (count/index fields are int32 bit patterns in float slots, never round/cast). The
// bounds-checked cursor itself lives in SanmodelByteCursor_SYS.h (ARCH §1.5 aspect split, keeps
// this file under the size ceiling).
#include "SanmodelRead_SYS.h"
#include "SanmodelByteCursor_SYS.h"

namespace SanmapGen {
namespace Sys {
namespace {

// Reads one [count][count*floatsPerElement floats] segment. `destination` receives the raw float
// values when non-null (segment 0, vertices); every other segment is walked (to stay correctly
// positioned) but discarded. False — with `outError` naming `segmentLabel` — on any bounds
// violation or a negative count.
bool ReadFloatSegment(SanmodelByteCursor& cursor, int floatsPerElement, const char* segmentLabel,
                      std::vector<float>* destination, std::string& outError) {
    std::int32_t elementCount = 0;
    if (!cursor.ReadInt32Bits(elementCount) || elementCount < 0) {
        outError = std::string("SanmodelRead: malformed or truncated ") + segmentLabel + " segment count.";
        return false;
    }
    const std::uint64_t floatCount = static_cast<std::uint64_t>(elementCount) * floatsPerElement;
    if (!destination) {
        if (!cursor.Skip(floatCount * 4)) {
            outError = std::string("SanmodelRead: ") + segmentLabel + " segment runs past end of buffer.";
            return false;
        }
        return true;
    }
    destination->reserve(destination->size() + static_cast<std::size_t>(floatCount));
    for (std::uint64_t i = 0; i < floatCount; ++i) {
        float value = 0.0f;
        if (!cursor.ReadFloat(value)) {
            outError = std::string("SanmodelRead: ") + segmentLabel + " segment runs past end of buffer.";
            return false;
        }
        destination->push_back(value);
    }
    return true;
}

// Segment 7: the same [count][count*3] shape as ReadFloatSegment, but `count` is the TRIANGLE
// count and every one of the 3*count values is itself an int32 bit pattern (a vertex index),
// never a float to round/cast.
bool ReadIndexSegment(SanmodelByteCursor& cursor, std::vector<std::uint32_t>& outIndices,
                      std::string& outError) {
    std::int32_t triangleCount = 0;
    if (!cursor.ReadInt32Bits(triangleCount) || triangleCount < 0) {
        outError = "SanmodelRead: malformed or truncated indices segment count.";
        return false;
    }
    const std::uint64_t indexCount = static_cast<std::uint64_t>(triangleCount) * 3;
    outIndices.reserve(outIndices.size() + static_cast<std::size_t>(indexCount));
    for (std::uint64_t i = 0; i < indexCount; ++i) {
        std::int32_t bits = 0;
        if (!cursor.ReadInt32Bits(bits)) {
            outError = "SanmodelRead: indices segment runs past end of buffer.";
            return false;
        }
        outIndices.push_back(static_cast<std::uint32_t>(bits));
    }
    return true;
}

} // namespace

SanmodelReadResult ReadSanmodelMesh(const unsigned char* bytes, std::size_t byteSize,
                                     const SanmodelReadLimits& limits) {
    SanmodelReadResult result;
    if (bytes == nullptr || byteSize == 0) {
        result.errorMessage = "SanmodelRead: empty input buffer.";
        return result;
    }
    if (byteSize > limits.maximumSourceByteSize) {
        result.errorMessage = "SanmodelRead: input exceeds maximumSourceByteSize, refusing to parse.";
        return result;
    }

    SanmodelByteCursor cursor(bytes, byteSize);
    std::string name;
    if (!cursor.ReadNulTerminatedString(name)) {
        result.errorMessage = "SanmodelRead: name string is never NUL-terminated (truncated file).";
        return result;
    }

    // Segments 0-6: vertices(3) normals(3) tangents(4) uv1(2) uv2(2) uv3(2) colors(4). Only
    // segment 0 (vertices) is kept.
    static constexpr int floatsPerElement[7] = { 3, 3, 4, 2, 2, 2, 4 };
    static const char* const segmentLabel[7] = {
        "vertices", "normals", "tangents", "uv1", "uv2", "uv3", "colors"
    };
    for (int segment = 0; segment < 7; ++segment) {
        std::vector<float>* destination = (segment == 0) ? &result.mesh.positions : nullptr;
        if (!ReadFloatSegment(cursor, floatsPerElement[segment], segmentLabel[segment], destination,
                              result.errorMessage))
            return result;
    }

    // Segment 7: indices (bit-pattern int32s, 3 per triangle).
    if (!ReadIndexSegment(cursor, result.mesh.triangleIndices, result.errorMessage))
        return result;

    // Segment 8 (bindposes) is only present in the byte stream when the source mesh was skinned.
    if (cursor.Remaining() > 0) {
        result.mesh.bWasSkinned = true;
        if (!ReadFloatSegment(cursor, 16, "bindposes", nullptr, result.errorMessage))
            return result;
    }

    result.bSucceeded = true;
    return result;
}

} // namespace Sys
} // namespace SanmapGen
