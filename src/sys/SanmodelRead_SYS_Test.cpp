// SanmodelRead_SYS_Test.cpp — acceptance test for SanmodelRead_SYS: builds a synthetic
// `.sanmodel` byte buffer for a single known triangle by hand (every segment, including the
// int32-bit-pattern-in-a-float-slot trick for counts and indices) and verifies the reader
// reproduces the exact vertex/index values byte-for-byte. Also covers a truncated buffer and an
// out-of-range count, both of which must fail cleanly rather than crash.
#include "SanmodelRead_SYS.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

using namespace SanmapGen::Sys;

namespace {
int gFailureCount = 0;

void Check(bool bCondition, const char* scenarioName, const char* message) {
    if (bCondition) return;
    std::printf("FAIL %s: %s\n", scenarioName, message);
    ++gFailureCount;
}

void AppendInt32Bits(std::vector<unsigned char>& bytes, std::int32_t value) {
    unsigned char raw[4];
    std::memcpy(raw, &value, 4);
    bytes.insert(bytes.end(), raw, raw + 4);
}
void AppendFloat(std::vector<unsigned char>& bytes, float value) {
    unsigned char raw[4];
    std::memcpy(raw, &value, 4);
    bytes.insert(bytes.end(), raw, raw + 4);
}
// [count][count*floatsPerElement floats] — the shape every segment except indices/bindposes-count
// shares. `values` must already hold count*floatsPerElement entries.
void AppendFloatSegment(std::vector<unsigned char>& bytes, std::int32_t count,
                        const std::vector<float>& values) {
    AppendInt32Bits(bytes, count);
    for (float value : values) AppendFloat(bytes, value);
}
// Segment 7's shape: `indexBits` holds triangleCount*3 raw int32 index values.
void AppendIndexSegment(std::vector<unsigned char>& bytes, std::int32_t triangleCount,
                        const std::vector<std::int32_t>& indexBits) {
    AppendInt32Bits(bytes, triangleCount);
    for (std::int32_t bits : indexBits) AppendInt32Bits(bytes, bits);
}

// One known triangle: 3 vertices, every non-vertex/index segment populated with distinct filler
// values (so a bug that misreads a segment boundary shows up as corrupted vertices/indices further
// along, not a silent pass). `bIncludeBindposes` appends segment 8 (skinned case).
std::vector<unsigned char> BuildValidTriangleBuffer(bool bIncludeBindposes) {
    std::vector<unsigned char> bytes;
    const char* name = "TestTriangle";
    bytes.insert(bytes.end(), name, name + std::strlen(name));
    bytes.push_back(0);   // NUL terminator

    AppendFloatSegment(bytes, 3, { 0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f });   // vertices
    AppendFloatSegment(bytes, 3, std::vector<float>(9, 9.0f));    // normals
    AppendFloatSegment(bytes, 3, std::vector<float>(12, 8.0f));   // tangents (N=4)
    AppendFloatSegment(bytes, 3, std::vector<float>(6, 7.0f));    // uv1
    AppendFloatSegment(bytes, 3, std::vector<float>(6, 6.0f));    // uv2
    AppendFloatSegment(bytes, 3, std::vector<float>(6, 5.0f));    // uv3
    AppendFloatSegment(bytes, 3, std::vector<float>(12, 4.0f));   // colors (N=4)
    AppendIndexSegment(bytes, 1, { 0, 1, 2 });                    // indices: one triangle

    if (bIncludeBindposes)
        AppendFloatSegment(bytes, 1, std::vector<float>(16, 3.0f));   // bindposes (N=16)
    return bytes;
}
} // namespace

int main() {
    // Non-skinned case: every field reproduced exactly, bWasSkinned == false.
    {
        const std::vector<unsigned char> bytes = BuildValidTriangleBuffer(false);
        const SanmodelReadResult result = ReadSanmodelMesh(bytes.data(), bytes.size());
        Check(result.bSucceeded, "valid-triangle", "expected bSucceeded == true");
        const std::vector<float> expectedPositions = { 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f };
        Check(result.mesh.positions == expectedPositions, "valid-triangle", "positions must match exactly");
        const std::vector<std::uint32_t> expectedIndices = { 0u, 1u, 2u };
        Check(result.mesh.triangleIndices == expectedIndices, "valid-triangle", "indices must match exactly");
        Check(!result.mesh.bWasSkinned, "valid-triangle", "expected bWasSkinned == false (no bindposes segment)");
    }

    // Skinned case: identical geometry, segment 8 present -> bWasSkinned == true, geometry unaffected.
    {
        const std::vector<unsigned char> bytes = BuildValidTriangleBuffer(true);
        const SanmodelReadResult result = ReadSanmodelMesh(bytes.data(), bytes.size());
        Check(result.bSucceeded, "skinned-triangle", "expected bSucceeded == true");
        Check(result.mesh.bWasSkinned, "skinned-triangle", "expected bWasSkinned == true (bindposes segment present)");
        Check(result.mesh.positions.size() == 9, "skinned-triangle", "positions count unaffected by bindposes segment");
    }

    // Truncated buffer: cut off mid-way through the indices segment. Must fail cleanly, never crash.
    {
        std::vector<unsigned char> bytes = BuildValidTriangleBuffer(false);
        bytes.resize(bytes.size() - 6);   // chop off the last index value and a half
        const SanmodelReadResult result = ReadSanmodelMesh(bytes.data(), bytes.size());
        Check(!result.bSucceeded, "truncated-buffer", "expected bSucceeded == false");
        Check(!result.errorMessage.empty(), "truncated-buffer", "expected a diagnostic message");
    }

    // A vertex count that would read past the end of the buffer entirely (a hostile/corrupt file).
    {
        std::vector<unsigned char> bytes;
        const char* name = "Bad";
        bytes.insert(bytes.end(), name, name + std::strlen(name));
        bytes.push_back(0);
        AppendInt32Bits(bytes, 1'000'000);   // claims a million vertices; buffer has none of them
        const SanmodelReadResult result = ReadSanmodelMesh(bytes.data(), bytes.size());
        Check(!result.bSucceeded, "oversize-count", "expected bSucceeded == false");
        Check(!result.errorMessage.empty(), "oversize-count", "expected a diagnostic message");
    }

    // Empty input and a size over the cap both degrade cleanly.
    {
        const SanmodelReadResult empty = ReadSanmodelMesh(nullptr, 0);
        Check(!empty.bSucceeded, "empty-input", "expected bSucceeded == false");

        std::vector<unsigned char> bytes = BuildValidTriangleBuffer(false);
        SanmodelReadLimits tinyLimit;
        tinyLimit.maximumSourceByteSize = 1;
        const SanmodelReadResult oversizeFile = ReadSanmodelMesh(bytes.data(), bytes.size(), tinyLimit);
        Check(!oversizeFile.bSucceeded, "over-cap-input", "expected bSucceeded == false");
    }

    if (gFailureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", gFailureCount);
    return 1;
}
