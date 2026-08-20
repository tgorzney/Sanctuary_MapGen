// Flow_Migrate_V2_IO_Test.cpp — acceptance test for Flow_Migrate_V2 (STEP40C). One migration, one
// hand-built OLD-shape fixture per case, exact NEW-shape assertions (IO_MIGRATION_SPEC.md §1).
#include "Flow_Migrate_V2_IO.h"
#include <cstdio>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL: %s\n", label); ++failureCount; }
}

// Acceptance test 1: a real 4-element FlowMapColor array relocates and converts correctly.
void CheckRelocatesAndConvertsColor() {
    nlohmann::json document = {
        {"mapGeneratorData", {
            {"FlowMapColor", {0.2f, 0.4f, 0.6f, 1.0f}}
        }}
    };
    Io::Flow_Migrate_V2(document);

    Check(!document["mapGeneratorData"].contains("FlowMapColor"),
          "Flow_Migrate_V2 removes FlowMapColor from mapGeneratorData");
    Check(document.contains("Flow") && document["Flow"]["FlowMapColor"].is_object(),
          "Flow_Migrate_V2 places FlowMapColor as an object under Flow");
    Check(document["Flow"]["FlowMapColor"]["r"] == 0.2f &&
          document["Flow"]["FlowMapColor"]["g"] == 0.4f &&
          document["Flow"]["FlowMapColor"]["b"] == 0.6f &&
          document["Flow"]["FlowMapColor"]["a"] == 1.0f,
          "Flow_Migrate_V2 converts the array to the correct {r,g,b,a} object");
}

// Acceptance test 4: a short (< 4 element) color array pads per ConvertColorArrayToRgbaObject's
// documented default-fill behavior (0/0/0 for missing r/g/b, 1 for a missing entirely).
void CheckPadsShortColorArray() {
    nlohmann::json document = {
        {"mapGeneratorData", {
            {"FlowMapColor", {0.5f, 0.75f}}
        }}
    };
    Io::Flow_Migrate_V2(document);

    Check(document["Flow"]["FlowMapColor"]["r"] == 0.5f &&
          document["Flow"]["FlowMapColor"]["g"] == 0.75f &&
          document["Flow"]["FlowMapColor"]["b"] == 0.0f &&
          document["Flow"]["FlowMapColor"]["a"] == 1.0f,
          "Flow_Migrate_V2 pads a short FlowMapColor array with the 0/0/0/1 defaults");
}

// Acceptance test 3: a document missing the relevant legacy field is a safe no-op — no Flow key,
// no incidental mapGeneratorData key, appears where none existed before.
void CheckNoOpWhenFieldMissing() {
    nlohmann::json emptyDocument = nlohmann::json::object();
    nlohmann::json before        = emptyDocument;
    Io::Flow_Migrate_V2(emptyDocument);
    Check(emptyDocument == before,
          "Flow_Migrate_V2 is a no-op on a document with no mapGeneratorData at all");

    nlohmann::json documentWithoutColor = {
        {"mapGeneratorData", { {"SomeOtherField", 1} }}
    };
    nlohmann::json beforeWithoutColor = documentWithoutColor;
    Io::Flow_Migrate_V2(documentWithoutColor);
    Check(documentWithoutColor == beforeWithoutColor,
          "Flow_Migrate_V2 is a no-op on a document whose mapGeneratorData lacks FlowMapColor");
}

} // namespace

int main() {
    CheckRelocatesAndConvertsColor();
    CheckPadsShortColorArray();
    CheckNoOpWhenFieldMissing();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
