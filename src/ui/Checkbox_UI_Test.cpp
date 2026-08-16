// Checkbox_UI_Test.cpp — acceptance test for the shared tick box and the exclusive bit group.
// Covers the immediate-commit contract of a boolean, the one-bit-only invariant of an XOR group
// (the symmetry axis masks), and the repair of a mask that arrives with several bits set. No imgui
// frame, no window, no GL: the interaction is pure by construction (Checkbox_UI.h). The ImDrawList
// rectangles themselves are a by-eye check against a live frame — nothing here asserts on pixels.
#include "Checkbox_UI.h"
#include <cstdio>

using namespace SanmapGen;

static int failureCount = 0;

static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

// The five symmetry axis bits of the plan, named so the checks below read like the tab does.
enum : int { kPointBit = 0, kAxisXBit = 1, kAxisZBit = 2, kAxisXYBit = 3, kRadialBit = 4, kAxisBitCount = 5 };

static void TestABooleanCommitsOnTheClickFrame() {
    bool bValue = false;
    Ui::WidgetChange idle = Ui::StepCheckboxInteraction(bValue, false);
    Check(!idle.bValueChanged && !idle.bCommitted, "an unclicked box reports nothing");
    Check(!bValue, "and does not move the value");

    Ui::WidgetChange clicked = Ui::StepCheckboxInteraction(bValue, true);
    Check(bValue, "a click flips the value");
    Check(clicked.bValueChanged && clicked.bCommitted, "a boolean commits on the same frame it changes");

    Ui::WidgetChange clickedBack = Ui::StepCheckboxInteraction(bValue, true);
    Check(!bValue && clickedBack.bCommitted, "and flips back on the next click");
}

static void TestAnExclusiveGroupHoldsAtMostOneBit() {
    unsigned int mask = 1u << kPointBit;
    Ui::WidgetChange picked = Ui::StepExclusiveCheckboxInteraction(mask, kAxisBitCount, kAxisZBit, true);
    Check(mask == (1u << kAxisZBit), "clicking a second box moves the tick rather than adding one");
    Check(picked.bValueChanged && picked.bCommitted, "and commits immediately");
    Check(Ui::IsExclusiveCheckboxBitSet(mask, kAxisZBit), "the reported bit is the one that is set");
    Check(!Ui::IsExclusiveCheckboxBitSet(mask, kPointBit), "and the previous bit is clear");

    Ui::WidgetChange unchanged = Ui::StepExclusiveCheckboxInteraction(mask, kAxisBitCount, -1, true);
    Check(mask == (1u << kAxisZBit) && !unchanged.bValueChanged, "no click, no change");

    // Re-clicking the set bit: allowed to clear only when the group may answer nothing.
    unsigned int clearableMask = 1u << kRadialBit;
    Ui::StepExclusiveCheckboxInteraction(clearableMask, kAxisBitCount, kRadialBit, true);
    Check(clearableMask == 0u, "re-clicking the set bit clears a group that allows none");
    unsigned int stickyMask = 1u << kRadialBit;
    Ui::WidgetChange refused = Ui::StepExclusiveCheckboxInteraction(stickyMask, kAxisBitCount, kRadialBit, false);
    Check(stickyMask == (1u << kRadialBit) && !refused.bValueChanged,
          "a group that must answer something ignores the click that would empty it");
}

static void TestAMalformedMaskIsRepairedNotObeyed() {
    // Several bits set — a recipe written before a bit existed, or a hand-edited file.
    Check(Ui::ResolvedExclusiveCheckboxMask((1u << kAxisXBit) | (1u << kRadialBit), kAxisBitCount)
              == (1u << kAxisXBit),
          "a multi-bit mask collapses onto its lowest bit");
    Check(Ui::ResolvedExclusiveCheckboxMask(1u << 9, kAxisBitCount) == 0u,
          "a bit outside the group is dropped rather than drawn off the row");
    Check(Ui::ResolvedExclusiveCheckboxMask(0xFFFFFFFFu, 0) == 0u, "an empty group resolves to nothing");
    Check(Ui::ResolvedExclusiveCheckboxMask(1u << 31, 64) == (1u << 31),
          "an oversized bit count is capped at the mask width instead of shifting illegally");

    // The repair also runs on the click path, so one stray frame cannot leave two ticks drawn.
    unsigned int mask = (1u << kAxisXBit) | (1u << kAxisZBit);
    Ui::WidgetChange repaired = Ui::StepExclusiveCheckboxInteraction(mask, kAxisBitCount, -1, true);
    Check(mask == (1u << kAxisXBit) && repaired.bValueChanged,
          "an idle frame repairs a malformed mask and reports the correction");

    // Out-of-range clicks are ignored, never shifted by an illegal amount.
    unsigned int guarded = 1u << kPointBit;
    Ui::StepExclusiveCheckboxInteraction(guarded, kAxisBitCount, kAxisBitCount + 3, true);
    Check(guarded == (1u << kPointBit), "a click outside the group leaves the mask alone");
    Check(!Ui::IsExclusiveCheckboxBitSet(guarded, -1), "a negative bit index answers false");
    Check(!Ui::IsExclusiveCheckboxBitSet(guarded, Ui::kMaximumExclusiveCheckboxCount), "and so does an over-wide one");
}

int main() {
    TestABooleanCommitsOnTheClickFrame();
    TestAnExclusiveGroupHoldsAtMostOneBit();
    TestAMalformedMaskIsRepairedNotObeyed();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
