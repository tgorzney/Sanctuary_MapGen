// MarkersTab_Links_UI_Test.cpp — STEP247 acceptance for the Links tier's own pure logic: "+Link"'s
// per-instance tagging (ARCH §19.33 correction — retires the old mint-and-move mechanism), its
// no-op guard, the color/override propagation resolvers (ARCH §19.31 Mechanism A), the rename
// cascade (Mechanism B), and Delete-Link's three clearing walks (instance-tier primary, Bundle/
// Layer-tier legacy fallback). Pure logic only — no imgui frame needed (mirrors
// MarkersTab_Bundles_UI_Test.cpp's own posture for the equivalent Bundle-tier Apply functions).
#include "MarkersTab_Links_UI.h"
#include "MarkersTab_ManualInstanceSelection_UI.h"
#include "MarkersTab_ManualLayerHelpers_UI.h"
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

void RunNextMarkerLinkIdChecks() {
    Check(NextMarkerLinkId({}) == 0, "an empty roster mints identifier 0");
    std::vector<Params::MarkerLink> links(2);
    links[0].identifier = 0; links[1].identifier = 3;
    Check(NextMarkerLinkId(links) == 4, "the next id is one past the current maximum, never reused");
}

// Verify: "+Link on a fresh, unlinked, cross-type selection mints exactly one Link, tags every
// selected instance's linkIdentifier directly, creates ZERO Bundle/Layer entries, and leaves every
// selected instance's layerIndex unchanged."
void RunApplyAddLinkActionTagsInPlaceChecks() {
    Params::MapRecipe recipe;
    recipe.markers.resize(2);
    recipe.markers[0].name = "Alloy";
    recipe.markers[0].transforms.resize(2);
    recipe.markers[0].transforms[0].instanceIdentifier = 1; recipe.markers[0].transforms[0].layerIndex = 5;
    recipe.markers[0].transforms[1].instanceIdentifier = 2; recipe.markers[0].transforms[1].layerIndex = 6;
    recipe.markers[1].name = "Plasma";
    recipe.markers[1].transforms.resize(1);
    recipe.markers[1].transforms[0].instanceIdentifier = 3; recipe.markers[1].transforms[0].layerIndex = 7;

    ApplyAddLinkAction(recipe, { 1, 2, 3 });

    Check(recipe.markerLinks.size() == 1, "exactly one Link is minted");
    const int linkIdentifier = recipe.markerLinks[0].identifier;
    Check(recipe.markerLayerBundles.empty(), "ZERO new MarkerLayerBundle entries are created");
    Check(recipe.markerLayers.empty(), "ZERO new MarkerInstanceLayer entries are created");

    Check(recipe.markers[0].transforms[0].linkIdentifier == linkIdentifier
       && recipe.markers[0].transforms[1].linkIdentifier == linkIdentifier
       && recipe.markers[1].transforms[0].linkIdentifier == linkIdentifier,
         "every selected instance is tagged directly with the new Link's identifier");

    Check(recipe.markers[0].transforms[0].layerIndex == 5
       && recipe.markers[0].transforms[1].layerIndex == 6
       && recipe.markers[1].transforms[0].layerIndex == 7,
         "every selected instance's layerIndex is UNCHANGED before and after -- existing "
         "layering/grouping stays completely untouched");
}

void RunApplyAddLinkActionEmptySelectionIsNoOpChecks() {
    Params::MapRecipe recipe;
    ApplyAddLinkAction(recipe, {});
    Check(recipe.markerLinks.empty(), "an empty selection mints nothing at all -- mirrors the "
         "button's own disabled-while-empty gate");
}

