// ScenarioScript_FullDataImport_IO.h -- the ONE disk-touching, human-triggered entry point that
// composes ALL FOUR pure Part-B/§15.11 extractors (the existing area-rectangle one plus the three
// STEP264/STEP265 ones) over the SAME source text with a SINGLE byte-capped disk read
// (`ARCH_15_14_ForeignScenarioFullDataImportAndUnitPlacement.md` Part B; the IO Architecture Expert's
// own orchestrator call, item 2 of Part B's closing paragraph). Layer: IO.
//
// Mirrors ScenarioScript_AreaImport_IO.h's own guard order and posture (filename refusal -> byte-size
// stat cap -> banner-line refusal -> extraction -> reconciliation), reusing the SAME
// kScenarioGeneratedFileBannerLine + SanGen-owned-filename guard and the SAME 4 MiB cap verbatim --
// never a second independently invented copy of the byte cap's VALUE, though each guard function
// itself is this translation unit's own private copy, per this family's established "each file owns
// its own copy" precedent (ScenarioScript_AreaImport_IO.cpp's own top-of-file note).
//
// **Only the area result auto-reconciles into recipe.areas** (identical collision policy to
// ScenarioScript_AreaImport_IO.h, unchanged). Match-conditions/slot-patterns/unit-placements are
// surfaced as raw candidate lists ONLY -- never auto-attached to any Params::ScenarioBody/Scenarios
// record. ARCH_15_14 Part B is explicit that wiring extracted data to a name/area is "a separate human
// authoring action," not something this orchestrator infers or bundles; recipe.scenarios is never
// touched by this function.
#pragma once
#include <cstddef>
#include <string>
#include <vector>
#include "ScenarioScript_AreaRectangleExtract_IO.h"
#include "ScenarioScript_MatchConditionExtract_IO.h"
#include "ScenarioScript_SlotPatternExtract_IO.h"
#include "ScenarioScript_UnitPlacementExtract_IO.h"

namespace SanmapGen {
namespace Params { struct MapRecipe; }
namespace Io {

// Constitution §6 byte cap -- SAME value as ScenarioScript_AreaImport_IO.h's own
// kMaxScenarioAreaImportSourceBytes (4 MiB), reused here as an independent named constant rather than
// referencing that header's constant directly, matching this family's "each file owns its own copy of
// a shared VALUE, not a shared symbol" precedent for guard machinery (see this header's own top note).
inline constexpr std::size_t kMaxScenarioFullDataImportSourceBytes = 4u * 1024u * 1024u;   // 4 MiB

// Composes all four extractors' own result types verbatim -- never a new merged/flattened shape.
// Only `areas` (via `.areas`/`.collisionIdentifiers`/`.nearMisses`/`.bRectangleCountCapExceeded`)
// drives the recipe.areas mutation below; the other three fields are read-only candidate surfaces for
// STEP266's UI to present for manual assignment.
struct ScenarioFullDataImportResult {
    ScenarioAreaExtractionResult           areas;              // existing item-9 auto-reconciliation
                                                                 // into recipe.areas, UNCHANGED behavior
    ScenarioMatchConditionExtractionResult matchConditions;     // raw candidates -- NOT auto-attached
    ScenarioSlotPatternExtractionResult    slotPatterns;        // raw candidates -- NOT auto-attached
    ScenarioUnitPlacementExtractionResult  unitPlacements;      // raw candidates -- NOT auto-attached

    bool bRefusedAsSanGenOwnedFile = false;   // filename or banner-line match -- same guard as
                                               // ScenarioScript_AreaImport_IO's item-1 refusal
    bool bRefusedUnreadableFile    = false;   // file missing, unreadable, or a stat failure -- mirrors
                                               // ScenarioAreaImportResult::bRefusedUnreadableFile
    bool bRefusedOversizedFile     = false;   // byte cap exceeded -- never scanned, mirrors
                                               // ScenarioAreaImportResult::bRefusedOversizedFile

    // Additive reconciliation bookkeeping for the ONE auto-attached result (areas) -- same shape as
    // ScenarioAreaImportResult's own writtenNames/skippedCollisionNames.
    std::vector<std::string> writtenAreaNames;
    std::vector<std::string> skippedCollisionAreaNames;

    std::string debugLog;
    void Log(const std::string& line) { debugLog += line; debugLog += '\n'; }
};

// sourceFilePath: any file the human picked (e.g. via FileDialog::OpenFilePath filtered to "*.lua" --
// UI wiring is STEP266's job, not built here). mapSize: the target map's Params::Geometry::mapSize,
// forwarded verbatim to the unit-placement extractor's own z-flip (it is NOT used by the other three
// extractors, which stay context-free). recipe.areas is mutated additively, in place, on success --
// left completely untouched on every refusal path; recipe.scenarios is NEVER mutated by this function.
// Never called implicitly (map open/export/generate/dirty-hash) -- an explicit, human-triggered
// authoring action only, per §15.11 item 8 as extended by ARCH_15_14 Part B.
ScenarioFullDataImportResult ImportFullScenarioDataFromScenarioScriptFile(
    const std::string& sourceFilePath, int mapSize, Params::MapRecipe& recipe);

} // namespace Io
} // namespace SanmapGen
