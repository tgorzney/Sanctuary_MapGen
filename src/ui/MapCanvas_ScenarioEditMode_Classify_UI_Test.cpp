// MapCanvas_ScenarioEditMode_Classify_UI_Test.cpp — acceptance test 1 (the six states each resolve
// distinctly, at the data level) and acceptance test 2's seeding half (a hollow spawn's resolved
// position is the REAL baked baseline, not zeroed). No imgui frame needed — pure classification.
#include "MapCanvas_ScenarioEditMode_UI.h"
#include "OverlayLayer_Settings_UI.h"
#include "PreviewComposite_TestScene_UI.h"
#include "../data/PlacementResults_DATA.h"
#include "../data/RuleBucketIndexSet_DATA.h"
#include "../params/Army_PARAMS.h"

namespace SanmapGen {
namespace Ui {
namespace {

void Check(bool bCondition, const char* label) { CheckPreviewExpectation(bCondition, label); }

Data::PlacementInstance MakeMarkerInstance(float worldX, float worldZ, int ruleIndex) {
    Data::PlacementInstance instance;
    instance.positionX = worldX; instance.positionZ = worldZ;
    instance.ruleIndex = ruleIndex;
    return instance;
}

struct Fixture {
    Data::PlacementResults    placements;
    Data::RuleBucketIndexSet  ruleBucketIndex;
    OverlayLayerSettings      overlaySettings;
    std::vector<Params::Army> armies;
    ScenarioEditModeResolveInput Input() const {
        ScenarioEditModeResolveInput input;
        input.overlayLayerSettings = &overlaySettings;
        input.placements = &placements;
        input.ruleBucketIndex = &ruleBucketIndex;
        input.armies = &armies;
        return input;
    }
};

// SpawnsArmies rule 0: two baked spawns (10,10) then (20,20) -> armies[0]/armies[1] positionally.
// Alloy rule 1: two baked alloys (30,30) then (40,40) -> "alloy_r1_0" / "alloy_r1_1".
Fixture BuildFixture() {
    Fixture fixture;
    fixture.armies.push_back(Params::Army()); fixture.armies[0].name = "ARMY_01";
    fixture.armies.push_back(Params::Army()); fixture.armies[1].name = "ARMY_02";

    fixture.placements.markers.Append(MakeMarkerInstance(10.0f, 10.0f, 0));
    fixture.placements.markers.Append(MakeMarkerInstance(20.0f, 20.0f, 0));
    fixture.placements.markers.Append(MakeMarkerInstance(30.0f, 30.0f, 1));
    fixture.placements.markers.Append(MakeMarkerInstance(40.0f, 40.0f, 1));
    const int ruleIndexColumn[4] = {0, 0, 1, 1};
    fixture.ruleBucketIndex.markers.Build(ruleIndexColumn, 4, 2);

    OverlayLayer_UI spawnsLayer; spawnsLayer.domainKind = OverlayDomainKind_UI::SpawnsArmies;
    spawnsLayer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, 0, true});
    OverlayLayer_UI alloyLayer; alloyLayer.domainKind = OverlayDomainKind_UI::Alloy;
    alloyLayer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, 1, true});
    fixture.overlaySettings.overlayLayers = {spawnsLayer, alloyLayer};
    return fixture;
}

