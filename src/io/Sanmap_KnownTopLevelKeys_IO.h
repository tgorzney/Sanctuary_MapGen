// Sanmap_KnownTopLevelKeys_IO.h — the maintained allowlist of `.sanmap` top-level document keys
// this build recognizes (STEP24_ImportNeverRefuses_IO ruling 4, IO_MIGRATION_SPEC.md §6's "Unknown
// Import passthrough"). Split out of `Sanmap_MigrationRunner_IO.*` (ARCH §1.5's `Type_Aspect_LAYER`
// split-file pattern — the same one `IO_MIGRATION_SPEC.md` §1 already uses for migration files) so
// the allowlist's own bulk (~90 entries) does not push the runner past its size ceiling.
#pragma once
#include <string>

namespace SanmapGen {
namespace Io {

// True when `key` is a top-level `.sanmap` document key this build recognizes — read by some
// `Read*Json` call in `MapImporter::ParseSanmapJsonText`, deliberately write-only-by-design (an
// export-only field with no importer yet), or runner-owned (`SanGenVersion`/`mapGeneratorData`).
// Any OTHER top-level key falls to the Unknown-Import bag instead of being silently dropped
// (`Sanmap_MigrationRunner_IO.cpp`'s `CaptureUnknownTopLevelKeys`). Exposed for
// `KnownTopLevelSanmapKeys_IO_Test` (`MapImporter_IO_Test.cpp`), the paired coverage test asserting
// every key `MapExporter::BuildSanmapJsonText` writes is present here.
bool IsKnownTopLevelSanmapKey(const std::string& key);

} // namespace Io
} // namespace SanmapGen
