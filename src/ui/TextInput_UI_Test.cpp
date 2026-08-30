// TextInput_UI_Test.cpp — acceptance test for the shared single-line text field.
// Covers the length cap, the character rules, the difference between the LIVE form (typing) and
// the SETTLED form (leaving the field), and the two-tier change/commit contract driven by a
// synthetic key sequence. No imgui frame, no window, no GL: the interaction is pure by
// construction (TextInput_UI.h). The edit box itself is a by-eye check against a live frame.
#include "TextInput_UI.h"
#include "ListWidget_TestFrame_UI.h"
#include <cstdio>
#include <string>

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

// STEP225 — `DrawTextInput` gained a `fixedWidthPixels` parameter (default 0.0f) so a caller can
// chain it beside other controls via SameLine() instead of always claiming the rest of the line
// (AreasTab_UI.cpp's own single-line Area detail row is the first such caller). The rendered width
// itself is not assertable headless, so this instead protects every one of DrawTextInput's ~15
// OTHER call sites, which still pass 3-5 positional args today: a real click+type+commit sequence
// run through the 6-argument form and through the explicit 7-argument (fixedWidthPixels=0.0f) form
// must land on the exact same committed value and the exact same WidgetChange shape.
struct TextInputRunResult {
    std::string value;
    bool        bValueChangedAnyFrame = false;
    bool        bCommittedAnyFrame    = false;
};

// One real mouse click to focus the field (hover/press/release, the same 3-frame sequence every
// other click-driven test in this codebase uses to focus/activate a widget), one typed character,
// then Enter (imgui's own commit-and-defocus for a single-line, non-multiline InputText) - run
// through `DrawTextInput` itself so this exercises the actual imgui draw path, not just the pure
// TextInputRules/StepTextInputInteraction helpers the checks above already cover.
static TextInputRunResult RunTypeAndCommitSequenceThroughDrawTextInput(bool bPassFixedWidthPixelsExplicitly) {
    Ui::HeadlessImguiSession session;
    std::string value = "Old";
    TextInputRunResult result;
    // The field fills the window's own content width (both call forms below resolve to <= 0.0f),
    // starting right after the default WindowPadding - well inside the field for any window this
    // size, regardless of the exact font metrics this build's default font atlas settles on.
    const ImVec2 kFieldClickPosition(50.0f, 18.0f);

    auto drawOneFrame = [&](const ImVec2& mousePosition, bool bMouseDown, bool bTypeCharacterThisFrame,
                            bool bEnterKeyDown) {
        ImGuiIO& io = ImGui::GetIO();
        io.AddMousePosEvent(mousePosition.x, mousePosition.y);
        io.AddMouseButtonEvent(ImGuiMouseButton_Left, bMouseDown);
        io.AddKeyEvent(ImGuiKey_Enter, bEnterKeyDown);
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(240.0f, 80.0f));
        ImGui::Begin("TextInputParityTestWindow", nullptr,
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);
        if (bTypeCharacterThisFrame) io.AddInputCharactersUTF8("Q");
        const Ui::WidgetChange change = bPassFixedWidthPixelsExplicitly
            ? Ui::DrawTextInput("Field", value, Ui::TextInputRules(), Ui::WidgetStyle(), nullptr,
                                /*bLabelHidden=*/true, /*fixedWidthPixels=*/0.0f)
            : Ui::DrawTextInput("Field", value, Ui::TextInputRules(), Ui::WidgetStyle(), nullptr,
                                /*bLabelHidden=*/true);
        ImGui::End();
        ImGui::Render();
        if (change.bValueChanged) result.bValueChangedAnyFrame = true;
        if (change.bCommitted)    result.bCommittedAnyFrame    = true;
    };

    drawOneFrame(kFieldClickPosition, /*bMouseDown=*/false, false, false);   // hover
    drawOneFrame(kFieldClickPosition, /*bMouseDown=*/true,  false, false);   // press: focuses the field
    drawOneFrame(kFieldClickPosition, /*bMouseDown=*/false, false, false);   // release
    drawOneFrame(kFieldClickPosition, /*bMouseDown=*/false, true,  false);   // type "Q" while focused
    drawOneFrame(kFieldClickPosition, /*bMouseDown=*/false, false, true);    // Enter down: commits + defocuses
    drawOneFrame(kFieldClickPosition, /*bMouseDown=*/false, false, false);   // Enter up

    result.value = value;
    return result;
}

static void TestFixedWidthPixelsDefaultIsByteIdenticalToTheSixArgumentForm() {
    const TextInputRunResult sixArgumentForm   = RunTypeAndCommitSequenceThroughDrawTextInput(false);
    const TextInputRunResult sevenArgumentForm = RunTypeAndCommitSequenceThroughDrawTextInput(true);

    Check(sixArgumentForm.value != "Old",
          "sanity: the typed character actually landed, so this comparison is not vacuously trivial");
    Check(sixArgumentForm.value == sevenArgumentForm.value,
          "omitting fixedWidthPixels and passing its 0.0f default explicitly commit the exact same "
          "value - the new parameter changes only the rendered width, never the typing/commit result");
    Check(sixArgumentForm.bValueChangedAnyFrame == sevenArgumentForm.bValueChangedAnyFrame
          && sixArgumentForm.bCommittedAnyFrame == sevenArgumentForm.bCommittedAnyFrame,
          "...and the same WidgetChange signal shape (bValueChanged/bCommitted) across the whole "
          "sequence - every one of DrawTextInput's other ~15 call sites is unaffected by this ticket");
}

int main() {
    TestTheLengthCapIsAlwaysEnforced();
    TestTypingIsNotTrimmedButLeavingIs();
    TestAnEmptyNameFallsBackOnlyWhenItMust();
    TestTypingChangesLiveAndLeavingCommits();
    TestFixedWidthPixelsDefaultIsByteIdenticalToTheSixArgumentForm();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
