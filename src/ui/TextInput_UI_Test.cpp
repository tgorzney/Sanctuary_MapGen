// TextInput_UI_Test.cpp — acceptance test for the shared single-line text field.
// Covers the length cap, the character rules, the difference between the LIVE form (typing) and
// the SETTLED form (leaving the field), and the two-tier change/commit contract driven by a
// synthetic key sequence. No imgui frame, no window, no GL: the interaction is pure by
// construction (TextInput_UI.h). The edit box itself is a by-eye check against a live frame.
#include "TextInput_UI.h"
#include <cstdio>

using namespace SanmapGen;

static int failureCount = 0;

static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

static bool IsSameText(const std::string& text, const char* expected) { return text == expected; }

static Ui::TextInputRules MakeRules(int maximumLength, bool bAllowEmpty) {
    Ui::TextInputRules rules;
    rules.maximumLength = maximumLength;
    rules.bAllowEmpty   = bAllowEmpty;
    return rules;
}

static void TestTheLengthCapIsAlwaysEnforced() {
    const Ui::TextInputRules shortRules = MakeRules(8, true);
    Check(Ui::ResolvedTextInputLength(shortRules) == 8, "a sane cap is the caller's own");
    Check(IsSameText(Ui::LiveTextInput("Bedrock", shortRules), "Bedrock"), "a name inside the cap is untouched");
    Check(IsSameText(Ui::LiveTextInput("Sedimentary", shortRules), "Sediment"), "a longer name is truncated to it");

    // No caller can ask for an unbounded field: the cap is held inside the staging buffer.
    const Ui::TextInputRules hugeRules = MakeRules(100000, true);
    Check(Ui::ResolvedTextInputLength(hugeRules) == Ui::kTextInputBufferCapacity - 1,
          "an over-wide cap is lowered to what the staging buffer can hold");
    const Ui::TextInputRules negativeRules = MakeRules(-5, true);
    Check(Ui::ResolvedTextInputLength(negativeRules) == 0, "a negative cap resolves to an empty field");
    Check(IsSameText(Ui::LiveTextInput("anything", negativeRules), ""), "and stores nothing");
}

static void TestTypingIsNotTrimmedButLeavingIs() {
    const Ui::TextInputRules rules = MakeRules(32, true);
    // The space just typed must survive, or the field fights the user mid-word.
    Check(IsSameText(Ui::LiveTextInput("Deep Water ", rules), "Deep Water "), "the live form keeps trailing spaces");
    Check(IsSameText(Ui::SanitizeTextInput("Deep Water ", rules), "Deep Water"), "the settled form trims them");
    Check(IsSameText(Ui::SanitizeTextInput("   Plateau   ", rules), "Plateau"), "on both ends");
    Check(IsSameText(Ui::SanitizeTextInput("Deep  Water", rules), "Deep  Water"), "but never inside the name");
    Check(IsSameText(Ui::SanitizeTextInput("     ", rules), ""), "an all-space name settles to empty");

    Ui::TextInputRules untrimmedRules = rules;
    untrimmedRules.bTrimSurroundingSpaces = false;
    Check(IsSameText(Ui::SanitizeTextInput(" kept ", untrimmedRules), " kept "), "trimming is a setting, not a law");

    // A one-line field never carries a pasted newline or tab into PARAMS.
    Check(IsSameText(Ui::LiveTextInput("Line\nTwo\tThree", rules), "LineTwoThree"),
          "control characters are dropped from a single-line field");
    Ui::TextInputRules rawRules = rules;
    rawRules.bStripControlCharacters = false;
    Check(Ui::LiveTextInput("Line\nTwo", rawRules).size() == 8u, "stripping is a setting too");
}

static void TestAnEmptyNameFallsBackOnlyWhenItMust() {
    const Ui::TextInputRules optionalRules = MakeRules(32, true);
    Check(IsSameText(Ui::SanitizeTextInput("", optionalRules), ""), "an empty value is allowed to stay empty");

    Ui::TextInputRules requiredRules = MakeRules(32, false);
    requiredRules.fallbackText = "Layer";
    Check(IsSameText(Ui::SanitizeTextInput("", requiredRules), "Layer"), "a required name falls back when emptied");
    Check(IsSameText(Ui::SanitizeTextInput("   ", requiredRules), "Layer"), "including when it is only spaces");
    Check(IsSameText(Ui::SanitizeTextInput("Ridge", requiredRules), "Ridge"), "and is left alone when it is not");

    Ui::TextInputRules nullFallbackRules = requiredRules;
    nullFallbackRules.fallbackText = nullptr;
    Check(IsSameText(Ui::SanitizeTextInput("", nullFallbackRules), ""),
          "a null fallback yields an empty string rather than being dereferenced");
}

static void TestTypingChangesLiveAndLeavingCommits() {
    const Ui::TextInputRules rules = MakeRules(16, false);
    std::string layerName = "Old";
    Ui::TextInputSignal typing;
    typing.bTextEditedThisFrame = true;

    const Ui::WidgetChange firstKey = Ui::StepTextInputInteraction(layerName, "Ne", rules, typing);
    Check(IsSameText(layerName, "Ne"), "typing moves the caller's string on the same frame");
    Check(firstKey.bValueChanged && !firstKey.bCommitted, "the live edit is not yet paid for");

    const Ui::WidgetChange spaceKey = Ui::StepTextInputInteraction(layerName, "New Ridge ", rules, typing);
    Check(IsSameText(layerName, "New Ridge "), "and the trailing space survives while typing");
    Check(spaceKey.bValueChanged && !spaceKey.bCommitted, "still nothing committed");

    Ui::TextInputSignal idle;
    idle.bTextEditedThisFrame = true;
    const Ui::WidgetChange unchanged = Ui::StepTextInputInteraction(layerName, "New Ridge ", rules, idle);
    Check(!unchanged.bValueChanged, "a frame that retypes the same text reports nothing");

    Ui::TextInputSignal leaving;
    leaving.bEditFinishedThisFrame = true;
    const Ui::WidgetChange committed = Ui::StepTextInputInteraction(layerName, "", rules, leaving);
    Check(IsSameText(layerName, "New Ridge"), "leaving the field settles the value");
    Check(committed.bValueChanged && committed.bCommitted, "and reports both the settle and the one commit");

    // Emptying a required name: the field commits the fallback, not the blank.
    std::string requiredName = "Ridge";
    const Ui::WidgetChange cleared = Ui::StepTextInputInteraction(requiredName, "", rules, typing);
    Check(IsSameText(requiredName, "") && cleared.bValueChanged, "the box may be emptied while typing");
    Ui::StepTextInputInteraction(requiredName, "", rules, leaving);
    Check(IsSameText(requiredName, "Unnamed"), "but leaving it empty installs the fallback");
}

int main() {
    TestTheLengthCapIsAlwaysEnforced();
    TestTypingIsNotTrimmedButLeavingIs();
    TestAnEmptyNameFallsBackOnlyWhenItMust();
    TestTypingChangesLiveAndLeavingCommits();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
