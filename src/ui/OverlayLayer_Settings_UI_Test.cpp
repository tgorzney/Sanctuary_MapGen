// OverlayLayer_Settings_UI_Test.cpp — STEP51 acceptance: the six-domain overlay stack's struct
// defaults, its launch-time default seeding against a real `Params::MapRecipe`, and
// `ResolveUnitsManualSubLayer`'s round-trip. Pure unit tests; no imgui frame, no window, no GL
// context — same posture `SlopeTab_UI_Test.cpp` already has.
#include "Application_Defaults_UI.h"
#include <cstdio>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

void RunStructDefaultChecks() {
    OverlayLayer_UI layer;
    Check(layer.bEnabled, "a default OverlayLayer_UI is enabled");
    Check(layer.opacity == 1.0f, "default opacity is 1.0");
    Check(layer.thumbnailLodThresholdPixels == 5.0f, "default LOD threshold is 5.0");
    Check(layer.subLayers.empty(), "no sub-layers by default");

    OverlaySubLayerRef_UI ref{OverlaySubLayerKind_UI::Manual, 3};
    Check(ref.bEnabled, "a default-constructed sub-layer ref is enabled");
}

void RunDefaultSeedingChecks() {
    const Params::MapRecipe recipe = MakeDefaultMapRecipe();
    OverlayLayerSettings overlaySettings;
    ConfigureDefaultOverlayLayers(overlaySettings, recipe);

    // ARCH_14_16_PerArmyUnitsOverlayRows.md §14.16-A: Units seeds one row PER ARMY, not one shared
    // row — the default recipe has zero armies, so it contributes zero Units rows here (five total,
    // not six).
    Check(overlaySettings.overlayLayers.size() == 5,
          "five rows: Alloy, SpawnsArmies, zero Units rows (no armies), Props, Reclaim, Decals");
    const OverlayDomainKind_UI expectedOrder[5] = {
        OverlayDomainKind_UI::Alloy, OverlayDomainKind_UI::SpawnsArmies,
        OverlayDomainKind_UI::Props, OverlayDomainKind_UI::Reclaim, OverlayDomainKind_UI::Decals};
    for (int i = 0; i < 5; ++i)
        Check(overlaySettings.overlayLayers[static_cast<std::size_t>(i)].domainKind == expectedOrder[i],
              "domain order/kind matches Alloy, SpawnsArmies, Props, Reclaim, Decals (no armies -> no Units rows)");

    const OverlayLayer_UI& alloyLayer        = overlaySettings.overlayLayers[0];
    const OverlayLayer_UI& spawnsArmiesLayer = overlaySettings.overlayLayers[1];
    const OverlayLayer_UI& propsLayer        = overlaySettings.overlayLayers[2];
    const OverlayLayer_UI& reclaimLayer      = overlaySettings.overlayLayers[3];
    const OverlayLayer_UI& decalsLayer       = overlaySettings.overlayLayers[4];

    Check(spawnsArmiesLayer.subLayers.size() == 1
          && spawnsArmiesLayer.subLayers[0].kind == OverlaySubLayerKind_UI::ProceduralRule
          && spawnsArmiesLayer.subLayers[0].index == 0,
          "the default recipe's one Spawn markerRule seeds SpawnsArmies with {ProceduralRule, 0}");
    Check(alloyLayer.subLayers.empty(), "Alloy gets nothing from the default recipe's only (Spawn) rule");

    Check(propsLayer.subLayers.size() == 1
          && propsLayer.subLayers[0].kind == OverlaySubLayerKind_UI::ProceduralRule
          && propsLayer.subLayers[0].index == 0,
          "the default recipe's one propRule seeds Props with {ProceduralRule, 0}, zero Manual");

    Check(reclaimLayer.subLayers.empty(), "Reclaim always stays empty — no data/rule yet");
    Check(decalsLayer.subLayers.empty(), "no decalRules/decalLayers in the default recipe");
}

