// MarkersTab_ManualInstanceSelection_UI_Test.cpp — STEP141 acceptance for
// MarkersTab_ManualInstanceSelection_UI.h's pure logic: Ctrl/Shift multi-select and
// ReassignManualInstanceLayers. Pure logic only — no imgui frame needed.
#include "MarkersTab_ManualInstanceSelection_UI.h"
#include "MarkersTab_Bundles_UI.h"
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

bool Equal(const std::vector<int>& actual, const std::vector<int>& expected) { return actual == expected; }

void RunPlainClickChecks() {
    const std::vector<int> rowOrder{ 10, 20, 30 };
    std::vector<int> selected{ 10, 20 };   // some prior selection
    int anchor = 10;

    ApplyManualInstanceSelectionClick(rowOrder, 30, /*ctrl=*/false, /*shift=*/false, selected, anchor);
    Check(Equal(selected, { 30 }), "a plain click REPLACES the set with just the clicked identifier");
    Check(anchor == 30, "and the anchor becomes the clicked identifier");
}

void RunCtrlClickTogglesChecks() {
    const std::vector<int> rowOrder{ 10, 20, 30 };
    std::vector<int> selected{ 10 };
    int anchor = 10;

    ApplyManualInstanceSelectionClick(rowOrder, 20, /*ctrl=*/true, /*shift=*/false, selected, anchor);
    Check(Equal(selected, { 10, 20 }), "Ctrl-click on an UNselected row ADDS it, keeping the rest");
    Check(anchor == 20, "and moves the anchor to it (so a following Shift-click ranges from here)");

    ApplyManualInstanceSelectionClick(rowOrder, 10, /*ctrl=*/true, /*shift=*/false, selected, anchor);
    Check(Equal(selected, { 20 }), "Ctrl-click on an ALREADY-selected row REMOVES just that one");
    Check(anchor == 10, "the anchor still moves to whatever was clicked, even when removing");
}

void RunShiftRangeChecks() {
    const std::vector<int> rowOrder{ 10, 20, 30, 40, 50 };
    std::vector<int> selected;
    int anchor = 20;   // row index 1

    ApplyManualInstanceSelectionClick(rowOrder, 40, /*ctrl=*/false, /*shift=*/true, selected, anchor);
    Check(Equal(selected, { 20, 30, 40 }), "Shift-click selects the CONTIGUOUS range [anchor..clicked] inclusive");
    Check(anchor == 20, "the anchor itself does NOT move on a Shift-click");

    // A second Shift-click from the SAME anchor, now going the other direction.
    ApplyManualInstanceSelectionClick(rowOrder, 10, /*ctrl=*/false, /*shift=*/true, selected, anchor);
    Check(Equal(selected, { 10, 20 }), "a later Shift-click still ranges from the SAME anchor, either direction");
    Check(anchor == 20, "and still leaves the anchor unmoved");
}

void RunShiftWithNoAnchorFallsBackToPlainChecks() {
    const std::vector<int> rowOrder{ 10, 20, 30 };
    std::vector<int> selected{ 10, 20 };
    int anchor = -1;   // nothing clicked yet this session

    ApplyManualInstanceSelectionClick(rowOrder, 30, /*ctrl=*/false, /*shift=*/true, selected, anchor);
    Check(Equal(selected, { 30 }), "Shift with no anchor yet has nothing to range from -- falls back to a plain click");
    Check(anchor == 30, "and still sets the anchor for a FUTURE Shift-click to use");
}

void RunShiftWithIdentifierNotInRowOrderFallsBackChecks() {
    const std::vector<int> rowOrder{ 10, 20, 30 };
    std::vector<int> selected{ 10 };
    int anchor = 999;   // stale -- not in THIS list (e.g. anchor was set in a different Layer's list)

    ApplyManualInstanceSelectionClick(rowOrder, 20, /*ctrl=*/false, /*shift=*/true, selected, anchor);
    Check(Equal(selected, { 20 }),
         "a Shift-click whose anchor is not present in THIS list's own rowOrder falls back to a plain click "
         "(range selection never spans separate lists)");
    Check(anchor == 20, "and the anchor resets to this list's own click");
}

