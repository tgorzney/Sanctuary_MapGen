// EntityIdBuffer_DATA_Test.cpp — acceptance test for EntityIdBuffer_DATA (M4-1).
//   g++ -O2 -std=c++17 -fsanitize=address,undefined EntityIdBuffer_DATA_Test.cpp -o t && ./t
#include "EntityIdBuffer_DATA.h"
#include <cstdio>

using namespace SanmapGen::Data;

static int failures = 0;
static void check(bool ok, const char* label) { if (!ok) { std::printf("FAIL: %s\n", label); ++failures; } }

// True when every cell of the buffer reads the empty sentinel.
static bool AllCellsEmpty(const EntityIdBuffer& buffer) {
    for (int y = 0; y < buffer.Height(); ++y)
        for (int x = 0; x < buffer.Width(); ++x)
            if (buffer.Get(x, y) != EntityIdBuffer::emptySentinel) return false;
    return true;
}

int main() {
    check(EntityIdBuffer::emptySentinel == 0xFFFFFFFFu, "sentinel value");

    EntityIdBuffer buffer(4, 3);
    check(buffer.Width() == 4 && buffer.Height() == 3, "dimensions");
    check(buffer.CellCount() == 12, "cell count");
    check(AllCellsEmpty(buffer), "resize starts empty");

    buffer.Set(2, 1, 7u);
    check(buffer.Get(2, 1) == 7u, "set/get");
    // Row-major contiguity: (x,y) lives at index y*width + x.
    check(buffer.Data()[1 * 4 + 2] == 7u, "row-major index");
    check(buffer.Get(1, 2) == EntityIdBuffer::emptySentinel, "x/y not transposed");

    // Corners exercise the full index range.
    buffer.Set(0, 0, 0u);
    buffer.Set(3, 2, 123456u);
    check(buffer.Get(0, 0) == 0u, "id zero is a valid id");
    check(buffer.Data()[0] == 0u && buffer.Data()[11] == 123456u, "corner indices");
    check(buffer.Get(3, 0) == EntityIdBuffer::emptySentinel, "untouched cell empty");

    buffer.Clear();
    check(AllCellsEmpty(buffer), "clear fills sentinel everywhere");
    check(buffer.Width() == 4 && buffer.CellCount() == 12, "clear keeps shape");

    // Resize reshapes and re-empties (stale ids never survive a preview resize).
    buffer.Set(1, 1, 55u);
    buffer.Resize(8, 5);
    check(buffer.Width() == 8 && buffer.Height() == 5 && buffer.CellCount() == 40, "resize");
    check(AllCellsEmpty(buffer), "resize re-empties");
    buffer.Set(7, 4, 9u);
    check(buffer.Data()[4 * 8 + 7] == 9u, "row-major index after resize");

    // Degenerate shapes stay safe: no allocation, no reads.
    EntityIdBuffer emptyBuffer;
    check(emptyBuffer.IsEmpty() && emptyBuffer.Width() == 0 && emptyBuffer.Height() == 0, "default empty");
    emptyBuffer.Resize(-4, 3);
    check(emptyBuffer.IsEmpty() && emptyBuffer.Width() == 0, "negative width clamps");
    emptyBuffer.Resize(0, 0);
    check(emptyBuffer.IsEmpty() && emptyBuffer.CellCount() == 0, "zero size");

    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}