// ARCH_14_16_PerArmyUnitsOverlayRows.md §14.16-A/D: N armies -> N Units-domain rows, named via
// ArmyRowLabel (displayName falls back to name falls back to "Army"), in roster order, seeded with
// the D-palette default color the moment Add Army/import backfill would give them (proven
// separately below/in ArmiesTab_UI_Test.cpp and MapImporter_Armies_IO_Test.cpp — this checks only
// the row-seeding shape).
void RunPerArmyUnitsRowSeedingChecks() {
    Params::MapRecipe recipe;
    Params::Army namedArmy;   namedArmy.displayName = "Alpha Strike";
    Params::Army fallbackArmy; fallbackArmy.name = "ARMY_02";   // empty displayName -> falls back to name
    Params::Army blankArmy;                                     // both empty -> falls back to "Army"
    recipe.armies = {namedArmy, fallbackArmy, blankArmy};

    OverlayLayerSettings overlaySettings;
    ConfigureDefaultOverlayLayers(overlaySettings, recipe);

    // Alloy(0), SpawnsArmies(1), Units x3 (2,3,4), Props(5), Reclaim(6), Decals(7).
    Check(overlaySettings.overlayLayers.size() == 8,
          "3 armies -> exactly 3 Units-domain rows, 8 rows total");
    for (std::size_t index = 2; index <= 4; ++index)
        Check(overlaySettings.overlayLayers[index].domainKind == OverlayDomainKind_UI::Units,
              "rows 2..4 are all Units-domain, one per army, in roster order");
    Check(overlaySettings.overlayLayers[2].name == "Alpha Strike", "row 0's name is ArmyRowLabel's displayName branch");
    Check(overlaySettings.overlayLayers[3].name == "ARMY_02", "row 1's name is ArmyRowLabel's name fallback");
    Check(overlaySettings.overlayLayers[4].name == "Army", "row 2's name is ArmyRowLabel's final \"Army\" fallback");
    Check(overlaySettings.overlayLayers[5].domainKind == OverlayDomainKind_UI::Props, "Props still follows the Units rows");
    Check(overlaySettings.overlayLayers[6].domainKind == OverlayDomainKind_UI::Reclaim, "Reclaim follows Props");
    Check(overlaySettings.overlayLayers[7].domainKind == OverlayDomainKind_UI::Decals, "Decals follows Reclaim");
}

// ARCH_14_16_PerArmyUnitsOverlayRows.md §14.16-B: a procedural UnitRule with an out-of-range
// armyIndex is dropped silently — attached to no row, never crashing.
void RunProceduralUnitOutOfRangeArmyIndexChecks() {
    Params::MapRecipe recipe;
    recipe.armies.assign(2, Params::Army());
    Params::UnitRule inRangeRule;  inRangeRule.armyIndex  = 1;
    Params::UnitRule negativeRule; negativeRule.armyIndex = -1;
    Params::UnitRule tooHighRule;  tooHighRule.armyIndex  = 2;   // == recipe.armies.size(), out of range
    recipe.unitRules = {inRangeRule, negativeRule, tooHighRule};

    OverlayLayerSettings overlaySettings;
    ConfigureDefaultOverlayLayers(overlaySettings, recipe);

    // Alloy(0), SpawnsArmies(1), Units army0(2), Units army1(3), Props(4), Reclaim(5), Decals(6).
    Check(overlaySettings.overlayLayers.size() == 7, "2 armies -> 2 Units rows, 7 rows total");
    const OverlayLayer_UI& armyZeroUnits = overlaySettings.overlayLayers[2];
    const OverlayLayer_UI& armyOneUnits  = overlaySettings.overlayLayers[3];
    Check(armyZeroUnits.subLayers.empty(), "army0 gets no procedural ref: the only in-range rule targets army1");
    Check(armyOneUnits.subLayers.size() == 1
          && armyOneUnits.subLayers[0].kind == OverlaySubLayerKind_UI::ProceduralRule
          && armyOneUnits.subLayers[0].index == 0,
          "army1 gets exactly {ProceduralRule, 0} — the in-range rule, by its recipe.unitRules position");
    for (const OverlayLayer_UI& layer : overlaySettings.overlayLayers) {
        for (const OverlaySubLayerRef_UI& ref : layer.subLayers)
            Check(!(ref.kind == OverlaySubLayerKind_UI::ProceduralRule && (ref.index == 1 || ref.index == 2)),
                  "the negative- and too-high-armyIndex rules (recipe.unitRules[1],[2]) attach to no row");
    }
}