void RunIsManualInstanceSelectedChecks() {
    const std::vector<int> selected{ 5, 10, 15 };
    Check(IsManualInstanceSelected(selected, 10), "a present identifier reads as selected");
    Check(!IsManualInstanceSelected(selected, 99), "an absent identifier does not");
}

void RunReassignManualInstanceLayersChecks() {
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].transforms.resize(3);
    markers[0].transforms[0].instanceIdentifier = 1; markers[0].transforms[0].layerIndex = 0;
    markers[0].transforms[1].instanceIdentifier = 2; markers[0].transforms[1].layerIndex = 0;
    markers[0].transforms[2].instanceIdentifier = 3; markers[0].transforms[2].layerIndex = 0;

    ReassignManualInstanceLayers(markers, { 1, 3 }, 2);

    Check(markers[0].transforms[0].layerIndex == 2, "a moved identifier (1) lands on the new layerIndex");
    Check(markers[0].transforms[1].layerIndex == 0, "an identifier NOT in the moved set (2) is untouched");
    Check(markers[0].transforms[2].layerIndex == 2, "the other moved identifier (3) lands on the new layerIndex too");
}

// STEP146 (human's own bug report — dragging an instance out of a Layer onto the Type-section's own
// base "Instances" list did nothing) — DrawBaseSectionManualInstanceList (MarkersTab_UI.cpp) now
// calls DrawManualLayerInstanceDropTarget(-1, ...) on that list; the reassignment underneath it is
// this same generic function, which never bounds-checked `newLayerIndex` — -1 flows through exactly
// like any other value, no new logic needed. This proves the -1 case explicitly.
void RunReassignManualInstanceLayersToUnassignedChecks() {
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].transforms.resize(2);
    markers[0].transforms[0].instanceIdentifier = 1; markers[0].transforms[0].layerIndex = 0;
    markers[0].transforms[1].instanceIdentifier = 2; markers[0].transforms[1].layerIndex = 0;

    ReassignManualInstanceLayers(markers, { 1 }, -1);

    Check(markers[0].transforms[0].layerIndex == -1,
         "dropping onto the base Instances list reassigns to layerIndex -1 (\"no layer\")");
    Check(markers[0].transforms[1].layerIndex == 0, "an identifier NOT in the moved set is untouched");
}

// STEP247 — "+Link"'s own per-instance tagging step, mirrors ReassignManualInstanceLayers's exact
// walk, one field over.
void RunTagManualInstancesWithLinkChecks() {
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].transforms.resize(3);
    markers[0].transforms[0].instanceIdentifier = 1; markers[0].transforms[0].linkIdentifier = -1;
    markers[0].transforms[1].instanceIdentifier = 2; markers[0].transforms[1].linkIdentifier = -1;
    markers[0].transforms[2].instanceIdentifier = 3; markers[0].transforms[2].linkIdentifier = -1;

    TagManualInstancesWithLink(markers, { 1, 3 }, 7);

    Check(markers[0].transforms[0].linkIdentifier == 7, "a tagged identifier (1) is set to the new linkIdentifier");
    Check(markers[0].transforms[1].linkIdentifier == -1, "an identifier NOT in the tagged set (2) is untouched");
    Check(markers[0].transforms[2].linkIdentifier == 7, "the other tagged identifier (3) is set too");
}

// STEP247 — "+Link"'s own no-op guard predicate.
void RunIsAnyManualInstanceSelectionAlreadyLinkedChecks() {
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].transforms.resize(3);
    markers[0].transforms[0].instanceIdentifier = 1; markers[0].transforms[0].linkIdentifier = -1;
    markers[0].transforms[1].instanceIdentifier = 2; markers[0].transforms[1].linkIdentifier = 5;
    markers[0].transforms[2].instanceIdentifier = 3; markers[0].transforms[2].linkIdentifier = -1;

    Check(!IsAnyManualInstanceSelectionAlreadyLinked(markers, { 1, 3 }),
         "a selection where every resolved identifier is unlinked is false");
    Check(IsAnyManualInstanceSelectionAlreadyLinked(markers, { 1, 2, 3 }),
         "true the moment ANY resolved identifier already carries linkIdentifier >= 0");
    Check(!IsAnyManualInstanceSelectionAlreadyLinked(markers, { 99 }),
         "an unresolved/stale identifier is skipped, never itself a reason to block");
    Check(!IsAnyManualInstanceSelectionAlreadyLinked(markers, {}),
         "an empty selection has nothing to check");
}

