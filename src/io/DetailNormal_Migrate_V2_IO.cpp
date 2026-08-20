// DetailNormal_Migrate_V2_IO.cpp — see the header for the full contract.
#include "DetailNormal_Migrate_V2_IO.h"
#include "JsonPrimitives_IO.h"

namespace SanmapGen {
namespace Io {

void DetailNormal_Migrate_V2(nlohmann::json& document) {
    if (!document.contains("mapGeneratorData") || !document["mapGeneratorData"].is_object()) return;
    nlohmann::json& legacy = document["mapGeneratorData"];
    nlohmann::json& section = document["DetailNormal"];
    if (!section.is_object()) section = nlohmann::json::object();

    MoveKey(legacy, "DetailNormalMapSize", section, "DetailNormalMapSize");
}

} // namespace Io
} // namespace SanmapGen
