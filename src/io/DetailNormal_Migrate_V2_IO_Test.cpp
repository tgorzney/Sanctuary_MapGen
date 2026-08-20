// DetailNormal_Migrate_V2_IO_Test.cpp — acceptance test (IO_MIGRATION_SPEC.md §1): one hand-built
// OLD-shape (V2) fixture, asserting the exact NEW (V3) shape after calling
// `DetailNormal_Migrate_V2` alone. Not a round-trip test, not a runner test.
#include "DetailNormal_Migrate_V2_IO.h"
#include <cstdio>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL: %s\n", label); ++failureCount; }
}

// The one confirmed legacy field relocates out of mapGeneratorData into the new top-level
// DetailNormal object, under the same key name, and is removed from its old location.
void CheckRelocatesTheOneField() {
    nlohmann::json document;
    document["mapGeneratorData"]["DetailNormalMapSize"] = 1024;
    // A neighboring legacy field this migration does NOT own must be left untouched.
    document["mapGeneratorData"]["MapSize"] = 512;

    Io::DetailNormal_Migrate_V2(document);

    Check(document["DetailNormal"]["DetailNormalMapSize"] == 1024,
          "DetailNormalMapSize relocates to the top-level DetailNormal object");
    Check(document["DetailNormal"].size() == 1,
          "DetailNormal carries exactly the 1 confirmed field — nothing more, nothing less");
    Check(!document["mapGeneratorData"].contains("DetailNormalMapSize"),
          "the relocated field is removed from the legacy mapGeneratorData blob");
    Check(document["mapGeneratorData"]["MapSize"] == 512,
          "a legacy field this migration does not own is left exactly where it started");
}

// A document with no mapGeneratorData at all is a total, safe no-op.
void CheckNoLegacyBlobIsNoOp() {
    nlohmann::json document = { {"someOtherField", 1} };
    Io::DetailNormal_Migrate_V2(document);
    Check(!document.contains("DetailNormal"),
          "a document with no mapGeneratorData at all produces no DetailNormal section");
    Check(document["someOtherField"] == 1, "the rest of the document is untouched");

    nlohmann::json before = document;
    Io::DetailNormal_Migrate_V2(document);
    Check(document == before, "a second call on an already-migrated (or field-less) document is a safe no-op");
}

} // namespace

int main() {
    CheckRelocatesTheOneField();
    CheckNoLegacyBlobIsNoOp();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
