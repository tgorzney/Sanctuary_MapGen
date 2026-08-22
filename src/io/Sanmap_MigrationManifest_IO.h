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
// `kCurrentSanGenVersion = 3` — STEP40F populated the real V2->V3 step (`Sanmap_MigrationManifest_
// IO.cpp`'s one `MigrationStep`, sourceVersion 2) with all 9 `<Domain>_Migrate_V2_IO` migration
// functions STEP40B-E already shipped and individually tested, and bumped this constant from 2 to
// 3 to match. A `.sanmap` at version 2 now genuinely walks forward on import; a document already at
// 3 is an unaffected zero-step passthrough (`Sanmap_MigrationRunner_IO.cpp` §4.2).
//
// `kCurrentSanGenVersion = 4` — STEP67 added the V3->V4 step (sourceVersion 3), wiring in
// `MarkersStack_Migrate_V3` (the flat-array -> two-level `MarkerRuleLayer` reshape STEP66's new
// importer/exporter expect), and bumped this constant from 3 to 4 to match. A `.sanmap` at version
// 3 now walks forward one more step on import; a document already at 4 is an unaffected zero-step
// passthrough.
#pragma once
#include <nlohmann/json.hpp>
#include <vector>

namespace SanmapGen {
namespace Io {

inline constexpr int kCurrentSanGenVersion = 4;

// One free function per migration file, operating on the WHOLE parsed document in place
// (IO_MIGRATION_SPEC.md §2 — cross-domain compensation needs the whole document, not a fragment).
using MigrationFunction = void (*)(nlohmann::json& document);

// The manifest's element type (IO_MIGRATION_SPEC.md §3) — a migration is never a bare function
// pointer. `name` is the migration's own identifier (e.g. "GeneralMapSettings_Migrate_V2"), what
// the UI-layer selective-apply feature (§6) shows a human. `description` is a human-readable
// one-line summary of what the migration does. `bIndependentlySelectable` defaults to false: a
// step is, by default, an atomic unit — selectable and appliable only as a whole; may only be set
// true by the migration's own author, with a one-line justification comment at the declaration
// site (§3).
//
// `bLosslessIfSkipped` (STEP26A, ratified ARCH ruling, §3) — a SECOND, ORTHOGONAL, author-declared
// exception. `bIndependentlySelectable` answers an ORDERING question ("safe to run out of order").
// `bLosslessIfSkipped` answers a DATA-SAFETY question ("safe to omit without losing data") — the two
// can diverge in either direction and must never be conflated or "fixed" to match one another.
// Defaults to false (the safe default). May only be set true by the migration's own author, with a
// one-line justification comment at the declaration site, once verified against the READERS (not
// just the migration's own transform) that every field it would have relocated is still recoverable
// by some current-shape reader or existing legacy-mapGeneratorData-gated fallback reader when this
// migration alone is skipped and every sibling in the step still runs. A dialog may only offer a
// genuine "skip this" checkbox for an entry where bIndependentlySelectable == true AND
// bLosslessIfSkipped == true (§3's dialog-gating law).
struct MigrationEntry {
    MigrationFunction function;
    const char*       name;
    const char*       description;
    bool              bIndependentlySelectable = false;
    bool              bLosslessIfSkipped       = false;
};

// One version step: `sourceVersion` -> `sourceVersion + 1`. Ordering inside `migrations` is load-
// bearing law (§2 rule 2) — a later migration in the same step must never re-read a key an earlier
// one already deleted. `legacyKeysToDelete` fires AFTER every migration in the step has run.
struct MigrationStep {
    int                          sourceVersion = 0;
    std::vector<MigrationEntry>  migrations;
    std::vector<const char*>     legacyKeysToDelete;
};

// One step (sourceVersion 2 -> 3) as of STEP40F. The next version-bump ticket appends its own
// step(s) inside the `.cpp` body — that single edit is the only wiring a future coder does to land
// a new version step.
const std::vector<MigrationStep>& SanmapMigrationManifest();

} // namespace Io
} // namespace SanmapGen