void RunCategorySplitChecks() {
    Params::MapRecipe recipe;
    Params::MarkerRuleLayer layer0;
    Params::MarkerRule spawnRule;   spawnRule.category  = Params::MarkerCategory::Spawn;
    Params::MarkerRule alloysRule;  alloysRule.category = Params::MarkerCategory::Alloys;
    Params::MarkerRule genericRule; genericRule.category = Params::MarkerCategory::Generic;
    layer0.rules = {spawnRule, alloysRule, genericRule};
    recipe.markerRuleLayers.push_back(layer0);

    OverlayLayerSettings overlaySettings;
    ConfigureDefaultOverlayLayers(overlaySettings, recipe);
    const OverlayLayer_UI& alloyLayer        = overlaySettings.overlayLayers[0];
    const OverlayLayer_UI& spawnsArmiesLayer = overlaySettings.overlayLayers[1];

    Check(spawnsArmiesLayer.subLayers.size() == 1 && spawnsArmiesLayer.subLayers[0].index == 0,
          "SpawnsArmies gets exactly the Spawn-category rule, at flat index 0");
    Check(alloyLayer.subLayers.size() == 2
          && alloyLayer.subLayers[0].index == 1 && alloyLayer.subLayers[1].index == 2,
          "Alloy gets the Alloys+Generic rules, order-preserved, at flat indices 1 and 2");

    // A second layer's rules continue the flat index rather than resetting per layer.
    Params::MapRecipe twoLayerRecipe;
    Params::MarkerRuleLayer layerA; layerA.rules = {genericRule, alloysRule};   // indices 0,1
    Params::MarkerRuleLayer layerB; layerB.rules = {spawnRule};                 // index 2
    twoLayerRecipe.markerRuleLayers = {layerA, layerB};

    OverlayLayerSettings twoLayerSettings;
    ConfigureDefaultOverlayLayers(twoLayerSettings, twoLayerRecipe);
    const OverlayLayer_UI& alloyLayer2        = twoLayerSettings.overlayLayers[0];
    const OverlayLayer_UI& spawnsArmiesLayer2 = twoLayerSettings.overlayLayers[1];
    Check(alloyLayer2.subLayers.size() == 2
          && alloyLayer2.subLayers[0].index == 0 && alloyLayer2.subLayers[1].index == 1,
          "layer A's two non-Spawn rules land at flat indices 0 and 1");
    Check(spawnsArmiesLayer2.subLayers.size() == 1 && spawnsArmiesLayer2.subLayers[0].index == 2,
          "layer B's Spawn rule continues the flat index at 2, not resetting to 0 per layer");
}

void RunManualProceduralOrderingChecks() {
    Params::MapRecipe recipe;
    recipe.propLayers.assign(2, Params::PropInstanceLayer());
    recipe.propRules.assign(3, Params::PropRule());
    recipe.decalLayers.assign(2, Params::DecalInstanceLayer());
    recipe.decalRules.assign(3, Params::DecalRule());

    OverlayLayerSettings overlaySettings;
    ConfigureDefaultOverlayLayers(overlaySettings, recipe);
    // No armies in this fixture -> zero Units rows: Alloy(0), SpawnsArmies(1), Props(2), Reclaim(3), Decals(4).
    const OverlayLayer_UI& propsLayer  = overlaySettings.overlayLayers[2];
    const OverlayLayer_UI& decalsLayer = overlaySettings.overlayLayers[4];

    auto checkManualThenProcedural = [](const OverlayLayer_UI& layerToCheck, const char* label) {
        Check(layerToCheck.subLayers.size() == 5, label);
        const OverlaySubLayerKind_UI expectedKind[5] = {
            OverlaySubLayerKind_UI::Manual, OverlaySubLayerKind_UI::Manual,
            OverlaySubLayerKind_UI::ProceduralRule, OverlaySubLayerKind_UI::ProceduralRule,
            OverlaySubLayerKind_UI::ProceduralRule};
        const int expectedIndex[5] = {0, 1, 0, 1, 2};
        for (int i = 0; i < 5; ++i) {
            Check(layerToCheck.subLayers[static_cast<std::size_t>(i)].kind == expectedKind[i], label);
            Check(layerToCheck.subLayers[static_cast<std::size_t>(i)].index == expectedIndex[i], label);
        }
    };
    checkManualThenProcedural(propsLayer, "Props: 2 Manual then 3 ProceduralRule, each own-index");
    checkManualThenProcedural(decalsLayer, "Decals: 2 Manual then 3 ProceduralRule, each own-index");
}

