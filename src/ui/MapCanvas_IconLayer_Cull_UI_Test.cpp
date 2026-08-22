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

} // namespace

void RunMapCanvasIconLayerCullChecks() {
    CheckAabbEarlyOut();
    CheckPairingMissLoggedOnce();
    CheckThumbnailModeAboveThreshold();
    CheckStrategicModeBelowThreshold();
}

} // namespace Ui
} // namespace SanmapGen
