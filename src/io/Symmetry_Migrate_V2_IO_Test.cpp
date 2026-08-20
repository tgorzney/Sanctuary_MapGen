// Symmetry_Migrate_V2_IO_Test.cpp — acceptance test (IO_MIGRATION_SPEC.md §1): one hand-built
// OLD-shape (V2) fixture, asserting the exact NEW (V3) shape after calling `Symmetry_Migrate_V2`
// alone. Not a round-trip test, not a runner test.
#include "Symmetry_Migrate_V2_IO.h"
#include <cstdio>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL: %s\n", label); ++failureCount; }
}

// All 9 confirmed legacy fields relocate out of mapGeneratorData into the new top-level Symmetry
// object, under the same key names, and are removed from their old location.
void CheckRelocatesTheNineFields() {
    nlohmann::json document;
    nlohmann::json& legacy = document["mapGeneratorData"];
    legacy["GlobalSymmetryMask"]        = 3;
    legacy["SnapImperfectSymmetry"]     = true;
    legacy["SymmetryDetectionTolerance"] = 0.05f;
    legacy["SymSuperpositionBlend"]     = 0.5f;
    legacy["SymmetryBlurRadius"]        = 4.0f;
    legacy["CrossFadeWidth"]            = 8.0f;
    legacy["CylinderZScale"]            = 1.25f;
    legacy["TorusMajorRadius"]          = 100.0f;
    legacy["TorusMinorRadius"]          = 25.0f;
    // A neighboring legacy field this migration does NOT own must be left untouched.
    legacy["MapSize"] = 512;

    Io::Symmetry_Migrate_V2(document);

    const nlohmann::json& symmetry = document["Symmetry"];
    Check(symmetry["GlobalSymmetryMask"] == 3, "GlobalSymmetryMask relocates");
    Check(symmetry["SnapImperfectSymmetry"] == true, "SnapImperfectSymmetry relocates");
    Check(symmetry["SymmetryDetectionTolerance"] == 0.05f, "SymmetryDetectionTolerance relocates");
    Check(symmetry["SymSuperpositionBlend"] == 0.5f, "SymSuperpositionBlend relocates");
    Check(symmetry["SymmetryBlurRadius"] == 4.0f, "SymmetryBlurRadius relocates");
    Check(symmetry["CrossFadeWidth"] == 8.0f, "CrossFadeWidth relocates");
    Check(symmetry["CylinderZScale"] == 1.25f, "CylinderZScale relocates");
    Check(symmetry["TorusMajorRadius"] == 100.0f, "TorusMajorRadius relocates");
    Check(symmetry["TorusMinorRadius"] == 25.0f, "TorusMinorRadius relocates");
    Check(symmetry.size() == 9, "Symmetry carries exactly the 9 confirmed fields — nothing more");

    Check(!legacy.contains("GlobalSymmetryMask") && !legacy.contains("SnapImperfectSymmetry")
          && !legacy.contains("SymmetryDetectionTolerance") && !legacy.contains("SymSuperpositionBlend")
          && !legacy.contains("SymmetryBlurRadius") && !legacy.contains("CrossFadeWidth")
          && !legacy.contains("CylinderZScale") && !legacy.contains("TorusMajorRadius")
          && !legacy.contains("TorusMinorRadius"),
          "all 9 relocated fields are removed from the legacy mapGeneratorData blob");
    Check(legacy["MapSize"] == 512, "a legacy field this migration does not own is left untouched");

    // SymAlgorithm/RadialSymmetryRepeatCount have no legacy source and must never be synthesized.
    Check(!symmetry.contains("SymAlgorithm"),
          "SymAlgorithm is never added — it does not exist anywhere in src/ (STEP16 ruling #1)");
    Check(!symmetry.contains("RadialSymmetryRepeatCount"),
          "RadialSymmetryRepeatCount is never added — it is genuinely new (ARCH §13)");
}

// A document with no mapGeneratorData at all is a total, safe no-op.
void CheckNoLegacyBlobIsNoOp() {
    nlohmann::json document = { {"someOtherField", 1} };
    Io::Symmetry_Migrate_V2(document);
    Check(!document.contains("Symmetry"),
          "a document with no mapGeneratorData at all produces no Symmetry section");
    Check(document["someOtherField"] == 1, "the rest of the document is untouched");

    nlohmann::json before = document;
    Io::Symmetry_Migrate_V2(document);
    Check(document == before, "a second call on an already-migrated (or field-less) document is a safe no-op");
}

} // namespace

int main() {
    CheckRelocatesTheNineFields();
    CheckNoLegacyBlobIsNoOp();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
