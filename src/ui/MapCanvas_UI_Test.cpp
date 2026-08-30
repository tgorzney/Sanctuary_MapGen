// MapCanvas_UI_Test.cpp — acceptance test for MapCanvas_UI (M5-5), main() only. The checks span
// three translation units (ARCH §1.5 ceilings): the render check needs a live GL context and one
// imgui frame, the view and picking checks need neither.
// The GL context is the shared hidden-window WGL harness in GpuResource_TestSupport_SYS.h —
// reused, not re-rolled. argv[1] = the directory holding the composite's *.glsl units.
// Returns 2 (and skips only the render part) when this machine has no GL context.
#include "MapCanvas_UI.h"
#include "PreviewComposite_TestScene_UI.h"
#include "../sys/GpuResource_TestSupport_SYS.h"
#include <string>

namespace SanmapGen {
namespace Ui {
// Defined in the sibling test translation units.
void RunMapCanvasRenderChecks(Sys::GpuResourceManager& manager);  // MapCanvas_Render_UI_Test.cpp
void RunMapCanvasViewChecks();                                    // MapCanvas_View_UI_Test.cpp
void RunMapCanvasPickingChecks();                                 // MapCanvas_Picking_UI_Test.cpp
// ARCH §19.25 — MapCanvas_Picking_UI_Test.cpp's own manual-selection coverage (canvas click ->
// manual marker, list click -> canvas, via SelectManualMarkerByInstanceIdentifier).
void RunManualMarkerSelectionChecks();
// STEP132 (ARCH §19.27) — the procedural sibling, MapCanvas_Picking_UI_Test.cpp.
void RunProceduralMarkerListSelectionChecks();
// STEP78 acceptance test 4 — MapCanvas_ScenarioEditModeOwnership_UI_Test.cpp.
void RunMapCanvasScenarioEditModeOwnershipChecks(Sys::GpuResourceManager& manager);
// STEP113 — MapCanvas_ActivePanelGate_UI_Test.cpp.
void RunMapCanvasActivePanelGateChecks(Sys::GpuResourceManager& manager);
// Human's own bug report — MapCanvas_ActivePanelGate_UI_Test.cpp.
void RunMapCanvasClickSelectsManualMarkerChecks(Sys::GpuResourceManager& manager);
// ARCH §21.2/§21.5 — MapCanvas_GestureOwnership_UI_Test.cpp.
void RunMapCanvasGestureOwnershipChecks(Sys::GpuResourceManager& manager);
// ARCH §14.18 Piece C — MapCanvas_AreaDragRecomposite_UI_Test.cpp.
void RunMapCanvasAreaDragRecompositeChecks(Sys::GpuResourceManager& manager);
// STEP214 — MapCanvas_AreaAltCenterResizeModifier_UI_Test.cpp.
void RunMapCanvasAreaAltCenterResizeModifierChecks(Sys::GpuResourceManager& manager);
// STEP227/ARCH §14.19 — MapCanvas_AreaOverlapHitTest_UI_Test.cpp.
void RunMapCanvasAreaOverlapHitTestChecks(Sys::GpuResourceManager& manager);
// STEP228 — MapCanvas_AreaOverlayPanelGate_UI_Test.cpp.
void RunMapCanvasAreaOverlayPanelGateChecks(Sys::GpuResourceManager& manager);
} // namespace Ui
} // namespace SanmapGen

using namespace SanmapGen;

int main(int argumentCount, char** argumentValues) {
    const std::string shaderDirectory = argumentCount > 1 ? argumentValues[1] : ".";
    Ui::RunMapCanvasViewChecks();
    Ui::RunMapCanvasPickingChecks();
    Ui::RunManualMarkerSelectionChecks();
    Ui::RunProceduralMarkerListSelectionChecks();

    HWND window = nullptr; HDC deviceContext = nullptr; HGLRC glContext = nullptr;
    if (!GpuResourceTest::CreateHiddenGlContext(window, deviceContext, glContext)) {
        std::printf("GPU SKIPPED (no GL context)\n");
        return Ui::previewTestFailureCount == 0 ? 2 : 1;
    }
    Sys::GpuResourceManager manager(shaderDirectory);
    Ui::CheckPreviewExpectation(manager.Initialize(), "the Gpu resource manager initializes");
    Ui::RunMapCanvasRenderChecks(manager);
    Ui::RunMapCanvasScenarioEditModeOwnershipChecks(manager);
    Ui::RunMapCanvasActivePanelGateChecks(manager);
    Ui::RunMapCanvasClickSelectsManualMarkerChecks(manager);
    Ui::RunMapCanvasGestureOwnershipChecks(manager);
    Ui::RunMapCanvasAreaDragRecompositeChecks(manager);
    Ui::RunMapCanvasAreaAltCenterResizeModifierChecks(manager);
    Ui::RunMapCanvasAreaOverlapHitTestChecks(manager);
    Ui::RunMapCanvasAreaOverlayPanelGateChecks(manager);
    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(glContext);
    ReleaseDC(window, deviceContext);
    DestroyWindow(window);

    if (Ui::previewTestFailureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", Ui::previewTestFailureCount);
    return 1;
}