void CheckSpawnStatesAndMaterializeSeed() {
    Fixture fixture = BuildFixture();
    Params::ScenarioBody body;
    body.spawns.push_back(Params::ScenarioSpawn());
    body.spawns[0].armyName = "ARMY_01"; body.spawns[0].positionX = 99.0f; body.spawns[0].positionZ = 99.0f;

    std::vector<ScenarioEditMarkerCandidate_UI> candidates;
    AppendScenarioEditModeSpawnCandidates(fixture.Input(), body, candidates);
    Check(candidates.size() == 2u, "one candidate per army");

    const ScenarioEditMarkerCandidate_UI* explicitCandidate = nullptr;
    const ScenarioEditMarkerCandidate_UI* hollowCandidate = nullptr;
    for (const auto& candidate : candidates) {
        if (candidate.state == ScenarioMarkerVisualState_UI::SpawnExplicit) explicitCandidate = &candidate;
        if (candidate.state == ScenarioMarkerVisualState_UI::SpawnNoOverride) hollowCandidate = &candidate;
    }
    Check(explicitCandidate != nullptr && explicitCandidate->worldX == 99.0f,
          "ARMY_01's explicit override renders at its OWN authored position, not the baseline");
    Check(hollowCandidate != nullptr && hollowCandidate->armyIndex == 1, "ARMY_02 (no override) is the hollow candidate");
    // Acceptance test 2's core claim: the hollow candidate's position IS the real baked baseline
    // (armies[1]'s positional baseline, world (20,20)) — never zeroed.
    Check(hollowCandidate != nullptr && hollowCandidate->worldX == 20.0f && hollowCandidate->worldZ == 20.0f,
          "the hollow spawn's resolved position is the REAL baseline instance, not (0,0)");
}

void CheckAlloyStatesDelta() {
    Fixture fixture = BuildFixture();
    Params::ScenarioBody body;
    body.alloyMode = Params::ScenarioAlloyMode::Delta;
    Params::ScenarioAlloyRemoval removal; removal.markerName = "alloy_r1_0";
    body.alloysToRemove.push_back(removal);
    Params::ScenarioAlloyOverride added; added.armyName = "ARMY_01"; added.markerName = "custom_0";
    added.positionX = 50.0f; added.positionZ = 50.0f;
    body.alloysToAdd.push_back(added);

    std::vector<ScenarioEditMarkerCandidate_UI> candidates;
    AppendScenarioEditModeAlloyCandidates(fixture.Input(), body, /*previewAsSlotPattern=*/"", candidates);
    Check(candidates.size() == 3u, "two baseline alloys + one alloysToAdd entry");

    int keptCount = 0, removedGhostCount = 0, addedCount = 0;
    for (const auto& candidate : candidates) {
        if (candidate.state == ScenarioMarkerVisualState_UI::AlloyKept) ++keptCount;
        if (candidate.state == ScenarioMarkerVisualState_UI::AlloyRemovedGhost) ++removedGhostCount;
        if (candidate.state == ScenarioMarkerVisualState_UI::AlloyAdded) ++addedCount;
    }
    Check(keptCount == 1, "the non-removed baseline alloy renders Kept under Delta");
    Check(removedGhostCount == 1, "the alloysToRemove entry's baseline alloy renders as the ghost-red-X state");
    Check(addedCount == 1, "the alloysToAdd entry renders as the added-tint state");
}

void CheckAlloyDeletedUnderOccupancy() {
    Fixture fixture = BuildFixture();
    Params::ScenarioBody body;
    body.alloyMode = Params::ScenarioAlloyMode::Occupancy;
    std::vector<ScenarioEditMarkerCandidate_UI> candidates;
    // Zero filled slots -> every baseline alloy (bucket positions 0 and 1) is at/past the filled
    // count -> both render AlloyDeleted (§0's own flagged Occupancy simplification).
    AppendScenarioEditModeAlloyCandidates(fixture.Input(), body, /*previewAsSlotPattern=*/"--", candidates);
    Check(candidates.size() == 2u, "two baseline alloys, no overrides under Occupancy");
    for (const auto& candidate : candidates)
        Check(candidate.state == ScenarioMarkerVisualState_UI::AlloyDeleted,
              "an Occupancy scenario with zero filled slots renders every baseline alloy Deleted");
}

} // namespace

void RunScenarioEditModeClassifyChecks() {
    CheckSpawnStatesAndMaterializeSeed();
    CheckAlloyStatesDelta();
    CheckAlloyDeletedUnderOccupancy();
}

} // namespace Ui
} // namespace SanmapGen
