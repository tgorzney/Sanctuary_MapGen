// PropsTab_UI_Test.cpp — tab-rebuild WO C4 acceptance, part 4: the Props tab and the manual prop
// layers block it hosts. Pure logic only — the rule<->widget mirrors, the layer-color override, the
// label fallbacks and the tpId probe — so the binary needs no imgui frame, no window and no GL
// context. Decals had their own mirror checks here until ARCH §20 split them onto their own tab —
// see DecalsTab_UI_Test.cpp.
// STEP22: the manual prop layers retype onto the real `Params::PropInstanceLayer`, and this file
// gains the clamp/renumber repair checks for `recipe.props[].transforms[].layerIndex` — the checks
// that matter: deleting a layer must never drop a prop instance (CLAMPS to layer 0, the DELIBERATE
// divergence from Step 20's army-removal DROP behavior), and reordering a layer must renumber every
// referencing `layerIndex` exactly the way Step 20 fixed for `armyIndex`.
#include "PropsTab_UI.h"
#include <cstdio>
#include <cstring>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

void RunPropRuleMirrorChecks() {
    Params::PropRule rule;
    rule.minSlope = 3.0f; rule.maxSlope = 25.0f; rule.minHeight = 0.2f; rule.maxHeight = 0.8f;
    PropsTabState state;
    LoadPropRuleValues(rule, state);
    Check(state.slopeValues.minimumValue == 3.0f && state.slopeValues.maximumValue == 25.0f
          && state.heightValues.minimumValue == 0.2f && state.heightValues.maximumValue == 0.8f,
          "both gate bands reach their widget mirrors");
    Check(!StorePropRuleValues(state, rule), "storing back what was loaded reports no move");
    state.slopeValues.minimumValue = 4.0f;
    Check(StorePropRuleValues(state, rule) && rule.minSlope == 4.0f,
          "and a real edit reports the move and lands on the rule");
    Check(state.densityRange.minimumValue == 0.0f && state.densityRange.maximumValue == 1.0f,
          "density is a 0-1 proportion");
}

// The tpId is a fixed 8-byte field whose last byte need not be a terminator, so "has a template"
// is a probe on the FIRST byte, never a strlen.
void RunTemplateIdentifierChecks() {
    Params::PropRule rule;
    Check(!PropRuleHasTemplateIdentifier(rule), "a fresh rule has no template typed yet");
    std::memcpy(rule.transform.templateIdentifier, "ual0001", 7u);
    Check(PropRuleHasTemplateIdentifier(rule), "and a typed tpId is seen");
    Check(rule.transform.templateIdentifier[7] == '\0',
          "the eighth byte stays clear, so the %.7s row label cannot run off the field");
}

// STEP22: manual prop layers are real recipe content now (`Params::PropInstanceLayer`), so the
// invariants are the same selection/override/label fences the retired `ManualPropGroup` checks
// exercised, retargeted onto the retyped vector `DrawManualPropLayers` is handed directly (it is no
// longer state-owned).
void RunManualPropLayerChecks() {
    ManualPropLayersState state;
    std::vector<Params::PropInstanceLayer> propLayers;
    Check(SelectedManualPropLayer(propLayers, state.selectedLayerIndex) == nullptr,
          "an empty block selects no layer");
    propLayers.push_back(Params::PropInstanceLayer());
    state.selectedLayerIndex = 0;
    Check(SelectedManualPropLayer(propLayers, state.selectedLayerIndex) == &propLayers[0],
          "the selected layer is reachable");
    state.selectedLayerIndex = 2;
    Check(SelectedManualPropLayer(propLayers, state.selectedLayerIndex) == nullptr,
          "an index past the last layer selects nothing");

    Params::PropInstanceLayer& layer = propLayers[0];
    layer.color[0]       = 0.25f;
    state.groupColor[0]  = 0.75f;
    Check(EffectiveManualPropLayerColor(state, layer) == layer.color,
          "with the shared tint off a layer draws its OWN color");
    state.bUseGroupColor = true;
    Check(EffectiveManualPropLayerColor(state, layer) == state.groupColor,
          "and with it on every layer draws the one shared color");

    Check(ManualPropLayerRowLabel(layer) != nullptr && ManualPropLayerRowLabel(layer)[0] != '\0',
          "an unnamed layer still draws a label");
    Check(state.iconScaleRange.minimumValue == 0.1f && state.iconScaleRange.maximumValue == 10.0f,
          "the icon scale sliders carry the plan's 0.1-10");
}

