// Section_UI_Test.cpp — acceptance test for the collapsing section header (A2, STEP104).
// Covers the open/closed state machine and the per-section state ownership with PURE checks (no
// imgui frame needed — the decision is pure by construction, Section_UI.h). STEP104 adds one real,
// headless imgui frame — mirroring MapCanvas_Render_UI_Test.cpp's technique (no window backend, no
// GL, draw data / item rects inspected instead of pixels) — to prove `reservedRightWidth` actually
// shrinks `DrawSectionBegin`'s own drawn/hit-test bar width, not just a documented intent. The bar,
// arrow and indent's exact PIXELS otherwise stay a by-eye check against a live frame.
#include "Section_UI.h"
#include <cstdio>
#include <imgui.h>

using namespace SanmapGen;

static int failureCount = 0;

static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

static bool NearlyEqual(float value, float expected) {
    const float difference = value - expected;
    return difference < 0.01f && difference > -0.01f;
}

// One imgui frame with no renderer backend, mirroring MapCanvas_Render_UI_Test.cpp's
// BeginHeadlessFrame: the font atlas is built the legacy way and the frame is only rendered into
// draw data / item rects, never presented.
constexpr unsigned long long kSectionTestFontAtlasIdentifier = 0xF0000104ull;

static void BeginHeadlessFrame() {
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(512.0f, 512.0f);
    io.DeltaTime = 1.0f / 60.0f;
    unsigned char* atlasPixels = nullptr;
    int atlasWidth = 0, atlasHeight = 0;
    io.Fonts->GetTexDataAsRGBA32(&atlasPixels, &atlasWidth, &atlasHeight);
    io.Fonts->SetTexID(static_cast<ImTextureID>(kSectionTestFontAtlasIdentifier));
    ImGui::NewFrame();
}

static void TestDefaultOpenSeedsTheState() {
    Ui::SectionOptions options;
    Check(Ui::InitialSectionState(options).bOpen, "sections open by default");
    options.bDefaultOpen = false;
    Check(!Ui::InitialSectionState(options).bOpen,
          "a section asked to start closed does (the Advanced-constants case)");
}

static void TestClickTogglesAndSilenceHolds() {
    Ui::SectionState state;                                   // open
    const Ui::SectionChange idle = Ui::StepSectionHeader(state, false);
    Check(!idle.bOpenChanged && idle.bBodyVisible, "an untouched open header keeps drawing its body");

    const Ui::SectionChange closing = Ui::StepSectionHeader(state, true);
    Check(closing.bOpenChanged && !closing.bBodyVisible, "a click closes the section on the same frame");
    Check(!state.bOpen, "and the caller's state records it");

    const Ui::SectionChange stayClosed = Ui::StepSectionHeader(state, false);
    Check(!stayClosed.bOpenChanged && !stayClosed.bBodyVisible, "a closed section stays closed");

    const Ui::SectionChange reopening = Ui::StepSectionHeader(state, true);
    Check(reopening.bOpenChanged && reopening.bBodyVisible, "a second click reopens it");
    Check(state.bOpen, "and the state agrees");
}

static void TestEachSectionCarriesItsOwnState() {
    // The v1 bug this library exists to kill: shared function-static state, where toggling one
    // control moved another. Two sections here must be completely independent.
    Ui::SectionState firstSection;
    Ui::SectionState secondSection;
    Ui::StepSectionHeader(firstSection, true);
    Check(!firstSection.bOpen && secondSection.bOpen, "closing one section leaves its neighbour open");

    Ui::SectionOptions closedByDefault;
    closedByDefault.bDefaultOpen = false;
    Ui::SectionState thirdSection = Ui::InitialSectionState(closedByDefault);
    Ui::StepSectionHeader(secondSection, true);
    Check(!thirdSection.bOpen, "and a third, seeded closed, is untouched by either");
    Check(Ui::StepSectionHeader(thirdSection, true).bBodyVisible, "which still opens on its own click");
}

