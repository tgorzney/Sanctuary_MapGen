// GeneralMapSettings_Migrate_V4_IO.h — renames `GeneralMapSettings.WorldUnitsPerCell` to
// `GeneralMapSettings.WorldUnitsPerGenerationCell` (same object, same tier — a pure key rename, not
// a relocation like `GeneralMapSettings_Migrate_V2`). The old name wrongly implied this scalar means
// something about a marker/prop/decal/unit's position; it does not (SANMAP_FORMAT_SPEC: entity
// position is always absolute world units, never scaled by this field) — it is SanGen's own
// generation-grid cell size, used only by field-layer baking/compositing (height, slope, flow,
// accumulation, stratum, MapArea rectangles). The new name says that outright.
#pragma once
#include <nlohmann/json.hpp>

namespace SanmapGen {
namespace Io {

void GeneralMapSettings_Migrate_V4(nlohmann::json& document);

} // namespace Io
} // namespace SanmapGen
