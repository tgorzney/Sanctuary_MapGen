// WorldFootprintSizeTable_IO_Test.cpp — acceptance test for STEP58_WorldFootprintSizeTable_IO.
// Pure unit tests, no file IO, no live sanpack -- verifies the domain-guessed default fallback,
// the SetFootprint/Resolve exact-match path, the documented last-write-wins policy, and the
// placeholder seed table's two hand-confirmed entries.
#include "WorldFootprintSizeTable_IO.h"
#include <cstdio>

using namespace SanmapGen;

static int failureCount = 0;

static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

static void CheckSize(const Io::WorldFootprintSize_IO& size, float expectedWidth,
                       float expectedDepth, const char* label) {
    Check(size.baseFootprintWidth == expectedWidth && size.baseFootprintDepth == expectedDepth,
          label);
}

static void TestUnseededUnitPrefixResolvesToUnitDefault() {
    const Io::WorldFootprintSizeTable table;
    CheckSize(table.Resolve("uca9999"), Io::kDefaultUnitFootprintSize.baseFootprintWidth,
              Io::kDefaultUnitFootprintSize.baseFootprintDepth,
              "unseeded u-prefixed id resolves to kDefaultUnitFootprintSize");
}

static void TestUnseededPropPrefixResolvesToPropDefault() {
    const Io::WorldFootprintSizeTable table;
    CheckSize(table.Resolve("epx9999"), Io::kDefaultPropFootprintSize.baseFootprintWidth,
              Io::kDefaultPropFootprintSize.baseFootprintDepth,
              "unseeded e-prefixed id resolves to kDefaultPropFootprintSize");
}

static void TestEmptyAndUnrecognizedPrefixResolveToUnknownDefault() {
    const Io::WorldFootprintSizeTable table;
    CheckSize(table.Resolve(""), Io::kDefaultUnknownFootprintSize.baseFootprintWidth,
              Io::kDefaultUnknownFootprintSize.baseFootprintDepth,
              "empty templateIdentifier resolves to kDefaultUnknownFootprintSize");
    CheckSize(table.Resolve("zzz"), Io::kDefaultUnknownFootprintSize.baseFootprintWidth,
              Io::kDefaultUnknownFootprintSize.baseFootprintDepth,
              "unrecognized first-char templateIdentifier resolves to kDefaultUnknownFootprintSize");
}

static void TestSetFootprintThenResolveReturnsExactValue() {
    Io::WorldFootprintSizeTable table;
    table.SetFootprint("uca1001", 1.2f, 1.2f);
    CheckSize(table.Resolve("uca1001"), 1.2f, 1.2f,
              "SetFootprint then Resolve returns the exact set value, not a default");
}

static void TestDuplicateSetFootprintIsLastWriteWins() {
    Io::WorldFootprintSizeTable table;
    table.SetFootprint("uca1001", 1.2f, 1.2f);
    table.SetFootprint("uca1001", 9.9f, 8.8f);
    CheckSize(table.Resolve("uca1001"), 9.9f, 8.8f,
              "duplicate SetFootprint calls for the same id are last-write-wins");
}

static void TestPlaceholderTableSeedEntries() {
    const Io::WorldFootprintSizeTable table = Io::BuildPlaceholderWorldFootprintSizeTable();
    CheckSize(table.Resolve("uca1001"), 1.2f, 1.2f,
              "placeholder table's uca1001 entry matches the hand-confirmed footprint");
    CheckSize(table.Resolve("ucl4005"), 18.4f, 18.4f,
              "placeholder table's ucl4005 entry matches the hand-confirmed footprint");
    Check(table.Count() == 2, "placeholder table contains exactly the two seeded entries");
}

int main() {
    TestUnseededUnitPrefixResolvesToUnitDefault();
    TestUnseededPropPrefixResolvesToPropDefault();
    TestEmptyAndUnrecognizedPrefixResolveToUnknownDefault();
    TestSetFootprintThenResolveReturnsExactValue();
    TestDuplicateSetFootprintIsLastWriteWins();
    TestPlaceholderTableSeedEntries();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