// Verify: "+Link on a selection where at least one instance already belongs to ANY existing Link
// does nothing at all -- no new Link, no tagging, including for the instances that WERE unlinked in
// that same selection."
void RunApplyAddLinkActionAlreadyLinkedGuardChecks() {
    Params::MapRecipe recipe;
    recipe.markers.resize(1);
    recipe.markers[0].name = "Alloy";
    recipe.markers[0].transforms.resize(3);
    recipe.markers[0].transforms[0].instanceIdentifier = 1; recipe.markers[0].transforms[0].linkIdentifier = -1;
    recipe.markers[0].transforms[1].instanceIdentifier = 2; recipe.markers[0].transforms[1].linkIdentifier = 9;   // already linked
    recipe.markers[0].transforms[2].instanceIdentifier = 3; recipe.markers[0].transforms[2].linkIdentifier = -1;
    Params::MarkerLink existingLink;
    existingLink.identifier = 9;
    recipe.markerLinks.push_back(existingLink);

    ApplyAddLinkAction(recipe, { 1, 2, 3 });

    Check(recipe.markerLinks.size() == 1, "no new Link is minted -- the roster is unchanged in size");
    Check(recipe.markers[0].transforms[0].linkIdentifier == -1,
         "an instance that WAS unlinked in the same blocked selection stays unlinked");
    Check(recipe.markers[0].transforms[1].linkIdentifier == 9,
         "the already-linked instance's membership is untouched");
    Check(recipe.markers[0].transforms[2].linkIdentifier == -1,
         "the other previously-unlinked instance in the blocked selection also stays untagged");
}

// Verify: "+Link on a selection entirely already in the SAME existing Link is also a no-op."
void RunApplyAddLinkActionEntirelySameLinkGuardChecks() {
    Params::MapRecipe recipe;
    recipe.markers.resize(1);
    recipe.markers[0].name = "Alloy";
    recipe.markers[0].transforms.resize(2);
    recipe.markers[0].transforms[0].instanceIdentifier = 1; recipe.markers[0].transforms[0].linkIdentifier = 4;
    recipe.markers[0].transforms[1].instanceIdentifier = 2; recipe.markers[0].transforms[1].linkIdentifier = 4;
    Params::MarkerLink existingLink;
    existingLink.identifier = 4;
    recipe.markerLinks.push_back(existingLink);

    ApplyAddLinkAction(recipe, { 1, 2 });

    Check(recipe.markerLinks.size() == 1, "no new Link is minted");
    Check(recipe.markers[0].transforms[0].linkIdentifier == 4 && recipe.markers[0].transforms[1].linkIdentifier == 4,
         "both instances' existing linkIdentifier is left exactly as it was");
}

// Verify: "Propagation resolvers: a Link-bound Layer's color/hidden read from the Link, not its own
// field, while linkIdentifier >= 0; reverts to its own field once un-linked." (Layer-tier mechanism,
// ARCH §19.31, unchanged by this ticket -- still exercised here as a regression guard.)
void RunEffectiveColorResolverChecks() {
    std::vector<Params::MarkerLink> links(1);
    links[0].identifier = 7;
    links[0].bColorOverrideEnabled = true;
    links[0].color[0] = 0.25f; links[0].color[1] = 0.5f; links[0].color[2] = 0.75f; links[0].color[3] = 1.0f;

    Params::MarkerInstanceLayer layer;
    layer.bColorOverrideEnabled = false;
    layer.color[0] = 0.1f; layer.color[1] = 0.1f; layer.color[2] = 0.1f; layer.color[3] = 1.0f;
    layer.linkIdentifier = 7;

    Check(EffectiveManualMarkerLayerColorOverrideEnabled(layer, links) == true,
         "a Link-bound Layer's effective override-enabled resolves from the LINK, not its own (false) field");
    const float* resolvedColor = EffectiveManualMarkerLayerColor(layer, links);
    Check(resolvedColor[0] == 0.25f && resolvedColor[1] == 0.5f && resolvedColor[2] == 0.75f,
         "a Link-bound Layer's effective color resolves from the LINK, not its own field");
    Check(layer.bColorOverrideEnabled == false && layer.color[0] == 0.1f,
         "the Layer's OWN fields are never written back to by the read-and-resolve resolvers");

    layer.linkIdentifier = -1;   // un-linked
    Check(EffectiveManualMarkerLayerColorOverrideEnabled(layer, links) == false,
         "once un-linked, the resolver reverts to the Layer's own (unchanged) field");
    const float* revertedColor = EffectiveManualMarkerLayerColor(layer, links);
    Check(revertedColor[0] == 0.1f, "and the color reverts to the Layer's own (unchanged) field too");

    layer.linkIdentifier = 999;   // dangling — Constitution §6 soft-degrade, never a structural error
    Check(EffectiveManualMarkerLayerColorOverrideEnabled(layer, links) == false,
         "a dangling linkIdentifier (no matching Link) soft-degrades to the Layer's own field");
}

