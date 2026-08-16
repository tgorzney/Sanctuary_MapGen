// MapCanvas_Picking_UI_Test.cpp — acceptance test, part 3: a click on a known entity resolves
// that entity, a click on empty space resolves nothing, and the only work the canvas can cause is
// the injected regeneration callback. One translation unit of the MapCanvas_UI_Test binary.
// The scene is composited first (the composite writes the entity-id buffer), then clicked
// through the canvas — i.e. the test goes through the same path the user does.
#include "MapCanvas_UI.h"
#include "PreviewComposite_TestScene_UI.h"

namespace SanmapGen {
namespace Ui {
namespace {

void check(bool bCondition, const char* label) { CheckPreviewExpectation(bCondition, label); }

constexpr int   pickPreviewResolution = 64;
constexpr float pickRegionSidePixels  = 256.0f;

// The one instance of the shared test scene sits at world (2,2) on a 4-cell map, which the
// composite resolves to preview pixel (31.5, 31.5) — the centre. Region-local 128 is that pixel.
void ComposeClickableScene(PreviewTestScene& scene, PreviewComposite& composite) {
    ConfigurePreviewSettings(composite.Settings());
    composite.Settings().previewResolution = pickPreviewResolution;
    composite.Settings().entityMarkRadiusPixels = 3.0f;
    composite.ComposeOnCpu();
    check(scene.entityIdentifiers.Width() == pickPreviewResolution,
          "the composite sized the entity-id buffer to the preview resolution");
}

} // namespace

void RunMapCanvasPickingChecks() {
    PreviewTestScene scene;
    BuildPreviewTestScene(scene);
    PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.fields,
                               scene.instances, scene.entityIdentifiers);
    ComposeClickableScene(scene, composite);

    MapCanvas canvas;
    canvas.SetPreviewTexture(nullptr, Sys::GpuTextureHandle(), pickPreviewResolution);
    canvas.View().SetRegionSide(pickRegionSidePixels);
    canvas.SetEntityIdentifierBuffer(&scene.entityIdentifiers);
    std::uint32_t reportedSelection = Data::EntityIdBuffer::emptySentinel;
    int selectionChangeCount = 0;
    canvas.SetSelectionChangedCallback([&](std::uint32_t identifier) {
        reportedSelection = identifier; ++selectionChangeCount;
    });

    const std::uint32_t pickedEntity = canvas.ApplyClick(128.0f, 128.0f);
    check(canvas.LastPickedPixel().pixelX == 32 && canvas.LastPickedPixel().pixelY == 32,
          "the click resolved to the preview pixel the entity was composited onto");
    check(pickedEntity == 0u && canvas.HasSelection(),
          "clicking the entity's pixel selects that entity (its PlacementInstances index)");
    check(reportedSelection == 0u && selectionChangeCount == 1,
          "the selection change is reported once through the injected callback");

    check(canvas.ApplyClick(4.0f, 4.0f) == Data::EntityIdBuffer::emptySentinel
       && !canvas.HasSelection(), "clicking empty space selects nothing");
    check(canvas.ApplyClick(-10.0f, 4.0f) == Data::EntityIdBuffer::emptySentinel
       && !canvas.LastPickedPixel().bInsideImage,
          "clicking outside the image selects nothing and resolves to no pixel");

    // Zoomed in, the same entity is still resolved — the click math and the drawn window are the
    // same view state, so they cannot disagree.
    canvas.ApplyScroll(128.0f, 128.0f, 4.0f);
    check(canvas.View().ZoomScale() > 1.0f, "the wheel zooms the view in");
    check(canvas.ApplyClick(128.0f, 128.0f) == 0u,
          "the entity under the cursor is still resolved after zooming");

    int regenerationCount = 0;
    canvas.SetRegenerationCallback([&]() { ++regenerationCount; });
    canvas.RequestRegeneration();
    check(regenerationCount == 1 && canvas.RegenerationRequestCount() == 1,
          "the canvas asks for a regeneration through the injected callback and nothing else");
}

} // namespace Ui
} // namespace SanmapGen
