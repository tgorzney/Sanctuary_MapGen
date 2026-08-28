// DecalsTab_Manual_UI_Test.cpp — STEP22 acceptance for the manual decal layers block, a
// mirror of PropsTab_UI_Test.cpp's manual prop layer checks over `Params::DecalInstanceLayer`/
// `recipe.decals`. The checks that matter: deleting a layer must never drop a decal instance
// (CLAMPS every referencing `layerIndex` to 0); reordering layers must renumber every referencing
// `layerIndex` (below-target, above-target, no-op); and two "Add Decal Layer" clicks must never
// collide on export. All of it is pure (DecalsTab_Manual_UI.h), so the binary needs no imgui
// frame, no window and no GL context.
#include "DecalsTab_Manual_UI.h"
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

// Manual decal layers are real recipe content (`Params::DecalInstanceLayer`), so the invariants are
// the selection/override/label fences over the vector `DrawManualDecalLayers` is handed directly.
void RunManualDecalLayerChecks() {
    ManualDecalLayersState state;
    std::vector<Params::DecalInstanceLayer> decalLayers;
    Check(SelectedManualDecalLayer(decalLayers, state.selectedLayerIndex) == nullptr,
          "an empty block selects no layer");
    decalLayers.push_back(Params::DecalInstanceLayer());
    state.selectedLayerIndex = 0;
    Check(SelectedManualDecalLayer(decalLayers, state.selectedLayerIndex) == &decalLayers[0],
          "the selected layer is reachable");
    state.selectedLayerIndex = 2;
    Check(SelectedManualDecalLayer(decalLayers, state.selectedLayerIndex) == nullptr,
          "an index past the last layer selects nothing");

    Params::DecalInstanceLayer& layer = decalLayers[0];
    layer.color[0]      = 0.25f;
    state.groupColor[0] = 0.75f;
    Check(EffectiveManualDecalLayerColor(state, layer) == layer.color,
          "with the shared tint off a layer draws its OWN color");
    state.bUseGroupColor = true;
    Check(EffectiveManualDecalLayerColor(state, layer) == state.groupColor,
          "and with it on every layer draws the one shared color");

    Check(ManualDecalLayerRowLabel(layer) != nullptr && ManualDecalLayerRowLabel(layer)[0] != '\0',
          "an unnamed layer still draws a label");
    Check(state.iconScaleRange.minimumValue == 0.1f && state.iconScaleRange.maximumValue == 10.0f,
          "the icon scale sliders carry the plan's 0.1-10");
}

// Two decal groups, four transforms across three layers, in recipe order: 0, 1, 0, 2.
std::vector<Params::DecalInstanceGroup> MakeDecalsWithLayers() {
    std::vector<Params::DecalInstanceGroup> decals(2);
    decals[0].transforms.resize(3);
    decals[0].transforms[0].layerIndex = 0;
    decals[0].transforms[1].layerIndex = 1;
    decals[0].transforms[2].layerIndex = 0;
    decals[1].transforms.resize(1);
    decals[1].transforms[0].layerIndex = 2;
    return decals;
}

int DecalCountAcrossGroups(const std::vector<Params::DecalInstanceGroup>& decals) {
    int count = 0;
    for (const auto& group : decals) count += static_cast<int>(group.transforms.size());
    return count;
}

// STEP22 ruling #5 — the DELIBERATE divergence from Step 20's `DropUnitRulesForRemovedArmy`: an
// orphaned decal transform CLAMPS to layer 0, it is never dropped.
void RunDecalLayerRemovalChecks() {
    std::vector<Params::DecalInstanceGroup> decals = MakeDecalsWithLayers();
    const int countBeforeRemoval = DecalCountAcrossGroups(decals);
    Check(ClampDecalLayerIndicesForRemovedLayer(decals, 1),
          "removing a layer with transforms reports the move");
    Check(DecalCountAcrossGroups(decals) == countBeforeRemoval,
          "not one decal transform is dropped - a decal losing its layer tag is still a real decal");
    Check(decals[0].transforms[0].layerIndex == 0 && decals[0].transforms[2].layerIndex == 0,
          "layers BELOW the removed one keep their index");
    Check(decals[1].transforms[0].layerIndex == 1,
          "and every layer above it shifts down one");

    decals = MakeDecalsWithLayers();
    Check(ClampDecalLayerIndicesForRemovedLayer(decals, 0),
          "removing layer 0 clamps every transform that named it");
    Check(decals[0].transforms[0].layerIndex == 0 && decals[0].transforms[2].layerIndex == 0,
          "the orphaned transforms clamp to layer 0, not -1 or any sentinel");
    Check(decals[0].transforms[1].layerIndex == 0 && decals[1].transforms[0].layerIndex == 1,
          "every surviving layer above the removed one shifts down one");

    decals = MakeDecalsWithLayers();
    Check(!ClampDecalLayerIndicesForRemovedLayer(decals, -1), "a signal about no layer at all changes nothing");
    std::vector<Params::DecalInstanceGroup> emptyDecals;
    Check(!ClampDecalLayerIndicesForRemovedLayer(emptyDecals, 0), "and an empty recipe reports no move");
}