// Verify (STEP241/ARCH §19.31 correction — RETRACTS STEP239's cascade-write): a Link rename commits
// ONLY the Link's own name; a bound Bundle's own `name` field is left completely untouched — it now
// resolves the Link's name live via EffectiveMarkerLayerBundleName instead (a separate resolver test,
// MarkersTab_MarkerLinkResolvers_UI_Test.cpp), never a copy.
void RunCommitMarkerLinkRenameChecks() {
    Params::MarkerLink link;
    link.identifier = 5; link.name = "Old";
    std::vector<Params::MarkerLayerBundle> bundles(1);
    bundles[0].linkIdentifier = 5; bundles[0].name = "Old";

    CommitMarkerLinkRename(link, "New Name");

    Check(link.name == "New Name", "the Link's own name commits");
    Check(bundles[0].name == "Old",
         "a bound Bundle's own name field is left COMPLETELY untouched — STEP241 retracts the cascade-write");
}

// Verify: "Delete-Link on a Link minted by this ticket's own ApplyAddLinkAction: every tagged
// instance's linkIdentifier resets to -1; no Bundle/Layer is touched (none exist to touch)."
void RunDeleteMarkerLinkInstanceTierChecks() {
    std::vector<Params::MarkerLink> links(2);
    links[0].identifier = 1; links[1].identifier = 2;
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].name = "Alloy";
    markers[0].transforms.resize(3);
    markers[0].transforms[0].instanceIdentifier = 1; markers[0].transforms[0].linkIdentifier = 1;
    markers[0].transforms[1].instanceIdentifier = 2; markers[0].transforms[1].linkIdentifier = 1;
    markers[0].transforms[2].instanceIdentifier = 3; markers[0].transforms[2].linkIdentifier = 2;   // different Link
    std::vector<Params::MarkerLayerBundle> bundles;   // none exist -- nothing to touch
    std::vector<Params::MarkerInstanceLayer> markerLayers;   // none exist -- nothing to touch

    DeleteMarkerLink(1, links, markers, bundles, markerLayers);

    Check(links.size() == 1 && links[0].identifier == 2, "only the deleted Link's own entry is erased");
    Check(markers[0].transforms[0].linkIdentifier == -1 && markers[0].transforms[1].linkIdentifier == -1,
         "every instance tagged with the deleted Link resets to -1");
    Check(markers[0].transforms[2].linkIdentifier == 2,
         "an instance tagged with a DIFFERENT Link is untouched");
    Check(bundles.empty() && markerLayers.empty(), "no Bundle/Layer is touched -- none exist to touch");
}

// Verify: "Delete-Link on a Link with pre-existing (legacy) Bundle/Layer-tier tags and no
// transform-tier tags: the existing two walks still clear those" -- a regression test for the
// "kept, not removed" backward-compat requirement.
void RunDeleteMarkerLinkLegacyBundleLayerTierChecks() {
    std::vector<Params::MarkerLink> links(2);
    links[0].identifier = 1; links[1].identifier = 2;
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].name = "Alloy";
    markers[0].transforms.resize(1);
    markers[0].transforms[0].instanceIdentifier = 1; markers[0].transforms[0].linkIdentifier = -1;   // no transform-tier tag
    std::vector<Params::MarkerLayerBundle> bundles(2);
    bundles[0].linkIdentifier = 1; bundles[0].name = "Alloy Group";
    bundles[1].linkIdentifier = 2; bundles[1].name = "Other Link's Group";
    std::vector<Params::MarkerInstanceLayer> markerLayers(2);
    markerLayers[0].linkIdentifier = 1; markerLayers[0].name = "Alloy Layer";
    markerLayers[1].linkIdentifier = 2; markerLayers[1].name = "Other Link's Layer";

    DeleteMarkerLink(1, links, markers, bundles, markerLayers);

    Check(links.size() == 1 && links[0].identifier == 2, "only the deleted Link's own entry is erased");
    Check(bundles.size() == 2, "the Group itself is NEVER erased by Delete-Link");
    Check(bundles[0].linkIdentifier == -1, "the deleted Link's own legacy Bundle-tier back-reference clears to -1");
    Check(bundles[1].linkIdentifier == 2, "an unrelated Group's own linkIdentifier is untouched");
    Check(markerLayers.size() == 2, "the Layer itself is NEVER erased by Delete-Link either");
    Check(markerLayers[0].linkIdentifier == -1, "the deleted Link's own legacy Layer-tier back-reference clears too");
    Check(markerLayers[1].linkIdentifier == 2, "an unrelated Layer's own linkIdentifier is untouched");
    Check(bundles[0].name == "Alloy Group" && markerLayers[0].name == "Alloy Layer",
         "every other field on the ungrouped Group/Layer is left exactly as it was");
    Check(markers[0].transforms[0].linkIdentifier == -1,
         "the instance-tier walk is a harmless no-op for a transform that was never tagged");
}

