// MapImporter_ArmyIdentityNormalize_IO.h — STEP76_ArmyIdentityNaming_IO §4: the import-side
// counterpart to Sanmap_ArmyIdentity_IO.h's AssignArmyIdentities. Layer: IO.
// ⚠️ ASSUMPTION (STEP76 §4): the policy below — normalize on import, preserve the original as
// `displayName`, rewrite Spawn marker key references, report loudly — rests on an inference from
// the human's stated workflow, not a direct ruling. Flagged in the ticket, not reopened here.
//
// Its own file, not folded into MapImporter_Armies_IO.cpp (already 125 lines — ARCH_01_05_
// FileSizeCeilings.md §1.5 hard ceiling), and NOT a `<Domain>_Migrate_V<N>_IO` unit: this defect is
// not version-correlated (SanGen shipped non-conforming names under the CURRENT SanGenVersion), so
// it must run unconditionally on every import, not gated on a version walk.
#pragma once

namespace SanmapGen {
namespace Params { struct MapRecipe; }
namespace Io {

struct MapImportResult;

// Positional and total, mirroring Sanmap_ArmyIdentity_IO.h's AssignArmyIdentities exactly, so import
// and UI can never disagree about what the canonical identity is. For each army whose `name` is not
// already its roster-position identity: preserves the old name into `displayName` (only if
// `displayName` is empty — never clobbers a human-authored label), rewrites `name` to the canonical
// identity, and rewrites every `markers["Spawn"].transforms[*].name` that referenced the old name.
// Idempotent: a no-op, and silent, on an already-canonical roster.
void NormalizeArmyIdentities(Params::MapRecipe& outRecipe, MapImportResult& result);

} // namespace Io
} // namespace SanmapGen
