// MapCanvas_Picking_UI_Test.cpp — acceptance test, part 3: a click on a known marker resolves
// that marker, a click on empty space resolves nothing — the canvas causes no work of its own —
// draw, pick, pan/zoom only. One translation unit of the MapCanvas_UI_Test binary.
// STEP48: the click resolves against `Data::SpatialGrid`/`Data::PlacementInstances` (world space,
// through STEP47's inverse projection), not the composite's baked, texel-space id buffer — the
// composite still runs first because it is still the source of the world<->preview-pixel mapping
// `MapCanvas::SetPreviewComposite` reads, but its id-buffer WRITE is no longer what the canvas
// reads.
#include "MapCanvas_UI.h"
#include "PreviewComposite_TestScene_UI.h"

namespace SanmapGen {
namespace Ui {
namespace {

void check(bool bCondition, const char* label) { CheckPreviewExpectation(bCondition, label); }

constexpr int   pickPreviewResolution   = 64;
constexpr float pickRegionSidePixels    = 256.0f;
constexpr float pickRadiusScreenPixels  = 8.0f;   // matches ApplicationSettings' default

// The one instance of the shared test scene sits at world (2,2) on a 4-cell map, which the
// composite resolves to preview pixel (31.5, 31.5) — the centre. Region-local 128 is that pixel.
void ComposeClickableScene(PreviewComposite& composite) {
    ConfigurePreviewSettings(composite.Settings());
    composite.Settings().previewResolution = pickPreviewResolution;
    composite.Settings().entityMarkRadiusPixels = 3.0f;
    composite.ComposeOnCpu();
}

} // namespace

void RunMapCanvasPickingChecks() {
    PreviewTestScene scene;
    BuildPreviewTestScene(scene);
    PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.fields,
                               scene.instances, scene.entityIdentifiers);
    ComposeClickableScene(composite);

    // The spatial index PIPELINE would build over the same resolved markers (GenerationAssembler::
    // BuildMarkerSpatialGrid) — built locally here since this scene has no assembler of its own.
    Data::SpatialGrid markerSpatialGrid;
    markerSpatialGrid.Configure(static_cast<float>(scene.geometry.mapSize) * scene.geometry.worldUnitsPerCell);
    markerSpatialGrid.Build(scene.instances.positionX.data(), scene.instances.positionZ.data(),
                            static_cast<std::int32_t>(scene.instances.Count()));

    MapCanvas canvas;
    canvas.SetPreviewTexture(nullptr, Sys::GpuTextureHandle(), pickPreviewResolution);
    canvas.View().SetRegionSide(pickRegionSidePixels);
    canvas.SetPreviewComposite(&composite);
    canvas.SetMarkerPickingSource(&scene.instances, &markerSpatialGrid);
    canvas.SetMarkerPickRadiusScreenPixels(pickRadiusScreenPixels);
    std::uint32_t reportedSelection = Data::EntityIdBuffer::emptySentinel;
    int selectionChangeCount = 0;
    canvas.SetSelectionChangedCallback([&](std::uint32_t identifier) {
        reportedSelection = identifier; ++selectionChangeCount;
    });

    const std::uint32_t pickedEntity = canvas.ApplyClick(128.0f, 128.0f);
    check(canvas.LastPickedPixel().pixelX == 32 && canvas.LastPickedPixel().pixelY == 32,
          "the click resolved to the preview pixel the marker was composited onto");
    check(pickedEntity == 0u && canvas.HasSelection(),
          "clicking the marker's pixel selects it (its PlacementInstances index)");
    check(reportedSelection == 0u && selectionChangeCount == 1,
          "the selection change is reported once through the injected callback");

    check(canvas.ApplyClick(4.0f, 4.0f) == Data::EntityIdBuffer::emptySentinel
       && !canvas.HasSelection(), "clicking empty space selects nothing");
    check(canvas.ApplyClick(-10.0f, 4.0f) == Data::EntityIdBuffer::emptySentinel
       && !canvas.LastPickedPixel().bInsideImage,
          "clicking outside the image selects nothing and resolves to no pixel");

    // Zoomed in, the same marker is still resolved — the world-space pick and a constant on-screen
    // radius cannot disagree with the drawn window the way the old texel-space id buffer could.
    canvas.ApplyScroll(128.0f, 128.0f, 4.0f);
    check(canvas.View().ZoomScale() > 1.0f, "the wheel zooms the view in");
    check(canvas.ApplyClick(128.0f, 128.0f) == 0u,
          "the marker under the cursor is still resolved after zooming");
}

} // namespace Ui
} // namespace SanmapGen