// STEP248 — the seam DrawMarkerLinkBody's hierarchical body actually walks: "+Link" tags a
// cross-type selection directly (STEP247's ApplyAddLinkAction), then PartitionLinkedManualInstancesByType
// (MarkersTab_ManualInstanceSelection_UI.h) must recover exactly those same instances, grouped by
// canonical type, as (groupIndex, transformIndex) pairs -- the item shape DrawSymmetryClusterInstanceList/
// DrawManualInstanceRow consume. Proves the two STEP247/STEP248 additions compose correctly end to end
// without needing an imgui frame.
void RunApplyAddLinkActionThenPartitionByTypeChecks() {
    Params::MapRecipe recipe;
    recipe.markers.resize(2);
    recipe.markers[0].name = "Alloy";
    recipe.markers[0].transforms.resize(2);
    recipe.markers[0].transforms[0].instanceIdentifier = 1;
    recipe.markers[0].transforms[1].instanceIdentifier = 2;
    recipe.markers[1].name = "Plasma";
    recipe.markers[1].transforms.resize(1);
    recipe.markers[1].transforms[0].instanceIdentifier = 3;

    ApplyAddLinkAction(recipe, { 1, 2, 3 });
    Check(recipe.markerLinks.size() == 1, "exactly one Link is minted");
    const int linkIdentifier = recipe.markerLinks[0].identifier;

    const auto byType = PartitionLinkedManualInstancesByType(recipe.markers, linkIdentifier);
    Check(byType.size() == 2, "one bucket per represented type the Link's own instances span");
    Check(byType.count("Alloy") == 1 && byType.at("Alloy").size() == 2,
         "both Alloy instances the Link tagged are recovered");
    Check(byType.count("Plasma") == 1 && byType.at("Plasma").size() == 1,
         "the single Plasma instance the Link tagged is recovered too");
    for (const std::pair<int, int>& groupTransformIndex : byType.at("Alloy"))
        Check(recipe.markers[static_cast<std::size_t>(groupTransformIndex.first)]
                .transforms[static_cast<std::size_t>(groupTransformIndex.second)].linkIdentifier
              == linkIdentifier,
             "every recovered pair really does resolve back to a transform tagged with this Link");

    // Deleting the Link (STEP247's own instance-tier walk) must leave nothing left to partition.
    DeleteMarkerLink(linkIdentifier, recipe.markerLinks, recipe.markers, recipe.markerLayerBundles,
                     recipe.markerLayers);
    Check(PartitionLinkedManualInstancesByType(recipe.markers, linkIdentifier).empty(),
         "after Delete-Link, the partition finds nothing left tagged with the deleted identifier");
}

} // namespace

int main() {
    RunNextMarkerLinkIdChecks();
    RunApplyAddLinkActionTagsInPlaceChecks();
    RunApplyAddLinkActionEmptySelectionIsNoOpChecks();
    RunApplyAddLinkActionAlreadyLinkedGuardChecks();
    RunApplyAddLinkActionEntirelySameLinkGuardChecks();
    RunEffectiveColorResolverChecks();
    RunCommitMarkerLinkRenameChecks();
    RunDeleteMarkerLinkInstanceTierChecks();
    RunDeleteMarkerLinkLegacyBundleLayerTierChecks();
    RunApplyAddLinkActionThenPartitionByTypeChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