// STEP235 — the "entirely one type" predicate "+ Group"/"+ Layer" gate their same-type move on.
void RunIsManualInstanceSelectionEntirelyTypeChecks() {
    std::vector<Params::MarkerInstanceGroup> markers(2);
    markers[0].name = "Alloy";
    markers[0].transforms.resize(2);
    markers[0].transforms[0].instanceIdentifier = 1;
    markers[0].transforms[1].instanceIdentifier = 2;
    markers[1].name = "Plasma";
    markers[1].transforms.resize(1);
    markers[1].transforms[0].instanceIdentifier = 3;

    Check(IsManualInstanceSelectionEntirelyType(markers, { 1, 2 }, "Alloy"),
         "a selection entirely resolving to the same type is true");
    Check(!IsManualInstanceSelectionEntirelyType(markers, { 1, 3 }, "Alloy"),
         "a selection spanning two types is false, even though the FIRST identifier matches");
    Check(!IsManualInstanceSelectionEntirelyType(markers, {}, "Alloy"),
         "an empty selection is never \"entirely this type\"");
    Check(!IsManualInstanceSelectionEntirelyType(markers, { 99 }, "Alloy"),
         "an identifier that resolves to no transform at all is false, same as a real type mismatch");
    Check(IsManualInstanceSelectionEntirelyType(markers, { 3 }, "Plasma"),
         "a single-type selection matching a DIFFERENT type-section name is true for THAT name");

    // Alias-folded group name — a real import's plural "Alloys" must still satisfy "Alloy".
    std::vector<Params::MarkerInstanceGroup> aliasedMarkers(1);
    aliasedMarkers[0].name = "Alloys";
    aliasedMarkers[0].transforms.resize(1);
    aliasedMarkers[0].transforms[0].instanceIdentifier = 7;
    Check(IsManualInstanceSelectionEntirelyType(aliasedMarkers, { 7 }, "Alloy"),
         "a real import's plural group name folds to the canonical singular Type-section name");
}

// STEP239 — "+Link"'s own cross-type partition, retained as a pure UI-display helper (STEP247
// retires its old role driving a PARAMS mutation; STEP248 reuses it for the Links-Section body's
// per-type grouping).
void RunPartitionSelectedManualInstancesByTypeChecks() {
    std::vector<Params::MarkerInstanceGroup> markers(2);
    markers[0].name = "Alloy";
    markers[0].transforms.resize(2);
    markers[0].transforms[0].instanceIdentifier = 1;
    markers[0].transforms[1].instanceIdentifier = 2;
    markers[1].name = "Plasma";
    markers[1].transforms.resize(1);
    markers[1].transforms[0].instanceIdentifier = 3;

    const auto byType = PartitionSelectedManualInstancesByType(markers, { 1, 2, 3, 99 });

    Check(byType.size() == 2, "one bucket per represented type");
    Check(byType.count("Alloy") == 1 && byType.at("Alloy") == std::vector<int>({ 1, 2 }),
         "the Alloy bucket holds both Alloy identifiers, in selection order");
    Check(byType.count("Plasma") == 1 && byType.at("Plasma") == std::vector<int>({ 3 }),
         "the Plasma bucket holds the single Plasma identifier");
    Check(byType.count("Nonexistent") == 0,
         "no bucket exists for a type that was never represented");
    for (const auto& typeAndIdentifiers : byType)
        for (const int identifier : typeAndIdentifiers.second)
            Check(identifier != 99, "a stale/unresolved identifier (99) is omitted from every bucket");
}