// Two prop groups, four transforms across three layers, in recipe order: 0, 1, 0, 2.
std::vector<Params::PropInstanceGroup> MakePropsWithLayers() {
    std::vector<Params::PropInstanceGroup> props(2);
    props[0].transforms.resize(3);
    props[0].transforms[0].layerIndex = 0;
    props[0].transforms[1].layerIndex = 1;
    props[0].transforms[2].layerIndex = 0;
    props[1].transforms.resize(1);
    props[1].transforms[0].layerIndex = 2;
    return props;
}

int PropCountAcrossGroups(const std::vector<Params::PropInstanceGroup>& props) {
    int count = 0;
    for (const auto& group : props) count += static_cast<int>(group.transforms.size());
    return count;
}

// STEP22 ruling #5 — the DELIBERATE divergence from Step 20's `DropUnitRulesForRemovedArmy`: an
// orphaned prop transform CLAMPS to layer 0, it is never dropped.
void RunPropLayerRemovalChecks() {
    std::vector<Params::PropInstanceGroup> props = MakePropsWithLayers();
    const int countBeforeRemoval = PropCountAcrossGroups(props);
    Check(ClampPropLayerIndicesForRemovedLayer(props, 1), "removing a layer with transforms reports the move");
    Check(PropCountAcrossGroups(props) == countBeforeRemoval,
          "not one prop transform is dropped - a prop losing its layer tag is still a real prop");
    Check(props[0].transforms[0].layerIndex == 0 && props[0].transforms[2].layerIndex == 0,
          "layers BELOW the removed one keep their index");
    Check(props[1].transforms[0].layerIndex == 1,
          "and every layer above it shifts down one");

    props = MakePropsWithLayers();
    Check(ClampPropLayerIndicesForRemovedLayer(props, 0),
          "removing layer 0 clamps every transform that named it");
    Check(props[0].transforms[0].layerIndex == 0 && props[0].transforms[2].layerIndex == 0,
          "the orphaned transforms clamp to layer 0, not -1 or any sentinel");
    Check(props[0].transforms[1].layerIndex == 0 && props[1].transforms[0].layerIndex == 1,
          "every surviving layer above the removed one shifts down one");

    props = MakePropsWithLayers();
    Check(!ClampPropLayerIndicesForRemovedLayer(props, -1), "a signal about no layer at all changes nothing");
    std::vector<Params::PropInstanceGroup> emptyProps;
    Check(!ClampPropLayerIndicesForRemovedLayer(emptyProps, 0), "and an empty recipe reports no move");
}

// STEP20 ruling #4's fix, applied to `layerIndex`: dragging a layer row must renumber it.
void RunPropLayerReorderRenumberChecks() {
    // Downward (below-target): layer 0 dragged onto layer 2 (of 3 total).
    std::vector<Params::PropInstanceGroup> props = MakePropsWithLayers();
    Check(RenumberPropLayerIndicesForReorder(props, 0, 2, 3),
          "a downward reorder (source below target) reports the move");
    Check(props[0].transforms[0].layerIndex == 2 && props[0].transforms[2].layerIndex == 2,
          "both transforms that named the dragged layer now name its new (target) slot");
    Check(props[0].transforms[1].layerIndex == 0, "layer 1's transform shifts down into layer 0's old slot");
    Check(props[1].transforms[0].layerIndex == 1, "layer 2's transform shifts down into layer 1's old slot");

    // Upward (above-target): layer 2 dragged onto layer 0.
    props = MakePropsWithLayers();
    Check(RenumberPropLayerIndicesForReorder(props, 2, 0, 3),
          "an upward reorder (source above target) reports the move");
    Check(props[1].transforms[0].layerIndex == 0, "the dragged layer's transform now names its new (target) slot");
    Check(props[0].transforms[0].layerIndex == 1 && props[0].transforms[2].layerIndex == 1,
          "layer 0's transforms shift up into layer 1's old slot");
    Check(props[0].transforms[1].layerIndex == 2, "layer 1's transform shifts up into layer 2's old slot");

    // No-op: source == target, and an out-of-range source.
    props = MakePropsWithLayers();
    Check(!RenumberPropLayerIndicesForReorder(props, 1, 1, 3), "dropping a row back on itself reports no move");
    Check(props[0].transforms[1].layerIndex == 1, "and changes nothing");
    Check(!RenumberPropLayerIndicesForReorder(props, -1, 1, 3), "a signal about no layer at all changes nothing");
    Check(!RenumberPropLayerIndicesForReorder(props, 5, 1, 3),
          "an out-of-range source is rejected rather than trusted");
}

