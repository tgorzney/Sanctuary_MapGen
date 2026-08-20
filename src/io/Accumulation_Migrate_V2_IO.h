// Accumulation_Migrate_V2_IO.h — reserves the empty V3 top-level `Accumulation` section
// (IO_MIGRATION_SPEC.md §1/§7, SANMAP_FORMAT_SPEC Correction 6). No legacy fields exist for this
// domain — this migration's entire job is to guarantee the key exists after a V2 document walks
// forward, never to relocate anything.
#pragma once
#include <nlohmann/json.hpp>

namespace SanmapGen {
namespace Io {

void Accumulation_Migrate_V2(nlohmann::json& document);

} // namespace Io
} // namespace SanmapGen
