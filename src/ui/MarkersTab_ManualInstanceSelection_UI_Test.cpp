// MarkersTab_ManualInstanceSelection_UI_Test.cpp — STEP141 acceptance for
// MarkersTab_ManualInstanceSelection_UI.h's pure logic: Ctrl/Shift multi-select and
// ReassignManualInstanceLayers. Pure logic only — no imgui frame needed.
#include "MarkersTab_ManualInstanceSelection_UI.h"
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

} // namespace

int main() {
    RunPlainClickChecks();
    RunCtrlClickTogglesChecks();
    RunShiftRangeChecks();
    RunShiftWithNoAnchorFallsBackToPlainChecks();
    RunShiftWithIdentifierNotInRowOrderFallsBackChecks();
    RunIsManualInstanceSelectedChecks();
    RunReassignManualInstanceLayersChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
