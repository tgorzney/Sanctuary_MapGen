// MapCanvas_IconLayer_Cull_UI_Test.cpp — acceptance test, part 1: §1's culling + LOD, headless (no
// imgui frame, no GL). One translation unit of the MapCanvas_IconLayer_UI_Test binary.
#include "MapCanvas_IconLayer_TestFixture_UI.h"

namespace SanmapGen {
namespace Ui {
namespace {

// An off-view layer's cached AABB never intersects the view rect, so its sub-layer is never walked.
void CheckAabbEarlyOut() {
    IconLayerTestFixture fixture;
    AppendPropInstance(fixture.placements, 1000.0f, 1000.0f, 0, "propA");
    fixture.ruleBucketIndex.props.Build(fixture.placements.props.ruleIndex.data(), 1, 1);
    OverlayLayer_UI offViewLayer; offViewLayer.domainKind = OverlayDomainKind_UI::Props;
    offViewLayer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, 0, true});
    fixture.overlaySettings.overlayLayers.push_back(offViewLayer);

    std::vector<OverlayVisibleInstance> candidates;
    IconLayerCullDiagnostics_UI diagnostics;
    ResolveVisibleCandidates(fixture.Input(), fixture.aabbCache, &diagnostics, candidates);
    check(candidates.empty(), "an off-view layer's candidate list is empty");
    check(diagnostics.subLayerWalksIssued == 0,
          "an off-view layer's cached AABB early-out means its sub-layer bucket is never walked");
}

// Dropped silently, logged once per unique id — not once per instance.
void CheckPairingMissLoggedOnce() {
    IconLayerTestFixture fixture;
    AppendPropInstance(fixture.placements, 2.0f, 2.0f, 0, "propMissing");
    AppendPropInstance(fixture.placements, 2.0f, 2.0f, 0, "propMissing");   // same id, second instance
    fixture.ruleBucketIndex.props.Build(fixture.placements.props.ruleIndex.data(), 2, 1);
    OverlayLayer_UI layer; layer.domainKind = OverlayDomainKind_UI::Props;
    layer.thumbnailLodThresholdPixels = 1.0f;
    layer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, 0, true});
    fixture.overlaySettings.overlayLayers.push_back(layer);

    std::vector<OverlayVisibleInstance> candidates;
    IconLayerCullDiagnostics_UI diagnostics;
    ResolveVisibleCandidates(fixture.Input(), fixture.aabbCache, &diagnostics, candidates);
    check(candidates.empty(), "an unresolved templateIdentifier draws nothing");
    check(diagnostics.loggedMissingTemplateIdentifiers.size() == 1,
          "two instances sharing one unresolved templateIdentifier log it exactly once");
}

// Above threshold: thumbnail mode, at the scaled size.
void CheckThumbnailModeAboveThreshold() {
    IconLayerTestFixture fixture;
    AppendPropInstance(fixture.placements, 2.0f, 2.0f, 0, "propA");
    fixture.ruleBucketIndex.props.Build(fixture.placements.props.ruleIndex.data(), 1, 1);
    SeedAtlasEntry(fixture.pairingLookup, fixture.atlasManifest, "propA", 0);
    OverlayLayer_UI layer; layer.domainKind = OverlayDomainKind_UI::Props;
    layer.thumbnailLodThresholdPixels = 1.0f;   // baseFootprint(2)*scale(1) = 2 >= 1
    layer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, 0, true});
    fixture.overlaySettings.overlayLayers.push_back(layer);

    std::vector<OverlayVisibleInstance> candidates;
    ResolveVisibleCandidates(fixture.Input(), fixture.aabbCache, nullptr, candidates);
    check(candidates.size() == 1, "the in-view, resolved instance produces exactly one candidate");
    if (!candidates.empty())
        check(candidates[0].screenSize > 1.99f && candidates[0].screenSize < 2.01f,
              "thumbnail mode's screen size is baseFootprint*scale/worldUnitsPerCell*pixelsPerCell*zoom");
}

// Below threshold: switches to strategic mode. STEP52's real IconAtlasPairingLookup has no public
// way to seed a non-default strategicIconId (ARCH_14_03_IconRenderingLod.md: bespoke strategic
// icons are separately-scoped, unshipped work) — so this instance's strategic id is unavoidably
// kInvalidIconId, and the observable, correct behaviour is the documented "a miss draws nothing"
// path, which also proves the LOD switch actually fired (a bug that always stayed in thumbnail
// mode would instead emit a candidate here).
void CheckStrategicModeBelowThreshold() {
    IconLayerTestFixture fixture;
    AppendPropInstance(fixture.placements, 2.0f, 2.0f, 0, "propA");
    fixture.ruleBucketIndex.props.Build(fixture.placements.props.ruleIndex.data(), 1, 1);
    SeedAtlasEntry(fixture.pairingLookup, fixture.atlasManifest, "propA", 0);
    OverlayLayer_UI layer; layer.domainKind = OverlayDomainKind_UI::Props;
    layer.thumbnailLodThresholdPixels = 100.0f;   // baseFootprint(2)*scale(1) = 2 < 100
    layer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, 0, true});
    fixture.overlaySettings.overlayLayers.push_back(layer);

    std::vector<OverlayVisibleInstance> candidates;
    IconLayerCullDiagnostics_UI diagnostics;
    ResolveVisibleCandidates(fixture.Input(), fixture.aabbCache, &diagnostics, candidates);
    check(candidates.empty(), "strategic mode with no authored icon yet draws nothing");
    check(diagnostics.loggedMissingTemplateIdentifiers.size() == 1,
          "the strategic-mode miss is logged, proving the threshold crossing actually happened");
}

// STEP83 §8 Item 2 — one propLayer, three PropInstanceGroups with distinct transform counts and
// alternating bReclaimable, all transforms visible/resolvable. Reused by every check below.
struct ReclaimManualFixture {
    IconLayerTestFixture fixture;
    OverlayLayer_UI propsLayer;
    OverlayLayer_UI reclaimLayer;

    ReclaimManualFixture() {
        fixture.recipe.propLayers.assign(1, Params::PropInstanceLayer());
        SeedAtlasEntry(fixture.pairingLookup, fixture.atlasManifest, "propGroup", 0);

        Params::PropInstanceGroup groupZero;   // reclaimable, 2 transforms
        groupZero.blueprintPath = "propGroup"; groupZero.bReclaimable = true;
        groupZero.transforms.assign(2, MakeVisibleTransform());
        Params::PropInstanceGroup groupOne;    // NOT reclaimable, 3 transforms
        groupOne.blueprintPath = "propGroup";  groupOne.bReclaimable = false;
        groupOne.transforms.assign(3, MakeVisibleTransform());
        Params::PropInstanceGroup groupTwo;    // reclaimable, 4 transforms
        groupTwo.blueprintPath = "propGroup";  groupTwo.bReclaimable = true;
        groupTwo.transforms.assign(4, MakeVisibleTransform());
        fixture.recipe.props = {groupZero, groupOne, groupTwo};

        propsLayer.domainKind = OverlayDomainKind_UI::Props;
        propsLayer.thumbnailLodThresholdPixels = 1.0f;
        propsLayer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::Manual, 0, true});
        reclaimLayer.domainKind = OverlayDomainKind_UI::Reclaim;
        reclaimLayer.thumbnailLodThresholdPixels = 1.0f;
        reclaimLayer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::Manual, 0, true});
    }

    static Params::PropTransform MakeVisibleTransform() {
        Params::PropTransform propTransform;
        propTransform.layerIndex = 0;
        propTransform.transform.positionX = 2.0f;   // in-view (fixture header comment)
        propTransform.transform.positionZ = 2.0f;
        propTransform.transform.scaleX = 1.0f;
        return propTransform;
    }
};

