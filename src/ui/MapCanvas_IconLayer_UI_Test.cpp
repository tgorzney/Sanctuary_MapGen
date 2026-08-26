// MapCanvas_IconLayer_UI_Test.cpp — acceptance test for STEP53's screen-space overlay icon draw
// pass, main() only. The checks span four translation units (ARCH §1.5 ceilings), mirroring
// MapCanvas_UI_Test.cpp's own split: culling+LOD and budget/cache are headless (no imgui frame, no
// GL); bucketing+bulk-write needs one live headless imgui frame (no GL, no window backend).
#include "PreviewComposite_TestScene_UI.h"

namespace SanmapGen {
namespace Ui {
// Defined in the sibling test translation units.
void RunMapCanvasIconLayerCullChecks();     // MapCanvas_IconLayer_Cull_UI_Test.cpp
// STEP201 — per-domain visibility-toggle acceptance (MapCanvas_IconLayer_Cull_OverlayDomainToggle_UI_Test.cpp)
void RunMapCanvasIconLayerOverlayDomainToggleChecks();
void RunMapCanvasIconLayerBudgetChecks();   // MapCanvas_IconLayer_Budget_UI_Test.cpp
void RunMapCanvasIconLayerCacheChecks();    // MapCanvas_IconLayer_Cache_UI_Test.cpp
void RunMapCanvasIconLayerDrawChecks();     // MapCanvas_IconLayer_Draw_UI_Test.cpp
void RunMapCanvasIconLayerDrawChunkChecks();  // MapCanvas_IconLayer_DrawChunk_UI_Test.cpp
// STEP133 — the per-Type Hide/Unhide preview filter (MapCanvas_IconLayer_MarkerTypeVisibility_UI_Test.cpp)
void RunMapCanvasIconLayerMarkerTypeVisibilityChecks();
} // namespace Ui
} // namespace SanmapGen

using namespace SanmapGen;

int main() {
    Ui::RunMapCanvasIconLayerCullChecks();
    Ui::RunMapCanvasIconLayerOverlayDomainToggleChecks();
    Ui::RunMapCanvasIconLayerBudgetChecks();
    Ui::RunMapCanvasIconLayerCacheChecks();
    Ui::RunMapCanvasIconLayerDrawChecks();
    Ui::RunMapCanvasIconLayerDrawChunkChecks();
    Ui::RunMapCanvasIconLayerMarkerTypeVisibilityChecks();

    if (Ui::previewTestFailureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", Ui::previewTestFailureCount);
    return 1;
}
