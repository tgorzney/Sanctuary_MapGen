// GeneralMapSettings_Migrate_V2_IO.h — carries the V2 `mapGeneratorData` shape's
// GeneralMapSettings-owned fields forward to the V3 top-level `GeneralMapSettings` section
// (IO_MIGRATION_SPEC.md §1/§7, SANMAP_FORMAT_SPEC Correction 2). Pure flat-key relocation: `Seed`,
// `ScaleFeaturesToMapSize`, `TerrainMinHeight`, `WorldUnitsPerCell` — same key names both sides
// (STEP40B corrected field list; NOT `GlobalGravity`, which `MapExporter_GeneralMapSettings_IO.cpp`
// and `SANMAP_FORMAT_SPEC.md` Correction 2 both confirm is a genuinely new field with zero legacy
// source).
#pragma once
#include <nlohmann/json.hpp>

namespace SanmapGen {
namespace Io {

void GeneralMapSettings_Migrate_V2(nlohmann::json& document);

} // namespace Io
} // namespace SanmapGen