// Group 1 (non-reclaimable, 3 transforms) is exactly Props' contribution; groups 0+2 (reclaimable,
// 2+4=6 transforms) are exactly Reclaim's. Union is the full 9, intersection empty.
void CheckManualReclaimPartitionCorrectness() {
    ReclaimManualFixture reclaimManual;
    reclaimManual.fixture.overlaySettings.overlayLayers = {reclaimManual.propsLayer};
    std::vector<OverlayVisibleInstance> propsCandidates;
    ResolveVisibleCandidates(reclaimManual.fixture.Input(), reclaimManual.fixture.aabbCache, nullptr, propsCandidates);
    check(propsCandidates.size() == 3, "Props' Manual walk over group 1 yields exactly 3 candidates");

    ReclaimManualFixture reclaimManual2;   // separate fixture — a fresh AABB cache, not a toggle
    reclaimManual2.fixture.overlaySettings.overlayLayers = {reclaimManual2.reclaimLayer};
    std::vector<OverlayVisibleInstance> reclaimCandidates;
    ResolveVisibleCandidates(reclaimManual2.fixture.Input(), reclaimManual2.fixture.aabbCache, nullptr, reclaimCandidates);
    check(reclaimCandidates.size() == 6, "Reclaim's Manual walk over groups 0+2 yields exactly 6 candidates");
    check(propsCandidates.size() + reclaimCandidates.size() == 9,
          "union of Props' and Reclaim's candidates covers all 9 transforms, intersection empty (no double count)");
}

// Group-level, proven not assumed: the predicate fires once per PropInstanceGroup (3), never once
// per PropTransform (9) — the check that catches a regression to a per-instance test.
void CheckManualReclaimPredicateIsGroupLevel() {
    ReclaimManualFixture reclaimManual;
    reclaimManual.fixture.overlaySettings.overlayLayers = {reclaimManual.propsLayer};
    std::vector<OverlayVisibleInstance> candidates;
    IconLayerCullDiagnostics_UI diagnostics;
    ResolveVisibleCandidates(reclaimManual.fixture.Input(), reclaimManual.fixture.aabbCache, &diagnostics, candidates);
    check(diagnostics.reclaimGroupPredicateEvaluations == 3,
          "the bReclaimable predicate runs exactly groupCount (3) times, not transformCount (9)");
}

// A propLayer holding only reclaimable groups contributes zero candidates to Props (and vice
// versa) — empty, never a crash.
void CheckManualReclaimEmptyContributionNeverCrashes() {
    IconLayerTestFixture fixture;
    fixture.recipe.propLayers.assign(1, Params::PropInstanceLayer());
    Params::PropInstanceGroup onlyReclaimable;
    onlyReclaimable.blueprintPath = "propGroup"; onlyReclaimable.bReclaimable = true;
    onlyReclaimable.transforms.assign(2, ReclaimManualFixture::MakeVisibleTransform());
    fixture.recipe.props = {onlyReclaimable};
    SeedAtlasEntry(fixture.pairingLookup, fixture.atlasManifest, "propGroup", 0);

    OverlayLayer_UI propsLayer; propsLayer.domainKind = OverlayDomainKind_UI::Props;
    propsLayer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::Manual, 0, true});
    fixture.overlaySettings.overlayLayers = {propsLayer};

    std::vector<OverlayVisibleInstance> candidates;
    ResolveVisibleCandidates(fixture.Input(), fixture.aabbCache, nullptr, candidates);
    check(candidates.empty(), "a propLayer holding only reclaimable groups contributes zero candidates to Props");
}

// Shared DATA/recipe/atlas content for the determinism + budget checks below — one procedural
// rule bucket (2 rules) plus one Manual reclaimable group, IconLayerTestFixture is non-copyable
// (owns a raw PreviewComposite*), so this seeds a freshly-constructed instance each call rather
// than copying one.
void SeedReclaimGuardrailFixture(IconLayerTestFixture& fixture) {
    // Procedural instances round-trip through Data::TemplateIdentifier's fixed 7-char buffer
    // (PlacementInstance_DATA.h), unlike the Manual path's raw std::string blueprintPath below —
    // these two ids must stay <= 7 characters or the pairing lookup seeded under the untruncated
    // string would silently miss the truncated one Resolve() actually sees.
    AppendPropInstance(fixture.placements, 2.0f, 2.0f, 0, "procA");
    AppendPropInstance(fixture.placements, 2.0f, 2.0f, 1, "procB");
    fixture.ruleBucketIndex.props.Build(fixture.placements.props.ruleIndex.data(), 2, 2);
    SeedAtlasEntry(fixture.pairingLookup, fixture.atlasManifest, "procA", 0);
    SeedAtlasEntry(fixture.pairingLookup, fixture.atlasManifest, "procB", 1);

    fixture.recipe.propLayers.assign(1, Params::PropInstanceLayer());
    Params::PropInstanceGroup manualGroup;
    manualGroup.blueprintPath = "manualProp"; manualGroup.bReclaimable = true;
    manualGroup.transforms.assign(1, ReclaimManualFixture::MakeVisibleTransform());
    fixture.recipe.props = {manualGroup};
    SeedAtlasEntry(fixture.pairingLookup, fixture.atlasManifest, "manualProp", 2);
}

OverlayLayer_UI MakeReclaimGuardrailPropsLayer() {
    OverlayLayer_UI propsLayer; propsLayer.domainKind = OverlayDomainKind_UI::Props;
    propsLayer.thumbnailLodThresholdPixels = 1.0f;
    propsLayer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, 0, true});
    return propsLayer;
}

OverlayLayer_UI MakeReclaimGuardrailReclaimLayer() {
    OverlayLayer_UI reclaimLayer; reclaimLayer.domainKind = OverlayDomainKind_UI::Reclaim;
    reclaimLayer.thumbnailLodThresholdPixels = 1.0f;
    reclaimLayer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, 1, true});
    reclaimLayer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::Manual, 0, true});
    return reclaimLayer;
}

// §14.11: overlay-side filtering is a read-side view only — it must never mutate
// Data::PlacementInstances or STEP50's Data::RuleBucketIndexSet. Exercises BOTH halves at once
// (a Manual reclaimable group + a procedural rule bucket routed to Reclaim via STEP83 Item 1's
// seeding), byte-compares DATA before/after a full partitioned draw, and proves budget non-
// inflation: both-enabled candidate count == Props-only + Reclaim-only, no double count.
void CheckReclaimDeterminismGuardrailAndBudgetNonInflation() {
    IconLayerTestFixture fixture;
    SeedReclaimGuardrailFixture(fixture);
    fixture.overlaySettings.overlayLayers = {MakeReclaimGuardrailPropsLayer(), MakeReclaimGuardrailReclaimLayer()};

    const std::vector<float> positionXBefore = fixture.placements.props.positionX;
    const std::vector<int>   ruleIndexBefore = fixture.placements.props.ruleIndex;
    const std::int32_t entryCountBefore = fixture.ruleBucketIndex.props.EntryCount();
    const int bucketCountBefore = fixture.ruleBucketIndex.props.BucketCount();

    std::vector<OverlayVisibleInstance> bothCandidates;
    ResolveVisibleCandidates(fixture.Input(), fixture.aabbCache, nullptr, bothCandidates);
    check(bothCandidates.size() == 3, "procA (Props) + procB (Reclaim) + manualProp (Reclaim) == 3");

    check(fixture.placements.props.positionX == positionXBefore
          && fixture.placements.props.ruleIndex == ruleIndexBefore,
          "§14.11: Data::PlacementInstances is bit-identical before/after a full partitioned draw");
    check(fixture.ruleBucketIndex.props.EntryCount() == entryCountBefore
          && fixture.ruleBucketIndex.props.BucketCount() == bucketCountBefore,
          "§14.11: STEP50's Data::RuleBucketIndexSet is untouched by overlay-side filtering");

    IconLayerTestFixture propsOnlyFixture;
    SeedReclaimGuardrailFixture(propsOnlyFixture);
    propsOnlyFixture.overlaySettings.overlayLayers = {MakeReclaimGuardrailPropsLayer()};
    std::vector<OverlayVisibleInstance> propsOnlyCandidates;
    ResolveVisibleCandidates(propsOnlyFixture.Input(), propsOnlyFixture.aabbCache, nullptr, propsOnlyCandidates);

    IconLayerTestFixture reclaimOnlyFixture;
    SeedReclaimGuardrailFixture(reclaimOnlyFixture);
    reclaimOnlyFixture.overlaySettings.overlayLayers = {MakeReclaimGuardrailReclaimLayer()};
    std::vector<OverlayVisibleInstance> reclaimOnlyCandidates;
    ResolveVisibleCandidates(reclaimOnlyFixture.Input(), reclaimOnlyFixture.aabbCache, nullptr, reclaimOnlyCandidates);

    check(bothCandidates.size() == propsOnlyCandidates.size() + reclaimOnlyCandidates.size(),
          "both-layers-enabled candidate count equals Props-only plus Reclaim-only — no instance counted twice");
}

