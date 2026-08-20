// DetailNormal_Migrate_V2_IO.h — carries the V2 `mapGeneratorData` shape's DetailNormal-owned
// field forward to the V3 top-level `DetailNormal` section (IO_MIGRATION_SPEC.md §1/§7,
// SANMAP_FORMAT_SPEC Correction 8). Pure flat-key relocation, 1 field, same key name both sides:
// `DetailNormalMapSize`.
#pragma once
#include <nlohmann/json.hpp>

namespace SanmapGen {
namespace Io {

void DetailNormal_Migrate_V2(nlohmann::json& document);

} // namespace Io
} // namespace SanmapGen