// STEP104: `reservedRightWidth` genuinely shrinks the header bar's own drawn width and hit-test
// region (the InvisibleButton `DrawSectionBegin` sizes itself to) rather than a caller-composed
// button overlapping a still-full-width header. `ImGui::GetItemRectSize()` right after
// `DrawSectionBegin` returns reads the InvisibleButton's own rect — the LAST item it drew.
static void TestReservedRightWidthShrinksTheHeaderBar() {
    ImGui::CreateContext();
    BeginHeadlessFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(400.0f, 200.0f));
    ImGui::Begin("SectionTestWindow", nullptr, ImGuiWindowFlags_NoSavedSettings);

    const float contentRegionAvailX = ImGui::GetContentRegionAvail().x;

    Ui::SectionState fullWidthState;
    Check(Ui::DrawSectionBegin("FullWidth", fullWidthState), "a default (0 reservedRightWidth) header opens");
    const float fullWidthBarWidth = ImGui::GetItemRectSize().x;
    Ui::DrawSectionEnd();
    Check(NearlyEqual(fullWidthBarWidth, contentRegionAvailX),
          "0 reservedRightWidth draws the full-width header, today's exact behavior");

    Ui::SectionState reservedState;
    Ui::SectionOptions reservedOptions;
    reservedOptions.reservedRightWidth = 120.0f;
    Check(Ui::DrawSectionBegin("Reserved", reservedState, reservedOptions),
          "a reserved-width header still opens");
    const float reservedBarWidth = ImGui::GetItemRectSize().x;
    Ui::DrawSectionEnd();
    Check(NearlyEqual(reservedBarWidth, contentRegionAvailX - 120.0f),
          "a reserved width genuinely shrinks the header's own drawn/hit-test bar, by exactly N");
    Check(reservedBarWidth < fullWidthBarWidth,
          "leaving real room for a caller-composed button beside it, not an overlap");

    Ui::SectionState degenerateState;
    Ui::SectionOptions degenerateOptions;
    degenerateOptions.reservedRightWidth = contentRegionAvailX + 5000.0f;   // far past the window
    Ui::DrawSectionBegin("Degenerate", degenerateState, degenerateOptions);
    Check(NearlyEqual(ImGui::GetItemRectSize().x, 1.0f),
          "a reservation wider than the row is held at the same >=1px floor the zero case already used");
    Ui::DrawSectionEnd();

    ImGui::End();
    ImGui::Render();
    ImGui::DestroyContext();
}

// STEP258 acceptance item 1 — pure StepSectionHeader, default behavior pinned unchanged: with
// bDoubleClickToggle defaulted false, StepSectionHeader(state, 1) toggles exactly as the
// pre-existing StepSectionHeader(state, true) did, and bPlainSingleClicked is false for every
// click count. A direct regression pin for the "zero blast radius on every other header" claim.
static void TestDoubleClickToggleDefaultedFalseMatchesOldBehavior() {
    Ui::SectionState state;   // open
    Check(!Ui::StepSectionHeader(state, 0).bPlainSingleClicked,
          "count 0, default options: never reports a plain single click");
    const Ui::SectionChange oneClick = Ui::StepSectionHeader(state, 1);
    Check(oneClick.bOpenChanged && !oneClick.bBodyVisible && !oneClick.bPlainSingleClicked,
          "count 1, default options: toggles exactly like the old bool-true call, never reports a plain single click");
    const Ui::SectionChange twoClick = Ui::StepSectionHeader(state, 2);
    Check(twoClick.bOpenChanged && twoClick.bBodyVisible && !twoClick.bPlainSingleClicked,
          "count 2, default options: still toggles (any click toggles) and never reports a plain single click");
    const Ui::SectionChange threeClick = Ui::StepSectionHeader(state, 3);
    Check(threeClick.bOpenChanged && !threeClick.bPlainSingleClicked,
          "count 3, default options: still toggles (any click toggles) and never reports a plain single click");
}

