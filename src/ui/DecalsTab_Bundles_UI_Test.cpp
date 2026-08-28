// DecalsTab_Bundles_UI_Test.cpp — acceptance for the Decals Group/Bundle tree's pure delete logic
// (ARCH §20), mirroring PropsTab_Bundles_UI_Test.cpp exactly at the Decal domain.
#include "DecalsTab_Bundles_UI.h"
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

void BuildFixture(std::vector<Params::DecalLayerBundle>& bundles,
                  std::vector<Params::DecalInstanceLayer>& layers,
                  std::vector<Params::DecalInstanceGroup>& decals) {
    bundles = {
        Params::DecalLayerBundle{ 1, "Root", -1, -1 },
        Params::DecalLayerBundle{ 2, "Child", 1, -1 },
        Params::DecalLayerBundle{ 3, "Grandchild", 2, -1 },
    };
    layers.resize(3);
    layers[0].name = "L0"; layers[0].parentBundleIdentifier = 1;
    layers[1].name = "L1"; layers[1].parentBundleIdentifier = 2;
    layers[2].name = "L2"; layers[2].parentBundleIdentifier = -1;

    Params::DecalInstanceGroup group;
    group.blueprintPath = "Decals/Test.santp";
    Params::DecalTransform t0; t0.layerIndex = 0; group.transforms.push_back(t0);
    Params::DecalTransform t1; t1.layerIndex = 1; group.transforms.push_back(t1);
    Params::DecalTransform t2; t2.layerIndex = 2; group.transforms.push_back(t2);
    decals.push_back(group);
}

void RunGroupOnlyDeleteChecks() {
    std::vector<Params::DecalLayerBundle> bundles;
    std::vector<Params::DecalInstanceLayer> layers;
    std::vector<Params::DecalInstanceGroup> decals;
    BuildFixture(bundles, layers, decals);

    DeleteDecalLayerBundleGroupOnly(2, bundles, layers);

    Check(bundles.size() == 2, "Group Only erases exactly the target bundle");
    for (const Params::DecalLayerBundle& bundle : bundles)
        Check(bundle.identifier != 2, "the erased bundle is gone");
    for (const Params::DecalLayerBundle& bundle : bundles)
        if (bundle.identifier == 3)
            Check(bundle.parentBundleIdentifier == 1,
                  "the erased bundle's own child is promoted to ITS parent (1), not orphaned");
    Check(layers[1].parentBundleIdentifier == 1,
          "a layer parented to the erased bundle is promoted to its parent too");
    Check(layers[0].parentBundleIdentifier == 1 && layers[2].parentBundleIdentifier == -1,
          "layers not parented to the erased bundle are untouched");
    Check(decals[0].transforms.size() == 3, "Group Only never touches decal transforms");
}

void RunCascadeDeleteChecks() {
    std::vector<Params::DecalLayerBundle> bundles;
    std::vector<Params::DecalInstanceLayer> layers;
    std::vector<Params::DecalInstanceGroup> decals;
    BuildFixture(bundles, layers, decals);

    DeleteDecalLayerBundleCascade(1, bundles, layers, decals);

    Check(bundles.empty(), "Cascade from the root erases every bundle in the chain");
    Check(layers.size() == 1 && layers[0].name == "L2",
          "Cascade erases every layer parented anywhere in the chain, keeping the ungrouped one");
    Check(decals[0].transforms.size() == 1,
          "Cascade erases every decal transform that referenced a deleted layer");
    if (!decals[0].transforms.empty())
        Check(decals[0].transforms[0].layerIndex == 0,
              "the surviving transform's layerIndex shifts down to match the surviving layer's new position");
}

void RunLayerOnlyDeleteChecks() {
    std::vector<Params::DecalInstanceLayer> layers(2);
    layers[0].name = "Kept"; layers[1].name = "Removed";
    std::vector<Params::DecalInstanceGroup> decals;
    Params::DecalInstanceGroup group;
    Params::DecalTransform t; t.layerIndex = 1;
    group.transforms.push_back(t);
    decals.push_back(group);

    DeleteDecalInstanceLayerOnly(1, layers, decals);

    Check(layers.size() == 1 && layers[0].name == "Kept", "Layer Only erases exactly the target layer");
    Check(decals[0].transforms.size() == 1 && decals[0].transforms[0].layerIndex == 0,
          "a transform that referenced the removed layer clamps to layer 0, never dropped");
}

void RunLayerCascadeDeleteChecks() {
    std::vector<Params::DecalInstanceLayer> layers(2);
    layers[0].name = "Kept"; layers[1].name = "Removed";
    std::vector<Params::DecalInstanceGroup> decals;
    Params::DecalInstanceGroup group;
    Params::DecalTransform kept; kept.layerIndex = 0;
    Params::DecalTransform removed; removed.layerIndex = 1;
    group.transforms.push_back(kept);
    group.transforms.push_back(removed);
    decals.push_back(group);

    DeleteDecalInstanceLayerCascade(1, layers, decals);

    Check(layers.size() == 1 && layers[0].name == "Kept", "Cascade erases exactly the target layer");
    Check(decals[0].transforms.size() == 1 && decals[0].transforms[0].layerIndex == 0,
          "Cascade erases every transform that referenced the removed layer, keeping the other");
}

void RunNextDecalLayerBundleIdChecks() {
    std::vector<Params::DecalLayerBundle> bundles;
    Check(NextDecalLayerBundleId(bundles) == 0, "an empty bundle vector mints id 0");
    bundles.push_back(Params::DecalLayerBundle{ 0, "A", -1, -1 });
    bundles.push_back(Params::DecalLayerBundle{ 5, "B", -1, -1 });
    Check(NextDecalLayerBundleId(bundles) == 6, "ids {0, 5} mint 6 - max-plus-one, not count-based");
}

} // namespace

int main() {
    RunGroupOnlyDeleteChecks();
    RunCascadeDeleteChecks();
    RunLayerOnlyDeleteChecks();
    RunLayerCascadeDeleteChecks();
    RunNextDecalLayerBundleIdChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
