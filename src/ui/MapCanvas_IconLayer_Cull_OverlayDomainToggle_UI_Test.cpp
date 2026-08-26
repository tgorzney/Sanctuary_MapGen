// MapCanvas_IconLayer_Cull_OverlayDomainToggle_UI_Test.cpp — STEP201 acceptance, item 1: toggling
// one OverlayLayer_UI::bEnabled off through ResolveVisibleCandidates changes ONLY that domain's
// candidate output, leaving every other enabled domain's candidate count unchanged. Covers all
// three domains STEP201's own routing-ambiguity question turned on: Alloy and SpawnsArmies (both
// markers, ARCH_14_06_OverlayDomainKindCoexistence.md's 2-way category re-slice of ONE collection)
// and Units (a fully separate Data::PlacementResults collection, per
// ARCH_14_02_DataModel.md §14.2's own binding sub-layer-to-data table — SeedUnitsManualSubLayers/
// SeedUnitsProceduralSubLayers over recipe.armies/unitRules, Application_OverlaySetup_Seed_UI.cpp,
// never SeedMarkerDomains; ARCH_14_16_PerArmyUnitsOverlayRows.md §14.16 further splits Units into
// one row PER ARMY, proven by the per-army fixture/checks below). DrawOverlayIconLayersInput hands
// ResolveVisibleCandidates every DATA pointer
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
        // STEP122: this fixture tests domain-toggle isolation, not scale composition — pin
        // scaleAlloy/scaleSpawn to 1.0f so GlobalMarkerSettings' own real 0.17f default (STEP122's
        // flagged, deliberate product value) doesn't drop these markers' thumbnailScreenSize below
        // thumbnailLodThresholdPixels(1.0f) and flip them into strategic mode (no icon seeded there,
        // so a miss draws nothing — unrelated to the toggle behavior under test).
        fixture.recipe.globalMarkerSettings.scaleAlloy = 1.0f;
        fixture.recipe.globalMarkerSettings.scaleSpawn = 1.0f;

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

// ARCH_14_16_PerArmyUnitsOverlayRows.md §14.16-A/B/C: two Units-domain rows, one per army, each
// carrying BOTH a Manual sub-layer (the army's own hand-placed UnitGroup) and a ProceduralRule
// sub-layer (a units.armyIndex-tagged scattered instance) — proving the row split isolates BOTH
// kinds of unit at once, not just one.
void SetUnitTemplateIdentifier(char (&buffer)[8], const char* text) {
    int index = 0;
    for (; index < 7 && text[index] != '\0'; ++index) buffer[index] = text[index];
    for (; index < 8; ++index) buffer[index] = '\0';
}

struct PerArmyUnitsFixture {
    IconLayerTestFixture fixture;

    PerArmyUnitsFixture() {
        SeedAtlasEntry(fixture.pairingLookup, fixture.atlasManifest, "manUnt0", 0);
        SeedAtlasEntry(fixture.pairingLookup, fixture.atlasManifest, "manUnt1", 1);
        SeedAtlasEntry(fixture.pairingLookup, fixture.atlasManifest, "prcUnt0", 2);
        SeedAtlasEntry(fixture.pairingLookup, fixture.atlasManifest, "prcUnt1", 3);

        Params::Army armyZero;
        armyZero.armyColor[0] = 1.0f; armyZero.armyColor[1] = 0.0f;
        armyZero.armyColor[2] = 0.0f; armyZero.armyColor[3] = 1.0f;   // red
        Params::UnitGroup groupZero;
        Params::UnitTransform unitZero;
        unitZero.positionX = 2.0f; unitZero.positionZ = 2.0f; unitZero.scaleX = 1.0f;
        SetUnitTemplateIdentifier(unitZero.templateIdentifier, "manUnt0");
        groupZero.units.push_back(unitZero);
        armyZero.groups.push_back(groupZero);

        Params::Army armyOne;
        armyOne.armyColor[0] = 0.0f; armyOne.armyColor[1] = 0.0f;
        armyOne.armyColor[2] = 1.0f; armyOne.armyColor[3] = 1.0f;   // blue
        Params::UnitGroup groupOne;
        Params::UnitTransform unitOne;
        unitOne.positionX = 2.0f; unitOne.positionZ = 2.0f; unitOne.scaleX = 1.0f;
        SetUnitTemplateIdentifier(unitOne.templateIdentifier, "manUnt1");
        groupOne.units.push_back(unitOne);
        armyOne.groups.push_back(groupOne);

        fixture.recipe.armies = {armyZero, armyOne};

        AppendUnitInstance(fixture.placements, 2.0f, 2.0f, /*ruleIndex*/0, /*armyIndex*/0, "prcUnt0");
        AppendUnitInstance(fixture.placements, 2.0f, 2.0f, /*ruleIndex*/1, /*armyIndex*/1, "prcUnt1");
        fixture.ruleBucketIndex.units.Build(fixture.placements.units.ruleIndex.data(), 2, 2);

        // Manual sub-layer index is the GLOBAL flat index over recipe.armies[*].groups
        // (ResolveUnitsManualSubLayer, unchanged by §14.16-A) — army0's one group is flat index 0,
        // army1's one group is flat index 1.
        OverlayLayer_UI unitsRowArmyZero;
        unitsRowArmyZero.domainKind = OverlayDomainKind_UI::Units;
        unitsRowArmyZero.thumbnailLodThresholdPixels = 1.0f;
        unitsRowArmyZero.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::Manual, 0, true});
        unitsRowArmyZero.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, 0, true});

        OverlayLayer_UI unitsRowArmyOne;
        unitsRowArmyOne.domainKind = OverlayDomainKind_UI::Units;
        unitsRowArmyOne.thumbnailLodThresholdPixels = 1.0f;
        unitsRowArmyOne.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::Manual, 1, true});
        unitsRowArmyOne.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, 1, true});

        fixture.overlaySettings.overlayLayers = {unitsRowArmyZero, unitsRowArmyOne};
    }
};