// STEP258 acceptance item 2 — pure StepSectionHeader, the new split: with bDoubleClickToggle=true,
// count 0 -> no toggle/no plain-single; count 1 -> no toggle, plain-single true; count 2 -> toggles,
// plain-single false; count 3 -> neither (inert). A follow-up count 1 after the count-2 toggle
// proves click 1 never re-toggles.
static void TestDoubleClickToggleSplitsSingleFromDouble() {
    Ui::SectionState state;   // open
    const bool bStartOpen = state.bOpen;

    const Ui::SectionChange idle = Ui::StepSectionHeader(state, 0, true);
    Check(!idle.bOpenChanged && !idle.bPlainSingleClicked, "count 0: no toggle, no plain single click");
    Check(state.bOpen == bStartOpen, "count 0: state untouched");

    const Ui::SectionChange single = Ui::StepSectionHeader(state, 1, true);
    Check(!single.bOpenChanged && single.bPlainSingleClicked,
          "count 1: does not toggle bOpen, reports a plain single click instead");
    Check(state.bOpen == bStartOpen, "count 1: state still untouched by a plain single click");

    const Ui::SectionChange doubleClick = Ui::StepSectionHeader(state, 2, true);
    Check(doubleClick.bOpenChanged && !doubleClick.bPlainSingleClicked,
          "count 2 (the second click of a double-click): toggles, does not report a plain single click");
    const bool bAfterToggle = state.bOpen;
    Check(bAfterToggle != bStartOpen, "count 2: the state actually flipped");

    const Ui::SectionChange triple = Ui::StepSectionHeader(state, 3, true);
    Check(!triple.bOpenChanged && !triple.bPlainSingleClicked,
          "count 3: inert by construction -- neither toggles nor reports a plain single click");
    Check(state.bOpen == bAfterToggle, "count 3: state left exactly where the count-2 toggle set it");

    const Ui::SectionChange singleAgain = Ui::StepSectionHeader(state, 1, true);
    Check(!singleAgain.bOpenChanged && singleAgain.bPlainSingleClicked,
          "a follow-up count 1 after the count-2 toggle reports a plain single click again");
    Check(state.bOpen == bAfterToggle, "and never re-toggles the state the count-2 click already set");
}