// STEP111 — procedural marker tint resolution (Data::PlacementInstances::category ->
// Params::MarkerCategory -> GlobalMarkerSettings). "Alloy" domainKind here is purely the layer's
// screen routing (§14.6) — the tint itself is keyed off the DATA-baked category, not domainKind.
void CheckMarkerAlloysCategoryResolvesColorAlloy() {
    IconLayerTestFixture fixture;
    fixture.recipe.globalMarkerSettings.colorAlloy[0] = 0.1f;
    fixture.recipe.globalMarkerSettings.colorAlloy[1] = 0.2f;
    fixture.recipe.globalMarkerSettings.colorAlloy[2] = 0.3f;
    // STEP122: this fixture tests tint resolution, not scale composition — pin scaleAlloy to 1.0f
    // so GlobalMarkerSettings' own real 0.17f default doesn't drop thumbnailScreenSize below
    // thumbnailLodThresholdPixels and flip into strategic mode (no icon seeded there).
    fixture.recipe.globalMarkerSettings.scaleAlloy = 1.0f;
    AppendMarkerInstance(fixture.placements, 2.0f, 2.0f, 0, Params::MarkerCategory::Alloys, "markerA");
    fixture.ruleBucketIndex.markers.Build(fixture.placements.markers.ruleIndex.data(), 1, 1);
    SeedAtlasEntry(fixture.pairingLookup, fixture.atlasManifest, "markerA", 0);
    OverlayLayer_UI layer; layer.domainKind = OverlayDomainKind_UI::Alloy;
    layer.thumbnailLodThresholdPixels = 1.0f;
    layer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, 0, true});
    fixture.overlaySettings.overlayLayers.push_back(layer);

    std::vector<OverlayVisibleInstance> candidates;
    ResolveVisibleCandidates(fixture.Input(), fixture.aabbCache, nullptr, candidates);
    check(candidates.size() == 1, "one Alloys-category marker resolves as a candidate");
    if (!candidates.empty())
        check(candidates[0].tintColorRed == 0.1f && candidates[0].tintColorGreen == 0.2f
              && candidates[0].tintColorBlue == 0.3f,
              "Alloys category tint resolves from GlobalMarkerSettings::colorAlloy");
}

void CheckMarkerSpawnCategoryResolvesColorSpawn() {
    IconLayerTestFixture fixture;
    fixture.recipe.globalMarkerSettings.colorSpawn[0] = 0.4f;
    fixture.recipe.globalMarkerSettings.colorSpawn[1] = 0.5f;
    fixture.recipe.globalMarkerSettings.colorSpawn[2] = 0.6f;
    // STEP122: this fixture tests tint resolution, not scale composition — see the identical pin
    // in CheckMarkerAlloysCategoryResolvesColorAlloy above.
    fixture.recipe.globalMarkerSettings.scaleSpawn = 1.0f;
    AppendMarkerInstance(fixture.placements, 2.0f, 2.0f, 0, Params::MarkerCategory::Spawn, "markerS");
    fixture.ruleBucketIndex.markers.Build(fixture.placements.markers.ruleIndex.data(), 1, 1);
    SeedAtlasEntry(fixture.pairingLookup, fixture.atlasManifest, "markerS", 0);
    OverlayLayer_UI layer; layer.domainKind = OverlayDomainKind_UI::SpawnsArmies;
    layer.thumbnailLodThresholdPixels = 1.0f;
    layer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, 0, true});
    fixture.overlaySettings.overlayLayers.push_back(layer);

    std::vector<OverlayVisibleInstance> candidates;
    ResolveVisibleCandidates(fixture.Input(), fixture.aabbCache, nullptr, candidates);
    check(candidates.size() == 1, "one Spawn-category marker resolves as a candidate");
    if (!candidates.empty())
        check(candidates[0].tintColorRed == 0.4f && candidates[0].tintColorGreen == 0.5f
              && candidates[0].tintColorBlue == 0.6f,
              "Spawn category tint resolves from GlobalMarkerSettings::colorSpawn");
}

// Generic/Expansion have no reserved color today — stay white even with non-default
// colorAlloy/colorSpawn set, proving no accidental bleed-through from the other two categories.
void CheckMarkerGenericAndExpansionStayWhite() {
    IconLayerTestFixture fixture;
    fixture.recipe.globalMarkerSettings.colorAlloy[0] = 0.1f;
    fixture.recipe.globalMarkerSettings.colorAlloy[1] = 0.2f;
    fixture.recipe.globalMarkerSettings.colorAlloy[2] = 0.3f;
    fixture.recipe.globalMarkerSettings.colorSpawn[0] = 0.4f;
    fixture.recipe.globalMarkerSettings.colorSpawn[1] = 0.5f;
    fixture.recipe.globalMarkerSettings.colorSpawn[2] = 0.6f;
    AppendMarkerInstance(fixture.placements, 2.0f, 2.0f, 0, Params::MarkerCategory::Generic, "markerG");
    AppendMarkerInstance(fixture.placements, 2.0f, 2.0f, 1, Params::MarkerCategory::Expansion, "markerE");
    const int ruleIndexColumn[2] = {0, 1};
    fixture.ruleBucketIndex.markers.Build(ruleIndexColumn, 2, 2);
    SeedAtlasEntry(fixture.pairingLookup, fixture.atlasManifest, "markerG", 0);
    SeedAtlasEntry(fixture.pairingLookup, fixture.atlasManifest, "markerE", 1);
    OverlayLayer_UI layer; layer.domainKind = OverlayDomainKind_UI::Alloy;
    layer.thumbnailLodThresholdPixels = 1.0f;
    layer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, 0, true});
    layer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, 1, true});
    fixture.overlaySettings.overlayLayers.push_back(layer);

    std::vector<OverlayVisibleInstance> candidates;
    ResolveVisibleCandidates(fixture.Input(), fixture.aabbCache, nullptr, candidates);
    check(candidates.size() == 2, "both Generic and Expansion markers resolve as candidates");
    for (const OverlayVisibleInstance& candidate : candidates)
        check(candidate.tintColorRed == 1.0f && candidate.tintColorGreen == 1.0f && candidate.tintColorBlue == 1.0f,
              "Generic/Expansion markers stay white despite non-default colorAlloy/colorSpawn");
}

// Manual Props/Decals — the layer's own authored color threads through, mirroring the Manual
// reclaim-partition fixtures above.
void CheckManualPropLayerColorThreadsThrough() {
    IconLayerTestFixture fixture;
    Params::PropInstanceLayer propLayer;
    propLayer.color[0] = 0.4f; propLayer.color[1] = 0.5f; propLayer.color[2] = 0.6f;
    fixture.recipe.propLayers.push_back(propLayer);
    SeedAtlasEntry(fixture.pairingLookup, fixture.atlasManifest, "propGroup", 0);

    Params::PropInstanceGroup group;
    group.blueprintPath = "propGroup";
    group.transforms.push_back(ReclaimManualFixture::MakeVisibleTransform());   // layerIndex = 0
    fixture.recipe.props.push_back(group);

    OverlayLayer_UI layer; layer.domainKind = OverlayDomainKind_UI::Props;
    layer.thumbnailLodThresholdPixels = 1.0f;
    layer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::Manual, 0, true});
    fixture.overlaySettings.overlayLayers.push_back(layer);

    std::vector<OverlayVisibleInstance> candidates;
    ResolveVisibleCandidates(fixture.Input(), fixture.aabbCache, nullptr, candidates);
    check(candidates.size() == 1, "one manual Prop candidate resolves");
    if (!candidates.empty())
        check(candidates[0].tintColorRed == 0.4f && candidates[0].tintColorGreen == 0.5f
              && candidates[0].tintColorBlue == 0.6f,
              "the manual Prop layer's own color threads through to the candidate's tint");
}

