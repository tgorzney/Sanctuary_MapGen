// Application_OverlaySetup_Seed_UI.cpp — STEP83 §1.5 split of Application_OverlaySetup_UI.cpp: the
// five launch-time seeding helpers `ConfigureDefaultOverlayLayers` calls. Split out once the
// procedural/manual Props-Reclaim partition (SeedPropReclaimDomains, STEP83 Item 1) pushed the
// original file over the soft-100 line ceiling (ARCH_01_05_FileSizeCeilings.md). Layer: UI. Same
// one-shot, launch-time-only posture as the file it was split from.
#include "Application_Defaults_UI.h"

namespace SanmapGen {
namespace Ui {

void PushProceduralRefs(std::vector<OverlaySubLayerRef_UI>& subLayers, int ruleCount) {
    for (int index = 0; index < ruleCount; ++index)
        subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, index, true});
}

void PushManualRefs(std::vector<OverlaySubLayerRef_UI>& subLayers, int recordCount) {
    for (int index = 0; index < recordCount; ++index)
        subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::Manual, index, true});
}

// CONFIRMED (STEP79 "Downstream authority ruling"): flat/global index over the layer-concatenated
// rule sequence — see this ticket's header note and STEP50's matching, now-confirmed assumption.
// Manual (ARCH_14_14, STEP97): route per-TRANSFORM, not per recipe.markerLayers[i] entry. A layer
// is a cross-cutting display bucket with no category field of its own (§16.1) — a single
// layerIndex legally mixes a Spawn-type transform and a non-Spawn transform, so it can legally
// push a ref into BOTH domains. Existence-checked, not unconditional (contrast
// SeedPropReclaimDomains' push-both-then-filter-at-draw pattern — deliberately NOT reused here,
// this ticket's precedent cross-check).
void SeedMarkerDomains(OverlayLayer_UI& alloyLayer, OverlayLayer_UI& spawnsArmiesLayer,
                       const Params::MapRecipe& recipe) {
    const std::size_t manualLayerCount = recipe.markerLayers.size();
    std::vector<bool> hasSpawnContribution(manualLayerCount, false);
    std::vector<bool> hasAlloyContribution(manualLayerCount, false);
    for (const Params::MarkerInstanceGroup& group : recipe.markers) {
        const bool bIsSpawnGroup = group.name == Params::kSpawnMarkerGroupName;
        for (const Params::MarkerTransform& transform : group.transforms) {
            if (transform.layerIndex < 0
                || static_cast<std::size_t>(transform.layerIndex) >= manualLayerCount) continue;
            const std::size_t layerIndex = static_cast<std::size_t>(transform.layerIndex);
            if (bIsSpawnGroup) hasSpawnContribution[layerIndex] = true;
            else               hasAlloyContribution[layerIndex] = true;
        }
    }
    for (std::size_t layerIndex = 0; layerIndex < manualLayerCount; ++layerIndex) {
        if (hasSpawnContribution[layerIndex])
            spawnsArmiesLayer.subLayers.push_back(OverlaySubLayerRef_UI{
                OverlaySubLayerKind_UI::Manual, static_cast<int>(layerIndex), true});
        if (hasAlloyContribution[layerIndex])
            alloyLayer.subLayers.push_back(OverlaySubLayerRef_UI{
                OverlaySubLayerKind_UI::Manual, static_cast<int>(layerIndex), true});
    }

    // Procedural — unchanged from STEP51's shipped body.
    int flatIndex = 0;
    for (const Params::MarkerRuleLayer& layer : recipe.markerRuleLayers) {
        for (const Params::MarkerRule& rule : layer.rules) {
            OverlayLayer_UI& target = rule.category == Params::MarkerCategory::Spawn
                                           ? spawnsArmiesLayer : alloyLayer;
            target.subLayers.push_back(OverlaySubLayerRef_UI{
                OverlaySubLayerKind_UI::ProceduralRule, flatIndex, true});
            ++flatIndex;
        }
    }
}

// ARCH_14_16_PerArmyUnitsOverlayRows.md §14.16-A: ResolveUnitsManualSubLayer's existing global
// flat-index formula over recipe.armies[*].groups is completely UNCHANGED (§14.4 composes as "flat
// within one army's row" now, not "flat within one shared row") — only the seeding target changes:
// each army's own groups push into unitsLayers[armyIndex], the row that army itself owns
// (army-index-aligned by construction, ConfigureDefaultOverlayLayers), mirroring
// SeedMarkerDomains'/SeedPropReclaimDomains' "push into whichever row owns it" pattern.
void SeedUnitsManualSubLayers(std::vector<OverlayLayer_UI>& unitsLayers, const Params::MapRecipe& recipe) {
    int flatIndex = 0;
    for (std::size_t armyIndex = 0; armyIndex < recipe.armies.size(); ++armyIndex) {
        const Params::Army& army = recipe.armies[armyIndex];
        for (std::size_t group = 0; group < army.groups.size(); ++group)
            unitsLayers[armyIndex].subLayers.push_back(
                OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::Manual, flatIndex++, true});
    }
}

// ARCH_14_16_PerArmyUnitsOverlayRows.md §14.16-B: procedurally-scattered units route into the SAME
// per-army rows as manual units, as ProceduralRule sub-layers — no separate "Units (Procedural)"
// bucket. An out-of-range armyIndex (corrupt/hand-edited data — never produced by the shipped UI)
// drops the ref silently, the same defensive floor ResolveUnitsManualSubLayer already applies to
// Manual refs; never throws or asserts.
void SeedUnitsProceduralSubLayers(std::vector<OverlayLayer_UI>& unitsLayers, const Params::MapRecipe& recipe) {
    for (std::size_t ruleIndex = 0; ruleIndex < recipe.unitRules.size(); ++ruleIndex) {
        const int armyIndex = recipe.unitRules[ruleIndex].armyIndex;
        if (armyIndex < 0 || static_cast<std::size_t>(armyIndex) >= unitsLayers.size()) continue;
        unitsLayers[static_cast<std::size_t>(armyIndex)].subLayers.push_back(
            OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, static_cast<int>(ruleIndex), true});
    }
}

// Props/Reclaim mutually-exclusively partition `recipe.propRules` by `bReclaimable` — the same
// seed-time routing SeedMarkerDomains performs on `category` (§14.6; STEP62's "exact same
// pattern"). `index` stays the GLOBAL recipe.propRules position, never a per-domain running
// count: it is what STEP50's CSR bucket is keyed on (bucketTotal == recipe.propRules.size()).
// Manual refs go to BOTH domains on purpose — one recipe.propLayers[k] can hold transforms from
// reclaimable AND non-reclaimable PropInstanceGroups, so a propLayer is not uniformly one
// domain's; that half of the partition closes at group granularity in the draw pass (STEP83 §5).
void SeedPropReclaimDomains(OverlayLayer_UI& propsLayer, OverlayLayer_UI& reclaimLayer,
                            const Params::MapRecipe& recipe) {
    const int manualLayerCount = static_cast<int>(recipe.propLayers.size());
    PushManualRefs(propsLayer.subLayers,   manualLayerCount);   // Manual first — STEP51's order
    PushManualRefs(reclaimLayer.subLayers, manualLayerCount);
    for (std::size_t index = 0; index < recipe.propRules.size(); ++index) {
        OverlayLayer_UI& target = recipe.propRules[index].bReclaimable ? reclaimLayer : propsLayer;
        target.subLayers.push_back(OverlaySubLayerRef_UI{
            OverlaySubLayerKind_UI::ProceduralRule, static_cast<int>(index), true});
    }
}

} // namespace Ui
} // namespace SanmapGen
