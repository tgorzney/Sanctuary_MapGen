// MapCanvas_IconLayer_MarkerTypeVisibility_UI_Test.cpp — STEP133 acceptance: the Markers tab's
// per-Type Hide/Unhide preview filter. Covers MarkerTypeVisibility_UI's own round-trip, the
// manual/procedural gates (including the category-ambiguity fixture the ticket's own "Ground truth"
// section flags — two MarkerRuleLayers sharing ONE MarkerCategory but different markerTypeName), the
// null-safe unfiltered baseline, and the C2 cache-invalidation regression this ticket exists to
// prevent (a live headless imgui frame, mirroring MapCanvas_IconLayer_Draw_UI_Test.cpp's own
// technique). One translation unit of the MapCanvas_IconLayer_UI_Test binary.
#include "MapCanvas_IconLayer_DrawInternal_UI.h"
#include "MapCanvas_IconLayer_TestFixture_UI.h"
#include "MarkerTypeVisibility_UI.h"
#include "ProceduralInstanceRuleIndex_UI.h"

namespace SanmapGen {
namespace Ui {
namespace {

// STEP133 Verify, item 1 — SetHidden/IsHidden round-trip; revision increments once per call (this
// struct's own deliberately-simple contract: even a redundant set-to-same-value bumps it, which is
// harmless — an extra cache rebuild, never a correctness bug — per MarkerTypeVisibility_UI.h's own
// Fix-section-provided shape).
void CheckSetHiddenIsHiddenRoundTrip() {
    MarkerTypeVisibility_UI visibility;
    check(!visibility.IsHidden("Alloy"), "an untouched type name starts visible (absent = false)");
    check(visibility.revision == 0, "a fresh struct starts at revision 0");

    visibility.SetHidden("Alloy", true);
    check(visibility.IsHidden("Alloy"), "SetHidden(true) makes IsHidden report true");
    check(visibility.revision == 1, "one SetHidden call bumps revision by exactly one");

    visibility.SetHidden("Alloy", false);
    check(!visibility.IsHidden("Alloy"), "SetHidden(false) makes IsHidden report false again");
    check(visibility.revision == 2, "a second SetHidden call bumps revision again");

    check(!visibility.IsHidden("Plasma"), "an unrelated type name is untouched by Alloy's own state");
}

// STEP133 Verify, item 5 — the procedural gate's own lookup, in isolation: two rules across two
// layers resolve to their OWN layer's markerTypeName, at the FlatMarkerRuleIndexBase numbering
// STEP132 already established (Fix section 6's own grounding requirement — "do not invent a second,
// possibly-inconsistent numbering scheme").
void CheckBuildMarkerRuleTypeNameLookupMatchesFlatIndexNumbering() {
    Params::MarkerRuleLayer alloyLayer;
    alloyLayer.markerTypeName = "Alloy";
    alloyLayer.rules.assign(2, Params::MarkerRule());   // flat indices 0, 1

    Params::MarkerRuleLayer plasmaLayer;
    plasmaLayer.markerTypeName = "Plasma";
    plasmaLayer.rules.assign(1, Params::MarkerRule());  // flat index 2

    const std::vector<Params::MarkerRuleLayer> layers{alloyLayer, plasmaLayer};
    const std::unordered_map<int, std::string> lookup = BuildMarkerRuleTypeNameLookup(layers);

    check(lookup.size() == 3, "three rules total across the two layers produce three lookup entries");
    check(lookup.at(0) == "Alloy" && lookup.at(1) == "Alloy",
          "both of the Alloy layer's own rules resolve its own markerTypeName");
    check(lookup.at(FlatMarkerRuleIndexBase(layers, 1)) == "Plasma",
          "the Plasma layer's own rule resolves at exactly FlatMarkerRuleIndexBase(layers, 1) — the "
          "SAME numbering STEP132's ProceduralInstanceRuleIndex_UI.h already establishes");
}

// STEP133 Verify, item 4 — manual gate: two MarkerInstanceGroups ("Alloys" and the reserved Spawn
// name), hiding "Alloys" produces candidates only from the Spawn group.
void CheckManualGateHidesOnlyTheHiddenGroup() {
    IconLayerTestFixture fixture;
    fixture.recipe.markerLayers.assign(1, Params::MarkerInstanceLayer());
    fixture.recipe.globalMarkerSettings.iconNameAlloy = "AlloyIcon";
    fixture.recipe.globalMarkerSettings.iconNameSpawn = "SpawnIcon";
    fixture.recipe.globalMarkerSettings.scaleAlloy = 1.0f;
    fixture.recipe.globalMarkerSettings.scaleSpawn = 1.0f;
    SeedAtlasEntry(fixture.pairingLookup, fixture.atlasManifest, "AlloyIcon", 0, /*atlasPage=*/0);
    SeedAtlasEntry(fixture.pairingLookup, fixture.atlasManifest, "SpawnIcon", 1, /*atlasPage=*/1);

    Params::MarkerTransform visibleTransform;
    visibleTransform.layerIndex = 0;
    visibleTransform.transform.positionX = 2.0f;   // in-view, IconLayerTestFixture's own header note
    visibleTransform.transform.positionZ = 2.0f;
    visibleTransform.transform.scaleX = 1.0f;

    Params::MarkerInstanceGroup alloysGroup;
    alloysGroup.name = "Alloys";
    alloysGroup.transforms.push_back(visibleTransform);
    Params::MarkerInstanceGroup spawnGroup;
    spawnGroup.name = Params::kSpawnMarkerGroupName;
    spawnGroup.transforms.push_back(visibleTransform);
    fixture.recipe.markers = {alloysGroup, spawnGroup};

    OverlayLayer_UI alloyLayer; alloyLayer.domainKind = OverlayDomainKind_UI::Alloy;
    alloyLayer.thumbnailLodThresholdPixels = 1.0f;
    alloyLayer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::Manual, 0, true});
    OverlayLayer_UI spawnsArmiesLayer; spawnsArmiesLayer.domainKind = OverlayDomainKind_UI::SpawnsArmies;
    spawnsArmiesLayer.thumbnailLodThresholdPixels = 1.0f;
    spawnsArmiesLayer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::Manual, 0, true});
    fixture.overlaySettings.overlayLayers = {alloyLayer, spawnsArmiesLayer};

    MarkerTypeVisibility_UI visibility;
    visibility.SetHidden("Alloys", true);
    DrawOverlayIconLayersInput input = fixture.Input();
    input.markerTypeVisibility = &visibility;

    std::vector<OverlayVisibleInstance> candidates;
    ResolveVisibleCandidates(input, fixture.aabbCache, nullptr, candidates);
    check(candidates.size() == 1, "hiding the Alloys group leaves exactly the Spawn group's one candidate");
    if (!candidates.empty())
        check(candidates[0].atlasPage == 1, "the surviving candidate is the Spawn group's own atlas page");
}

