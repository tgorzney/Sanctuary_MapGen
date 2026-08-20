// Flow_Migrate_V2_IO.h — V2->V3 migration (IO_MIGRATION_SPEC.md §1, §7 Correction 6): relocates
// the legacy `mapGeneratorData.FlowMapColor` field into the new `Flow` section, converting it from
// a legacy 4-element [r,g,b,a] array into this format's current {r,g,b,a} object shape.
#pragma once
#include <nlohmann/json.hpp>

namespace SanmapGen {
namespace Io {

// Moves mapGeneratorData.FlowMapColor -> Flow.FlowMapColor, converting the array to an object at
// the destination. Total and idempotent: a safe no-op when mapGeneratorData or FlowMapColor is
// absent (STEP40C acceptance test 3) — never creates the Flow key unless there is something to
// move into it.
void Flow_Migrate_V2(nlohmann::json& document);

} // namespace Io
} // namespace SanmapGen
