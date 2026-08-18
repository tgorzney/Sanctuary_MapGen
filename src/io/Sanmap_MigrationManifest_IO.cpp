// Sanmap_MigrationManifest_IO.cpp — the manifest table itself. See the header for the full
// contract. Empty for this ticket (Step 6): zero migration steps exist yet.
#include "Sanmap_MigrationManifest_IO.h"

namespace SanmapGen {
namespace Io {

const std::vector<MigrationStep>& SanmapMigrationManifest() {
    // The next version-bump ticket appends its MigrationStep entries here, one line per step
    // (IO_MIGRATION_SPEC.md §3 — "the only file a coder edits to wire a new version step").
    static const std::vector<MigrationStep> manifest = {};
    return manifest;
}

} // namespace Io
} // namespace SanmapGen