// `PropGroups` exports as a plain array (STEP22 ruling #6), so this is cosmetic UX parity with
// Armies/Areas, not a data-loss fix — but two "Add Prop Layer" clicks must still produce distinct
// names per the acceptance test.
void RunPropLayerNameUniquenessChecks() {
    std::vector<Params::PropInstanceLayer> propLayers;
    Params::PropInstanceLayer firstLayer;
    firstLayer.name = NextPropLayerName(static_cast<int>(propLayers.size()));
    propLayers.push_back(firstLayer);
    Params::PropInstanceLayer secondLayer;
    secondLayer.name = NextPropLayerName(static_cast<int>(propLayers.size()));
    propLayers.push_back(secondLayer);
    Check(propLayers[0].name != propLayers[1].name,
          "two 'Add Prop Layer' clicks in a row already produce distinct names");
    Check(!MakeNamesUnique(propLayers), "and the shared repair confirms nothing needed fixing");

    propLayers[1].name = propLayers[0].name;
    Check(MakeNamesUnique(propLayers), "a genuine collision reports the repair");
    Check(propLayers[0].name != propLayers[1].name, "and the later row is the one that gets suffixed");
}

// STEP96_FootprintBakeAndStalenessCheck_IO.md acceptance tests 1/2: ApplyResolvedFootprintBake is
// the bake button's pure core (imgui-free) — a found record overwrites baseFootprintWidth/Depth/
// footprintBakeFingerprint ONLY, a miss (nullptr) leaves the rule completely untouched.
void RunResolveFootprintBakeChecks() {
    Params::PropRule rule;
    rule.spacingMinimum = 3.5f;               // must survive a bake untouched
    rule.obstacleDistanceMinimum = 2.5f;      // must survive a bake untouched
    Check(!ApplyResolvedFootprintBake(rule, nullptr),
          "a miss (nullptr record) reports no bake and changes nothing");
    Check(!rule.footprintBakeFingerprint.IsValid() && rule.baseFootprintWidth == 4.0f,
          "the rule keeps its never-baked default after a miss");

    Io::TemplateFootprintRecord record;
    record.baseFootprintWidth = 5.5f;
    record.baseFootprintDepth = 6.5f;
    record.sourceFingerprint.sourcePath   = "Templates/Props/rock_01.santp";
    record.sourceFingerprint.byteSize     = 4096ull;
    record.sourceFingerprint.modifiedTime = 1700000000ull;
    record.sourceFingerprint.contentHash  = 123456789ull;
    Check(ApplyResolvedFootprintBake(rule, &record), "a found record reports a real bake");
    Check(rule.baseFootprintWidth == 5.5f && rule.baseFootprintDepth == 6.5f
          && rule.footprintBakeFingerprint.IsValid()
          && rule.footprintBakeFingerprint.sourcePath == "Templates/Props/rock_01.santp"
          && rule.footprintBakeFingerprint.byteSize == 4096ull
          && rule.footprintBakeFingerprint.modifiedTime == 1700000000ull
          && rule.footprintBakeFingerprint.contentHash == 123456789ull,
          "baseFootprintWidth/Depth and every fingerprint field land on the rule");
    Check(rule.spacingMinimum == 3.5f && rule.obstacleDistanceMinimum == 2.5f,
          "spacingMinimum/obstacleDistanceMinimum are never touched by a bake");
}

