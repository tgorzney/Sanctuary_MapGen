// SpatialGrid_DATA_Test.cpp — acceptance test for M4-0b (SpatialGrid_DATA).
//   g++ -O2 -std=c++17 -fsanitize=address,undefined SpatialGrid_DATA_Test.cpp -o t && ./t
#include "SpatialGrid_DATA.h"
#include "PlacementInstances_DATA.h"
#include <cstdio>
#include <cstdint>
#include <vector>

using namespace SanmapGen::Data;

static int failures = 0;
static void check(bool ok, const char* label) { if (!ok) { std::printf("FAIL: %s\n", label); ++failures; } }

int main() {
    // ---- Configuration is carried, not hardcoded (ARCH §8.3 / Constitution §8).
    SpatialGrid grid;
    check(grid.ChunkResolution() == 32 && grid.CellCount() == 1024, "default 32x32 chunks");
    grid.Configure(1024.0f, 8);
    check(grid.ChunkResolution() == 8 && grid.CellCount() == 64 && grid.MapWorldSize() == 1024.0f,
          "configure stores resolution and world size");

    // ---- CellIndexAt clamps instead of indexing out of bounds.
    check(grid.CellIndexAt(10.0f, 10.0f) == 0, "cell mapping origin");
    check(grid.CellIndexAt(1000.0f, 10.0f) == 7, "cell mapping +x");
    check(grid.CellIndexAt(10.0f, 1000.0f) == 56, "cell mapping +y");
    check(grid.CellIndexAt(-99999.0f, -99999.0f) == 0, "clamp below range");
    check(grid.CellIndexAt(99999.0f, 99999.0f) == 63, "clamp above range");
    check(grid.CellIndexAt(1024.0f, 1024.0f) == 63, "clamp at exact upper bound");
    // ---- Empty grid is valid and answers every query with an empty range.
    check(grid.IsEmpty() && grid.EntryCount() == 0, "starts empty");
    for (int cell = 0; cell < grid.CellCount(); ++cell)
        if (grid.BucketBegin(cell) != 0 || grid.BucketEnd(cell) != 0) { check(false, "empty range"); break; }
    check(grid.BucketBegin(-1) == 0 && grid.BucketEnd(-1) == 0, "out-of-range cell empty");
    check(grid.BucketBegin(999999) == 0 && grid.BucketEnd(999999) == 0, "huge cell index empty");

    // ---- A known deterministic scatter, plus out-of-range positions that must clamp.
    const std::int32_t instanceCount = 4096;
    std::vector<float> positionX(instanceCount), positionY(instanceCount);
    std::uint32_t randomState = 12345u;
    for (std::int32_t entry = 0; entry < instanceCount; ++entry) {
        randomState = randomState * 1664525u + 1013904223u;
        positionX[entry] = static_cast<float>(randomState >> 8) * (1024.0f / 16777216.0f);
        randomState = randomState * 1664525u + 1013904223u;
        positionY[entry] = static_cast<float>(randomState >> 8) * (1024.0f / 16777216.0f);
    }
    positionX[0] = -5000.0f; positionY[0] = -5000.0f;       // must land in cell 0
    positionX[1] =  5000.0f; positionY[1] =  5000.0f;       // must land in cell 63

    grid.Build(positionX.data(), positionY.data(), instanceCount);
    check(grid.EntryCount() == instanceCount, "every instance indexed");
    // Prefix sum monotonic, bucketStart[cellCount] == count, ranges contiguous and complete.
    std::int32_t previousEnd = 0;
    std::vector<int> timesSeen(static_cast<std::size_t>(instanceCount), 0);
    for (int cell = 0; cell < grid.CellCount(); ++cell) {
        const std::int32_t begin = grid.BucketBegin(cell), end = grid.BucketEnd(cell);
        if (begin != previousEnd || end < begin) { check(false, "prefix sum monotonic"); break; }
        previousEnd = end;
        for (std::int32_t position = begin; position < end; ++position) {
            const std::int32_t instance = grid.InstanceIndexAt(position);
            // Each instance sits in the chunk its own position maps to.
            if (grid.CellIndexAt(positionX[instance], positionY[instance]) != cell)
                { check(false, "instance in wrong chunk"); break; }
            ++timesSeen[static_cast<std::size_t>(instance)];
        }
    }
    check(previousEnd == instanceCount, "bucketStart[cellCount] == count");
    for (std::int32_t entry = 0; entry < instanceCount; ++entry)
        if (timesSeen[static_cast<std::size_t>(entry)] != 1) { check(false, "each instance exactly once"); break; }
    check(grid.BucketEnd(0) > grid.BucketBegin(0) && grid.BucketEnd(63) > grid.BucketBegin(63),
          "clamped out-of-range instances landed in the edge chunks");
    // ---- Rebuilding replaces, it does not accumulate.
    grid.Build(positionX.data(), positionY.data(), instanceCount);
    grid.Build(positionX.data(), positionY.data(), instanceCount);
    check(grid.EntryCount() == instanceCount, "rebuild replaces");
    // ---- count == 0, a null column, and Clear() are all safe.
    grid.Build(positionX.data(), positionY.data(), 0);
    check(grid.IsEmpty() && grid.BucketEnd(63) == 0, "count == 0 build safe");
    grid.Build(nullptr, nullptr, instanceCount);
    check(grid.IsEmpty(), "null columns safe");
    grid.Build(positionX.data(), positionY.data(), instanceCount);
    grid.Clear();
    check(grid.IsEmpty() && grid.BucketBegin(10) == 0 && grid.BucketEnd(10) == 0, "clear empties");
    // ---- Entries are indices into the resolved SoA, not keys (ARCH §8.3).
    PlacementInstances instances;
    for (int entry = 0; entry < 4; ++entry) {
        PlacementInstance instance;
        instance.positionX = 100.0f + 300.0f * entry;   // 100, 400, 700, 1000 -> chunks 0,3,5,7
        instance.positionZ = 100.0f;                    // positionY is height; X/Z are horizontal
        instance.ruleIndex = entry;
        instances.Append(instance);
    }
    SpatialGrid instanceGrid;
    instanceGrid.Configure(1024.0f, 8);
    instanceGrid.Build(instances.positionX.data(), instances.positionZ.data(),
                       static_cast<std::int32_t>(instances.Count()));
    const int chunkOfThird = instanceGrid.CellIndexAt(700.0f, 100.0f);
    check(instanceGrid.BucketEnd(chunkOfThird) - instanceGrid.BucketBegin(chunkOfThird) == 1, "one per chunk");
    const std::int32_t found = instanceGrid.InstanceIndexAt(instanceGrid.BucketBegin(chunkOfThird));
    check(instances.ruleIndex[static_cast<std::size_t>(found)] == 2, "index resolves into the SoA");

    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}
