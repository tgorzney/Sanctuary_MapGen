// MapExporter_BlueprintValidation_IO.cpp — `ValidatePropAndDecalBlueprintPaths` and
// `BlueprintValidationReport::SummaryText`. Layer: IO. Declared in the PUBLIC MapExporter_IO.h
// (not MapExporter_Recipe_IO.h) — the UI layer calls this directly, a deliberate exception to the
// "one domain, one file pair" convention: it is cross-domain validation over BOTH `props` and
// `decals` together, not a JSON builder/reader for either (STEP5_PropsDecalsValidation_UI).
//
// Design ruling (work-order "warn, never block, for EVERY caller"): this function only REPORTS —
// it never refuses an export and it touches no disk itself. `assetPack` MUST already be
// `Open()`+`ReadCentralDirectoryOnce()`'d by the caller; this stays a sibling pre-flight step, the
// same tier as `recipe.IsValid()`, never called from inside `BuildSanmapJsonText`.
#include "MapExporter_IO.h"
#include "SanpackReader_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {

BlueprintValidationReport ValidatePropAndDecalBlueprintPaths(const Params::MapRecipe& recipe,
                                                              const SanpackReader& assetPack) {
    BlueprintValidationReport report;
    // GROUP-level only (finalized IO plumbing item 3): one lookup per unique blueprintPath, not
    // per transform — tens of lookups on a human-triggered export click, not a hot path.
    for (const Params::PropInstanceGroup& group : recipe.props)
        if (!assetPack.HasEntry(group.blueprintPath))
            report.unresolvedBlueprintPaths.push_back(group.blueprintPath);
    for (const Params::DecalInstanceGroup& group : recipe.decals)
        if (!assetPack.HasEntry(group.blueprintPath))
            report.unresolvedBlueprintPaths.push_back(group.blueprintPath);
    return report;
}

// ONE wording, shared by the UI confirm-dialog body and the IO debugLog line — do not duplicate
// the phrasing at either call site.
std::string BlueprintValidationReport::SummaryText() const {
    if (AllResolved()) return std::string();
    std::string text = std::to_string(unresolvedBlueprintPaths.size())
        + " prop/decal blueprintPath value(s) were not found in the loaded sanpack:";
    for (const std::string& path : unresolvedBlueprintPaths)
        text += "\n  " + (path.empty() ? std::string("(empty blueprintPath)") : path);
    text += "\nThe live game aborts loading everything after props/decals when one of these is "
            "missing (mapUtils.lua:107). Exporting anyway is allowed; fix the path(s) or make sure "
            "the asset ships before this map does.";
    return text;
}

} // namespace Io
} // namespace SanmapGen
