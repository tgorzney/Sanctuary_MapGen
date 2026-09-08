// GeneralMapSettings_Migrate_V4_IO.cpp — see the header for the full contract.
#include "GeneralMapSettings_Migrate_V4_IO.h"
#include "JsonPrimitives_IO.h"

namespace SanmapGen {
namespace Io {

void GeneralMapSettings_Migrate_V4(nlohmann::json& document) {
    if (!document.contains("GeneralMapSettings") || !document["GeneralMapSettings"].is_object()) return;
    RenameKey(document["GeneralMapSettings"], "WorldUnitsPerCell", "WorldUnitsPerGenerationCell");
}

} // namespace Io
} // namespace SanmapGen