void CheckManualDecalLayerColorThreadsThrough() {
    IconLayerTestFixture fixture;
    Params::DecalInstanceLayer decalLayer;
    decalLayer.color[0] = 0.7f; decalLayer.color[1] = 0.8f; decalLayer.color[2] = 0.9f;
    fixture.recipe.decalLayers.push_back(decalLayer);
    SeedAtlasEntry(fixture.pairingLookup, fixture.atlasManifest, "decalGroup", 0);

    Params::DecalInstanceGroup group;
    group.blueprintPath = "decalGroup";
    Params::DecalTransform decalTransform;
    decalTransform.layerIndex = 0;
    decalTransform.transform.positionX = 2.0f; decalTransform.transform.positionZ = 2.0f;
    decalTransform.transform.scaleX = 1.0f;
    group.transforms.push_back(decalTransform);
    fixture.recipe.decals.push_back(group);

    OverlayLayer_UI layer; layer.domainKind = OverlayDomainKind_UI::Decals;
    layer.thumbnailLodThresholdPixels = 1.0f;
    layer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::Manual, 0, true});
    fixture.overlaySettings.overlayLayers.push_back(layer);

    std::vector<OverlayVisibleInstance> candidates;
    ResolveVisibleCandidates(fixture.Input(), fixture.aabbCache, nullptr, candidates);
    check(candidates.size() == 1, "one manual Decal candidate resolves");
    if (!candidates.empty())
        check(candidates[0].tintColorRed == 0.7f && candidates[0].tintColorGreen == 0.8f
              && candidates[0].tintColorBlue == 0.9f,
              "the manual Decal layer's own color threads through to the candidate's tint");
}

// Procedural Props (no color/layer-association PARAMS field) and manual Units (no color PARAMS
// field reaches this pipeline at all) stay white regardless of any GlobalMarkerSettings/
// PropInstanceLayer::color values set on the fixture — proves no accidental cross-talk from the
// Markers-only category resolution.
void CheckProceduralPropsAndManualUnitsStayWhite() {
    IconLayerTestFixture fixture;
    fixture.recipe.globalMarkerSettings.colorAlloy[0] = 0.9f;
    fixture.recipe.propLayers.push_back(Params::PropInstanceLayer());
    fixture.recipe.propLayers[0].color[0] = 0.9f;
    fixture.recipe.propLayers[0].color[1] = 0.1f;
    fixture.recipe.propLayers[0].color[2] = 0.1f;

    AppendPropInstance(fixture.placements, 2.0f, 2.0f, 0, "propA");
    fixture.ruleBucketIndex.props.Build(fixture.placements.props.ruleIndex.data(), 1, 1);
    SeedAtlasEntry(fixture.pairingLookup, fixture.atlasManifest, "propA", 0);
    OverlayLayer_UI proceduralPropsLayer; proceduralPropsLayer.domainKind = OverlayDomainKind_UI::Props;
    proceduralPropsLayer.thumbnailLodThresholdPixels = 1.0f;
    proceduralPropsLayer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, 0, true});

    Params::Army army; army.name = "ARMY_01";
    Params::UnitGroup group;
    Params::UnitTransform unit;
    unit.positionX = 2.0f; unit.positionZ = 2.0f; unit.scaleX = 1.0f;
    const char* unitTemplateIdentifier = "unitA";
    for (int index = 0; unitTemplateIdentifier[index] != '\0' && index < 7; ++index)
        unit.templateIdentifier[index] = unitTemplateIdentifier[index];
    group.units.push_back(unit);
    army.groups.push_back(group);
    fixture.recipe.armies.push_back(army);
    SeedAtlasEntry(fixture.pairingLookup, fixture.atlasManifest, "unitA", 1);
    OverlayLayer_UI unitsLayer; unitsLayer.domainKind = OverlayDomainKind_UI::Units;
    unitsLayer.thumbnailLodThresholdPixels = 1.0f;
    unitsLayer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::Manual, 0, true});

    fixture.overlaySettings.overlayLayers = {proceduralPropsLayer, unitsLayer};

    std::vector<OverlayVisibleInstance> candidates;
    ResolveVisibleCandidates(fixture.Input(), fixture.aabbCache, nullptr, candidates);
    check(candidates.size() == 2, "one procedural Prop candidate plus one manual Unit candidate resolve");
    for (const OverlayVisibleInstance& candidate : candidates)
        check(candidate.tintColorRed == 1.0f && candidate.tintColorGreen == 1.0f && candidate.tintColorBlue == 1.0f,
              "procedural Props and manual Units stay white — no color PARAMS field reaches either");
}

// The C2 cache's selected-instance replay path (§4's "run steps 1-3 fresh for only the selected
// instance") resolves the same category tint as the normal full-walk path — closes the "flashes
// white on a cache-valid frame" risk called out in the ticket's Fix §7.
void CheckSelectedInstanceCandidateResolvesMarkerColor() {
    IconLayerTestFixture fixture;
    fixture.recipe.globalMarkerSettings.colorAlloy[0] = 0.1f;
    fixture.recipe.globalMarkerSettings.colorAlloy[1] = 0.2f;
    fixture.recipe.globalMarkerSettings.colorAlloy[2] = 0.3f;
    AppendMarkerInstance(fixture.placements, 2.0f, 2.0f, 0, Params::MarkerCategory::Alloys, "markerA");
    fixture.ruleBucketIndex.markers.Build(fixture.placements.markers.ruleIndex.data(), 1, 1);
    SeedAtlasEntry(fixture.pairingLookup, fixture.atlasManifest, "markerA", 0);
    OverlayLayer_UI layer; layer.domainKind = OverlayDomainKind_UI::Alloy;
    layer.thumbnailLodThresholdPixels = 1.0f;
    layer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, 0, true});
    fixture.overlaySettings.overlayLayers.push_back(layer);

    DrawOverlayIconLayersInput input = fixture.Input();
    input.selectedInstanceKey = OverlayInstanceKey_UI{PlacementCollectionKind_UI::Markers, 0, true};

    std::vector<OverlayVisibleInstance> candidates;
    const bool bResolved = ResolveSelectedInstanceCandidate(input, candidates);
    check(bResolved && candidates.size() == 1, "the C2 replay path resolves exactly one selected candidate");
    if (!candidates.empty())
        check(candidates[0].tintColorRed == 0.1f && candidates[0].tintColorGreen == 0.2f
              && candidates[0].tintColorBlue == 0.3f,
              "ResolveSelectedInstanceCandidate resolves the same Alloys tint as the full cull path");
}

// Params::ResolvePropInstanceLayerColor/ResolveDecalInstanceLayerColor's own out-of-range-safe
// contract, called directly (mirrors IsMarkerInstanceLayerLocked's convention, STEP106 §3).
void CheckLayerColorOutOfRangeDefaultsWhite() {
    std::vector<Params::PropInstanceLayer> emptyPropLayers;
    float propRed = 0.0f, propGreen = 0.0f, propBlue = 0.0f;
    Params::ResolvePropInstanceLayerColor(0, emptyPropLayers, propRed, propGreen, propBlue);
    check(propRed == 1.0f && propGreen == 1.0f && propBlue == 1.0f,
          "an out-of-range Prop layer index defaults to white");

    std::vector<Params::DecalInstanceLayer> emptyDecalLayers;
    float decalRed = 0.0f, decalGreen = 0.0f, decalBlue = 0.0f;
    Params::ResolveDecalInstanceLayerColor(3, emptyDecalLayers, decalRed, decalGreen, decalBlue);
    check(decalRed == 1.0f && decalGreen == 1.0f && decalBlue == 1.0f,
          "an out-of-range Decal layer index defaults to white");
}

