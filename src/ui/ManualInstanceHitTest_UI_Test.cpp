// ManualInstanceHitTest_UI_Test.cpp — headless acceptance coverage for the generic
// HitTestManualInstances<GroupT>/CollectManualInstancesInWorldRegion<GroupT> (ARCH §21.3/§21.5).
// Exercises all three domains (Markers/Props/Decals) to prove the template genuinely genericizes
// (not just compiles for one instantiation), plus the §21.5 lock-gate this ticket adds that the old
// Marker-only HitTestManualMarkers never had.
#include "ManualInstanceHitTest_UI.h"
#include "PreviewComposite_TestScene_UI.h"
#include "../params/MarkerInstance_PARAMS.h"
#include "../params/PropInstance_PARAMS.h"
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

struct Fixture {
    PreviewTestScene scene;
    PreviewComposite* composite;
    MapCanvasView view;
    Fixture() {
        BuildPreviewTestScene(scene);
        composite = new PreviewComposite(scene.geometry, scene.water, scene.strata, scene.areas, scene.fields,
                                         scene.instances, scene.entityIdentifiers);
        ConfigurePreviewSettings(composite->Settings());
        composite->ComposeOnCpu();
        view.SetPreviewResolution(composite->Resolution());
        view.SetRegionSide(256.0f);
    }
    ~Fixture() { delete composite; }
    Fixture(const Fixture&) = delete;
    Fixture& operator=(const Fixture&) = delete;
};

RegionLocalPoint ScreenPointFor(const Fixture& fixture, float worldX, float worldZ) {
    const PreviewComposite::PreviewPixelPoint previewPixel = fixture.composite->WorldToPreviewPixel(worldX, worldZ);
    return fixture.view.ProjectPreviewPixelToRegionLocal(previewPixel.pixelX, previewPixel.pixelY);
}

Params::MarkerTransform MakeMarkerTransform(float x, float z, int layerIndex = 0) {
    Params::MarkerTransform t; t.transform.positionX = x; t.transform.positionZ = z; t.layerIndex = layerIndex;
    return t;
}
Params::PropTransform MakePropTransform(float x, float z, int layerIndex = 0) {
    Params::PropTransform t; t.transform.positionX = x; t.transform.positionZ = z; t.layerIndex = layerIndex;
    return t;
}

const std::function<bool(int)> kAlwaysUnlocked = [](int) { return false; };

// ---- HitTestManualInstances: parity with the old Marker-only behavior, now also proven for Props.

void RunHitTestParityChecks() {
    Fixture fixture;
    std::vector<Params::MarkerInstanceGroup> markers(2);
    markers[0].transforms.push_back(MakeMarkerTransform(1.0f, 1.0f));
    markers[1].transforms.push_back(MakeMarkerTransform(3.0f, 3.0f));

    const RegionLocalPoint onA = ScreenPointFor(fixture, 1.0f, 1.0f);
    int groupIndex = -99, transformIndex = -99;
    Check(HitTestManualInstances<Params::MarkerInstanceGroup>(
              markers, *fixture.composite, fixture.view, onA.regionLocalX, onA.regionLocalY, 8.0f,
              kAlwaysUnlocked, groupIndex, transformIndex),
          "a press exactly on an instance's projected point hits it");
    Check(groupIndex == 0 && transformIndex == 0, "resolves the correct (group, transform) pair");

    groupIndex = -99; transformIndex = -99;
    Check(!HitTestManualInstances<Params::MarkerInstanceGroup>(
              markers, *fixture.composite, fixture.view, -500.0f, -500.0f, 8.0f, kAlwaysUnlocked,
              groupIndex, transformIndex),
          "a press far from every instance misses");
    Check(groupIndex == -1 && transformIndex == -1, "a miss leaves both out-params at -1");

    Check(!HitTestManualInstances<Params::MarkerInstanceGroup>(
              {}, *fixture.composite, fixture.view, 0.0f, 0.0f, 8.0f, kAlwaysUnlocked,
              groupIndex, transformIndex),
          "an empty roster never hits");

    // Same algorithm, a second domain: Props.
    std::vector<Params::PropInstanceGroup> props(1);
    props[0].transforms.push_back(MakePropTransform(5.0f, 5.0f));
    const RegionLocalPoint onProp = ScreenPointFor(fixture, 5.0f, 5.0f);
    groupIndex = -99; transformIndex = -99;
    Check(HitTestManualInstances<Params::PropInstanceGroup>(
              props, *fixture.composite, fixture.view, onProp.regionLocalX, onProp.regionLocalY, 8.0f,
              kAlwaysUnlocked, groupIndex, transformIndex),
          "the same template instantiated for Props finds a hit identically");
}

// ---- ARCH §21.5: a locked instance is skipped entirely, never becomes a hit candidate at all.

