// Sanmap_MigrationPreview_IO.h — a UI-layer-only preview/selective-apply surface for the
// no-version-marker case (IO_MIGRATION_SPEC.md §6, STEP26A). Sibling to `Sanmap_MigrationManifest_IO`
// / `Sanmap_MigrationRunner_IO` — migration-system machinery, not a per-domain fragment.
//
// This is DELIBERATELY separate from `RunSanmapMigrations` (`Sanmap_MigrationRunner_IO.h`), which
// stays the non-fallible, always-runs, always-committed automatic path. Conflating the two risks the
// preview path becoming reachable from the automatic one — no dry-run flag is added to the runner for
// this reason (spec's own instruction).
//
// Caller contract: a human may only reach this surface after `MapImportResult::bNoVersionMarkerFound`
// (`MapImporter_IO.h`) came back true from an ordinary import — this preview assumes starting version
// 1 and walks the FULL manifest, which is only a safe thing to offer when no version marker was found
// at all (§6). If the human accepts some or all of what the preview finds, that second pass's result
// REPLACES the direct-read result entirely — the two are never merged (§6).
#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace SanmapGen {
namespace Io {

struct MapImportResult;

// One migration's own preview row. `bIndependentlySelectable`/`bLosslessIfSkipped` are copied
// straight from the manifest's own `MigrationEntry` (`Sanmap_MigrationManifest_IO.h`) — a caller
// building a selective-apply dialog may only offer a genuine "skip this" checkbox for a row where
// BOTH are true (IO_MIGRATION_SPEC.md §3's dialog-gating law); an entry failing either still runs
// unconditionally whenever the step it belongs to runs at all.
struct MigrationPreviewEntry {
    const char*     name;
    const char*     description;
    bool            bIndependentlySelectable;
    bool            bLosslessIfSkipped;
    bool            bWouldChangeDocument;
    nlohmann::json  diffPatch;   // json::diff(before, after) for this entry alone.
};

// One version step's preview. `legacyKeysToDelete` is an INFORMATIONAL MIRROR only — this preview
// never deletes anything; a caller applying the whole step for real is what actually fires deletion
// (`ApplySelectedSanmapMigrations`, below).
struct MigrationPreviewStep {
    int                                 sourceVersion;
    std::vector<MigrationPreviewEntry>  entries;
    std::vector<const char*>            legacyKeysToDelete;
};

// The whole preview report. `assumedStartingVersion` is always 1 here — this surface exists
// specifically for the no-version-marker case (§6), where the runner itself never resolves a
// starting version at all.
struct MigrationPreviewReport {
    int                                assumedStartingVersion = 1;
    std::vector<MigrationPreviewStep> steps;
};

// Non-mutating. Walks the manifest from `assumedStartingVersion` (1) to `kCurrentSanGenVersion`,
// running each step's migrations IN ORDER against a working copy of `document` so each entry's own
// diff reflects what that entry actually contributes ON TOP OF its already-applied siblings — the
// same cross-domain-read/additive-write semantics `RunSanmapMigrations` itself relies on (§2), not an
// isolated diff against the untouched original. `document` itself is never modified. Caller must have
// already confirmed via `MapImportResult::bNoVersionMarkerFound` that no version marker was found
// before calling this.
MigrationPreviewReport PreviewSanmapMigrationWalk(const nlohmann::json& document);

// Mutates `document` in place per the human's selection. `selectedNames` are the
// `MigrationEntry::name` values opted IN. Every `bIndependentlySelectable == false` entry in a step
// runs unconditionally whenever any entry of that step is selected — full-step semantics for
// non-independent entries, unchanged. Only writes `document["SanGenVersion"] = sourceVersion + 1` for
// a step where EVERY entry (independent and not) was selected; a partial application leaves
// `SanGenVersion` untouched and does not proceed into any further step (a later step's migrations may
// assume the prior step's shape is complete). `legacyKeysToDelete` fires only on full-step
// application, per the existing law this does not change (§3).
void ApplySelectedSanmapMigrations(nlohmann::json& document,
                                   const std::vector<std::string>& selectedNames,
                                   MapImportResult& result);

} // namespace Io
} // namespace SanmapGen
