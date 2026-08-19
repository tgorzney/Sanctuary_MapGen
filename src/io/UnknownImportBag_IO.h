// UnknownImportBag_IO.h — the IO-layer-only passthrough for `.sanmap` top-level document data this
// build's readers do not recognize (IO_MIGRATION_SPEC.md §6, "Unknown Import passthrough";
// STEP24_ImportNeverRefuses_IO ruling 4).
//
// HOMING (why not Params::MapRecipe, why not MapImportResult): cannot live on `Params::MapRecipe` —
// `src/params/` has zero `nlohmann::json` includes anywhere in the tree, and Constitution §1's
// `IO -> {DATA, PARAMS}` one-directional dependency table forbids pulling `nlohmann/json.hpp`
// upward into PARAMS. Cannot live on `MapImportResult` either — its one real consumer
// (`FilesTab_Actions_UI.cpp`) is a local, transient, discarded value that does not survive to a
// later export call, and homing it there would force a JSON include into every UI translation unit
// that touches the Files tab. This type is the one honest home: IO-only, and long-lived across a
// caller's load-edit-save session.
//
// THREADING: nullable out-param on `MapImporter::LoadSanmap`/`ParseSanmapJsonText`, nullable
// in-param on `MapExporter::BuildSanmapJsonText` — the exact same pattern already established by
// `Data::MapFields* outFields`. Owned by whatever caller already threads `recipe` across the
// load-edit-save session (`FilesTab_UI.h`'s `FilesTabState`).
#pragma once
#include <nlohmann/json.hpp>

namespace SanmapGen {
namespace Io {

struct UnknownImportBag {
    nlohmann::json unknownTopLevelKeys = nlohmann::json::object();
};

} // namespace Io
} // namespace SanmapGen
