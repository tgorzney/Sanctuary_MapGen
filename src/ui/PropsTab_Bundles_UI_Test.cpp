// PropsTab_Bundles_UI_Test.cpp — acceptance for the Props Group/Bundle tree's pure delete logic
// (ARCH §20), mirroring MarkersTab_Bundles_UI_Test.cpp's own delete-function coverage at this
// file's trimmed scope (no RuleLayer branch — PropRuleLayer doesn't exist yet). Pure logic only,
// no imgui frame needed.
#include "PropsTab_Bundles_UI.h"
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

// Bundle 1 (root) -> Bundle 2 (child of 1) -> Bundle 3 (child of 2). Layer 0 parented to Bundle 1,
// Layer 1 parented to Bundle 2, Layer 2 ungrouped (parentBundleIdentifier == -1).
void BuildFixture(std::vector<Params::PropLayerBundle>& bundles,
                  std::vector<Params::PropInstanceLayer>& layers,
                  std::vector<Params::PropInstanceGroup>& props) {
    bundles = {
        Params::PropLayerBundle{ 1, "Root", -1, "Prop", -1 },
        Params::PropLayerBundle{ 2, "Child", 1, "Prop", -1 },
        Params::PropLayerBundle{ 3, "Grandchild", 2, "Prop", -1 },
    };
    layers.resize(3);
    layers[0].name = "L0"; layers[0].parentBundleIdentifier = 1;
    layers[1].name = "L1"; layers[1].parentBundleIdentifier = 2;
    layers[2].name = "L2"; layers[2].parentBundleIdentifier = -1;

    Params::PropInstanceGroup group;
    group.blueprintPath = "Props/Test.santp";
    Params::PropTransform t0; t0.layerIndex = 0; group.transforms.push_back(t0);
    Params::PropTransform t1; t1.layerIndex = 1; group.transforms.push_back(t1);
    Params::PropTransform t2; t2.layerIndex = 2; group.transforms.push_back(t2);
    props.push_back(group);
}

void RunGroupOnlyDeleteChecks() {
    std::vector<Params::PropLayerBundle> bundles;
    std::vector<Params::PropInstanceLayer> layers;
    std::vector<Params::PropInstanceGroup> props;
    BuildFixture(bundles, layers, props);

    DeletePropLayerBundleGroupOnly(2, bundles, layers);   // erase the middle bundle only

    Check(bundles.size() == 2, "Group Only erases exactly the target bundle");
    for (const Params::PropLayerBundle& bundle : bundles)
        Check(bundle.identifier != 2, "the erased bundle is gone");
    for (const Params::PropLayerBundle& bundle : bundles)
        if (bundle.identifier == 3)
            Check(bundle.parentBundleIdentifier == 1,
                  "the erased bundle's own child is promoted to ITS parent (1), not orphaned");
    Check(layers[1].parentBundleIdentifier == 1,
          "a layer parented to the erased bundle is promoted to its parent too");
    Check(layers[0].parentBundleIdentifier == 1 && layers[2].parentBundleIdentifier == -1,
          "layers not parented to the erased bundle are untouched");
    Check(props[0].transforms.size() == 3, "Group Only never touches prop transforms");
}

void RunCascadeDeleteChecks() {
    std::vector<Params::PropLayerBundle> bundles;
    std::vector<Params::PropInstanceLayer> layers;
    std::vector<Params::PropInstanceGroup> props;
    BuildFixture(bundles, layers, props);

    DeletePropLayerBundleCascade(1, bundles, layers, props);   // erase the root -> everything under it

    Check(bundles.empty(), "Cascade from the root erases every bundle in the chain");
    Check(layers.size() == 1 && layers[0].name == "L2",
          "Cascade erases every layer parented anywhere in the chain, keeping the ungrouped one");
    Check(props[0].transforms.size() == 1,
          "Cascade erases every prop transform that referenced a deleted layer");
    if (!props[0].transforms.empty())
        Check(props[0].transforms[0].layerIndex == 0,
              "the surviving transform's layerIndex shifts down to match the surviving layer's new position");
}

void RunLayerOnlyDeleteChecks() {
    std::vector<Params::PropInstanceLayer> layers(2);
    layers[0].name = "Kept"; layers[1].name = "Removed";
    std::vector<Params::PropInstanceGroup> props;
    Params::PropInstanceGroup group;
    Params::PropTransform t; t.layerIndex = 1;
    group.transforms.push_back(t);
    props.push_back(group);

    DeletePropInstanceLayerOnly(1, layers, props);

    Check(layers.size() == 1 && layers[0].name == "Kept", "Layer Only erases exactly the target layer");
    Check(props[0].transforms.size() == 1 && props[0].transforms[0].layerIndex == 0,
          "a transform that referenced the removed layer clamps to layer 0, never dropped");
}

void RunLayerCascadeDeleteChecks() {
    std::vector<Params::PropInstanceLayer> layers(2);
    layers[0].name = "Kept"; layers[1].name = "Removed";
    std::vector<Params::PropInstanceGroup> props;
    Params::PropInstanceGroup group;
    Params::PropTransform kept; kept.layerIndex = 0;
    Params::PropTransform removed; removed.layerIndex = 1;
    group.transforms.push_back(kept);
    group.transforms.push_back(removed);
    props.push_back(group);

    DeletePropInstanceLayerCascade(1, layers, props);

    Check(layers.size() == 1 && layers[0].name == "Kept", "Cascade erases exactly the target layer");
    Check(props[0].transforms.size() == 1 && props[0].transforms[0].layerIndex == 0,
          "Cascade erases every transform that referenced the removed layer, keeping the other");
}

void RunNextPropLayerBundleIdChecks() {
    std::vector<Params::PropLayerBundle> bundles;
    Check(NextPropLayerBundleId(bundles) == 0, "an empty bundle vector mints id 0");
    bundles.push_back(Params::PropLayerBundle{ 0, "A", -1, "Prop", -1 });
    bundles.push_back(Params::PropLayerBundle{ 5, "B", -1, "Prop", -1 });
    Check(NextPropLayerBundleId(bundles) == 6, "ids {0, 5} mint 6 - max-plus-one, not count-based");
}

} // namespace

int main() {
    RunGroupOnlyDeleteChecks();
    RunCascadeDeleteChecks();
    RunLayerOnlyDeleteChecks();
    RunLayerCascadeDeleteChecks();
    RunNextPropLayerBundleIdChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
