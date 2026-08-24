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

int main() {
    TestDefaultOpenSeedsTheState();
    TestClickTogglesAndSilenceHolds();
    TestEachSectionCarriesItsOwnState();
    TestReservedRightWidthShrinksTheHeaderBar();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