// STEP248 — the Links Section's own hierarchical body needs: "every instanceIdentifier currently
// tagged to Link X, grouped by canonical type name," returning (groupIndex, transformIndex) pairs.
void RunPartitionLinkedManualInstancesByTypeChecks() {
    std::vector<Params::MarkerInstanceGroup> markers(2);
    markers[0].name = "Alloy";
    markers[0].transforms.resize(3);
    markers[0].transforms[0].instanceIdentifier = 1; markers[0].transforms[0].linkIdentifier = 7;
    markers[0].transforms[1].instanceIdentifier = 2; markers[0].transforms[1].linkIdentifier = -1;   // unlinked
    markers[0].transforms[2].instanceIdentifier = 3; markers[0].transforms[2].linkIdentifier = 9;    // a DIFFERENT Link
    markers[1].name = "Plasma";
    markers[1].transforms.resize(1);
    markers[1].transforms[0].instanceIdentifier = 4; markers[1].transforms[0].linkIdentifier = 7;

    const auto byType = PartitionLinkedManualInstancesByType(markers, 7);

    Check(byType.size() == 2, "one bucket per represented type actually tagged to THIS Link");
    Check(byType.count("Alloy") == 1 && byType.at("Alloy").size() == 1
       && byType.at("Alloy")[0] == std::make_pair(0, 0),
         "the Alloy bucket holds only the (groupIndex, transformIndex) pair tagged to this Link");
    Check(byType.count("Plasma") == 1 && byType.at("Plasma").size() == 1
       && byType.at("Plasma")[0] == std::make_pair(1, 0),
         "the Plasma bucket holds its own tagged (groupIndex, transformIndex) pair");

    const auto emptyByType = PartitionLinkedManualInstancesByType(markers, 123);
    Check(emptyByType.empty(), "a Link identifier with zero tagged instances partitions to nothing");
}

// Alias-folded group name — a real import's plural "Alloys" must still land under the canonical
// singular "Alloy" bucket, same convention PartitionSelectedManualInstancesByType already applies.
void RunPartitionLinkedManualInstancesByTypeAliasFoldingChecks() {
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].name = "Alloys";
    markers[0].transforms.resize(1);
    markers[0].transforms[0].instanceIdentifier = 5; markers[0].transforms[0].linkIdentifier = 2;

    const auto byType = PartitionLinkedManualInstancesByType(markers, 2);

    Check(byType.count("Alloy") == 1 && byType.count("Alloys") == 0,
         "a plural import group name folds to the canonical singular type-section name");
}

// STEP235 — "+ Group": the branch itself (DrawMarkersTab's bAddGroupClicked) is the predicate above
// gating ONE call to ApplyPendingCreateLayerForBundle -- the SAME already-tested "mint a Layer +
// reassign" function a drop onto a bare Group's own header already uses
// (MarkersTab_Bundles_UI_Test.cpp's own TestApplyPendingCreateLayerForBundle). This proves the exact
// composed sequence the branch performs, mirroring MarkersTab_Bundles_UI_Test.cpp's own
// "test the Apply function directly" convention rather than driving the real header button through
// imgui.
void RunAddGroupSameTypeSelectionMovesIntoNewGroupChecks() {
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].name = "Alloy";
    markers[0].transforms.resize(2);
    markers[0].transforms[0].instanceIdentifier = 1; markers[0].transforms[0].layerIndex = -1;
    markers[0].transforms[1].instanceIdentifier = 2; markers[0].transforms[1].layerIndex = -1;
    std::vector<Params::MarkerInstanceLayer> markerLayers;
    const std::vector<int> selected{ 1, 2 };
    const int newBundleIdentifier = 7;

    Check(IsManualInstanceSelectionEntirelyType(markers, selected, "Alloy"),
         "a same-type selection gates the new Group's move on");
    ApplyPendingCreateLayerForBundle(newBundleIdentifier, "Alloy", selected, markerLayers, markers);

    Check(markerLayers.size() == 1, "the new Group's first Manual Layer is created");
    Check(markerLayers[0].parentBundleIdentifier == newBundleIdentifier, "parented to the new Group");
    Check(markers[0].transforms[0].layerIndex == 0 && markers[0].transforms[1].layerIndex == 0,
         "both same-type selected instances land on the new Group's new Layer");
}

