// Application_PanelSystem_UI.cpp — the bodies of the SYSTEM group: Performance and Files.
// Layer: UI. Behind Application_UI.h (ARCH §1.5).
//
// Performance is the ONE panel the shell draws controls for rather than delegating, and the reason
// is stated in SystemTab_UI.h SCOPE NOTE 2: `Sys::DispatchPolicy` is per stage and PIPELINE fans out
// only context and global backend, so the per-stage fan-out is the app shell's job. The panel is
// therefore the shell's four rows OVER the existing execution tab, which keeps its own settings —
// there is no rival backend toggle, only one policy object per stage (ARCH §4).
#include "Application_UI.h"
#include "Checkbox_UI.h"
#include "FilesTab_UI.h"
#include "Section_UI.h"
#include "SystemTab_UI.h"
#include "../io/FootprintBakeStaleness_IO.h"
#include <imgui.h>

namespace SanmapGen {
namespace Ui {

// TAB_REBUILD_PLAN "Performance": Use GPU Terrain, Use GPU Flow, WYSIWYG Baking. v1's fourth row,
// "GPU Preview Iterations", is NOT repeated here — in v2 that budget is per stage and the Flow tab
// already owns both of its iteration limits (FlowTab_UI.h SCOPE NOTE 2), so a second control for it
// would be exactly the rival toggle ARCH §4 forbids.
// The panel only EDITS the toggles; the fan-out onto the stages runs once per frame in
// Application::ApplyExecutionPolicy(), so a setting keeps holding while another panel is on screen.
void Application::DrawPerformancePanel() {
    LoadExecutionSettings(assembler, executionSettings);
    if (DrawSectionBegin("Hardware Acceleration", tabState.performanceSection)) {
        DrawCheckbox("Use Gpu Terrain Generation", executionSettings.bUseGpuTerrain);
        DrawCheckbox("Use Gpu Flow Accumulation", executionSettings.bUseGpuFlow);
        DrawCheckbox("Wysiwyg Baking (the output pass runs the preview pass)",
                     executionSettings.bWysiwygBaking);
        ImGui::TextWrapped("Cpu is the accuracy path, Gpu the speed path. Deterministic mode below "
                           "still forces Cpu for the Exact-class stages.");
        DrawSectionEnd();
    }
    ImGui::Separator();
    // STEP96_FootprintBakeAndStalenessCheck_IO.md §3.1 call site 2: the staleness check runs ONLY
    // after a real "Force Regenerate" click (never every frame, never inside PreviewDriver's own
    // continuous recompute — ARCH_18_02_IngestedDataDeterminism.md §18.2 rule 2) and NEVER gates the
    // click itself — DrawSystemTab already called RequestMapUpdate() unconditionally before
    // returning here. This is the one place both `recipe` and `assetBridge.templateIngestReport` are
    // reachable alongside the button — SystemTab_UI.cpp itself still touches neither.
    if (DrawSystemTab(tabState.system, &dispatchPolicy, &assembler, &previewDriver)) {
        const Io::FootprintBakeStalenessReport stalenessReport =
            Io::CheckFootprintBakeStaleness(recipe, assetBridge.templateIngestReport);
        tabState.system.lastRegenerateStalenessWarning =
            stalenessReport.AllFresh() ? std::string() : stalenessReport.SummaryText();
    }
    ImGui::Separator();
    DrawAssetPanel();
}

void Application::DrawSystemGroupPanel() {
    switch (tabState.activePanel) {
        case ApplicationPanel::Performance: DrawPerformancePanel(); break;
        // The Files tab writes the recipe on an import and reads the BAKED fields on an export, so
        // it is handed the assembler's own `Data::MapFields` — read-only for every export action,
        // and the one destination an import's textures may land in (FilesTab_UI.h SCOPE NOTE 2).
        case ApplicationPanel::Files:
            DrawFilesTab(recipe, tabState.files, &assembler.Fields(), &previewDriver);
            break;
        default: break;
    }
}

} // namespace Ui
} // namespace SanmapGen