// STEP83 §8 — the seed-time Props/Reclaim partition. Four propRules, alternating bReclaimable, so
// the partition, the trap (per-domain renumbering), the mutual-exclusivity, and the two all-one-way
// edges are each independently provable from one fixture family.
void RunPropReclaimPartitionChecks() {
    Params::MapRecipe recipe;
    recipe.propRules.assign(4, Params::PropRule());
    recipe.propRules[0].bReclaimable = false;
    recipe.propRules[1].bReclaimable = true;
    recipe.propRules[2].bReclaimable = false;
    recipe.propRules[3].bReclaimable = true;

    OverlayLayerSettings overlaySettings;
    ConfigureDefaultOverlayLayers(overlaySettings, recipe);
    // No armies in this fixture -> zero Units rows: Alloy(0), SpawnsArmies(1), Props(2), Reclaim(3), Decals(4).
    const OverlayLayer_UI& propsLayer   = overlaySettings.overlayLayers[2];
    const OverlayLayer_UI& reclaimLayer = overlaySettings.overlayLayers[3];

    // The indices ARE the assertion: a per-domain renumbering bug would produce {0,1} here instead
    // of the real recipe.propRules positions {0,2}/{1,3}.
    Check(propsLayer.subLayers.size() == 2
          && propsLayer.subLayers[0].kind == OverlaySubLayerKind_UI::ProceduralRule
          && propsLayer.subLayers[0].index == 0
          && propsLayer.subLayers[1].index == 2,
          "Props' procedural refs are exactly {ProceduralRule,0},{ProceduralRule,2} — global indices, not renumbered");
    Check(reclaimLayer.subLayers.size() == 2
          && reclaimLayer.subLayers[0].kind == OverlaySubLayerKind_UI::ProceduralRule
          && reclaimLayer.subLayers[0].index == 1
          && reclaimLayer.subLayers[1].index == 3,
          "Reclaim's procedural refs are exactly {ProceduralRule,1},{ProceduralRule,3} — global indices, not renumbered");

    // Mutual exclusivity, mechanically: every recipe.propRules index appears in exactly one set,
    // union covers [0, propRules.size()) with no gap, order-preserving within each domain.
    bool seenInProps[4]   = {false, false, false, false};
    bool seenInReclaim[4] = {false, false, false, false};
    for (const OverlaySubLayerRef_UI& ref : propsLayer.subLayers)   seenInProps[ref.index]   = true;
    for (const OverlaySubLayerRef_UI& ref : reclaimLayer.subLayers) seenInReclaim[ref.index] = true;
    for (int i = 0; i < 4; ++i) {
        Check(seenInProps[i] != seenInReclaim[i],
              "each recipe.propRules index appears in exactly one of Props/Reclaim, never both, never neither");
    }

    // Edge: all four reclaimable.
    Params::MapRecipe allReclaimable;
    allReclaimable.propRules.assign(4, Params::PropRule());
    for (Params::PropRule& rule : allReclaimable.propRules) rule.bReclaimable = true;
    OverlayLayerSettings allReclaimableSettings;
    ConfigureDefaultOverlayLayers(allReclaimableSettings, allReclaimable);
    Check(allReclaimableSettings.overlayLayers[2].subLayers.empty(),
          "all-reclaimable: Props gets zero procedural refs");
    Check(allReclaimableSettings.overlayLayers[3].subLayers.size() == 4,
          "all-reclaimable: Reclaim gets all four procedural refs");
    Check(allReclaimableSettings.overlayLayers.size() == 5,
          "all-reclaimable: no armies -> zero Units rows; Props and Reclaim rows still exist for the View toolbar");

    // Edge: all four non-reclaimable.
    Params::MapRecipe noneReclaimable;
    noneReclaimable.propRules.assign(4, Params::PropRule());
    OverlayLayerSettings noneReclaimableSettings;
    ConfigureDefaultOverlayLayers(noneReclaimableSettings, noneReclaimable);
    Check(noneReclaimableSettings.overlayLayers[2].subLayers.size() == 4,
          "none-reclaimable: Props gets all four procedural refs");
    Check(noneReclaimableSettings.overlayLayers[3].subLayers.empty(),
          "none-reclaimable: Reclaim gets zero procedural refs");
    Check(noneReclaimableSettings.overlayLayers.size() == 5,
          "none-reclaimable: no armies -> zero Units rows; Props and Reclaim rows still exist for the View toolbar");

    // Manual refs go to BOTH domains, Manual-before-Procedural order preserved in both.
    Params::MapRecipe manualRecipe;
    manualRecipe.propLayers.assign(3, Params::PropInstanceLayer());
    OverlayLayerSettings manualSettings;
    ConfigureDefaultOverlayLayers(manualSettings, manualRecipe);
    auto checkThreeManualRefs = [](const OverlayLayer_UI& layerToCheck, const char* label) {
        Check(layerToCheck.subLayers.size() == 3, label);
        for (int i = 0; i < 3; ++i) {
            Check(layerToCheck.subLayers[static_cast<std::size_t>(i)].kind == OverlaySubLayerKind_UI::Manual, label);
            Check(layerToCheck.subLayers[static_cast<std::size_t>(i)].index == i, label);
        }
    };
    checkThreeManualRefs(manualSettings.overlayLayers[2], "Props carries [{Manual,0},{Manual,1},{Manual,2}]");
    checkThreeManualRefs(manualSettings.overlayLayers[3], "Reclaim carries [{Manual,0},{Manual,1},{Manual,2}]");

    // Unchanged domains: Alloy/SpawnsArmies/Decals are byte-identical to STEP51's own seeding (none
    // of these fixtures touch markerRuleLayers/unitRules/decalRules/decalLayers); Units stays at
    // zero ROWS (not merely empty sub-layers) since none of these fixtures populate recipe.armies.
    for (const OverlayLayerSettings* settings : {&overlaySettings, &allReclaimableSettings,
                                                  &noneReclaimableSettings, &manualSettings}) {
        Check(settings->overlayLayers[0].subLayers.empty(), "Alloy stays empty across every fixture above");
        Check(settings->overlayLayers[1].subLayers.empty(), "SpawnsArmies stays empty across every fixture above");
        Check(settings->overlayLayers.size() == 5, "no armies in these fixtures: zero Units rows, 5 total rows");
        Check(settings->overlayLayers[4].subLayers.empty(), "Decals stays empty across every fixture above");
    }
}

