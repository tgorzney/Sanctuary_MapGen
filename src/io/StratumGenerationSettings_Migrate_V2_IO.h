// StratumGenerationSettings_Migrate_V2_IO.h — relocates each V2 `mapGeneratorData.Stratums[i]`'s 8
// slope-gate fields verbatim into the V3 top-level `StratumGenerationSettings[i]` array,
// index-aligned with `stratumLayers[9]`, padded to exactly 9 entries (SANMAP_FORMAT_SPEC
// Correction 12). Soil physics (the array's other 6 fields) has no legacy source anywhere and is
// left at whatever it already held — not this migration's job. N = 0 (no `mapGeneratorData`, no
// `Stratums` array, or an empty one): no write to `StratumGenerationSettings` at all — mirrors the
// sibling `SlopeDefaults_Migrate_V2`'s short-circuit (STEP41_PostMigrationImportGaps_IO).
//
// NOT a `bIndependentlySelectable` candidate: it shares its read source
// (`mapGeneratorData.Stratums[]`) AND its write destination (`StratumGenerationSettings[i]`) with
// the sibling `SlopeDefaults_Migrate_V2` (STEP40D), which MUST run BEFORE this one in the step —
// that migration sets `StratumGenerationSettings[i]["SlopeUseGlobal"]` on the same per-index
// objects this one writes its own 8 keys onto. This migration's writes are strictly ADDITIVE per
// entry (never `document["StratumGenerationSettings"] = newArray`) so the sibling's write survives;
// see the .cpp for the exact discipline.
#pragma once
#include <nlohmann/json.hpp>

namespace SanmapGen {
namespace Io {

void StratumGenerationSettings_Migrate_V2(nlohmann::json& document);

} // namespace Io
} // namespace SanmapGen
