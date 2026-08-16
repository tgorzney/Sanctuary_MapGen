// Combo_UI_Test.cpp — acceptance test for the shared dropdown.
// Covers the selection resolve (including the index left dangling when the option list shrinks
// under a saved recipe), the never-null preview label, and the pick/idle commit contract. No imgui
// frame, no window, no GL: the interaction is pure by construction (Combo_UI.h). The popup list
// itself is a by-eye check against a live frame — nothing here asserts on pixels.
#include "Combo_UI.h"
#include <cstdio>
#include <cstring>

using namespace SanmapGen;

static int failureCount = 0;

static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

static bool IsSameText(const char* text, const char* expected) {
    return text != nullptr && expected != nullptr && std::strcmp(text, expected) == 0;
}

// The plan's height-blend modes, the shortest real option table in the rebuild.
static const char* const blendModeLabels[] = {"Add", "Subtract", "Multiply", "Overlay", "Max", "Min"};

static Ui::ComboOptions MakeBlendModeOptions() {
    Ui::ComboOptions options;
    options.labels = blendModeLabels;
    options.count  = 6;
    return options;
}

static void TestSelectionResolvesIntoTheList() {
    const Ui::ComboOptions options = MakeBlendModeOptions();
    Check(Ui::ResolvedComboSelection(3, options) == 3, "an in-range index is left alone");
    Check(Ui::ResolvedComboSelection(0, options) == 0, "including the first entry");
    Check(Ui::ResolvedComboSelection(5, options) == 5, "and the last");
    Check(Ui::ResolvedComboSelection(6, options) == -1, "one past the end is not a selection");
    Check(Ui::ResolvedComboSelection(-4, options) == -1, "a negative index resolves to nothing picked");

    // A list that shrank under a saved recipe — a sanpack swapped for one with fewer materials.
    Ui::ComboOptions shrunken = options;
    shrunken.count = 2;
    Check(Ui::ResolvedComboSelection(4, shrunken) == -1, "an index past a shrunken list resolves to nothing");

    Ui::ComboOptions emptyOptions;
    Check(Ui::ResolvedComboSelection(0, emptyOptions) == -1, "an empty list can never have a selection");
    Ui::ComboOptions countWithoutLabels;
    countWithoutLabels.count = 4;
    Check(Ui::ResolvedComboSelection(1, countWithoutLabels) == -1,
          "a count with no label table is refused rather than dereferenced");
}

static void TestThePreviewLabelIsNeverNull() {
    const Ui::ComboOptions options = MakeBlendModeOptions();
    Check(IsSameText(Ui::ComboSelectionLabel(2, options), "Multiply"), "the picked entry names itself");
    Check(IsSameText(Ui::ComboSelectionLabel(-1, options), "<none>"), "nothing picked shows the empty label");
    Check(IsSameText(Ui::ComboSelectionLabel(99, options), "<none>"), "and so does an out-of-range index");

    Ui::ComboOptions namedEmpty = options;
    namedEmpty.emptyLabel = "(inherit)";
    Check(IsSameText(Ui::ComboSelectionLabel(-1, namedEmpty), "(inherit)"), "the empty label is the caller's");
    Ui::ComboOptions nullEmpty = options;
    nullEmpty.emptyLabel = nullptr;
    Check(IsSameText(Ui::ComboSelectionLabel(-1, nullEmpty), ""), "a null empty label still draws something");

    const char* const holedLabels[] = {"First", nullptr, "Third"};
    Ui::ComboOptions holed;
    holed.labels = holedLabels;
    holed.count  = 3;
    Check(IsSameText(Ui::ComboSelectionLabel(1, holed), "<none>"), "a null entry falls back instead of crashing");
}

static void TestAPickCommitsAndAnIdleFrameRepairs() {
    const Ui::ComboOptions options = MakeBlendModeOptions();
    int selectedIndex = 0;

    Ui::WidgetChange idle = Ui::StepComboInteraction(selectedIndex, options, -1);
    Check(selectedIndex == 0 && !idle.bValueChanged && !idle.bCommitted, "an idle frame on a valid index is silent");

    Ui::WidgetChange picked = Ui::StepComboInteraction(selectedIndex, options, 4);
    Check(selectedIndex == 4, "a pick writes the caller's index");
    Check(picked.bValueChanged && picked.bCommitted, "and commits on the same frame — there is no drag to defer");

    Ui::WidgetChange repicked = Ui::StepComboInteraction(selectedIndex, options, 4);
    Check(!repicked.bValueChanged && !repicked.bCommitted, "re-picking the current entry costs nothing");

    Ui::WidgetChange refused = Ui::StepComboInteraction(selectedIndex, options, 12);
    Check(selectedIndex == 4 && !refused.bValueChanged, "a pick outside the list is ignored, not stored");

    // The dangling index is corrected the moment the control is drawn, not at the next click.
    Ui::ComboOptions shrunken = options;
    shrunken.count = 3;
    int danglingIndex = 5;
    Ui::WidgetChange repaired = Ui::StepComboInteraction(danglingIndex, shrunken, -1);
    Check(danglingIndex == -1, "an idle frame clears an index the list no longer has");
    Check(repaired.bValueChanged && repaired.bCommitted, "and reports the correction so the caller re-runs");
}

int main() {
    TestSelectionResolvesIntoTheList();
    TestThePreviewLabelIsNeverNull();
    TestAPickCommitsAndAnIdleFrameRepairs();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
