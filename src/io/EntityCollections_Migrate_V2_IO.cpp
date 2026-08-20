// EntityCollections_Migrate_V2_IO.cpp — see the header for the full contract and the
// top-level-vs-nested `Aliases` finding.
#include "EntityCollections_Migrate_V2_IO.h"
#include "JsonPrimitives_IO.h"
#include <string>

namespace SanmapGen {
namespace Io {
namespace {

// Sub-task 1: legacy["Armies"][key].Color -> document["armies"][key].armyColor. `legacy["Armies"]`
// (the mapGeneratorData dump) and `document["armies"]` (the format-native dictionary
// MapImporter_Armies_IO.cpp/MapExporter_Armies_IO.cpp already round-trip armyColor/alias through)
// are DIFFERENT collections despite the near-identical name — work-order STEP40E's own warning.
void MigrateArmyColors(nlohmann::json& legacy, nlohmann::json& document) {
    if (!legacy.contains("Armies") || !legacy["Armies"].is_object()) return;
    for (auto& [armyKey, legacyArmyJson] : legacy["Armies"].items()) {
        if (!legacyArmyJson.is_object()) continue;
        ConvertColorArrayToRgbaObject(legacyArmyJson, "Color");
        MoveKey(legacyArmyJson, "Color", document["armies"][armyKey], "armyColor");
    }
}

// Sub-task 2 (the genuinely bespoke part): for each (aliasName, markerTransformName) pair in the
// legacy Aliases dict, search every marker-type group's transforms for a name match and set that
// transform's `alias`. `markers[groupName].transforms[transformName]` (confirmed against
// MapImporter_Markers_IO.cpp/MapExporter_Markers_IO.cpp) is the only JSON shape in the format
// holding a named, alias-able marker transform — `chains` markers carry no `alias` field
// (SANMAP_FORMAT_SPEC's Entity-collections note), so `markers` is the whole search scope, but every
// group inside it must be searched (a marker type is its own group, not a separate top-level key).
// A name matching nowhere is a safe no-op for that one entry — no MapImportResult-style logging
// channel reaches this function (MigrationFunction is `void(nlohmann::json&)`), so this silently
// skips per the work-order's own documented fallback.
void MigrateAliasesIntoMarkerTransforms(nlohmann::json& legacy, nlohmann::json& document) {
    if (!legacy.contains("Aliases") || !legacy["Aliases"].is_object()) return;
    if (!document.contains("markers") || !document["markers"].is_object()) return;
    nlohmann::json& markers = document["markers"];

    for (const auto& [aliasName, markerTransformNameJson] : legacy["Aliases"].items()) {
        if (!markerTransformNameJson.is_string()) continue;
        const std::string markerTransformName = markerTransformNameJson.get<std::string>();

        for (auto& [groupName, groupJson] : markers.items()) {
            if (!groupJson.is_object()) continue;
            if (!groupJson.contains("transforms") || !groupJson["transforms"].is_object()) continue;
            nlohmann::json& transforms = groupJson["transforms"];
            if (transforms.contains(markerTransformName))
                transforms[markerTransformName]["alias"] = aliasName;
        }
    }
}

} // namespace

void EntityCollections_Migrate_V2(nlohmann::json& document) {
    if (!document.contains("mapGeneratorData") || !document["mapGeneratorData"].is_object()) return;
    nlohmann::json& legacy = document["mapGeneratorData"];

    MigrateArmyColors(legacy, document);
    MigrateAliasesIntoMarkerTransforms(legacy, document);
}

} // namespace Io
} // namespace SanmapGen
