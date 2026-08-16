// Section_UI_Test.cpp — acceptance test for the collapsing section header (A2).
// Covers the open/closed state machine and the per-section state ownership. No imgui frame, no
// window, no GL: the decision is pure by construction (Section_UI.h). The bar, arrow and indent
// are a by-eye check against a live frame — nothing here asserts on pixels.
#include "Section_UI.h"
#include <cstdio>

using namespace SanmapGen;

static int failureCount = 0;

static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
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

int main() {
    TestDefaultOpenSeedsTheState();
    TestClickTogglesAndSilenceHolds();
    TestEachSectionCarriesItsOwnState();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
