// MapCanvas_ScenarioEditMode_Commit_UI.cpp — whether the two right-click actions can actually
// commit for the open scenario's alloyMode, why not when they cannot, and the commit itself.
// Layer: UI. Pure/imgui-free/headless-testable.
//
// Only `Delta`/`Explicit` are enabled, and only for the ONE action each mode's PARAMS shape can
// actually express (MAP_SCENARIO_SPEC §5): `Occupancy`/`KeepAll` carry no per-instance
// add/remove-override array at all — there is nothing this module could honestly write for them,
// so both are disabled+tooltipped rather than silently accepted (STEP78 acceptance test 3 names
// KeepAll explicitly; this module applies the SAME honesty to Occupancy for the identical reason,
// not because the ticket named it, but because pretending otherwise would be a silent no-op the
// ticket's own principle forbids).
#include "MapCanvas_ScenarioEditMode_Ops_UI.h"
#include <cstdlib>

namespace SanmapGen {
namespace Ui {
namespace {

int NextCustomAlloyMarkerSuffix(const Params::ScenarioBody& body) {
    int highestSeen = 0;
    auto ScanForHigherSuffix = [&highestSeen](const std::string& markerName) {
        const std::string prefix = "custom_";
        if (markerName.rfind(prefix, 0) != 0) return;
        const int suffix = std::atoi(markerName.c_str() + prefix.size());
        if (suffix >= highestSeen) highestSeen = suffix + 1;
    };
    for (const Params::ScenarioAlloyOverride& entry : body.alloys)      ScanForHigherSuffix(entry.markerName);
    for (const Params::ScenarioAlloyOverride& entry : body.alloysToAdd) ScanForHigherSuffix(entry.markerName);
    return highestSeen;
}

} // namespace

bool CanRemoveBaselineAlloyForScenario(Params::ScenarioAlloyMode alloyMode) {
    return alloyMode == Params::ScenarioAlloyMode::Delta;
}
const char* RemoveBaselineAlloyDisabledReason(Params::ScenarioAlloyMode alloyMode) {
    switch (alloyMode) {
        case Params::ScenarioAlloyMode::KeepAll:
            return "Keep All never deletes markers, even for empty slots.";
        case Params::ScenarioAlloyMode::Occupancy:
            return "Occupancy derives deletions from empty slots automatically; switch to Delta to remove one marker explicitly.";
        case Params::ScenarioAlloyMode::Explicit:
            return "Explicit mode deletes by army roster (the Alloys list below), not by individual baseline marker.";
        default: return "";
    }
}
bool CanAddAlloyMarkerForScenario(Params::ScenarioAlloyMode alloyMode) {
    return alloyMode == Params::ScenarioAlloyMode::Delta || alloyMode == Params::ScenarioAlloyMode::Explicit;
}
const char* AddAlloyMarkerDisabledReason(Params::ScenarioAlloyMode alloyMode) {
    switch (alloyMode) {
        case Params::ScenarioAlloyMode::KeepAll:   return "Keep All has no override list to add a marker to.";
        case Params::ScenarioAlloyMode::Occupancy: return "Occupancy has no override list; switch to Explicit or Delta to add a marker.";
        default: return "";
    }
}

void CommitScenarioEditModeContextMenu(Params::ScenarioBody& body,
                                       const ScenarioEditModeState::ContextMenuRequest& request) {
    using RequestKind = ScenarioEditModeState::ContextMenuRequest::Kind;
    if (request.kind == RequestKind::RemoveBaselineAlloy) {
        if (!CanRemoveBaselineAlloyForScenario(body.alloyMode)) return;
        Params::ScenarioAlloyRemoval removal;
        removal.armyName = request.armyName;
        removal.markerName = request.markerName;
        body.alloysToRemove.push_back(removal);
    } else if (request.kind == RequestKind::AddAlloyForArmy) {
        if (!CanAddAlloyMarkerForScenario(body.alloyMode)) return;
        Params::ScenarioAlloyOverride entry;
        entry.armyName = request.armyName;
        entry.markerName = "custom_" + std::to_string(NextCustomAlloyMarkerSuffix(body));
        entry.positionX = request.worldX; entry.positionY = request.worldY; entry.positionZ = request.worldZ;
        if (body.alloyMode == Params::ScenarioAlloyMode::Explicit) body.alloys.push_back(entry);
        else body.alloysToAdd.push_back(entry);
    }
}

} // namespace Ui
} // namespace SanmapGen