void CheckTogglingOneArmyRowHidesOnlyThatArmysManualAndProceduralUnits() {
    PerArmyUnitsFixture perArmyFixture;
    std::vector<OverlayVisibleInstance> allEnabledCandidates;
    ResolveVisibleCandidates(perArmyFixture.fixture.Input(), perArmyFixture.fixture.aabbCache, nullptr,
                             allEnabledCandidates);
    check(allEnabledCandidates.size() == 4u,
          "both armies enabled: one Manual + one ProceduralRule candidate apiece = 4");

    perArmyFixture.fixture.overlaySettings.overlayLayers[0].bEnabled = false;   // army0's row
    std::vector<OverlayVisibleInstance> armyZeroHiddenCandidates;
    ResolveVisibleCandidates(perArmyFixture.fixture.Input(), perArmyFixture.fixture.aabbCache, nullptr,
                             armyZeroHiddenCandidates);
    check(armyZeroHiddenCandidates.size() == 2u,
          "disabling army0's row drops BOTH its Manual and ProceduralRule units, leaving army1's 2");
    for (const OverlayVisibleInstance& instance : armyZeroHiddenCandidates)
        check(instance.layerIndex == 1, "every surviving candidate belongs to army1's row (layerIndex 1)");

    check(perArmyFixture.fixture.placements.units.Count() == 2u,
          "toggling a Units row never mutates the underlying unit instance count");
}

// ARCH_14_16_PerArmyUnitsOverlayRows.md §14.16-C: each Units icon's tint is its OWN army's real
// armyColor, not a shared session default — covers both Manual and ProceduralRule sub-layers.
void CheckPerArmyUnitsRenderWithOwningArmysColor() {
    PerArmyUnitsFixture perArmyFixture;
    std::vector<OverlayVisibleInstance> candidates;
    ResolveVisibleCandidates(perArmyFixture.fixture.Input(), perArmyFixture.fixture.aabbCache, nullptr, candidates);
    check(candidates.size() == 4u, "four candidates: army0's Manual+Procedural, army1's Manual+Procedural");

    int armyZeroTintedCount = 0, armyOneTintedCount = 0;
    for (const OverlayVisibleInstance& instance : candidates) {
        if (instance.layerIndex == 0) {
            check(instance.tintColorRed == 1.0f && instance.tintColorGreen == 0.0f && instance.tintColorBlue == 0.0f,
                  "army0's icons (Manual and Procedural alike) tint with army0's red armyColor");
            ++armyZeroTintedCount;
        } else if (instance.layerIndex == 1) {
            check(instance.tintColorRed == 0.0f && instance.tintColorGreen == 0.0f && instance.tintColorBlue == 1.0f,
                  "army1's icons (Manual and Procedural alike) tint with army1's blue armyColor");
            ++armyOneTintedCount;
        }
    }
    check(armyZeroTintedCount == 2 && armyOneTintedCount == 2,
          "both sub-layer kinds counted for each army's row");
}

} // namespace

void RunMapCanvasIconLayerOverlayDomainToggleChecks() {
    CheckTogglingAlloyHidesOnlyAlloy();
    CheckTogglingSpawnsArmiesHidesOnlySpawnsArmies();
    CheckTogglingUnitsHidesOnlyUnits();
    CheckTogglingOneArmyRowHidesOnlyThatArmysManualAndProceduralUnits();
    CheckPerArmyUnitsRenderWithOwningArmysColor();
}

} // namespace Ui
} // namespace SanmapGen
