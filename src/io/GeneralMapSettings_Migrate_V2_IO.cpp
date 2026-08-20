// GeneralMapSettings_Migrate_V2_IO.cpp — see the header for the full contract.
#include "GeneralMapSettings_Migrate_V2_IO.h"
#include "JsonPrimitives_IO.h"

namespace SanmapGen {
namespace Io {

void GeneralMapSettings_Migrate_V2(nlohmann::json& document) {
    if (!document.contains("mapGeneratorData") || !document["mapGeneratorData"].is_object()) return;
    nlohmann::json& legacy = document["mapGeneratorData"];
    nlohmann::json& section = document["GeneralMapSettings"];
    if (!section.is_object()) section = nlohmann::json::object();

    MoveKey(legacy, "Seed",                   section, "Seed");
    MoveKey(legacy, "ScaleFeaturesToMapSize",  section, "ScaleFeaturesToMapSize");
    MoveKey(legacy, "TerrainMinHeight",        section, "TerrainMinHeight");
    MoveKey(legacy, "WorldUnitsPerCell",       section, "WorldUnitsPerCell");
}

} // namespace Io
} // namespace SanmapGen