// STEP114: ResolveMarkerIconTemplateIdentifier's own resolution order, isolated from the full cull
// walk — override wins regardless of group.name; empty override falls through to the group-name
// mapping; an unrecognized name falls back to itself, verbatim.
void CheckResolveMarkerIconTemplateIdentifier() {
    Params::GlobalMarkerSettings settings;
    settings.iconNameAlloy  = "AlloyIcon";
    settings.iconNamePlasma = "PlasmaIcon";
    settings.iconNameSpawn  = "SpawnIcon";

    Params::MarkerTransform overriddenTransform;
    overriddenTransform.iconNameOverride = "OverrideIcon";
    Params::MarkerInstanceGroup spawnGroup; spawnGroup.name = Params::kSpawnMarkerGroupName;
    check(ResolveMarkerIconTemplateIdentifier(overriddenTransform, spawnGroup, settings) == "OverrideIcon",
          "a non-empty transform.iconNameOverride always wins regardless of group.name");

    Params::MarkerTransform plainTransform;   // no override
    check(ResolveMarkerIconTemplateIdentifier(plainTransform, spawnGroup, settings) == "SpawnIcon",
          "an empty override with group.name == the reserved Spawn name resolves to iconNameSpawn");

    Params::MarkerInstanceGroup alloysGroup; alloysGroup.name = "Alloys";
    check(ResolveMarkerIconTemplateIdentifier(plainTransform, alloysGroup, settings) == "AlloyIcon",
          "group.name == \"Alloys\" resolves to iconNameAlloy");

    Params::MarkerInstanceGroup plasmaGroup; plasmaGroup.name = "Plasma";
    check(ResolveMarkerIconTemplateIdentifier(plainTransform, plasmaGroup, settings) == "PlasmaIcon",
          "group.name == \"Plasma\" resolves to iconNamePlasma");

    Params::MarkerInstanceGroup expansionGroup; expansionGroup.name = "Expansion";
    check(ResolveMarkerIconTemplateIdentifier(plainTransform, expansionGroup, settings) == "Expansion",
          "an unrecognized group.name (e.g. \"Expansion\") resolves to group.name itself, verbatim");

    Params::MarkerInstanceGroup genericGroup; genericGroup.name = "Generic";
    check(ResolveMarkerIconTemplateIdentifier(plainTransform, genericGroup, settings) == "Generic",
          "an unrecognized group.name (e.g. \"Generic\") resolves to group.name itself, verbatim");
}

// STEP114: ResolveMarkersManual end to end, through the real ResolveManualSubLayer switch —
// mirrors ReclaimManualFixture's exact shape, built over fixture.recipe.markers/markerLayers/
// globalMarkerSettings instead of recipe.props/propLayers.
struct ManualMarkerTestFixture {
    IconLayerTestFixture fixture;
    OverlayLayer_UI alloyLayer;
    OverlayLayer_UI spawnsArmiesLayer;

    ManualMarkerTestFixture() {
        fixture.recipe.markerLayers.assign(1, Params::MarkerInstanceLayer());
        fixture.recipe.globalMarkerSettings.iconNameAlloy = "Alloy";   // the fixture's own default
        // STEP122: this fixture underpins tint/override/partition tests, not scale composition —
        // pin scaleAlloy/scaleSpawn to 1.0f so GlobalMarkerSettings' own real 0.17f default doesn't
        // drop these fixtures' markers below thumbnailLodThresholdPixels and flip them into
        // strategic mode (no icon seeded there, so a miss draws nothing instead of one candidate).
        // CheckManualMarkerScaleComposesEndToEnd/CheckManualMarkerScaleUnrecognizedGroupNameStaysNoOp
        // set their own non-default values on top of this baseline where scale is actually the
        // thing under test.
        fixture.recipe.globalMarkerSettings.scaleAlloy = 1.0f;
        fixture.recipe.globalMarkerSettings.scaleSpawn = 1.0f;
        SeedAtlasEntry(fixture.pairingLookup, fixture.atlasManifest, "Alloy", 0);
        // globalMarkerSettings.iconNameSpawn's own default ("Spawn") — seeded too, so the
        // SpawnsArmies-domain half of the group-name partition check also resolves a candidate
        // rather than logging a pairing miss.
        SeedAtlasEntry(fixture.pairingLookup, fixture.atlasManifest, "Spawn", 2);

        Params::MarkerInstanceGroup alloysGroup;
        alloysGroup.name = "Alloys";
        alloysGroup.transforms.push_back(MakeVisibleMarkerTransform());
        Params::MarkerInstanceGroup spawnGroup;
        spawnGroup.name = Params::kSpawnMarkerGroupName;
        spawnGroup.transforms.push_back(MakeVisibleMarkerTransform());
        fixture.recipe.markers = {alloysGroup, spawnGroup};

        alloyLayer.domainKind = OverlayDomainKind_UI::Alloy;
        alloyLayer.thumbnailLodThresholdPixels = 1.0f;
        alloyLayer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::Manual, 0, true});
        spawnsArmiesLayer.domainKind = OverlayDomainKind_UI::SpawnsArmies;
        spawnsArmiesLayer.thumbnailLodThresholdPixels = 1.0f;
        spawnsArmiesLayer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::Manual, 0, true});
    }

    static Params::MarkerTransform MakeVisibleMarkerTransform() {
        Params::MarkerTransform transform;
        transform.layerIndex = 0;
        transform.transform.positionX = 2.0f;   // in-view (fixture header comment)
        transform.transform.positionZ = 2.0f;
        transform.transform.scaleX = 1.0f;
        return transform;
    }
};

// Group-name partition: the Alloy domain walks only the Alloys group, the SpawnsArmies domain
// walks only the Spawn group, under the SAME {Manual, 0} sub-layer ref.
void CheckManualMarkerGroupNamePartition() {
    ManualMarkerTestFixture alloyFixture;
    alloyFixture.fixture.overlaySettings.overlayLayers = {alloyFixture.alloyLayer};
    std::vector<OverlayVisibleInstance> alloyCandidates;
    ResolveVisibleCandidates(alloyFixture.fixture.Input(), alloyFixture.fixture.aabbCache, nullptr, alloyCandidates);
    check(alloyCandidates.size() == 1, "resolving an Alloy-domain layer with {Manual, 0} yields exactly 1 candidate");

    ManualMarkerTestFixture spawnFixture;   // separate fixture — a fresh AABB cache, not a toggle
    spawnFixture.fixture.overlaySettings.overlayLayers = {spawnFixture.spawnsArmiesLayer};
    std::vector<OverlayVisibleInstance> spawnCandidates;
    ResolveVisibleCandidates(spawnFixture.fixture.Input(), spawnFixture.fixture.aabbCache, nullptr, spawnCandidates);
    check(spawnCandidates.size() == 1,
          "resolving a SpawnsArmies-domain layer with the same {Manual, 0} yields exactly 1 candidate");
}

// Override-wins-over-type-default, proven end to end: a transform with a non-empty
// iconNameOverride resolves to the OVERRIDE entry's atlas placement, not the type-default entry's.
void CheckManualMarkerIconOverrideWinsEndToEnd() {
    ManualMarkerTestFixture overrideFixture;
    overrideFixture.fixture.recipe.markers[0].transforms[0].iconNameOverride = "OverrideEntry";
    SeedAtlasEntry(overrideFixture.fixture.pairingLookup, overrideFixture.fixture.atlasManifest,
                   "OverrideEntry", 1, /*atlasPage=*/1);
    overrideFixture.fixture.overlaySettings.overlayLayers = {overrideFixture.alloyLayer};

    std::vector<OverlayVisibleInstance> candidates;
    ResolveVisibleCandidates(overrideFixture.fixture.Input(), overrideFixture.fixture.aabbCache, nullptr, candidates);
    check(candidates.size() == 1, "the overridden Alloys-group transform still resolves as one candidate");
    if (!candidates.empty())
        check(candidates[0].atlasPage == 1,
              "the emitted candidate's atlasPage matches the OVERRIDE entry, not the type-default \"Alloy\" entry");
}

