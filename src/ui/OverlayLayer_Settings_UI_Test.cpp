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

    Check(overlaySettings.overlayLayers.size() == 6, "six domains are always seeded");
    const OverlayDomainKind_UI expectedOrder[6] = {
        OverlayDomainKind_UI::Alloy, OverlayDomainKind_UI::SpawnsArmies, OverlayDomainKind_UI::Units,
        OverlayDomainKind_UI::Props, OverlayDomainKind_UI::Reclaim, OverlayDomainKind_UI::Decals};
    for (int i = 0; i < 6; ++i)
        Check(overlaySettings.overlayLayers[static_cast<std::size_t>(i)].domainKind == expectedOrder[i],
              "domain order/kind matches Alloy, SpawnsArmies, Units, Props, Reclaim, Decals");

    const OverlayLayer_UI& alloyLayer        = overlaySettings.overlayLayers[0];
    const OverlayLayer_UI& spawnsArmiesLayer = overlaySettings.overlayLayers[1];
    const OverlayLayer_UI& unitsLayer        = overlaySettings.overlayLayers[2];
    const OverlayLayer_UI& propsLayer        = overlaySettings.overlayLayers[3];
    const OverlayLayer_UI& reclaimLayer      = overlaySettings.overlayLayers[4];
    const OverlayLayer_UI& decalsLayer       = overlaySettings.overlayLayers[5];

    Check(spawnsArmiesLayer.subLayers.size() == 1
          && spawnsArmiesLayer.subLayers[0].kind == OverlaySubLayerKind_UI::ProceduralRule
          && spawnsArmiesLayer.subLayers[0].index == 0,
          "the default recipe's one Spawn markerRule seeds SpawnsArmies with {ProceduralRule, 0}");
    Check(alloyLayer.subLayers.empty(), "Alloy gets nothing from the default recipe's only (Spawn) rule");

    Check(propsLayer.subLayers.size() == 1
          && propsLayer.subLayers[0].kind == OverlaySubLayerKind_UI::ProceduralRule
          && propsLayer.subLayers[0].index == 0,
          "the default recipe's one propRule seeds Props with {ProceduralRule, 0}, zero Manual");

    Check(unitsLayer.subLayers.empty(), "no armies/unitRules in the default recipe");
    Check(reclaimLayer.subLayers.empty(), "Reclaim always stays empty — no data/rule yet");
    Check(decalsLayer.subLayers.empty(), "no decalRules/decalLayers in the default recipe");
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
    const OverlayLayer_UI& propsLayer  = overlaySettings.overlayLayers[3];
    const OverlayLayer_UI& decalsLayer = overlaySettings.overlayLayers[5];

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

void RunResolveUnitsManualSubLayerChecks() {
    Params::MapRecipe recipe;
    Params::Army armyZero; armyZero.groups.assign(2, Params::UnitGroup());
    Params::Army armyOne;                                              // zero groups
    Params::Army armyTwo; armyTwo.groups.assign(3, Params::UnitGroup());
    recipe.armies = {armyZero, armyOne, armyTwo};

    OverlayLayerSettings overlaySettings;
    ConfigureDefaultOverlayLayers(overlaySettings, recipe);
    const OverlayLayer_UI& unitsLayer = overlaySettings.overlayLayers[2];
    Check(unitsLayer.subLayers.size() == 5, "SeedUnitsManualSubLayers emits one ref per group, army1 contributes none");

    struct Expectation { int flatIndex; int expectedArmy; int expectedGroup; };
    const Expectation expectations[5] = {
        {0, 0, 0}, {1, 0, 1}, {2, 2, 0}, {3, 2, 1}, {4, 2, 2}};
    for (const Expectation& expectation : expectations) {
        int resolvedArmy = -1, resolvedGroup = -1;
        const bool bResolved = ResolveUnitsManualSubLayer(recipe, expectation.flatIndex, resolvedArmy, resolvedGroup);
        Check(bResolved && resolvedArmy == expectation.expectedArmy && resolvedGroup == expectation.expectedGroup,
              "flat index resolves to the expected (army, group) pair");
        Check(static_cast<int>(unitsLayer.subLayers[static_cast<std::size_t>(expectation.flatIndex)].index)
              == expectation.flatIndex,
              "the seeded ref's own flat index agrees with the resolution's input");
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
    RunCategorySplitChecks();
    RunManualProceduralOrderingChecks();
    RunResolveUnitsManualSubLayerChecks();
    RunReadWriteCorrectnessChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
