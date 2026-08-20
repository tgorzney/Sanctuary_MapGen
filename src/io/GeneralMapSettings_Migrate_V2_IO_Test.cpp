// GeneralMapSettings_Migrate_V2_IO_Test.cpp — acceptance test (IO_MIGRATION_SPEC.md §1): one
// hand-built OLD-shape (V2) fixture, asserting the exact NEW (V3) shape after calling
// `GeneralMapSettings_Migrate_V2` alone. Not a round-trip test, not a runner test.
#include "GeneralMapSettings_Migrate_V2_IO.h"
#include <cstdio>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL: %s\n", label); ++failureCount; }
}

// The 4 confirmed legacy fields relocate out of mapGeneratorData into the new top-level
// GeneralMapSettings object, under the same key names, and are removed from their old location.
void CheckRelocatesTheFourFields() {
    nlohmann::json document;
    document["mapGeneratorData"]["Seed"]                   = 12345;
    document["mapGeneratorData"]["ScaleFeaturesToMapSize"] = true;
    document["mapGeneratorData"]["TerrainMinHeight"]       = -50.0f;
    document["mapGeneratorData"]["WorldUnitsPerCell"]      = 2.5f;
    // A neighboring legacy field this migration does NOT own must be left untouched.
    document["mapGeneratorData"]["MapSize"] = 512;

    Io::GeneralMapSettings_Migrate_V2(document);

    Check(document["GeneralMapSettings"]["Seed"] == 12345,
          "Seed relocates to the top-level GeneralMapSettings object");
    Check(document["GeneralMapSettings"]["ScaleFeaturesToMapSize"] == true,
          "ScaleFeaturesToMapSize relocates to the top-level GeneralMapSettings object");
    Check(document["GeneralMapSettings"]["TerrainMinHeight"] == -50.0f,
          "TerrainMinHeight relocates to the top-level GeneralMapSettings object");
    Check(document["GeneralMapSettings"]["WorldUnitsPerCell"] == 2.5f,
          "WorldUnitsPerCell relocates to the top-level GeneralMapSettings object");
    Check(document["GeneralMapSettings"].size() == 4,
          "GeneralMapSettings carries exactly the 4 confirmed fields — nothing more, nothing less");

    Check(!document["mapGeneratorData"].contains("Seed")
          && !document["mapGeneratorData"].contains("ScaleFeaturesToMapSize")
          && !document["mapGeneratorData"].contains("TerrainMinHeight")
          && !document["mapGeneratorData"].contains("WorldUnitsPerCell"),
          "the 4 relocated fields are removed from the legacy mapGeneratorData blob");
    Check(document["mapGeneratorData"]["MapSize"] == 512,
          "a legacy field this migration does not own is left exactly where it started");

    // GlobalGravity has no legacy source and must never be synthesized by this migration.
    Check(!document["GeneralMapSettings"].contains("GlobalGravity"),
          "GlobalGravity is never added — it has zero legacy source (its PARAMS default applies)");
}

// A document with no mapGeneratorData at all is a total, safe no-op — no GeneralMapSettings
// section is fabricated out of nothing.
void CheckNoLegacyBlobIsNoOp() {
    nlohmann::json document = { {"someOtherField", 1} };
    Io::GeneralMapSettings_Migrate_V2(document);
    Check(!document.contains("GeneralMapSettings"),
          "a document with no mapGeneratorData at all produces no GeneralMapSettings section");
    Check(document["someOtherField"] == 1, "the rest of the document is untouched");

    // Idempotency: calling it again changes nothing further.
    nlohmann::json before = document;
    Io::GeneralMapSettings_Migrate_V2(document);
    Check(document == before, "a second call on an already-migrated (or field-less) document is a safe no-op");
}

} // namespace

int main() {
    CheckRelocatesTheFourFields();
    CheckNoLegacyBlobIsNoOp();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