// STEP97 (ARCH_14_14) — Manual Alloy/SpawnsArmies routing, per-transform existence check, not
// SeedPropReclaimDomains' unconditional dual-push. Mirrors RunPropReclaimPartitionChecks' fixture
// shape while proving the deliberately-different "existence-checked" behavior.
void RunMarkerManualPartitionChecks() {
    // 1. Pure-Spawn layer: one markerLayers entry, one Spawn group with two transforms at
    // layerIndex 0. Only SpawnsArmies gets the Manual ref.
    {
        Params::MapRecipe recipe;
        recipe.markerLayers.assign(1, Params::MarkerInstanceLayer());
        Params::MarkerInstanceGroup spawnGroup; spawnGroup.name = "Spawn";
        spawnGroup.transforms.assign(2, Params::MarkerTransform());
        spawnGroup.transforms[0].layerIndex = 0;
        spawnGroup.transforms[1].layerIndex = 0;
        recipe.markers = {spawnGroup};

        OverlayLayerSettings overlaySettings;
        ConfigureDefaultOverlayLayers(overlaySettings, recipe);
        const OverlayLayer_UI& alloyLayer        = overlaySettings.overlayLayers[0];
        const OverlayLayer_UI& spawnsArmiesLayer = overlaySettings.overlayLayers[1];
        Check(spawnsArmiesLayer.subLayers.size() == 1
              && spawnsArmiesLayer.subLayers[0].kind == OverlaySubLayerKind_UI::Manual
              && spawnsArmiesLayer.subLayers[0].index == 0,
              "pure-Spawn layer: SpawnsArmies gets exactly {Manual, 0}");
        Check(alloyLayer.subLayers.empty(), "pure-Spawn layer: Alloy gets nothing");
    }

    // 2. Pure-non-Spawn layer: same shape, group name "Alloys". Mirror image of case 1.
    {
        Params::MapRecipe recipe;
        recipe.markerLayers.assign(1, Params::MarkerInstanceLayer());
        Params::MarkerInstanceGroup alloysGroup; alloysGroup.name = "Alloys";
        alloysGroup.transforms.assign(2, Params::MarkerTransform());
        alloysGroup.transforms[0].layerIndex = 0;
        alloysGroup.transforms[1].layerIndex = 0;
        recipe.markers = {alloysGroup};

        OverlayLayerSettings overlaySettings;
        ConfigureDefaultOverlayLayers(overlaySettings, recipe);
        const OverlayLayer_UI& alloyLayer        = overlaySettings.overlayLayers[0];
        const OverlayLayer_UI& spawnsArmiesLayer = overlaySettings.overlayLayers[1];
        Check(alloyLayer.subLayers.size() == 1
              && alloyLayer.subLayers[0].kind == OverlaySubLayerKind_UI::Manual
              && alloyLayer.subLayers[0].index == 0,
              "pure-non-Spawn layer: Alloy gets exactly {Manual, 0}");
        Check(spawnsArmiesLayer.subLayers.empty(), "pure-non-Spawn layer: SpawnsArmies gets nothing");
    }

    // 3. Mixed layer — the case STEP97/ARCH_14_14 exists for: one markerLayers entry, two groups
    // ("Spawn" and "Alloys") each contributing one transform at layerIndex 0. Both domains get the
    // same {Manual, 0} ref — legal, not a bug.
    {
        Params::MapRecipe recipe;
        recipe.markerLayers.assign(1, Params::MarkerInstanceLayer());
        Params::MarkerInstanceGroup spawnGroup; spawnGroup.name = "Spawn";
        spawnGroup.transforms.assign(1, Params::MarkerTransform());
        spawnGroup.transforms[0].layerIndex = 0;
        Params::MarkerInstanceGroup alloysGroup; alloysGroup.name = "Alloys";
        alloysGroup.transforms.assign(1, Params::MarkerTransform());
        alloysGroup.transforms[0].layerIndex = 0;
        recipe.markers = {spawnGroup, alloysGroup};

        OverlayLayerSettings overlaySettings;
        ConfigureDefaultOverlayLayers(overlaySettings, recipe);
        const OverlayLayer_UI& alloyLayer        = overlaySettings.overlayLayers[0];
        const OverlayLayer_UI& spawnsArmiesLayer = overlaySettings.overlayLayers[1];
        Check(alloyLayer.subLayers.size() == 1
              && alloyLayer.subLayers[0].kind == OverlaySubLayerKind_UI::Manual
              && alloyLayer.subLayers[0].index == 0,
              "mixed layer: Alloy also gets {Manual, 0}");
        Check(spawnsArmiesLayer.subLayers.size() == 1
              && spawnsArmiesLayer.subLayers[0].kind == OverlaySubLayerKind_UI::Manual
              && spawnsArmiesLayer.subLayers[0].index == 0,
              "mixed layer: SpawnsArmies also gets {Manual, 0} — legal dual membership");
    }

    // 4. Untouched layer: markerLayers[1] has no contributing transform at all (every transform in
    // this fixture uses layerIndex 0). Neither domain gets a {Manual, 1} ref — proves the
    // existence-check, not a blanket PushManualRefs(markerLayerCount).
    {
        Params::MapRecipe recipe;
        recipe.markerLayers.assign(2, Params::MarkerInstanceLayer());
        Params::MarkerInstanceGroup spawnGroup; spawnGroup.name = "Spawn";
        spawnGroup.transforms.assign(1, Params::MarkerTransform());
        spawnGroup.transforms[0].layerIndex = 0;
        recipe.markers = {spawnGroup};

        OverlayLayerSettings overlaySettings;
        ConfigureDefaultOverlayLayers(overlaySettings, recipe);
        const OverlayLayer_UI& alloyLayer        = overlaySettings.overlayLayers[0];
        const OverlayLayer_UI& spawnsArmiesLayer = overlaySettings.overlayLayers[1];
        for (const OverlaySubLayerRef_UI& ref : alloyLayer.subLayers)
            Check(ref.index != 1, "untouched layer: Alloy gets no {Manual, 1} ref");
        for (const OverlaySubLayerRef_UI& ref : spawnsArmiesLayer.subLayers)
            Check(ref.index != 1, "untouched layer: SpawnsArmies gets no {Manual, 1} ref");
    }

    // 5. Manual-before-Procedural ordering: the pure-Spawn recipe from case 1, extended with one
    // markerRuleLayers Spawn rule. SpawnsArmies must be [{Manual, 0}, {ProceduralRule, 0}].
    OverlayLayerSettings orderingSettings;
    {
        Params::MapRecipe recipe;
        recipe.markerLayers.assign(1, Params::MarkerInstanceLayer());
        Params::MarkerInstanceGroup spawnGroup; spawnGroup.name = "Spawn";
        spawnGroup.transforms.assign(2, Params::MarkerTransform());
        spawnGroup.transforms[0].layerIndex = 0;
        spawnGroup.transforms[1].layerIndex = 0;
        recipe.markers = {spawnGroup};

        Params::MarkerRuleLayer ruleLayer;
        Params::MarkerRule spawnRule; spawnRule.category = Params::MarkerCategory::Spawn;
        ruleLayer.rules = {spawnRule};
        recipe.markerRuleLayers = {ruleLayer};

        ConfigureDefaultOverlayLayers(orderingSettings, recipe);
        const OverlayLayer_UI& spawnsArmiesLayer = orderingSettings.overlayLayers[1];
        Check(spawnsArmiesLayer.subLayers.size() == 2
              && spawnsArmiesLayer.subLayers[0].kind == OverlaySubLayerKind_UI::Manual
              && spawnsArmiesLayer.subLayers[0].index == 0
              && spawnsArmiesLayer.subLayers[1].kind == OverlaySubLayerKind_UI::ProceduralRule
              && spawnsArmiesLayer.subLayers[1].index == 0,
              "Manual-before-Procedural: SpawnsArmies is [{Manual,0},{ProceduralRule,0}] in that order");
    }

    // 6. Unchanged-domain guard: Props/Reclaim/Decals stay empty across every fixture above (none
    // populate propRules/propLayers/decalRules/decalLayers); Units stays at zero ROWS (no armies
    // populated), so Alloy(0), SpawnsArmies(1), Props(2), Reclaim(3), Decals(4).
    Check(orderingSettings.overlayLayers.size() == 5, "no armies in this fixture: zero Units rows, 5 total rows");
    Check(orderingSettings.overlayLayers[2].subLayers.empty(), "unchanged-domain guard: Props stays empty");
    Check(orderingSettings.overlayLayers[3].subLayers.empty(), "unchanged-domain guard: Reclaim stays empty");
    Check(orderingSettings.overlayLayers[4].subLayers.empty(), "unchanged-domain guard: Decals stays empty");
}

