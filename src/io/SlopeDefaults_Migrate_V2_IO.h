// SlopeDefaults_Migrate_V2_IO.h — carries the V2 `mapGeneratorData.Stratums[i]` shape's slope-gate
// fields forward into the V3 top-level `SlopeDefaults` object (IO_MIGRATION_SPEC.md §2's own
// load-bearing cross-domain example, SANMAP_FORMAT_SPEC Correction 5). Not a relocation: SYNTHESIZES
// ONE global record from every stratum's own values (mode for the 3 booleans, mean for the 5
// floats — the .cpp carries the exact rule) and sets a `bSlopeUseGlobal`-equivalent
// `StratumGenerationSettings[i]["SlopeUseGlobal"]` flag per stratum.
//
// NOT a `bIndependentlySelectable` candidate: it shares its read source
// (`mapGeneratorData.Stratums[]`) AND its write destination (`StratumGenerationSettings[i]`) with
// the sibling `StratumGenerationSettings_Migrate_V2` (STEP40D) — this migration MUST run first in
// the step (see that sibling's own header for the additive-write discipline this implies).
#pragma once
#include <nlohmann/json.hpp>

namespace SanmapGen {
namespace Io {

void SlopeDefaults_Migrate_V2(nlohmann::json& document);

} // namespace Io
} // namespace SanmapGen
