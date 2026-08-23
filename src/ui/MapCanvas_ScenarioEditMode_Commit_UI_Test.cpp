// MapCanvas_ScenarioEditMode_Commit_UI_Test.cpp — acceptance test 3: `alloyMode == KeepAll` ->
// "Remove for this scenario" is disabled with a non-empty tooltip reason (never silently absent,
// never a silent no-op click), and Delta/Explicit can actually commit their own one action each.
#include "MapCanvas_ScenarioEditMode_Ops_UI.h"
#include "PreviewComposite_TestScene_UI.h"

namespace SanmapGen {
namespace Ui {
namespace {

void Check(bool bCondition, const char* label) { CheckPreviewExpectation(bCondition, label); }

void CheckRemoveBaselineAlloyGates() {
    Check(!CanRemoveBaselineAlloyForScenario(Params::ScenarioAlloyMode::KeepAll),
          "KeepAll cannot commit 'Remove for this scenario'");
    Check(RemoveBaselineAlloyDisabledReason(Params::ScenarioAlloyMode::KeepAll)[0] != '\0',
          "KeepAll's disabled reason is never an empty string (a real tooltip, not a silent disable)");
    Check(!CanRemoveBaselineAlloyForScenario(Params::ScenarioAlloyMode::Occupancy)
       && RemoveBaselineAlloyDisabledReason(Params::ScenarioAlloyMode::Occupancy)[0] != '\0',
          "Occupancy is disabled with its own explanation too (the same honesty, not silent)");
    Check(!CanRemoveBaselineAlloyForScenario(Params::ScenarioAlloyMode::Explicit)
       && RemoveBaselineAlloyDisabledReason(Params::ScenarioAlloyMode::Explicit)[0] != '\0',
          "Explicit is disabled with its own explanation (roster-scoped delete, not per-marker)");
    Check(CanRemoveBaselineAlloyForScenario(Params::ScenarioAlloyMode::Delta),
          "Delta is the one mode that can actually commit a baseline removal");
}

void CheckCommitRemoveBaselineAlloy() {
    Params::ScenarioBody body;
    body.alloyMode = Params::ScenarioAlloyMode::Delta;
    ScenarioEditModeState::ContextMenuRequest request;
    request.kind = ScenarioEditModeState::ContextMenuRequest::Kind::RemoveBaselineAlloy;
    request.markerName = "alloy_r0_0";
    CommitScenarioEditModeContextMenu(body, request);
    Check(body.alloysToRemove.size() == 1u && body.alloysToRemove[0].markerName == "alloy_r0_0",
          "committing under Delta appends exactly one ScenarioAlloyRemoval");

    Params::ScenarioBody keepAllBody; keepAllBody.alloyMode = Params::ScenarioAlloyMode::KeepAll;
    CommitScenarioEditModeContextMenu(keepAllBody, request);
    Check(keepAllBody.alloysToRemove.empty(),
          "an ungated commit against KeepAll is a documented no-op, never a crash or a write");
}

void CheckAddAlloyMarkerGates() {
    Check(CanAddAlloyMarkerForScenario(Params::ScenarioAlloyMode::Explicit), "Explicit can add to body.alloys");
    Check(CanAddAlloyMarkerForScenario(Params::ScenarioAlloyMode::Delta), "Delta can add to body.alloysToAdd");
    Check(!CanAddAlloyMarkerForScenario(Params::ScenarioAlloyMode::Occupancy)
       && AddAlloyMarkerDisabledReason(Params::ScenarioAlloyMode::Occupancy)[0] != '\0',
          "Occupancy has no override list to add to, and says so");
    Check(!CanAddAlloyMarkerForScenario(Params::ScenarioAlloyMode::KeepAll)
       && AddAlloyMarkerDisabledReason(Params::ScenarioAlloyMode::KeepAll)[0] != '\0',
          "KeepAll has no override list to add to, and says so");
}

void CheckCommitAddAlloyMarker() {
    Params::ScenarioBody explicitBody; explicitBody.alloyMode = Params::ScenarioAlloyMode::Explicit;
    ScenarioEditModeState::ContextMenuRequest request;
    request.kind = ScenarioEditModeState::ContextMenuRequest::Kind::AddAlloyForArmy;
    request.armyName = "ARMY_01"; request.worldX = 5.0f; request.worldZ = 6.0f;
    CommitScenarioEditModeContextMenu(explicitBody, request);
    Check(explicitBody.alloys.size() == 1u && explicitBody.alloys[0].armyName == "ARMY_01"
       && explicitBody.alloys[0].positionX == 5.0f, "Explicit commit appends to body.alloys at the clicked position");

    Params::ScenarioBody deltaBody; deltaBody.alloyMode = Params::ScenarioAlloyMode::Delta;
    CommitScenarioEditModeContextMenu(deltaBody, request);
    Check(deltaBody.alloysToAdd.size() == 1u, "Delta commit appends to body.alloysToAdd instead");
    Check(!explicitBody.alloys[0].markerName.empty() && !deltaBody.alloysToAdd[0].markerName.empty(),
          "a synthesized custom markerName is never left empty");
}

} // namespace

void RunScenarioEditModeCommitChecks() {
    CheckRemoveBaselineAlloyGates();
    CheckCommitRemoveBaselineAlloy();
    CheckAddAlloyMarkerGates();
    CheckCommitAddAlloyMarker();
}

} // namespace Ui
} // namespace SanmapGen