// STEP20 ruling #4's fix, applied to decal `layerIndex`: dragging a layer row must renumber it.
void RunDecalLayerReorderRenumberChecks() {
    // Downward (below-target): layer 0 dragged onto layer 2 (of 3 total).
    std::vector<Params::DecalInstanceGroup> decals = MakeDecalsWithLayers();
    Check(RenumberDecalLayerIndicesForReorder(decals, 0, 2, 3),
          "a downward reorder (source below target) reports the move");
    Check(decals[0].transforms[0].layerIndex == 2 && decals[0].transforms[2].layerIndex == 2,
          "both transforms that named the dragged layer now name its new (target) slot");
    Check(decals[0].transforms[1].layerIndex == 0, "layer 1's transform shifts down into layer 0's old slot");
    Check(decals[1].transforms[0].layerIndex == 1, "layer 2's transform shifts down into layer 1's old slot");

    // Upward (above-target): layer 2 dragged onto layer 0.
    decals = MakeDecalsWithLayers();
    Check(RenumberDecalLayerIndicesForReorder(decals, 2, 0, 3),
          "an upward reorder (source above target) reports the move");
    Check(decals[1].transforms[0].layerIndex == 0, "the dragged layer's transform now names its new (target) slot");
    Check(decals[0].transforms[0].layerIndex == 1 && decals[0].transforms[2].layerIndex == 1,
          "layer 0's transforms shift up into layer 1's old slot");
    Check(decals[0].transforms[1].layerIndex == 2, "layer 1's transform shifts up into layer 2's old slot");

    // No-op: source == target, and an out-of-range source.
    decals = MakeDecalsWithLayers();
    Check(!RenumberDecalLayerIndicesForReorder(decals, 1, 1, 3), "dropping a row back on itself reports no move");
    Check(decals[0].transforms[1].layerIndex == 1, "and changes nothing");
    Check(!RenumberDecalLayerIndicesForReorder(decals, -1, 1, 3), "a signal about no layer at all changes nothing");
    Check(!RenumberDecalLayerIndicesForReorder(decals, 5, 1, 3),
          "an out-of-range source is rejected rather than trusted");
}

// `DecalGroups` exports as a plain array (STEP22 ruling #6), so this is cosmetic UX parity with
// Armies/Areas, not a data-loss fix — but two "Add Decal Layer" clicks must still produce distinct
// names per the acceptance test.
void RunDecalLayerNameUniquenessChecks() {
    std::vector<Params::DecalInstanceLayer> decalLayers;
    Params::DecalInstanceLayer firstLayer;
    firstLayer.name = NextDecalLayerName(static_cast<int>(decalLayers.size()));
    decalLayers.push_back(firstLayer);
    Params::DecalInstanceLayer secondLayer;
    secondLayer.name = NextDecalLayerName(static_cast<int>(decalLayers.size()));
    decalLayers.push_back(secondLayer);
    Check(decalLayers[0].name != decalLayers[1].name,
          "two 'Add Decal Layer' clicks in a row already produce distinct names");
    Check(!MakeNamesUnique(decalLayers), "and the shared repair confirms nothing needed fixing");

    decalLayers[1].name = decalLayers[0].name;
    Check(MakeNamesUnique(decalLayers), "a genuine collision reports the repair");
    Check(decalLayers[0].name != decalLayers[1].name, "and the later row is the one that gets suffixed");
}

// STEP56 (`ARCH_14_13_OpenItems.md` §14.13 item 3, Work-Order A): decal-typed mirror of
// `RunNextPropLayerIdChecks`.
void RunNextDecalLayerIdChecks() {
    std::vector<Params::DecalInstanceLayer> decalLayers;
    Check(NextDecalLayerId(decalLayers) == 0, "an empty decalLayers vector mints id 0");

    Params::DecalInstanceLayer firstLayer;
    firstLayer.layerId = 0;
    decalLayers.push_back(firstLayer);
    Params::DecalInstanceLayer secondLayer;
    secondLayer.layerId = 2;
    decalLayers.push_back(secondLayer);
    Check(NextDecalLayerId(decalLayers) == 3,
          "ids {0, 2} mint 3 - max-plus-one, not count-based");
}

// STEP108: empty vector and any index resolves to false; an out-of-range index (negative and
// >= size()) against a non-empty vector resolves to false too. Decal-typed mirror of
// RunIsPropInstanceLayerLockedChecks (PropsTab_UI_Test.cpp).
void RunIsDecalInstanceLayerLockedChecks() {
    std::vector<Params::DecalInstanceLayer> emptyLayers;
    Check(!IsDecalInstanceLayerLocked(emptyLayers, 0), "an empty vector resolves to false at any index");
    Check(!IsDecalInstanceLayerLocked(emptyLayers, -1), "and at a negative index too");

    std::vector<Params::DecalInstanceLayer> decalLayers(2);
    decalLayers[1].bLocked = true;
    Check(!IsDecalInstanceLayerLocked(decalLayers, 0), "index 0 (unlocked) resolves to false");
    Check(IsDecalInstanceLayerLocked(decalLayers, 1), "index 1 (locked) resolves to true");
    Check(!IsDecalInstanceLayerLocked(decalLayers, -1),
          "a negative index against a non-empty vector still resolves to false");
    Check(!IsDecalInstanceLayerLocked(decalLayers, 2),
          "an index at size() resolves to false, never trusted as 'locked'");
}

} // namespace

int main() {
    RunManualDecalLayerChecks();
    RunDecalLayerRemovalChecks();
    RunDecalLayerReorderRenumberChecks();
    RunDecalLayerNameUniquenessChecks();
    RunNextDecalLayerIdChecks();
    RunIsDecalInstanceLayerLockedChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