// STEP258 acceptance item 3 — a real mouse double-click sequence through DrawSectionBegin itself,
// mirroring MarkersTab_TypeSectionHideToggle_UI_Test.cpp's own multi-frame press/release harness.
// NOTE: unlike a regular ImGui::Button/SmallButton (which fires on RELEASE), DrawSectionBegin's own
// InvisibleButton click is read via ImGui::IsItemClicked() == IsMouseClicked() && IsItemHovered() —
// i.e. the PRESS transition, not the release (imgui.cpp:6673) — this is pre-existing behavior,
// unchanged by this ticket. Every click/toggle assertion below is therefore made on the PRESS frame.
static void TestRealMouseDoubleClickSequenceThroughDrawSectionBegin() {
    ImGui::CreateContext();
    BeginHeadlessFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(400.0f, 200.0f));
    ImGui::Begin("SectionDoubleClickTestWindow", nullptr, ImGuiWindowFlags_NoSavedSettings);

    Ui::SectionState state;
    Ui::SectionOptions options;
    options.bDoubleClickToggle = true;
    bool bPlainSingleClicked = false;

    Check(Ui::DrawSectionBegin("LinkLike", state, options, Ui::WidgetStyle(), &bPlainSingleClicked),
          "settling frame: header opens with no click yet");
    const ImVec2 rectMin = ImGui::GetItemRectMin();
    const ImVec2 rectMax = ImGui::GetItemRectMax();
    Ui::DrawSectionEnd();
    ImGui::End();
    ImGui::Render();
    const ImVec2 headerCenter((rectMin.x + rectMax.x) * 0.5f, (rectMin.y + rectMax.y) * 0.5f);

    // Settle: move the mouse onto the header without pressing yet (real click-ownership routing
    // keys off the item having been visited/hovered on a PRIOR frame).
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(headerCenter.x, headerCenter.y);
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(400.0f, 200.0f));
    ImGui::Begin("SectionDoubleClickTestWindow", nullptr, ImGuiWindowFlags_NoSavedSettings);
    const bool bBodyVisibleBeforeClicks =
        Ui::DrawSectionBegin("LinkLike", state, options, Ui::WidgetStyle(), &bPlainSingleClicked);
    if (bBodyVisibleBeforeClicks) Ui::DrawSectionEnd();
    ImGui::End();
    ImGui::Render();

    // First press (one click — IsItemClicked fires on THIS frame, the press transition).
    io.AddMouseButtonEvent(0, true);
    BeginHeadlessFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(400.0f, 200.0f));
    ImGui::Begin("SectionDoubleClickTestWindow", nullptr, ImGuiWindowFlags_NoSavedSettings);
    const bool bBodyVisibleOnFirstPress =
        Ui::DrawSectionBegin("LinkLike", state, options, Ui::WidgetStyle(), &bPlainSingleClicked);
    if (bBodyVisibleOnFirstPress) Ui::DrawSectionEnd();
    ImGui::End();
    ImGui::Render();
    Check(bPlainSingleClicked, "one click: outPlainSingleClicked reads true on the press frame");
    Check(bBodyVisibleOnFirstPress == bBodyVisibleBeforeClicks,
          "one click: the returned body-visible bool is UNCHANGED (bDoubleClickToggle=true, no toggle yet)");

    // Release (settle, no new click), then a second press well under io.MouseDoubleClickTime at the
    // same position — the toggle fires on THIS second press frame (GetMouseClickedCount reads 2).
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(400.0f, 200.0f));
    ImGui::Begin("SectionDoubleClickTestWindow", nullptr, ImGuiWindowFlags_NoSavedSettings);
    const bool bBodyVisibleAfterRelease =
        Ui::DrawSectionBegin("LinkLike", state, options, Ui::WidgetStyle(), &bPlainSingleClicked);
    if (bBodyVisibleAfterRelease) Ui::DrawSectionEnd();
    ImGui::End();
    ImGui::Render();

    io.AddMouseButtonEvent(0, true);
    BeginHeadlessFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(400.0f, 200.0f));
    ImGui::Begin("SectionDoubleClickTestWindow", nullptr, ImGuiWindowFlags_NoSavedSettings);
    const bool bBodyVisibleOnSecondPress =
        Ui::DrawSectionBegin("LinkLike", state, options, Ui::WidgetStyle(), &bPlainSingleClicked);
    if (bBodyVisibleOnSecondPress) Ui::DrawSectionEnd();
    ImGui::End();
    ImGui::Render();
    Check(bBodyVisibleOnSecondPress != bBodyVisibleAfterRelease,
          "second click (the double-click): the returned body-visible bool has flipped -- the toggle fired");
    Check(!bPlainSingleClicked, "second click: outPlainSingleClicked reads false");
    io.AddMouseButtonEvent(0, false);   // release, settling the button state before context teardown

    ImGui::DestroyContext();

    // A companion pass with default SectionOptions (no bDoubleClickToggle): the FIRST single click
    // still toggles immediately -- live-frame-verifying item 1's pure-logic claim end to end.
    ImGui::CreateContext();
    BeginHeadlessFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(400.0f, 200.0f));
    ImGui::Begin("SectionDefaultClickTestWindow", nullptr, ImGuiWindowFlags_NoSavedSettings);
    Ui::SectionState defaultState;
    Check(Ui::DrawSectionBegin("Plain", defaultState), "settling frame: default header opens");
    const ImVec2 defaultRectMin = ImGui::GetItemRectMin();
    const ImVec2 defaultRectMax = ImGui::GetItemRectMax();
    Ui::DrawSectionEnd();
    ImGui::End();
    ImGui::Render();
    const ImVec2 defaultHeaderCenter((defaultRectMin.x + defaultRectMax.x) * 0.5f,
                                     (defaultRectMin.y + defaultRectMax.y) * 0.5f);

    ImGuiIO& defaultIo = ImGui::GetIO();
    defaultIo.AddMousePosEvent(defaultHeaderCenter.x, defaultHeaderCenter.y);
    defaultIo.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(400.0f, 200.0f));
    ImGui::Begin("SectionDefaultClickTestWindow", nullptr, ImGuiWindowFlags_NoSavedSettings);
    const bool bDefaultBodyVisibleBeforeClick = Ui::DrawSectionBegin("Plain", defaultState);
    if (bDefaultBodyVisibleBeforeClick) Ui::DrawSectionEnd();
    ImGui::End();
    ImGui::Render();

    // The FIRST press is the click frame (IsItemClicked fires on press) -- the toggle fires HERE.
    defaultIo.AddMouseButtonEvent(0, true);
    BeginHeadlessFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(400.0f, 200.0f));
    ImGui::Begin("SectionDefaultClickTestWindow", nullptr, ImGuiWindowFlags_NoSavedSettings);
    const bool bDefaultBodyVisibleOnPress = Ui::DrawSectionBegin("Plain", defaultState);
    if (bDefaultBodyVisibleOnPress) Ui::DrawSectionEnd();
    ImGui::End();
    ImGui::Render();
    Check(bDefaultBodyVisibleOnPress != bDefaultBodyVisibleBeforeClick,
          "default SectionOptions: the FIRST single click still toggles immediately, on the press frame");
    defaultIo.AddMouseButtonEvent(0, false);   // release, settling the button state before teardown

    ImGui::DestroyContext();
}

int main() {
    TestDefaultOpenSeedsTheState();
    TestClickTogglesAndSilenceHolds();
    TestEachSectionCarriesItsOwnState();
    TestReservedRightWidthShrinksTheHeaderBar();
    TestDoubleClickToggleDefaultedFalseMatchesOldBehavior();
    TestDoubleClickToggleSplitsSingleFromDouble();
    TestRealMouseDoubleClickSequenceThroughDrawSectionBegin();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