// ARCH_14_16_PerArmyUnitsOverlayRows.md §14.16-A: ResolveUnitsManualSubLayer's global flat-index
// formula over recipe.armies[*].groups is completely UNCHANGED — this re-proves the exact same
// (flatIndex -> army, group) table STEP79 already established, only now each flat ref lands in the
// ROW belonging to that same resolved army, instead of one shared row.
void RunResolveUnitsManualSubLayerChecks() {
    Params::MapRecipe recipe;
    Params::Army armyZero; armyZero.groups.assign(2, Params::UnitGroup());
    Params::Army armyOne;                                              // zero groups
    Params::Army armyTwo; armyTwo.groups.assign(3, Params::UnitGroup());
    recipe.armies = {armyZero, armyOne, armyTwo};

    OverlayLayerSettings overlaySettings;
    ConfigureDefaultOverlayLayers(overlaySettings, recipe);
    // Alloy(0), SpawnsArmies(1), Units army0(2), Units army1(3), Units army2(4), Props(5), Reclaim(6), Decals(7).
    Check(overlaySettings.overlayLayers.size() == 8, "3 armies -> 3 Units rows, 8 rows total");
    const OverlayLayer_UI& unitsRowArmyZero = overlaySettings.overlayLayers[2];
    const OverlayLayer_UI& unitsRowArmyOne  = overlaySettings.overlayLayers[3];
    const OverlayLayer_UI& unitsRowArmyTwo  = overlaySettings.overlayLayers[4];

    Check(unitsRowArmyZero.subLayers.size() == 2
          && unitsRowArmyZero.subLayers[0].index == 0 && unitsRowArmyZero.subLayers[1].index == 1,
          "army0's own row carries its two groups, at the global flat indices 0 and 1");
    Check(unitsRowArmyOne.subLayers.empty(), "army1 (zero groups) contributes no sub-layer refs to its own row");
    Check(unitsRowArmyTwo.subLayers.size() == 3
          && unitsRowArmyTwo.subLayers[0].index == 2 && unitsRowArmyTwo.subLayers[1].index == 3
          && unitsRowArmyTwo.subLayers[2].index == 4,
          "army2's own row carries its three groups, continuing the global flat index at 2, 3, 4");

    struct Expectation { int flatIndex; int expectedArmy; int expectedGroup; };
    const Expectation expectations[5] = {
        {0, 0, 0}, {1, 0, 1}, {2, 2, 0}, {3, 2, 1}, {4, 2, 2}};
    for (const Expectation& expectation : expectations) {
        int resolvedArmy = -1, resolvedGroup = -1;
        const bool bResolved = ResolveUnitsManualSubLayer(recipe, expectation.flatIndex, resolvedArmy, resolvedGroup);
        Check(bResolved && resolvedArmy == expectation.expectedArmy && resolvedGroup == expectation.expectedGroup,
              "flat index resolves to the expected (army, group) pair — the resolution formula itself is unchanged");
    }

    int outArmy = -1, outGroup = -1;
    Check(!ResolveUnitsManualSubLayer(recipe, 5, outArmy, outGroup) && outArmy == -1 && outGroup == -1,
          "one past the last valid flat index fails and leaves outputs at -1");
    outArmy = -1; outGroup = -1;
    Check(!ResolveUnitsManualSubLayer(recipe, -1, outArmy, outGroup) && outArmy == -1 && outGroup == -1,
          "a negative flat index fails and leaves outputs at -1");
}

