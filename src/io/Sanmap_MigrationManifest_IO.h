// Sanmap_MigrationManifest_IO.h — the ONE file touched on every future `.sanmap` version bump
// (IO_MIGRATION_SPEC.md §3). One ordered table: for each source version `N` present, the ordered
// list of migration functions that carry a document from `N` to `N+1`, plus (optionally) the
// legacy top-level keys to delete once every migration in that step has run.
//
// Sparse by construction — most version steps touch a handful of domains, not all of them; the
// table has exactly as many entries per step as that step needs. No self-registration / auto-
// discovery: a migration function is wired in by one explicit, visible line in the `.cpp`, never a
// static initializer (§3's own reasoning — undefined static-init order across translation units is
// a genuinely hard failure to diagnose, and this pack's AI-legibility principle forbids the
// implicit magic an auto-discovered list would be).
//
// `kCurrentSanGenVersion` lives here, as ONE constant — `highest manifest step + 1`. The exporter
// (`MapExporter_Recipe_IO.cpp`) writes `SanGenVersion` from this SAME constant, never a second
// independently-maintained number, closing the exact defect this subsystem exists to fix (today's
// write-only `mapGeneratorDataVersion` literal with no reader anywhere).
//
// THIS TICKET (Step 6): `kCurrentSanGenVersion = 2` — NOT 3. This retroactively and honestly names
// TODAY's already-shipped `mapGeneratorData` blob shape as "SanGenVersion 2." The manifest is
// EMPTY: zero migration steps exist yet. Bumping to 3 and writing the real Correction-2-through-11
// migration files (SPEC-4) is a future ticket's job — do not pre-build entries for it here.
#pragma once
#include <nlohmann/json.hpp>
#include <vector>

namespace SanmapGen {
namespace Io {

inline constexpr int kCurrentSanGenVersion = 2;

// One free function per migration file, operating on the WHOLE parsed document in place
// (IO_MIGRATION_SPEC.md §2 — cross-domain compensation needs the whole document, not a fragment).
using MigrationFunction = void (*)(nlohmann::json& document);

// One version step: `sourceVersion` -> `sourceVersion + 1`. Ordering inside `migrations` is load-
// bearing law (§2 rule 2) — a later migration in the same step must never re-read a key an earlier
// one already deleted. `legacyKeysToDelete` fires AFTER every migration in the step has run.
struct MigrationStep {
    int                             sourceVersion = 0;
    std::vector<MigrationFunction>  migrations;
    std::vector<const char*>        legacyKeysToDelete;
};

// Empty for this ticket. The next version-bump ticket appends its step(s) inside the `.cpp` body —
// that single edit is the only wiring a future coder does to land a new version step.
const std::vector<MigrationStep>& SanmapMigrationManifest();

} // namespace Io
} // namespace SanmapGen
