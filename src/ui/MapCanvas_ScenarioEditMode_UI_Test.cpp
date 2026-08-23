// MapCanvas_ScenarioEditMode_UI_Test.cpp — STEP78 acceptance test, main() only. Spans several
// translation units (ARCH §1.5 ceilings): classification/interaction/commit/Preview-As are
// headless (no imgui frame, no GL); the draw pass needs one live headless imgui frame (no GL, no
// window backend) — mirrors MapCanvas_IconLayer_UI_Test's own split.
#include "PreviewComposite_TestScene_UI.h"

namespace SanmapGen {
namespace Ui {
void RunScenarioEditModeClassifyChecks();      // MapCanvas_ScenarioEditMode_Classify_UI_Test.cpp
void RunScenarioEditModeInteractionChecks();   // MapCanvas_ScenarioEditMode_Interaction_UI_Test.cpp
void RunScenarioEditModeCommitChecks();        // MapCanvas_ScenarioEditMode_Commit_UI_Test.cpp
void RunScenarioEditModePreviewAsChecks();     // MapCanvas_ScenarioEditMode_PreviewAs_UI_Test.cpp
void RunScenarioEditModeDrawChecks();          // MapCanvas_ScenarioEditMode_DrawMarkers_UI_Test.cpp
} // namespace Ui
} // namespace SanmapGen

using namespace SanmapGen;

int main() {
    Ui::RunScenarioEditModeClassifyChecks();
    Ui::RunScenarioEditModeInteractionChecks();
    Ui::RunScenarioEditModeCommitChecks();
    Ui::RunScenarioEditModePreviewAsChecks();
    Ui::RunScenarioEditModeDrawChecks();

    if (Ui::previewTestFailureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", Ui::previewTestFailureCount);
    return 1;
}