void RunReadWriteCorrectnessChecks() {
    const Params::MapRecipe recipe = MakeDefaultMapRecipe();
    OverlayLayerSettings overlaySettings;
    ConfigureDefaultOverlayLayers(overlaySettings, recipe);

    std::swap(overlaySettings.overlayLayers[0], overlaySettings.overlayLayers[1]);
    Check(overlaySettings.overlayLayers[0].domainKind == OverlayDomainKind_UI::SpawnsArmies
          && overlaySettings.overlayLayers[1].domainKind == OverlayDomainKind_UI::Alloy,
          "a reordering swap is visible through the same vector");

    overlaySettings.overlayLayers[0].bEnabled = false;
    Check(!overlaySettings.overlayLayers[0].bEnabled && overlaySettings.overlayLayers[1].bEnabled,
          "flipping one layer's bEnabled does not perturb another");

    overlaySettings.overlayLayers[0].opacity = 0.4f;
    Check(overlaySettings.overlayLayers[0].opacity == 0.4f && overlaySettings.overlayLayers[1].opacity == 1.0f,
          "an opacity edit is isolated to the edited layer");

    if (!overlaySettings.overlayLayers[0].subLayers.empty()) {
        overlaySettings.overlayLayers[0].subLayers[0].bEnabled = false;
        Check(!overlaySettings.overlayLayers[0].subLayers[0].bEnabled,
              "toggling a sub-layer's bEnabled is visible through the same accessor path");
    }

    overlaySettings.alloyAppearance.color[0]  = 0.25f;
    overlaySettings.alloyAppearance.iconScale = 2.0f;
    Check(overlaySettings.alloyAppearance.color[0] == 0.25f && overlaySettings.alloyAppearance.iconScale == 2.0f,
          "alloyAppearance color/iconScale write then read back correctly");
    Check(overlaySettings.spawnsArmiesAppearance.color[0] == 1.0f,
          "writing alloyAppearance does not perturb spawnsArmiesAppearance");
}

} // namespace

int main() {
    RunStructDefaultChecks();
    RunDefaultSeedingChecks();
    RunPerArmyUnitsRowSeedingChecks();
    RunProceduralUnitOutOfRangeArmyIndexChecks();
    RunCategorySplitChecks();
    RunManualProceduralOrderingChecks();
    RunPropReclaimPartitionChecks();
    RunMarkerManualPartitionChecks();
    RunResolveUnitsManualSubLayerChecks();
    RunReadWriteCorrectnessChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
