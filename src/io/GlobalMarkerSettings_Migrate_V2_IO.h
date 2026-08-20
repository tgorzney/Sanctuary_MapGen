// GlobalMarkerSettings_Migrate_V2_IO.h — V2->V3 migration (IO_MIGRATION_SPEC.md §1): relocates 9
// legacy mapGeneratorData fields into the new GlobalMarkerSettings section — 3 plain string icon
// fields, 3 legacy [r,g,b,a] color arrays (converted to the current {r,g,b,a} object shape), and 3
// plain scalar scale fields.
#pragma once
#include <nlohmann/json.hpp>

namespace SanmapGen {
namespace Io {

// Moves GlobalIconAlloy/Plasma/Spawn, MarkerColorAlloy/Plasma/Spawn (array -> object converted),
// and MarkerScaleAlloy/Plasma/Spawn from mapGeneratorData into GlobalMarkerSettings. Total and
// idempotent: a safe no-op when mapGeneratorData is absent, or present but none of the 9 fields
// are — never creates the GlobalMarkerSettings key unless something actually moves into it.
void GlobalMarkerSettings_Migrate_V2(nlohmann::json& document);

} // namespace Io
} // namespace SanmapGen
