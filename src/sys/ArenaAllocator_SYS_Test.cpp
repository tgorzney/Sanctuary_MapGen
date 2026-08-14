// ArenaAllocator_SYS_Test.cpp — acceptance test for ArenaAllocator_SYS (M0-6).
//   g++ -O2 -std=c++17 ArenaAllocator_SYS_Test.cpp -o t && ./t
#include "ArenaAllocator_SYS.h"
#include <cstdio>
#include <cstdint>

using namespace SanmapGen::Sys;

static int failures = 0;
static void check(bool ok, const char* label) { if (!ok) { std::printf("FAIL: %s\n", label); ++failures; } }

int main() {
    ArenaAllocator arena(1024);
    check(arena.CapacityBytes() == 1024, "capacity");
    check(arena.BytesUsed() == 0, "starts empty");

    // Alignment: request 32-byte alignment, pointer must be 32-aligned.
    void* a = arena.Allocate(10, 32);
    check(a != nullptr, "alloc a");
    check((reinterpret_cast<std::uintptr_t>(a) & 31u) == 0, "a 32-aligned");

    void* b = arena.Allocate(10, 32);
    check(b != nullptr, "alloc b");
    check((reinterpret_cast<std::uintptr_t>(b) & 31u) == 0, "b 32-aligned");
    // Non-overlap: b must be at least 10 bytes past a.
    check(reinterpret_cast<unsigned char*>(b) >= reinterpret_cast<unsigned char*>(a) + 10, "b after a");

    // Typed array allocation.
    float* floats = arena.AllocateArray<float>(16);
    check(floats != nullptr, "alloc float[16]");
    check((reinterpret_cast<std::uintptr_t>(floats) & (alignof(float) - 1)) == 0, "floats aligned");
    for (int i = 0; i < 16; ++i) floats[i] = float(i);   // must be writable, no crash
    check(floats[15] == 15.0f, "floats writable");

    // Out-of-space returns nullptr and does not corrupt the arena.
    std::size_t usedBefore = arena.BytesUsed();
    void* tooBig = arena.Allocate(2048, 16);
    check(tooBig == nullptr, "over-capacity -> nullptr");
    check(arena.BytesUsed() == usedBefore, "failed alloc left offset unchanged");

    // Reset reclaims everything.
    arena.Reset();
    check(arena.BytesUsed() == 0, "reset clears");
    void* afterReset = arena.Allocate(64, 16);
    check(afterReset == a || afterReset != nullptr, "alloc works after reset");

    // Exact-fill then one-too-many.
    ArenaAllocator small(128);
    check(small.Allocate(128, 1) != nullptr, "exact fill 128");
    check(small.Allocate(1, 1) == nullptr, "one past full -> nullptr");

    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}
