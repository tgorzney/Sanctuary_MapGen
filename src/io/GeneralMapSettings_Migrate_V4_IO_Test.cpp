// GeneralMapSettings_Migrate_V4_IO_Test.cpp — acceptance test (IO_MIGRATION_SPEC.md §1): one
// hand-built OLD-shape (V4) fixture, asserting the exact NEW (V5) shape after calling
// `GeneralMapSettings_Migrate_V4` alone. Not a round-trip test, not a runner test.
#include "GeneralMapSettings_Migrate_V4_IO.h"
#include <cstdio>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL: %s\n", label); ++failureCount; }
}

// WorldUnitsPerCell renames in place to WorldUnitsPerGenerationCell, same object, everything else
// in GeneralMapSettings untouched.
void CheckRenamesInPlace() {
    nlohmann::json document;
    document["GeneralMapSettings"]["Seed"]              = 12345;
    document["GeneralMapSettings"]["WorldUnitsPerCell"] = 2.5f;

    Io::GeneralMapSettings_Migrate_V4(document);

    Check(document["GeneralMapSettings"]["WorldUnitsPerGenerationCell"] == 2.5f,
          "WorldUnitsPerCell renames to WorldUnitsPerGenerationCell");
    Check(!document["GeneralMapSettings"].contains("WorldUnitsPerCell"),
          "the old key name is gone after the rename");
    Check(document["GeneralMapSettings"]["Seed"] == 12345,
          "a neighboring field this migration does not own is left exactly where it started");
}

// A document with no GeneralMapSettings object at all, or one missing the old key, is a safe no-op.
void CheckMissingKeyOrSectionIsNoOp() {
    nlohmann::json noSection = { {"someOtherField", 1} };
    Io::GeneralMapSettings_Migrate_V4(noSection);
    Check(!noSection.contains("GeneralMapSettings"),
          "a document with no GeneralMapSettings object at all produces no section");

    nlohmann::json noOldKey;
    noOldKey["GeneralMapSettings"]["Seed"] = 1;
    Io::GeneralMapSettings_Migrate_V4(noOldKey);
    Check(!noOldKey["GeneralMapSettings"].contains("WorldUnitsPerGenerationCell"),
          "an already-current (or never-legacy) document is not given a fabricated new key");

    // Idempotency: calling it again on an already-renamed document changes nothing further.
    nlohmann::json document;
    document["GeneralMapSettings"]["WorldUnitsPerCell"] = 4.0f;
    Io::GeneralMapSettings_Migrate_V4(document);
    nlohmann::json before = document;
    Io::GeneralMapSettings_Migrate_V4(document);
    Check(document == before, "a second call on an already-migrated document is a safe no-op");
}

} // namespace

int main() {
    CheckRenamesInPlace();
    CheckMissingKeyOrSectionIsNoOp();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