// STEP235 — mixed-type/empty selection: the real branch never calls ApplyPendingCreateLayerForBundle/
// ReassignManualInstanceLayers in either case (the predicate gates the call, not any behavior inside
// those already-tested functions) -- asserted here by confirming the predicate itself refuses both,
// which is the WHOLE of what protects "every instance's layerIndex unchanged" and "the new container
// stays empty" (there is nothing else in either branch that could move an instance).
void RunAddGroupMixedOrEmptySelectionDoesNotGateTheMoveChecks() {
    std::vector<Params::MarkerInstanceGroup> markers(2);
    markers[0].name = "Alloy";
    markers[0].transforms.resize(1);
    markers[0].transforms[0].instanceIdentifier = 1; markers[0].transforms[0].layerIndex = -1;
    markers[1].name = "Plasma";
    markers[1].transforms.resize(1);
    markers[1].transforms[0].instanceIdentifier = 2; markers[1].transforms[0].layerIndex = -1;

    Check(!IsManualInstanceSelectionEntirelyType(markers, { 1, 2 }, "Alloy"),
         "a mixed-type selection does not gate the move on");
    Check(!IsManualInstanceSelectionEntirelyType(markers, {}, "Alloy"),
         "an empty selection does not gate the move on either -- unchanged from today's "
         "create-empty-container behavior");
    Check(markers[0].transforms[0].layerIndex == -1 && markers[1].transforms[0].layerIndex == -1,
         "every instance's layerIndex is left exactly as it started -- nothing in either branch "
         "touches it when the gate does not fire");
}

// STEP235 — "+ Layer": the branch's own move is one call to the already-tested
// ReassignManualInstanceLayers, gated by the same predicate -- mirrors the Group test above one tier
// simpler (no Layer to mint, the button already minted it).
void RunAddManualLayerSameTypeSelectionMovesIntoNewLayerChecks() {
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].name = "Plasma";
    markers[0].transforms.resize(2);
    markers[0].transforms[0].instanceIdentifier = 5; markers[0].transforms[0].layerIndex = -1;
    markers[0].transforms[1].instanceIdentifier = 6; markers[0].transforms[1].layerIndex = -1;
    const std::vector<int> selected{ 5, 6 };
    const int newLayerIndex = 3;   // mirrors "recipe.markerLayers.size() - 1" right after the push_back

    Check(IsManualInstanceSelectionEntirelyType(markers, selected, "Plasma"),
         "a same-type selection gates the new Layer's move on");
    ReassignManualInstanceLayers(markers, selected, newLayerIndex);

    Check(markers[0].transforms[0].layerIndex == newLayerIndex
         && markers[0].transforms[1].layerIndex == newLayerIndex,
         "both same-type selected instances land on the new Layer directly");
}

} // namespace

int main() {
    RunPlainClickChecks();
    RunCtrlClickTogglesChecks();
    RunShiftRangeChecks();
    RunShiftWithNoAnchorFallsBackToPlainChecks();
    RunShiftWithIdentifierNotInRowOrderFallsBackChecks();
    RunIsManualInstanceSelectedChecks();
    RunReassignManualInstanceLayersChecks();
    RunReassignManualInstanceLayersToUnassignedChecks();
    RunTagManualInstancesWithLinkChecks();
    RunIsAnyManualInstanceSelectionAlreadyLinkedChecks();
    RunIsManualInstanceSelectionEntirelyTypeChecks();
    RunPartitionSelectedManualInstancesByTypeChecks();
    RunPartitionLinkedManualInstancesByTypeChecks();
    RunPartitionLinkedManualInstancesByTypeAliasFoldingChecks();
    RunAddGroupSameTypeSelectionMovesIntoNewGroupChecks();
    RunAddGroupMixedOrEmptySelectionDoesNotGateTheMoveChecks();
    RunAddManualLayerSameTypeSelectionMovesIntoNewLayerChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