// The positional layerIndex != subLayerArrayIndex filter: a transform on layerIndex 1 against a
// {Manual, 0} sub-layer ref yields zero candidates.
void CheckManualMarkerLayerIndexFilter() {
    ManualMarkerTestFixture fixture;
    fixture.fixture.recipe.markers[0].transforms[0].layerIndex = 1;
    fixture.fixture.overlaySettings.overlayLayers = {fixture.alloyLayer};

    std::vector<OverlayVisibleInstance> candidates;
    ResolveVisibleCandidates(fixture.fixture.Input(), fixture.fixture.aabbCache, nullptr, candidates);
    check(candidates.empty(),
          "a transform on layerIndex 1 against a {Manual, 0} sub-layer ref yields zero candidates");
}

// STEP116: ResolveMarkersManual's type-default color resolution, end to end through the real
// ResolveManualSubLayer switch — mirrors ManualMarkerTestFixture's own shape. Leaves
// markerLayers[0].bColorOverrideEnabled at its default `false`.
void CheckManualMarkerTypeDefaultColorResolvesEndToEnd() {
    // Alloy domain resolves colorAlloy.
    {
        ManualMarkerTestFixture fixture;
        fixture.fixture.recipe.globalMarkerSettings.colorAlloy[0] = 0.1f;
        fixture.fixture.recipe.globalMarkerSettings.colorAlloy[1] = 0.2f;
        fixture.fixture.recipe.globalMarkerSettings.colorAlloy[2] = 0.3f;
        fixture.fixture.overlaySettings.overlayLayers = {fixture.alloyLayer};

        std::vector<OverlayVisibleInstance> candidates;
        ResolveVisibleCandidates(fixture.fixture.Input(), fixture.fixture.aabbCache, nullptr, candidates);
        check(candidates.size() == 1, "the Alloys manual group resolves exactly one candidate");
        if (!candidates.empty())
            check(candidates[0].tintColorRed == 0.1f && candidates[0].tintColorGreen == 0.2f
                  && candidates[0].tintColorBlue == 0.3f,
                  "the icon-overlay path's Alloy-domain candidate resolves the group's type-default colorAlloy");
    }

    // SpawnsArmies domain resolves colorSpawn.
    {
        ManualMarkerTestFixture fixture;
        fixture.fixture.recipe.globalMarkerSettings.colorSpawn[0] = 0.4f;
        fixture.fixture.recipe.globalMarkerSettings.colorSpawn[1] = 0.5f;
        fixture.fixture.recipe.globalMarkerSettings.colorSpawn[2] = 0.6f;
        fixture.fixture.overlaySettings.overlayLayers = {fixture.spawnsArmiesLayer};

        std::vector<OverlayVisibleInstance> candidates;
        ResolveVisibleCandidates(fixture.fixture.Input(), fixture.fixture.aabbCache, nullptr, candidates);
        check(candidates.size() == 1, "the Spawn manual group resolves exactly one candidate");
        if (!candidates.empty())
            check(candidates[0].tintColorRed == 0.4f && candidates[0].tintColorGreen == 0.5f
                  && candidates[0].tintColorBlue == 0.6f,
                  "the icon-overlay path's SpawnsArmies-domain candidate resolves the group's type-default colorSpawn");
    }
}

// STEP116: an explicit layer color override wins over the group's type-default color — proven end
// to end, with a non-default colorAlloy also set to prove no bleed.
void CheckManualMarkerLayerOverrideWinsOverTypeDefault() {
    ManualMarkerTestFixture fixture;
    fixture.fixture.recipe.markerLayers[0].bColorOverrideEnabled = true;
    fixture.fixture.recipe.markerLayers[0].color[0] = 0.7f;
    fixture.fixture.recipe.markerLayers[0].color[1] = 0.8f;
    fixture.fixture.recipe.markerLayers[0].color[2] = 0.9f;
    fixture.fixture.recipe.globalMarkerSettings.colorAlloy[0] = 0.1f;
    fixture.fixture.recipe.globalMarkerSettings.colorAlloy[1] = 0.2f;
    fixture.fixture.recipe.globalMarkerSettings.colorAlloy[2] = 0.3f;
    fixture.fixture.overlaySettings.overlayLayers = {fixture.alloyLayer};

    std::vector<OverlayVisibleInstance> candidates;
    ResolveVisibleCandidates(fixture.fixture.Input(), fixture.fixture.aabbCache, nullptr, candidates);
    check(candidates.size() == 1, "the Alloys manual group resolves exactly one candidate");
    if (!candidates.empty())
        check(candidates[0].tintColorRed == 0.7f && candidates[0].tintColorGreen == 0.8f
              && candidates[0].tintColorBlue == 0.9f,
              "the layer's explicit color override wins over the group's type-default colorAlloy");
}

// STEP116: a MANUAL group named "Generic" stays white despite non-default colorAlloy/colorSpawn —
// mirrors CheckMarkerGenericAndExpansionStayWhite's own shape, but for the manual (not procedural)
// marker path.
void CheckManualMarkerGenericGroupStaysWhite() {
    IconLayerTestFixture fixture;
    fixture.recipe.markerLayers.assign(1, Params::MarkerInstanceLayer());
    fixture.recipe.globalMarkerSettings.colorAlloy[0] = 0.1f;
    fixture.recipe.globalMarkerSettings.colorAlloy[1] = 0.2f;
    fixture.recipe.globalMarkerSettings.colorAlloy[2] = 0.3f;
    fixture.recipe.globalMarkerSettings.colorSpawn[0] = 0.4f;
    fixture.recipe.globalMarkerSettings.colorSpawn[1] = 0.5f;
    fixture.recipe.globalMarkerSettings.colorSpawn[2] = 0.6f;
    SeedAtlasEntry(fixture.pairingLookup, fixture.atlasManifest, "Generic", 0);

    Params::MarkerInstanceGroup genericGroup;
    genericGroup.name = "Generic";
    genericGroup.transforms.push_back(ManualMarkerTestFixture::MakeVisibleMarkerTransform());
    fixture.recipe.markers = {genericGroup};

    OverlayLayer_UI layer; layer.domainKind = OverlayDomainKind_UI::Alloy;
    layer.thumbnailLodThresholdPixels = 1.0f;
    layer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::Manual, 0, true});

    fixture.overlaySettings.overlayLayers = {layer};
    std::vector<OverlayVisibleInstance> candidates;
    ResolveVisibleCandidates(fixture.Input(), fixture.aabbCache, nullptr, candidates);
    check(candidates.size() == 1, "the manual Generic group resolves exactly one candidate");
    if (!candidates.empty())
        check(candidates[0].tintColorRed == 1.0f && candidates[0].tintColorGreen == 1.0f
              && candidates[0].tintColorBlue == 1.0f,
              "a manual group named \"Generic\" stays white despite non-default colorAlloy/colorSpawn");
}

// STEP122 — ResolveMarkerGroupTypeScale in isolation, mirroring
// CheckResolveMarkerIconTemplateIdentifier's own posture above.
void CheckResolveMarkerGroupTypeScale() {
    Params::GlobalMarkerSettings settings;
    settings.scaleAlloy = 2.0f;
    settings.scalePlasma = 3.0f;
    settings.scaleSpawn = 4.0f;

    check(Params::ResolveMarkerGroupTypeScale("Spawn", settings) == 4.0f, "\"Spawn\" resolves scaleSpawn");
    check(Params::ResolveMarkerGroupTypeScale(Params::kSpawnMarkerGroupName, settings) == 4.0f,
          "kSpawnMarkerGroupName resolves scaleSpawn");
    check(Params::ResolveMarkerGroupTypeScale("Spawns", settings) == 4.0f, "\"Spawns\" resolves scaleSpawn");
    check(Params::ResolveMarkerGroupTypeScale("Alloy", settings) == 2.0f, "\"Alloy\" resolves scaleAlloy");
    check(Params::ResolveMarkerGroupTypeScale("Alloys", settings) == 2.0f, "\"Alloys\" resolves scaleAlloy");
    check(Params::ResolveMarkerGroupTypeScale("Plasma", settings) == 3.0f, "\"Plasma\" resolves scalePlasma");
    check(Params::ResolveMarkerGroupTypeScale("Plasmas", settings) == 3.0f, "\"Plasmas\" resolves scalePlasma");
    check(Params::ResolveMarkerGroupTypeScale("Generic", settings) == 1.0f,
          "\"Generic\" resolves 1.0f, the multiplicative no-op");
    check(Params::ResolveMarkerGroupTypeScale("Expansion", settings) == 1.0f,
          "\"Expansion\" resolves 1.0f, the multiplicative no-op");
    check(Params::ResolveMarkerGroupTypeScale("FreeformName", settings) == 1.0f,
          "an arbitrary freeform name resolves 1.0f, the multiplicative no-op");
}

