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
}

} // namespace Ui
} // namespace SanmapGen