// STEP133 Verify, item 6 — procedural gate, the category-ambiguity fixture: two MarkerRuleLayers
// SHARING MarkerCategory::Alloys (proving category alone cannot disambiguate) but typed "Alloy" and
// "Plasma" respectively; hiding "Plasma" produces candidates only from the Alloy-typed rule.
void CheckProceduralGateResolvesByMarkerTypeNameNotCategory() {
    IconLayerTestFixture fixture;
    AppendMarkerInstance(fixture.placements, 2.0f, 2.0f, /*ruleIndex*/0, Params::MarkerCategory::Alloys, "alloyRl");
    AppendMarkerInstance(fixture.placements, 2.0f, 2.0f, /*ruleIndex*/1, Params::MarkerCategory::Alloys, "plasmRl");
    const int ruleIndexColumn[2] = {0, 1};
    fixture.ruleBucketIndex.markers.Build(ruleIndexColumn, 2, 2);
    SeedAtlasEntry(fixture.pairingLookup, fixture.atlasManifest, "alloyRl", 0, /*atlasPage=*/0);
    SeedAtlasEntry(fixture.pairingLookup, fixture.atlasManifest, "plasmRl", 1, /*atlasPage=*/1);

    Params::MarkerRuleLayer alloyTypedLayer;
    alloyTypedLayer.markerTypeName = "Alloy";
    alloyTypedLayer.rules.assign(1, Params::MarkerRule());
    alloyTypedLayer.rules[0].category = Params::MarkerCategory::Alloys;
    Params::MarkerRuleLayer plasmaTypedLayer;
    plasmaTypedLayer.markerTypeName = "Plasma";
    plasmaTypedLayer.rules.assign(1, Params::MarkerRule());
    plasmaTypedLayer.rules[0].category = Params::MarkerCategory::Alloys;   // SAME category, deliberately
    fixture.recipe.markerRuleLayers = {alloyTypedLayer, plasmaTypedLayer};

    OverlayLayer_UI alloyDomainLayer; alloyDomainLayer.domainKind = OverlayDomainKind_UI::Alloy;
    alloyDomainLayer.thumbnailLodThresholdPixels = 1.0f;
    alloyDomainLayer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, 0, true});
    alloyDomainLayer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, 1, true});
    fixture.overlaySettings.overlayLayers = {alloyDomainLayer};

    // Baseline, unfiltered: both rules resolve (proves the fixture itself is sound before the gate
    // is exercised, and doubles as the null-safe/unfiltered-baseline check, Verify item 7).
    std::vector<OverlayVisibleInstance> unfilteredCandidates;
    ResolveVisibleCandidates(fixture.Input(), fixture.aabbCache, nullptr, unfilteredCandidates);
    check(unfilteredCandidates.size() == 2,
          "with markerTypeVisibility == nullptr, both same-category rules resolve — the unfiltered baseline");

    MarkerTypeVisibility_UI visibility;
    visibility.SetHidden("Plasma", true);
    DrawOverlayIconLayersInput input = fixture.Input();
    input.markerTypeVisibility = &visibility;

    IconLayerAabbCache_UI freshAabbCache;   // a fresh cache — the hidden rule must never contribute
    std::vector<OverlayVisibleInstance> filteredCandidates;
    ResolveVisibleCandidates(input, freshAabbCache, nullptr, filteredCandidates);
    check(filteredCandidates.size() == 1,
          "hiding \"Plasma\" leaves exactly the Alloy-typed rule's one candidate, despite both rules "
          "sharing MarkerCategory::Alloys — proving the gate resolves markerTypeName, not category");
    if (!filteredCandidates.empty())
        check(filteredCandidates[0].atlasPage == 0,
              "the surviving candidate is the Alloy-typed rule's own atlas page");
}

