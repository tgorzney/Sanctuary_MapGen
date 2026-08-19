// MapExporter_BlueprintValidation_IO.h — `BlueprintValidationReport` + the export-side
// `props`/`decals` blueprintPath validation pass (STEP32; STEP5_PropsDecalsValidation_UI).
// Layer: IO. A DELIBERATE PUBLIC-HEADER EXCEPTION: the UI layer (`FilesTab_Draw_UI.cpp`) calls
// `ValidatePropAndDecalBlueprintPaths` directly, so this stays a public header rather than folding
// into a `MapExporter_Recipe_IO.h`-style module-internal one — the same cross-domain-validation
// posture the sibling `.cpp`'s own header comment already documents in full.
#pragma once
#include <string>
#include <vector>

namespace SanmapGen {
namespace Params { struct MapRecipe; }
namespace Io {

class SanpackReader;

// One `props`/`decals` blueprintPath validation pass (MapExporter_BlueprintValidation_IO.cpp).
// Warn-not-block: AllResolved() gates nothing here — the caller decides what to do with a finding.
struct BlueprintValidationReport {
    std::vector<std::string> unresolvedBlueprintPaths;   // literal strings, props then decals
    bool AllResolved() const { return unresolvedBlueprintPaths.empty(); }
    std::string SummaryText() const;   // ONE wording — shared by the UI dialog body and debugLog
};

// `assetPack` MUST already be `Open()`+`ReadCentralDirectoryOnce()`'d by the caller. Pure/read-only,
// touches no disk, never called from inside BuildSanmapJsonText (same tier as `recipe.IsValid()`).
BlueprintValidationReport ValidatePropAndDecalBlueprintPaths(const Params::MapRecipe& recipe,
                                                              const SanpackReader& assetPack);

} // namespace Io
} // namespace SanmapGen
