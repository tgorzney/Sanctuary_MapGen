// FootprintBakeFingerprint_IO.h — Build/Read for the FootprintBakeFingerprint wire object, plus the
// cross-type staleness compare FootprintBakeStaleness_IO.h needs. Layer: IO (legally depends on
// PARAMS, never the reverse -- Constitution §1).
// work_orders/STEP96_FootprintBakeAndStalenessCheck_IO.md §3/§5.
#pragma once
#include <nlohmann/json.hpp>
#include "../params/FootprintBakeFingerprint_PARAMS.h"

namespace SanmapGen {
namespace Io {

// AssetAtlasCache_IO.h's real, shipped shape -- only a const-ref parameter is needed here, so a
// forward declaration keeps this header light (the .cpp includes the real definition).
struct SourceFingerprint;

// A plain sub-object: {"SourcePath":..., "ByteSize":..., "ModifiedTime":..., "ContentHash":...},
// PascalCase members matching the top-level `.sanmap` convention (ARCH_01_06_SanmapKeyCasing.md).
nlohmann::ordered_json BuildFootprintBakeFingerprintJson(const Params::FootprintBakeFingerprint& fingerprint);

// Never-refuse-on-absence (IO_MIGRATION_SPEC.md): a missing/absent `key`, or a present key that is
// not an object, leaves `out` untouched -- an older .sanmap that predates this ticket still loads
// with IsValid() == false, exactly the "never baked" state.
void ReadFootprintBakeFingerprintJson(const nlohmann::json& parent, const char* key,
                                      Params::FootprintBakeFingerprint& out);

// True when the two disagree on ANY field -- reuses the design doc's fingerprint mechanism
// (path+size+mtime+contentHash, modelled on the real Io::SourceFingerprint, AssetAtlasCache_IO.h:38)
// rather than inventing a second comparison scheme. Cross-type on purpose --
// Params::FootprintBakeFingerprint (the baked snapshot) vs. Io::SourceFingerprint (the live
// ingestion's current value for the same templateIdentifier) are never compared as the same type
// (§1.1's "deliberate mirror, not a shared type").
bool FootprintBakeFingerprintIsStale(const Params::FootprintBakeFingerprint& baked,
                                     const SourceFingerprint& current);

} // namespace Io
} // namespace SanmapGen