// STEP122 — ResolveMarkersManual composes layerIconScale * groupTypeScale into the manual marker's
// rendered screenSize, end to end through the real ResolveManualSubLayer switch. Compares against a
// baseline (scaleAlloy pinned to 1.0f to isolate the multiplier under test) rather than a hardcoded
// footprint constant, so this stays correct regardless of WorldFootprintSizeTable's own default.
void CheckManualMarkerScaleComposesEndToEnd() {
    ManualMarkerTestFixture baselineFixture;
    baselineFixture.fixture.recipe.globalMarkerSettings.scaleAlloy = 1.0f;   // isolate the layer term
    baselineFixture.fixture.overlaySettings.overlayLayers = {baselineFixture.alloyLayer};
    std::vector<OverlayVisibleInstance> baselineCandidates;
    ResolveVisibleCandidates(baselineFixture.fixture.Input(), baselineFixture.fixture.aabbCache, nullptr, baselineCandidates);
    check(baselineCandidates.size() == 1, "the baseline (unscaled) Alloys manual group resolves exactly one candidate");

    ManualMarkerTestFixture scaledFixture;
    scaledFixture.fixture.recipe.markerLayers[0].iconScale = 2.0f;
    scaledFixture.fixture.recipe.globalMarkerSettings.scaleAlloy = 3.0f;
    scaledFixture.fixture.overlaySettings.overlayLayers = {scaledFixture.alloyLayer};
    std::vector<OverlayVisibleInstance> scaledCandidates;
    ResolveVisibleCandidates(scaledFixture.fixture.Input(), scaledFixture.fixture.aabbCache, nullptr, scaledCandidates);
    check(scaledCandidates.size() == 1, "the scaled Alloys manual group resolves exactly one candidate");

    if (!baselineCandidates.empty() && !scaledCandidates.empty()) {
        const float expected = baselineCandidates[0].screenSize * 2.0f * 3.0f;
        check(scaledCandidates[0].screenSize > expected * 0.99f && scaledCandidates[0].screenSize < expected * 1.01f,
              "layerIconScale(2.0) * scaleAlloy(3.0) composes into the manual marker's rendered screenSize");
    }
}

// STEP122 — an unrecognized manual group name is a no-op on the group-type term; only the per-layer
// iconScale term applies.
void CheckManualMarkerScaleUnrecognizedGroupNameStaysNoOp() {
    IconLayerTestFixture baselineFixture;
    baselineFixture.recipe.markerLayers.assign(1, Params::MarkerInstanceLayer());
    SeedAtlasEntry(baselineFixture.pairingLookup, baselineFixture.atlasManifest, "Generic", 0);
    Params::MarkerInstanceGroup baselineGroup;
    baselineGroup.name = "Generic";
    baselineGroup.transforms.push_back(ManualMarkerTestFixture::MakeVisibleMarkerTransform());
    baselineFixture.recipe.markers = {baselineGroup};
    OverlayLayer_UI baselineLayer; baselineLayer.domainKind = OverlayDomainKind_UI::Alloy;
    baselineLayer.thumbnailLodThresholdPixels = 1.0f;
    baselineLayer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::Manual, 0, true});
    baselineFixture.overlaySettings.overlayLayers = {baselineLayer};
    std::vector<OverlayVisibleInstance> baselineCandidates;
    ResolveVisibleCandidates(baselineFixture.Input(), baselineFixture.aabbCache, nullptr, baselineCandidates);
    check(baselineCandidates.size() == 1, "the unscaled Generic manual group resolves exactly one candidate");

    IconLayerTestFixture scaledFixture;
    scaledFixture.recipe.markerLayers.assign(1, Params::MarkerInstanceLayer());
    scaledFixture.recipe.markerLayers[0].iconScale = 2.0f;
    SeedAtlasEntry(scaledFixture.pairingLookup, scaledFixture.atlasManifest, "Generic", 0);
    Params::MarkerInstanceGroup scaledGroup;
    scaledGroup.name = "Generic";
    scaledGroup.transforms.push_back(ManualMarkerTestFixture::MakeVisibleMarkerTransform());
    scaledFixture.recipe.markers = {scaledGroup};
    OverlayLayer_UI scaledLayer; scaledLayer.domainKind = OverlayDomainKind_UI::Alloy;
    scaledLayer.thumbnailLodThresholdPixels = 1.0f;
    scaledLayer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::Manual, 0, true});
    scaledFixture.overlaySettings.overlayLayers = {scaledLayer};
    std::vector<OverlayVisibleInstance> scaledCandidates;
    ResolveVisibleCandidates(scaledFixture.Input(), scaledFixture.aabbCache, nullptr, scaledCandidates);
    check(scaledCandidates.size() == 1, "the layer-scaled Generic manual group resolves exactly one candidate");

    if (!baselineCandidates.empty() && !scaledCandidates.empty()) {
        const float expected = baselineCandidates[0].screenSize * 2.0f;
        check(scaledCandidates[0].screenSize > expected * 0.99f && scaledCandidates[0].screenSize < expected * 1.01f,
              "an unrecognized group name leaves the group-type term at 1.0f — only layerIconScale(2.0) applies");
    }
}