// STEP133 Verify, item 3 — the cache-invalidation regression this ticket exists to prevent: toggling
// MarkerTypeVisibility_UI's hidden state between frame 1 and frame 2, with the view/selection
// otherwise UNCHANGED, still forces a full rebuild on frame 2 (never a stale replay).
void CheckToggleAloneInvalidatesTheRenderCacheAcrossFrames() {
    IconLayerTestFixture fixture;
    AppendPropInstance(fixture.placements, 2.0f, 2.0f, 0, "propA");
    fixture.ruleBucketIndex.props.Build(fixture.placements.props.ruleIndex.data(), 1, 1);
    SeedAtlasEntry(fixture.pairingLookup, fixture.atlasManifest, "propA", 0, /*atlasPage=*/0);
    OverlayLayer_UI layer; layer.domainKind = OverlayDomainKind_UI::Props;
    layer.thumbnailLodThresholdPixels = 1.0f;
    layer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, 0, true});
    fixture.overlaySettings.overlayLayers.push_back(layer);

    MarkerTypeVisibility_UI visibility;   // starts empty/visible; never even resolved by Props above —
                                          // only its OWN revision counter matters to this check.
    DrawOverlayIconLayersInput input = fixture.Input();
    input.markerTypeVisibility = &visibility;
    input.markerTypeVisibilityRevision = visibility.revision;   // mirrors MapCanvas_Draw_UI.cpp's own wiring

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(256.0f, 256.0f);
    io.DeltaTime = 1.0f / 60.0f;
    unsigned char* atlasPixels = nullptr; int atlasWidth = 0, atlasHeight = 0;
    io.Fonts->GetTexDataAsRGBA32(&atlasPixels, &atlasWidth, &atlasHeight);

    IconLayerGenerationDiagnostics_UI generationDiagnostics;
    auto runOneFrame = [&] {
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(256.0f, 256.0f));
        ImGui::Begin("MarkerTypeVisibilityCacheTestWindow");
        DrawOverlayIconLayers(input, fixture.aabbCache, fixture.frameCache, *ImGui::GetWindowDrawList(),
                              nullptr, nullptr, &generationDiagnostics);
        ImGui::End();
        ImGui::Render();
    };

    runOneFrame();   // frame 1: a fresh cache always rebuilds
    check(generationDiagnostics.fullGenerationCount == 1, "frame 1 (fresh cache) does a full generation");

    runOneFrame();   // frame 2: identical view/selection/toggle — a REPLAY, proving the fixture is sound
    check(generationDiagnostics.fullGenerationCount == 1,
          "frame 2, nothing changed at all, replays the cache rather than rebuilding — the control");

    // The toggle ALONE, nothing else: view/selection stay byte-identical.
    visibility.SetHidden("Alloy", true);
    input.markerTypeVisibilityRevision = visibility.revision;
    runOneFrame();   // frame 3
    check(generationDiagnostics.fullGenerationCount == 2,
          "the Hide/Unhide toggle alone, nothing else changed, still forces a full rebuild on the next "
          "frame — the exact regression this ticket exists to prevent");

    ImGui::DestroyContext();
}

} // namespace

void RunMapCanvasIconLayerMarkerTypeVisibilityChecks() {
    CheckSetHiddenIsHiddenRoundTrip();
    CheckBuildMarkerRuleTypeNameLookupMatchesFlatIndexNumbering();
    CheckManualGateHidesOnlyTheHiddenGroup();
    CheckProceduralGateResolvesByMarkerTypeNameNotCategory();
    CheckToggleAloneInvalidatesTheRenderCacheAcrossFrames();
}

} // namespace Ui
} // namespace SanmapGen
