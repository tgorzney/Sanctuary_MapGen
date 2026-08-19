// Sanmap_MigrationRunner_IO.h — resolves a `.sanmap` document's version and walks it forward
// (IO_MIGRATION_SPEC.md §4). Runs BEFORE any domain block reader (§4.4): `MapImporter::
// ParseSanmapJsonText` calls this immediately after confirming `document.is_object()`, before even
// the `name`/`credits`/`height`/`width` reads that follow it.
//
// Version resolution (§4.1): reads top-level `document["SanGenVersion"]` first; only falls back to
// the legacy nested `document["mapGeneratorData"]["MapGeneratorDataVersion"]` field if that key is
// absent, and LOGS that the fallback fired (never silent).
//
// RECOVERY LAW (Constitution §6 / IO_MIGRATION_SPEC.md §6, as ratified by STEP24_
// ImportNeverRefuses_IO — supersedes this file's earlier refusal law): a `.sanmap`'s declared
// schema version is never grounds to refuse the file. This function is NON-FALLIBLE — it always
// returns having mutated `document` as far as it safely can, loud-warning-logged, never a
// caller-visible gate:
//  - Newer than `kCurrentSanGenVersion` -> loud warning, no migration steps run (nothing forward to
//    migrate to). Block readers then read whatever current-shape keys are present.
//  - No version marker of any kind (neither `SanGenVersion` nor its legacy predecessor) -> loud
//    warning, and the runner does NOT resolve a starting version or walk any migration — the
//    document is handed to the readers exactly as found, current-shape keys only. This is a
//    DELIBERATE choice, not a shortcut: the migration transform primitives (RenameKey/MoveKey/
//    WrapScalarAsVector) actively rewrite keys and are not miss-safe like the `Read*` accessors, so
//    blind-walking an unconfirmed-origin document as if it were version 1 risks corrupting data a
//    plain reader would have recovered correctly on its own. A separate, UI-layer-only preview/
//    selective-apply feature may offer that walk later (STEP26, out of scope here).
//  - Old, in-range version (present, less than `kCurrentSanGenVersion`) -> unchanged, walks forward
//    per §4.2, loud-logged.
//  - A document already at `kCurrentSanGenVersion` is a no-op pass-through.
// A file that fails to parse, is not a JSON object, or fails the size/header checks is UNCHANGED by
// this ticket — that refusal still happens, upstream of this function, in `MapImporter::
// ParseSanmapJsonText`/`LoadSanmap`. Only a version marker's VALUE has stopped being refusal-worthy.
#pragma once
#include "Sanmap_KnownTopLevelKeys_IO.h"
#include <nlohmann/json.hpp>

namespace SanmapGen {
namespace Io {

struct MapImportResult;
struct UnknownImportBag;

// Mutates `document` in place, walking it forward to `kCurrentSanGenVersion` when it safely can.
// `outUnknownData` is nullable (see `UnknownImportBag_IO.h`) — when given, every top-level key this
// build does not recognize (`IsKnownTopLevelSanmapKey`, `Sanmap_KnownTopLevelKeys_IO.h`) is copied
// into it, AFTER the forward-walk above so a legacy key a migration step deliberately deleted via
// `DeleteKeyIfPresent` is already physically gone from `document` by the time this runs (ordering
// alone distinguishes "deliberately deleted" from "genuinely unknown" — no extra bookkeeping).
void RunSanmapMigrations(nlohmann::json& document, MapImportResult& result,
                         UnknownImportBag* outUnknownData);

} // namespace Io
} // namespace SanmapGen