void RunHitTestLockGateChecks() {
    Fixture fixture;
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].transforms.push_back(MakeMarkerTransform(2.0f, 2.0f, /*layerIndex=*/3));
    const RegionLocalPoint onIt = ScreenPointFor(fixture, 2.0f, 2.0f);

    const std::function<bool(int)> lockLayerThree = [](int layerIndex) { return layerIndex == 3; };
    int groupIndex = -99, transformIndex = -99;
    Check(!HitTestManualInstances<Params::MarkerInstanceGroup>(
              markers, *fixture.composite, fixture.view, onIt.regionLocalX, onIt.regionLocalY, 8.0f,
              lockLayerThree, groupIndex, transformIndex),
          "a locked owning layer's instance is never a click-select candidate, even sitting exactly under the cursor");
    Check(groupIndex == -1 && transformIndex == -1, "the out-params stay at -1, not a stale/partial hit");

    Check(HitTestManualInstances<Params::MarkerInstanceGroup>(
              markers, *fixture.composite, fixture.view, onIt.regionLocalX, onIt.regionLocalY, 8.0f,
              kAlwaysUnlocked, groupIndex, transformIndex),
          "the same instance IS a candidate once its layer is not the locked one");

    // A nearer LOCKED instance never beats a farther UNLOCKED one — the lock gate excludes it
    // before distance comparison even runs, not merely deprioritizes it.
    std::vector<Params::MarkerInstanceGroup> two(2);
    two[0].transforms.push_back(MakeMarkerTransform(2.0f, 2.0f, /*layerIndex=*/3));   // locked, nearer
    two[1].transforms.push_back(MakeMarkerTransform(2.5f, 2.5f, /*layerIndex=*/0));   // unlocked, farther
    groupIndex = -99; transformIndex = -99;
    // A generous radius (500px, matching this file's own PickMarker-mirrored tie-test precedent):
    // the point of this check is the lock exclusion + ranking behavior, not the radius boundary.
    const bool bHitFarther = HitTestManualInstances<Params::MarkerInstanceGroup>(
        two, *fixture.composite, fixture.view, onIt.regionLocalX, onIt.regionLocalY, 500.0f,
        lockLayerThree, groupIndex, transformIndex);
    Check(bHitFarther && groupIndex == 1,
          "a nearer locked instance never wins over a farther unlocked one — excluded before ranking");
}

// ---- CollectManualInstancesInWorldRegion: box membership + the same lock gate, and it APPENDS
// (never clears) since §21.2's marquee resolver concatenates multiple domains into one list.

void RunCollectRegionChecks() {
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].transforms.push_back(MakeMarkerTransform(10.0f, 10.0f, /*layerIndex=*/0));   // inside
    markers[0].transforms.push_back(MakeMarkerTransform(999.0f, 999.0f, /*layerIndex=*/0)); // outside
    markers[0].transforms.push_back(MakeMarkerTransform(11.0f, 11.0f, /*layerIndex=*/5));   // inside, locked

    std::vector<std::pair<int, int>> hits;
    hits.emplace_back(77, 77);   // pre-existing entries from an earlier-queried domain: must survive
    const std::function<bool(int)> lockLayerFive = [](int layerIndex) { return layerIndex == 5; };
    CollectManualInstancesInWorldRegion<Params::MarkerInstanceGroup>(
        markers, 0.0f, 0.0f, 20.0f, 20.0f, lockLayerFive, hits);
    Check(hits.size() == 2, "the pre-existing entry survives (append, not clear) and exactly one new hit is added");
    Check(hits[0] == std::make_pair(77, 77), "the earlier domain's entry is untouched, still first");
    Check(hits[1] == std::make_pair(0, 0), "the unlocked in-box instance is collected; the locked one is not");

    // Same algorithm, Decals — proving the third domain too.
    std::vector<Params::DecalInstanceGroup> decals(1);
    Params::DecalTransform decalTransform; decalTransform.transform.positionX = 5.0f;
    decalTransform.transform.positionZ = 5.0f;
    decals[0].transforms.push_back(decalTransform);
    std::vector<std::pair<int, int>> decalHits;
    CollectManualInstancesInWorldRegion<Params::DecalInstanceGroup>(
        decals, 0.0f, 0.0f, 10.0f, 10.0f, kAlwaysUnlocked, decalHits);
    Check(decalHits.size() == 1 && decalHits[0] == std::make_pair(0, 0),
          "the same template instantiated for Decals collects a box hit identically");

    // A degenerate box collects nothing, never crashes.
    std::vector<std::pair<int, int>> degenerateHits;
    CollectManualInstancesInWorldRegion<Params::MarkerInstanceGroup>(
        markers, 20.0f, 20.0f, 0.0f, 0.0f, kAlwaysUnlocked, degenerateHits);
    Check(degenerateHits.empty(), "a degenerate box (min > max) collects nothing");
}

} // namespace

int main() {
    RunHitTestParityChecks();
    RunHitTestLockGateChecks();
    RunCollectRegionChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
