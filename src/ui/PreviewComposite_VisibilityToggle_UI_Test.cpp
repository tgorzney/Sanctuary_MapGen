// PreviewComposite_VisibilityToggle_UI_Test.cpp — STEP200 acceptance: the View popup's new
// left-of-name visibility icon (DraggableListWidget_UI.h's Flat row) applies through the existing
// `ToggleVisibility` signal into `PreviewFieldLayer::bEnabled`
// (Application_ViewLayersPopup_UI.h's `ApplyViewLayerSignal`, unchanged by STEP200), and
// `bEnabled == false` removes the layer from the COMPOSITE only — never the BAKED field data it
// would have sampled. Confirms PreviewComposite_Prepare_UI.cpp's existing
// `if (!layer.bEnabled) continue;` (BuildLayerConfigurations) is the only thing a visibility
// toggle touches: the underlying mapFields the disabled layer would have sampled must be
// byte-identical before and after Compose(), the same discipline
// TestSlopeLayerSamplesTheBakedField (PreviewComposite_UI_Test.cpp) already applies to a bake the
// composite DOES read.
// Runs the Cpu twin, no GL context needed.
#include "PreviewComposite_TestScene_UI.h"
#include <cstring>

using namespace SanmapGen;

namespace {

void check(bool bCondition, const char* label) { Ui::CheckPreviewExpectation(bCondition, label); }

void TestVisibilityOffSkipsCompositeButNotBake() {
    Ui::PreviewTestScene scene;
    Ui::BuildPreviewTestScene(scene);
    Ui::PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.fields,
                                   scene.instances, scene.entityIdentifiers);
    Ui::ConfigurePreviewSettings(composite.Settings());
    composite.Settings().bEntitiesEnabled = false;
    // The scene's own Flow field (fieldLayers[2]) is what this test disables.
    const Data::FloatField bakedFlow = scene.fields.flow;   // Mask/Flow-stage output, read-only here
    composite.Compose();
    const int passCountWithFlowVisible = composite.ExecutedPassCount();

    // The View popup's ToggleVisibility signal only ever flips this one bool
    // (Application_ViewLayersPopup_UI.h's ApplyViewLayerSignal) — flipping it directly here is
    // exactly what that signal application does, without needing an imgui frame.
    composite.Settings().fieldLayers[2].bEnabled = false;
    composite.Compose();

    check(composite.ExecutedPassCount() == passCountWithFlowVisible - 1,
          "toggling a layer off costs exactly the one composite pass it used to draw");
    check(std::memcmp(scene.fields.flow.Data(), bakedFlow.Data(),
                      bakedFlow.CellCount() * sizeof(float)) == 0,
          "the flow field the hidden layer would have sampled is untouched by the toggle");

    // Toggling it back on restores the pass — the skip is a per-Compose() read of bEnabled, not a
    // one-way destructive edit of anything BuildLayerConfigurations built from the bake.
    composite.Settings().fieldLayers[2].bEnabled = true;
    composite.Compose();
    check(composite.ExecutedPassCount() == passCountWithFlowVisible,
          "re-enabling the layer composites it again from the same, never-touched bake");
}

} // namespace

int main() {
    TestVisibilityOffSkipsCompositeButNotBake();
    if (Ui::previewTestFailureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", Ui::previewTestFailureCount);
    return 1;
}
