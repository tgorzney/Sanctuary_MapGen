// MarkersStack_Migrate_V3_IO.h — carries the V3 top-level `MarkersStack` shape (a flat array of
// `MarkerRule` objects, each carrying its own `SymmetryUseGlobal`/`SymmetryMask`/
// `RadialSymmetryRepeatCount`) forward to the V4 two-level `MarkerRuleLayer` shape STEP66's
// importer/exporter already expect (IO_MIGRATION_SPEC.md §1-7, ARCH_16_06_MigrationRouting.md
// §16.6). See MarkersStack_Migrate_V3_IO.cpp for the grouping algorithm.
#pragma once
#include <nlohmann/json.hpp>

namespace SanmapGen {
namespace Io {

void MarkersStack_Migrate_V3(nlohmann::json& document);

} // namespace Io
} // namespace SanmapGen