// STEP56 (`ARCH_14_13_OpenItems.md` §14.13 item 3, Work-Order A): a newly created layer's `layerId`
// derives as max-plus-one across the current in-memory `propLayers`, never a stored counter.
void RunNextPropLayerIdChecks() {
    std::vector<Params::PropInstanceLayer> propLayers;
    Check(NextPropLayerId(propLayers) == 0, "an empty propLayers vector mints id 0");

    Params::PropInstanceLayer firstLayer;
    firstLayer.layerId = 0;
    propLayers.push_back(firstLayer);
    Params::PropInstanceLayer secondLayer;
    secondLayer.layerId = 2;
    propLayers.push_back(secondLayer);
    Check(NextPropLayerId(propLayers) == 3,
          "ids {0, 2} mint 3 - max-plus-one, not count-based");
}

// STEP108: empty vector and any index resolves to false; an out-of-range index (negative and
// >= size()) against a non-empty vector resolves to false too. Mirrors STEP106's
// IsMarkerInstanceLayerLocked test shape.
void RunIsPropInstanceLayerLockedChecks() {
    std::vector<Params::PropInstanceLayer> emptyLayers;
    Check(!IsPropInstanceLayerLocked(emptyLayers, 0), "an empty vector resolves to false at any index");
    Check(!IsPropInstanceLayerLocked(emptyLayers, -1), "and at a negative index too");

    std::vector<Params::PropInstanceLayer> propLayers(2);
    propLayers[1].bLocked = true;
    Check(!IsPropInstanceLayerLocked(propLayers, 0), "index 0 (unlocked) resolves to false");
    Check(IsPropInstanceLayerLocked(propLayers, 1), "index 1 (locked) resolves to true");
    Check(!IsPropInstanceLayerLocked(propLayers, -1),
          "a negative index against a non-empty vector still resolves to false");
    Check(!IsPropInstanceLayerLocked(propLayers, 2),
          "an index at size() resolves to false, never trusted as 'locked'");
}

// ARCH §20: the `||`-composed, not-XOR shape — type-mismatch-only, bundle-membership-only, both,
// and neither — mirrors IsMarkerInstanceLayerRowSuppressed's own four-case coverage.
void RunIsPropInstanceLayerRowSuppressedChecks() {
    Params::PropInstanceLayer typeMismatchOnly;
    typeMismatchOnly.parentBundleIdentifier = -1;
    typeMismatchOnly.propTypeName           = "Prop";
    Check(!IsPropInstanceLayerRowSuppressed(typeMismatchOnly, "Prop"),
          "an ungrouped layer whose own type matches the filter is NOT suppressed");
    Check(IsPropInstanceLayerRowSuppressed(typeMismatchOnly, "Reclaim"),
          "an ungrouped layer whose own type does NOT match the filter IS suppressed (type-mismatch only)");

    Params::PropInstanceLayer bundleMembershipOnly;
    bundleMembershipOnly.parentBundleIdentifier = 5;
    bundleMembershipOnly.propTypeName           = "Prop";
    Check(IsPropInstanceLayerRowSuppressed(bundleMembershipOnly, "Prop"),
          "a bundled layer IS suppressed even when its own type matches the filter (bundle-membership only)");

    Params::PropInstanceLayer both;
    both.parentBundleIdentifier = 5;
    both.propTypeName           = "Prop";
    Check(IsPropInstanceLayerRowSuppressed(both, "Reclaim"),
          "a bundled layer with a mismatched type IS suppressed (the compound case, not an XOR)");
}

} // namespace

int main() {
    RunPropRuleMirrorChecks();
    RunIsPropInstanceLayerRowSuppressedChecks();
    RunTemplateIdentifierChecks();
    RunResolveFootprintBakeChecks();
    RunManualPropLayerChecks();
    RunPropLayerRemovalChecks();
    RunPropLayerReorderRenumberChecks();
    RunPropLayerNameUniquenessChecks();
    RunNextPropLayerIdChecks();
    RunIsPropInstanceLayerLockedChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
