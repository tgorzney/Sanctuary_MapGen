// Sanmap_MigrationRunner_IO.h — resolves a `.sanmap` document's version and walks it forward
// (IO_MIGRATION_SPEC.md §4). Runs BEFORE any domain block reader (§4.4): `MapImporter::
// ParseSanmapJsonText` calls this immediately after confirming `document.is_object()`, before even
// the inline `height`/`width` reads that used to run first — a version mismatch must not silently
// degrade (Constitution §6), so nothing may read the document until the runner has cleared it.
//
// Version resolution (§4.1): reads top-level `document["SanGenVersion"]` first; only falls back to
// the legacy nested `document["mapGeneratorData"]["MapGeneratorDataVersion"]` field if that key is
// absent, and LOGS that the fallback fired (never silent). Neither present -> refuse (§6) rather
// than guess version 1.
//
// Refusal law (§6): a resolved version newer than `kCurrentSanGenVersion` refuses outright with a
// clear logged reason. No version marker at all refuses outright, distinctly. A document already
// at `kCurrentSanGenVersion` is a no-op pass-through (still runs resolution and the refusal check,
// calls no migration). Anything strictly between the resolved version and `kCurrentSanGenVersion`
// walks the manifest's ordered per-step migrations, generically, per §4.2 — this ticket's manifest
// is empty, so that loop simply does not iterate today.
#pragma once
#include <nlohmann/json.hpp>

namespace SanmapGen {
namespace Io {

struct MapImportResult;

// Mutates `document` in place, walking it forward to `kCurrentSanGenVersion`. Returns false (with
// the refusal reason logged onto `result`) when the document is refused; `result`'s own
// `bSucceeded` is left for the caller to set, mirroring `MapImporter::ParseSanmapJsonText`'s own
// existing return-bool-not-bSucceeded convention (`MapImporter_IO.h`, §4.5's "mirrors the existing
// pattern" instruction).
bool RunSanmapMigrations(nlohmann::json& document, MapImportResult& result);

} // namespace Io
} // namespace SanmapGen
