// GlobalMarkerSettings_Migrate_V2_IO.cpp — see the header for the full contract.
#include "GlobalMarkerSettings_Migrate_V2_IO.h"
#include "JsonPrimitives_IO.h"

namespace SanmapGen {
namespace Io {
namespace {

// All 9 legacy field names relocate under the same key name — no renames, only a move.
constexpr const char* kRelocatedFields[9] = {
    "GlobalIconAlloy",  "GlobalIconPlasma",  "GlobalIconSpawn",
    "MarkerColorAlloy", "MarkerColorPlasma", "MarkerColorSpawn",
    "MarkerScaleAlloy", "MarkerScalePlasma", "MarkerScaleSpawn",
};

// The 3 of the 9 above that are legacy [r,g,b,a] arrays needing the object-shape conversion.
constexpr const char* kColorFields[3] = { "MarkerColorAlloy", "MarkerColorPlasma", "MarkerColorSpawn" };

} // namespace

// bIndependentlySelectable candidate (IO_MIGRATION_SPEC.md §3): all 9 fields are this migration's
// own exclusive source keys, read or written by no sibling V2->V3 migration — safe to apply alone,
// proven by this file's own paired test (GlobalMarkerSettings_Migrate_V2_IO_Test.cpp), which calls
// it in isolation. Wiring bIndependentlySelectable = true into the manifest entry itself is
// STEP40F's job.
void GlobalMarkerSettings_Migrate_V2(nlohmann::json& document) {
    if (!document.contains("mapGeneratorData") || !document["mapGeneratorData"].is_object()) return;
    nlohmann::json& source = document["mapGeneratorData"];

    bool bAnyFieldPresent = false;
    for (const char* field : kRelocatedFields) {
        if (source.contains(field)) { bAnyFieldPresent = true; break; }
    }
    if (!bAnyFieldPresent) return;

    // Move first, convert at the destination — same reasoning as Flow_Migrate_V2.
    nlohmann::json& destination = document["GlobalMarkerSettings"];
    for (const char* field : kRelocatedFields) MoveKey(source, field, destination, field);
    for (const char* colorField : kColorFields) ConvertColorArrayToRgbaObject(destination, colorField);
}

} // namespace Io
} // namespace SanmapGen
