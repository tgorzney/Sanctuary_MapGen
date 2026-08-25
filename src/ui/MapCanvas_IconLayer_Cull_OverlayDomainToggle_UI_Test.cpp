// MapCanvas_IconLayer_Cull_OverlayDomainToggle_UI_Test.cpp — STEP201 acceptance, item 1: toggling
// one OverlayLayer_UI::bEnabled off through ResolveVisibleCandidates changes ONLY that domain's
// candidate output, leaving every other enabled domain's candidate count unchanged. Covers all
// three domains STEP201's own routing-ambiguity question turned on: Alloy and SpawnsArmies (both
// markers, ARCH_14_06_OverlayDomainKindCoexistence.md's 2-way category re-slice of ONE collection)
// and Units (a fully separate Data::PlacementResults collection, per
// ARCH_14_02_DataModel.md §14.2's own binding sub-layer-to-data table — SeedUnitsManualSubLayers/
// PushProceduralRefs over recipe.armies/unitRules, Application_OverlaySetup_UI.cpp, never
// SeedMarkerDomains). DrawOverlayIconLayersInput hands ResolveVisibleCandidates every DATA pointer
// as `const*` (MapCanvas_IconLayer_Ops_UI.h) — toggling visibility cannot mutate
// Data::PlacementInstances by construction; the instance-count re-checks below confirm that directly
// rather than only trusting the header.
// One translation unit of the MapCanvas_IconLayer_UI_Test binary.
#include "MapCanvas_IconLayer_TestFixture_UI.h"

namespace SanmapGen {
namespace Ui {
namespace {

// Alloy (ruleIndex 0) + SpawnsArmies (ruleIndex 1) share the markers collection; Units (ruleIndex 0
// of its OWN collection) is unrelated data. All three domains enabled by default, one in-view,
// resolvable instance apiece.
struct DomainToggleFixture {
    IconLayerTestFixture fixture;

    DomainToggleFixture() {
        // 7-char max (TemplateIdentifier's own storage limit, PlacementInstance_DATA.h) — a longer
        // string here silently truncates on Append and then fails IconAtlasPairingLookup::Resolve
        // against the un-truncated key this fixture registers below.
        SeedAtlasEntry(fixture.pairingLookup, fixture.atlasManifest, "alloyMk", 0);
        SeedAtlasEntry(fixture.pairingLookup, fixture.atlasManifest, "spawnMk", 1);
        SeedAtlasEntry(fixture.pairingLookup, fixture.atlasManifest, "unitTpl", 2);

        AppendMarkerInstance(fixture.placements, 2.0f, 2.0f, 0, Params::MarkerCategory::Alloys, "alloyMk");
        AppendMarkerInstance(fixture.placements, 2.0f, 2.0f, 1, Params::MarkerCategory::Spawn, "spawnMk");
        fixture.ruleBucketIndex.markers.Build(fixture.placements.markers.ruleIndex.data(), 2, 2);

        Data::PlacementInstance unitInstance;
        unitInstance.positionX = 2.0f; unitInstance.positionZ = 2.0f;
        unitInstance.scaleX = unitInstance.scaleY = unitInstance.scaleZ = 1.0f;
        unitInstance.ruleIndex = 0;
        unitInstance.templateIdentifier = Data::MakeTemplateIdentifier("unitTpl");
        fixture.placements.units.Append(unitInstance);
        fixture.ruleBucketIndex.units.Build(fixture.placements.units.ruleIndex.data(), 1, 1);

        OverlayLayer_UI alloyLayer;
        alloyLayer.domainKind = OverlayDomainKind_UI::Alloy;
        alloyLayer.thumbnailLodThresholdPixels = 1.0f;
        alloyLayer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, 0, true});

        OverlayLayer_UI spawnsArmiesLayer;
        spawnsArmiesLayer.domainKind = OverlayDomainKind_UI::SpawnsArmies;
        spawnsArmiesLayer.thumbnailLodThresholdPixels = 1.0f;
        spawnsArmiesLayer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, 1, true});

        OverlayLayer_UI unitsLayer;
        unitsLayer.domainKind = OverlayDomainKind_UI::Units;
        unitsLayer.thumbnailLodThresholdPixels = 1.0f;
        unitsLayer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, 0, true});

        fixture.overlaySettings.overlayLayers = {alloyLayer, spawnsArmiesLayer, unitsLayer};
    }
};

// index 0 = Alloy, 1 = SpawnsArmies, 2 = Units — DomainToggleFixture's own construction order above.
void CheckTogglingOneDomainHidesOnlyItsOwnCandidate(std::size_t toggledRowIndex,
                                                    const char* toggledDomainLabel) {
    DomainToggleFixture domainFixture;
    std::vector<OverlayVisibleInstance> allEnabledCandidates;
    ResolveVisibleCandidates(domainFixture.fixture.Input(), domainFixture.fixture.aabbCache, nullptr,
                             allEnabledCandidates);
    check(allEnabledCandidates.size() == 3u, "all three domains enabled: one candidate apiece");

    domainFixture.fixture.overlaySettings.overlayLayers[toggledRowIndex].bEnabled = false;
    std::vector<OverlayVisibleInstance> toggledCandidates;
    ResolveVisibleCandidates(domainFixture.fixture.Input(), domainFixture.fixture.aabbCache, nullptr,
                             toggledCandidates);
    check(toggledCandidates.size() == 2u, toggledDomainLabel);

    // Toggling visibility never mutates Data::PlacementInstances — reconfirmed directly, not only
    // trusted from DrawOverlayIconLayersInput's const* field types.
    check(domainFixture.fixture.placements.markers.Count() == 2u,
          "toggling a domain's visibility never mutates the marker instance count");
    check(domainFixture.fixture.placements.units.Count() == 1u,
          "toggling a domain's visibility never mutates the unit instance count");
}

void CheckTogglingAlloyHidesOnlyAlloy() {
    CheckTogglingOneDomainHidesOnlyItsOwnCandidate(0,
        "disabling Alloy drops exactly its own one candidate; SpawnsArmies and Units stay");
}

void CheckTogglingSpawnsArmiesHidesOnlySpawnsArmies() {
    CheckTogglingOneDomainHidesOnlyItsOwnCandidate(1,
        "disabling SpawnsArmies drops exactly its own one candidate; Alloy and Units stay");
}

void CheckTogglingUnitsHidesOnlyUnits() {
    CheckTogglingOneDomainHidesOnlyItsOwnCandidate(2,
        "disabling Units drops exactly its own one candidate; Alloy and SpawnsArmies stay");
}

} // namespace

void RunMapCanvasIconLayerOverlayDomainToggleChecks() {
    CheckTogglingAlloyHidesOnlyAlloy();
    CheckTogglingSpawnsArmiesHidesOnlySpawnsArmies();
    CheckTogglingUnitsHidesOnlyUnits();
}

} // namespace Ui
} // namespace SanmapGen
