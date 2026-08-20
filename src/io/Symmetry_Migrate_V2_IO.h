// Symmetry_Migrate_V2_IO.h — carries the V2 `mapGeneratorData` shape's Symmetry-owned fields
// forward to the V3 top-level `Symmetry` section (IO_MIGRATION_SPEC.md §1/§7, SANMAP_FORMAT_SPEC
// Correction 4). Pure flat-key relocation, 9 fields, same key names both sides (STEP40B corrected
// field list): `GlobalSymmetryMask`, `SnapImperfectSymmetry`, `SymmetryDetectionTolerance`,
// `SymSuperpositionBlend`, `SymmetryBlurRadius`, `CrossFadeWidth`, `CylinderZScale`,
// `TorusMajorRadius`, `TorusMinorRadius`. NOT `SymAlgorithm` (`MapExporter_Symmetry_IO.cpp` and the
// spec both confirm it doesn't exist anywhere in `src/` — STEP16 ruling #1) and NOT
// `RadialSymmetryRepeatCount` (genuinely new, ARCH §13, defaults to 3 already) — neither has a
// legacy source.
#pragma once
#include <nlohmann/json.hpp>

namespace SanmapGen {
namespace Io {

void Symmetry_Migrate_V2(nlohmann::json& document);

} // namespace Io
} // namespace SanmapGen
