// Flow_Migrate_V2_IO.cpp — see the header for the full contract.
#include "Flow_Migrate_V2_IO.h"
#include "JsonPrimitives_IO.h"

namespace SanmapGen {
namespace Io {

// bIndependentlySelectable candidate (IO_MIGRATION_SPEC.md §3): FlowMapColor is not read or
// written by any sibling V2->V3 migration, so this migration is safe to apply alone — proven by
// this file's own paired test (Flow_Migrate_V2_IO_Test.cpp), which calls it in isolation, with no
// step-level legacyKeysToDelete run alongside it. Wiring bIndependentlySelectable = true into the
// manifest entry itself is STEP40F's job.
void Flow_Migrate_V2(nlohmann::json& document) {
    if (!document.contains("mapGeneratorData") || !document["mapGeneratorData"].is_object()) return;
    nlohmann::json& source = document["mapGeneratorData"];
    if (!source.contains("FlowMapColor")) return;

    // Move first, convert at the destination: reads more naturally as "relocate the field to its
    // permanent home, then normalize the shape sitting there" — both primitives are total/no-op-
    // safe, so either order is correct (work-order STEP40C).
    nlohmann::json& flow = document["Flow"];
    MoveKey(source, "FlowMapColor", flow, "FlowMapColor");
    ConvertColorArrayToRgbaObject(flow, "FlowMapColor");
}

} // namespace Io
} // namespace SanmapGen