// STEP122 — ResolveProceduralSubLayer composes ResolveMarkerCategoryScale into the procedural
// marker's rendered screenSize, one case per resolvable category plus the Generic/Expansion no-op.
void CheckProceduralMarkerScaleComposesEndToEnd() {
    // Alloys category: scaleAlloy composes.
    {
        IconLayerTestFixture baselineFixture;
        baselineFixture.recipe.globalMarkerSettings.scaleAlloy = 1.0f;   // isolate the composed multiplier
        AppendMarkerInstance(baselineFixture.placements, 2.0f, 2.0f, 0, Params::MarkerCategory::Alloys, "markerA");
        baselineFixture.ruleBucketIndex.markers.Build(baselineFixture.placements.markers.ruleIndex.data(), 1, 1);
        SeedAtlasEntry(baselineFixture.pairingLookup, baselineFixture.atlasManifest, "markerA", 0);
        OverlayLayer_UI baselineLayer; baselineLayer.domainKind = OverlayDomainKind_UI::Alloy;
        baselineLayer.thumbnailLodThresholdPixels = 1.0f;
        baselineLayer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, 0, true});
        baselineFixture.overlaySettings.overlayLayers.push_back(baselineLayer);
        std::vector<OverlayVisibleInstance> baselineCandidates;
        ResolveVisibleCandidates(baselineFixture.Input(), baselineFixture.aabbCache, nullptr, baselineCandidates);
        check(baselineCandidates.size() == 1, "the baseline Alloys-category procedural marker resolves exactly one candidate");

        IconLayerTestFixture scaledFixture;
        scaledFixture.recipe.globalMarkerSettings.scaleAlloy = 3.0f;
        AppendMarkerInstance(scaledFixture.placements, 2.0f, 2.0f, 0, Params::MarkerCategory::Alloys, "markerA");
        scaledFixture.ruleBucketIndex.markers.Build(scaledFixture.placements.markers.ruleIndex.data(), 1, 1);
        SeedAtlasEntry(scaledFixture.pairingLookup, scaledFixture.atlasManifest, "markerA", 0);
        OverlayLayer_UI scaledLayer; scaledLayer.domainKind = OverlayDomainKind_UI::Alloy;
        scaledLayer.thumbnailLodThresholdPixels = 1.0f;
        scaledLayer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, 0, true});
        scaledFixture.overlaySettings.overlayLayers.push_back(scaledLayer);
        std::vector<OverlayVisibleInstance> scaledCandidates;
        ResolveVisibleCandidates(scaledFixture.Input(), scaledFixture.aabbCache, nullptr, scaledCandidates);
        check(scaledCandidates.size() == 1, "the scaled Alloys-category procedural marker resolves exactly one candidate");

        if (!baselineCandidates.empty() && !scaledCandidates.empty()) {
            const float expected = baselineCandidates[0].screenSize * 3.0f;
            check(scaledCandidates[0].screenSize > expected * 0.99f && scaledCandidates[0].screenSize < expected * 1.01f,
                  "GlobalMarkerSettings::scaleAlloy(3.0) composes into the Alloys-category procedural marker's screenSize");
        }
    }

    // Spawn category: scaleSpawn composes.
    {
        IconLayerTestFixture baselineFixture;
        baselineFixture.recipe.globalMarkerSettings.scaleSpawn = 1.0f;   // isolate the composed multiplier
        AppendMarkerInstance(baselineFixture.placements, 2.0f, 2.0f, 0, Params::MarkerCategory::Spawn, "markerS");
        baselineFixture.ruleBucketIndex.markers.Build(baselineFixture.placements.markers.ruleIndex.data(), 1, 1);
        SeedAtlasEntry(baselineFixture.pairingLookup, baselineFixture.atlasManifest, "markerS", 0);
        OverlayLayer_UI baselineLayer; baselineLayer.domainKind = OverlayDomainKind_UI::SpawnsArmies;
        baselineLayer.thumbnailLodThresholdPixels = 1.0f;
        baselineLayer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, 0, true});
        baselineFixture.overlaySettings.overlayLayers.push_back(baselineLayer);
        std::vector<OverlayVisibleInstance> baselineCandidates;
        ResolveVisibleCandidates(baselineFixture.Input(), baselineFixture.aabbCache, nullptr, baselineCandidates);
        check(baselineCandidates.size() == 1, "the baseline Spawn-category procedural marker resolves exactly one candidate");

        IconLayerTestFixture scaledFixture;
        scaledFixture.recipe.globalMarkerSettings.scaleSpawn = 5.0f;
        AppendMarkerInstance(scaledFixture.placements, 2.0f, 2.0f, 0, Params::MarkerCategory::Spawn, "markerS");
        scaledFixture.ruleBucketIndex.markers.Build(scaledFixture.placements.markers.ruleIndex.data(), 1, 1);
        SeedAtlasEntry(scaledFixture.pairingLookup, scaledFixture.atlasManifest, "markerS", 0);
        OverlayLayer_UI scaledLayer; scaledLayer.domainKind = OverlayDomainKind_UI::SpawnsArmies;
        scaledLayer.thumbnailLodThresholdPixels = 1.0f;
        scaledLayer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, 0, true});
        scaledFixture.overlaySettings.overlayLayers.push_back(scaledLayer);
        std::vector<OverlayVisibleInstance> scaledCandidates;
        ResolveVisibleCandidates(scaledFixture.Input(), scaledFixture.aabbCache, nullptr, scaledCandidates);
        check(scaledCandidates.size() == 1, "the scaled Spawn-category procedural marker resolves exactly one candidate");

        if (!baselineCandidates.empty() && !scaledCandidates.empty()) {
            const float expected = baselineCandidates[0].screenSize * 5.0f;
            check(scaledCandidates[0].screenSize > expected * 0.99f && scaledCandidates[0].screenSize < expected * 1.01f,
                  "GlobalMarkerSettings::scaleSpawn(5.0) composes into the Spawn-category procedural marker's screenSize");
        }
    }

    // Generic category: no MarkerCategory scale field exists — screenSize stays unscaled even with
    // non-default scaleAlloy/scaleSpawn set, mirroring CheckMarkerGenericAndExpansionStayWhite's own
    // no-bleed-through posture for tint.
    {
        IconLayerTestFixture baselineFixture;
        AppendMarkerInstance(baselineFixture.placements, 2.0f, 2.0f, 0, Params::MarkerCategory::Generic, "markerG");
        baselineFixture.ruleBucketIndex.markers.Build(baselineFixture.placements.markers.ruleIndex.data(), 1, 1);
        SeedAtlasEntry(baselineFixture.pairingLookup, baselineFixture.atlasManifest, "markerG", 0);
        OverlayLayer_UI baselineLayer; baselineLayer.domainKind = OverlayDomainKind_UI::Alloy;
        baselineLayer.thumbnailLodThresholdPixels = 1.0f;
        baselineLayer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, 0, true});
        baselineFixture.overlaySettings.overlayLayers.push_back(baselineLayer);
        std::vector<OverlayVisibleInstance> baselineCandidates;
        ResolveVisibleCandidates(baselineFixture.Input(), baselineFixture.aabbCache, nullptr, baselineCandidates);
        check(baselineCandidates.size() == 1, "the Generic-category procedural marker resolves exactly one candidate");

        IconLayerTestFixture noOpFixture;
        noOpFixture.recipe.globalMarkerSettings.scaleAlloy = 5.0f;   // non-default; must not leak into Generic
        noOpFixture.recipe.globalMarkerSettings.scaleSpawn = 5.0f;
        AppendMarkerInstance(noOpFixture.placements, 2.0f, 2.0f, 0, Params::MarkerCategory::Generic, "markerG");
        noOpFixture.ruleBucketIndex.markers.Build(noOpFixture.placements.markers.ruleIndex.data(), 1, 1);
        SeedAtlasEntry(noOpFixture.pairingLookup, noOpFixture.atlasManifest, "markerG", 0);
        OverlayLayer_UI noOpLayer; noOpLayer.domainKind = OverlayDomainKind_UI::Alloy;
        noOpLayer.thumbnailLodThresholdPixels = 1.0f;
        noOpLayer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, 0, true});
        noOpFixture.overlaySettings.overlayLayers.push_back(noOpLayer);
        std::vector<OverlayVisibleInstance> noOpCandidates;
        ResolveVisibleCandidates(noOpFixture.Input(), noOpFixture.aabbCache, nullptr, noOpCandidates);
        check(noOpCandidates.size() == 1, "the non-default-scale Generic-category procedural marker resolves exactly one candidate");

        if (!baselineCandidates.empty() && !noOpCandidates.empty())
            check(baselineCandidates[0].screenSize > noOpCandidates[0].screenSize * 0.99f
                  && baselineCandidates[0].screenSize < noOpCandidates[0].screenSize * 1.01f,
                  "Generic category's screenSize is unaffected by non-default scaleAlloy/scaleSpawn — category-scale no-op == 1.0f");
    }
}

} // namespace

void RunMapCanvasIconLayerCullChecks() {
    CheckAabbEarlyOut();
    CheckPairingMissLoggedOnce();
    CheckThumbnailModeAboveThreshold();
    CheckStrategicModeBelowThreshold();
    CheckManualReclaimPartitionCorrectness();
    CheckManualReclaimPredicateIsGroupLevel();
    CheckManualReclaimEmptyContributionNeverCrashes();
    CheckReclaimDeterminismGuardrailAndBudgetNonInflation();
    CheckMarkerAlloysCategoryResolvesColorAlloy();
    CheckMarkerSpawnCategoryResolvesColorSpawn();
    CheckMarkerGenericAndExpansionStayWhite();
    CheckManualPropLayerColorThreadsThrough();
    CheckManualDecalLayerColorThreadsThrough();
    CheckProceduralPropsAndManualUnitsStayWhite();
    CheckSelectedInstanceCandidateResolvesMarkerColor();
    CheckLayerColorOutOfRangeDefaultsWhite();
    CheckResolveMarkerIconTemplateIdentifier();
    CheckManualMarkerGroupNamePartition();
    CheckManualMarkerIconOverrideWinsEndToEnd();
    CheckManualMarkerLayerIndexFilter();
    CheckManualMarkerTypeDefaultColorResolvesEndToEnd();
    CheckManualMarkerLayerOverrideWinsOverTypeDefault();
    CheckManualMarkerGenericGroupStaysWhite();
    CheckResolveMarkerGroupTypeScale();
    CheckManualMarkerScaleComposesEndToEnd();
    CheckManualMarkerScaleUnrecognizedGroupNameStaysNoOp();
    CheckProceduralMarkerScaleComposesEndToEnd();
}

} // namespace Ui
} // namespace SanmapGen
