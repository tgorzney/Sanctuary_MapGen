// GlobalMarkerSettings_Migrate_V2_IO_Test.cpp — acceptance test for GlobalMarkerSettings_Migrate_V2
// (STEP40C). One migration, one hand-built OLD-shape fixture per case, exact NEW-shape assertions
// (IO_MIGRATION_SPEC.md §1).
#include "GlobalMarkerSettings_Migrate_V2_IO.h"
#include <cstdio>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL: %s\n", label); ++failureCount; }
}

// Acceptance test 2: all 9 legacy fields populated (colors as 4-element arrays) relocate into
// GlobalMarkerSettings, with the 3 color fields correctly converted to {r,g,b,a} objects.
void CheckRelocatesAllNineFields() {
    nlohmann::json document = {
        {"mapGeneratorData", {
            {"GlobalIconAlloy",   "IconAlloy.dds"},
            {"GlobalIconPlasma",  "IconPlasma.dds"},
            {"GlobalIconSpawn",   "IconSpawn.dds"},
            {"MarkerColorAlloy",  {1.0f, 0.0f, 0.0f, 1.0f}},
            {"MarkerColorPlasma", {0.0f, 1.0f, 0.0f, 1.0f}},
            {"MarkerColorSpawn",  {0.0f, 0.0f, 1.0f, 1.0f}},
            {"MarkerScaleAlloy",  1.5f},
            {"MarkerScalePlasma", 2.0f},
            {"MarkerScaleSpawn",  0.75f},
        }}
    };
    Io::GlobalMarkerSettings_Migrate_V2(document);

    const nlohmann::json& generatorData = document["mapGeneratorData"];
    Check(!generatorData.contains("GlobalIconAlloy") && !generatorData.contains("MarkerColorAlloy") &&
          !generatorData.contains("MarkerScaleAlloy"),
          "GlobalMarkerSettings_Migrate_V2 removes the relocated fields from mapGeneratorData");

    Check(document.contains("GlobalMarkerSettings"), "GlobalMarkerSettings_Migrate_V2 creates the GlobalMarkerSettings section");
    const nlohmann::json& settings = document["GlobalMarkerSettings"];

    Check(settings["GlobalIconAlloy"] == "IconAlloy.dds" && settings["GlobalIconPlasma"] == "IconPlasma.dds" &&
          settings["GlobalIconSpawn"] == "IconSpawn.dds",
          "GlobalMarkerSettings_Migrate_V2 relocates the 3 icon strings unchanged");

    Check(settings["MarkerScaleAlloy"] == 1.5f && settings["MarkerScalePlasma"] == 2.0f &&
          settings["MarkerScaleSpawn"] == 0.75f,
          "GlobalMarkerSettings_Migrate_V2 relocates the 3 scale scalars unchanged");

    Check(settings["MarkerColorAlloy"].is_object() && settings["MarkerColorAlloy"]["r"] == 1.0f &&
          settings["MarkerColorAlloy"]["g"] == 0.0f && settings["MarkerColorAlloy"]["b"] == 0.0f &&
          settings["MarkerColorAlloy"]["a"] == 1.0f,
          "GlobalMarkerSettings_Migrate_V2 converts MarkerColorAlloy to the correct {r,g,b,a} object");
    Check(settings["MarkerColorPlasma"].is_object() && settings["MarkerColorPlasma"]["g"] == 1.0f,
          "GlobalMarkerSettings_Migrate_V2 converts MarkerColorPlasma to an object");
    Check(settings["MarkerColorSpawn"].is_object() && settings["MarkerColorSpawn"]["b"] == 1.0f,
          "GlobalMarkerSettings_Migrate_V2 converts MarkerColorSpawn to an object");
}

// Acceptance test 4: a short (< 4 element) color array pads per ConvertColorArrayToRgbaObject's
// documented default-fill behavior — exercised here on a different field/omission than the Flow
// migration's own test (missing alpha entirely, rather than missing blue and alpha).
void CheckPadsShortColorArray() {
    nlohmann::json document = {
        {"mapGeneratorData", {
            {"MarkerColorSpawn", {0.1f, 0.2f, 0.3f}}
        }}
    };
    Io::GlobalMarkerSettings_Migrate_V2(document);

    const nlohmann::json& color = document["GlobalMarkerSettings"]["MarkerColorSpawn"];
    Check(color["r"] == 0.1f && color["g"] == 0.2f && color["b"] == 0.3f && color["a"] == 1.0f,
          "GlobalMarkerSettings_Migrate_V2 pads a short MarkerColorSpawn array with the missing-alpha default of 1");
}

// Acceptance test 3: a document missing the relevant legacy fields is a safe no-op — no
// GlobalMarkerSettings key, no incidental mapGeneratorData key, appears where none existed before.
void CheckNoOpWhenFieldsMissing() {
    nlohmann::json emptyDocument = nlohmann::json::object();
    nlohmann::json before        = emptyDocument;
    Io::GlobalMarkerSettings_Migrate_V2(emptyDocument);
    Check(emptyDocument == before,
          "GlobalMarkerSettings_Migrate_V2 is a no-op on a document with no mapGeneratorData at all");

    nlohmann::json documentWithoutFields = {
        {"mapGeneratorData", { {"SomeOtherField", 1} }}
    };
    nlohmann::json beforeWithoutFields = documentWithoutFields;
    Io::GlobalMarkerSettings_Migrate_V2(documentWithoutFields);
    Check(documentWithoutFields == beforeWithoutFields,
          "GlobalMarkerSettings_Migrate_V2 is a no-op on a document whose mapGeneratorData lacks all 9 fields");
}

} // namespace

int main() {
    CheckRelocatesAllNineFields();
    CheckPadsShortColorArray();
    CheckNoOpWhenFieldsMissing();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
