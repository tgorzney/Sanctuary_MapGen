// Accumulation_Migrate_V2_IO.cpp — see the header for the full contract.
#include "Accumulation_Migrate_V2_IO.h"
#include "JsonPrimitives_IO.h"

namespace SanmapGen {
namespace Io {

void Accumulation_Migrate_V2(nlohmann::json& document) {
    DefaultIfMissing(document, "Accumulation", nlohmann::json::object());
}

} // namespace Io
} // namespace SanmapGen
