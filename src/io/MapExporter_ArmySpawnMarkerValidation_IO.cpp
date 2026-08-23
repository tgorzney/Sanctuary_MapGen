// MapExporter_ArmySpawnMarkerValidation_IO.cpp — `ValidateArmiesHaveSpawnMarkers` and
// `ArmySpawnMarkerValidationReport::SummaryText`. Layer: IO. STEP82.
//
// The matching rule, and it is format truth: `Params::Army::name` is compared against
// `Params::MarkerTransform::name` exactly, case-sensitively, byte-for-byte -- the engine reads a
// raw JSON dictionary key (MapExporter_Markers_IO.cpp writes `markerTransform.name` as that key),
// so a match SanGen would accept but the engine would not is worse than no check at all. NEVER
// `MarkerTransform::alias` -- alias is a SanGen-added field the game never reads for this purpose.
//
// Design ruling (work-order "warn, never block"): this function only REPORTS -- it never mutates
// `recipe`, touches no disk, and stays a sibling pre-flight step, the same tier as
// `recipe.IsValid()`, never called from inside `BuildSanmapJsonText`.
#include "MapExporter_ArmySpawnMarkerValidation_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include <algorithm>

namespace SanmapGen {
namespace Io {

ArmySpawnMarkerValidationReport ValidateArmiesHaveSpawnMarkers(const Params::MapRecipe& recipe) {
    ArmySpawnMarkerValidationReport report;

    // Union of every "Spawn"-named group's transforms (defensive against duplicate group names --
    // recipe.markers is a std::vector, so a duplicate is representable in memory even though the
    // JSON dictionary would collapse it). Linear scan: army/marker counts are tens, this runs once
    // per human export click.
    std::vector<const std::string*> spawnMarkerNames;
    for (const Params::MarkerInstanceGroup& group : recipe.markers) {
        if (group.name != spawnMarkerGroupName) continue;
        report.bSpawnMarkerGroupPresent = true;
        for (const Params::MarkerTransform& markerTransform : group.transforms)
            spawnMarkerNames.push_back(&markerTransform.name);
    }

    for (const Params::Army& army : recipe.armies) {
        const bool bHasMatchingSpawnMarker =
            std::any_of(spawnMarkerNames.begin(), spawnMarkerNames.end(),
                        [&army](const std::string* spawnName) { return *spawnName == army.name; });
        if (bHasMatchingSpawnMarker) continue;
        const bool bAlreadyReported =
            std::find(report.armyNamesWithoutSpawnMarker.begin(),
                     report.armyNamesWithoutSpawnMarker.end(), army.name)
            != report.armyNamesWithoutSpawnMarker.end();
        if (!bAlreadyReported) report.armyNamesWithoutSpawnMarker.push_back(army.name);
    }
    return report;
}

// ONE wording, shared by every call site -- do not restate the phrasing elsewhere.
std::string ArmySpawnMarkerValidationReport::SummaryText() const {
    if (AllArmiesHaveSpawnMarkers()) return std::string();
    std::string text = std::to_string(armyNamesWithoutSpawnMarker.size())
        + " army(s) have no matching entry in the \"" + spawnMarkerGroupName + "\" marker group:";
    for (const std::string& armyName : armyNamesWithoutSpawnMarker)
        text += "\n  " + armyName;
    if (!bSpawnMarkerGroupPresent)
        text += "\nThis map has no \"" + std::string(spawnMarkerGroupName) + "\" marker group at all.";
    text += "\nThe game resolves an army's start position by looking that army's own name up as a "
            "key in markers." + std::string(spawnMarkerGroupName) + ".transforms. An army with no "
            "such key gets no spawn marker and starts the match with no commander and no "
            "start-position units. Nothing was changed: SanGen never creates a " + spawnMarkerGroupName
            + " marker for you and never removes an army. Add a " + spawnMarkerGroupName
            + " marker named after each army listed above in the Markers tab if that was not intended.";
    return text;
}

} // namespace Io
} // namespace SanmapGen
