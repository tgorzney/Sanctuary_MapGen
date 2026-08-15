// PreviewIntegration_UI_Test.cpp — the M4-5 end-to-end acceptance test: the pipeline, the
// composite and picking wired behind the two-tier dirty model. This file owns main() and the
// first refresh; the two tiers live in PreviewIntegration_DirtyTier_UI_Test.cpp and the
// picking / preview-matches-the-bake checks in PreviewIntegration_Picking_UI_Test.cpp
// (ARCH §1.5 file ceilings — one binary, three translation units).
// Cpu twin throughout, so no GL context is needed; the Gpu composite has its own parity test.
#include "PreviewIntegration_TestScene_UI.h"
#include <cstdio>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

void RunMapUpdateTierChecks(PreviewIntegrationScene& scene);
void RunPreviewTierChecks(PreviewIntegrationScene& scene);
void RunPickingChecks(PreviewIntegrationScene& scene);
void RunBakeMatchChecks(PreviewIntegrationScene& scene);

int main() {
    PreviewIntegrationScene scene;
    CheckPreviewExpectation(scene.driver.NeedsMapUpdate(), "a fresh driver owes a map update");
    CheckPreviewExpectation(!scene.driver.NeedsPreviewRender(), "and owes no preview render");
    CheckPreviewExpectation(scene.assembler.SpatialGridBuildCount() == 0,
                            "nothing is built before the first refresh");

    CheckPreviewExpectation(scene.driver.Refresh() == Pipeline::RefreshTier::MapUpdate,
                            "the first refresh generates the map");
    CheckPreviewExpectation(scene.driver.StagesThatRan().size() == 7,
                            "the first refresh ran every stage");
    CheckPreviewExpectation(scene.assembler.SpatialGridBuildCount() == 1,
                            "PIPELINE built the marker spatial grid immediately after Placement");
    CheckPreviewExpectation(scene.driver.PreviewCompositeCount() == 1,
                            "a map update composites exactly once");
    CheckPreviewExpectation(scene.composite.CompositeTexels().size()
                                == static_cast<std::size_t>(previewIntegrationResolution)
                                 * previewIntegrationResolution,
                            "the composite produced a full preview image");
    CheckPreviewExpectation(scene.driver.Refresh() == Pipeline::RefreshTier::Nothing,
                            "an idle refresh does nothing");
    CheckPreviewExpectation(scene.driver.PreviewCompositeCount() == 1,
                            "an idle refresh does not recomposite");

    RunMapUpdateTierChecks(scene);
    RunPreviewTierChecks(scene);
    RunPickingChecks(scene);
    RunBakeMatchChecks(scene);

    if (previewTestFailureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", previewTestFailureCount);
    return 1;
}
