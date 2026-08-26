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
#include "../params/MapRecipe_PARAMS.h"

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
    // ARCH §19.25 — the callback now carries the full OverlayInstanceKey_UI; this test only cares
    // about the procedural entity id it reports, so it reads `.instanceIndex` back the same way
    // MapCanvas::SelectedEntityIdentifier() itself does (a thin cast, `bValid == false` -> emptySentinel).
    canvas.SetSelectionChangedCallback([&](const OverlayInstanceKey_UI& key) {
        reportedSelection = static_cast<std::uint32_t>(key.instanceIndex);
        ++selectionChangeCount;
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

// ARCH_19_25_SelectionRepresentationUnification.md items 3-5: the linear manual-marker hit-test
// fallback (a canvas click at a manual marker's screen position, with no procedural hit) and the
// shell-mediated list-click landing point (SelectManualMarkerByInstanceIdentifier) — both drive
// MapCanvas's own widened selectedInstanceKey through the SAME canonical SetSelection the
// procedural checks above already exercise, never a second, divergent setter.
void RunManualMarkerSelectionChecks() {
    PreviewTestScene scene;
    BuildPreviewTestScene(scene);   // one PROCEDURAL marker at world (2,2)
    PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.fields,
                               scene.instances, scene.entityIdentifiers);
    ComposeClickableScene(composite);

    Data::SpatialGrid markerSpatialGrid;
    markerSpatialGrid.Configure(static_cast<float>(scene.geometry.mapSize) * scene.geometry.worldUnitsPerCell);
    markerSpatialGrid.Build(scene.instances.positionX.data(), scene.instances.positionZ.data(),
                            static_cast<std::int32_t>(scene.instances.Count()));

    // One MANUAL marker, far enough from the lone procedural one that a click on it MISSES
    // PickMarker first (the procedural branch ApplyClick tries before the linear fallback).
    std::vector<Params::MarkerInstanceGroup> markers(1);
    Params::MarkerTransform manualTransform;
    manualTransform.transform.positionX = 0.5f;
    manualTransform.transform.positionZ = 0.5f;
    manualTransform.instanceIdentifier = 99;
    markers[0].transforms.push_back(manualTransform);
    Params::MapRecipe recipe;

    MapCanvas canvas;
    canvas.SetPreviewTexture(nullptr, Sys::GpuTextureHandle(), pickPreviewResolution);
    canvas.View().SetRegionSide(pickRegionSidePixels);
    canvas.SetPreviewComposite(&composite);
    canvas.SetMarkerPickingSource(&scene.instances, &markerSpatialGrid);
    canvas.SetMarkerPickRadiusScreenPixels(pickRadiusScreenPixels);
    canvas.SetManualMarkerDragSource(&markers, nullptr, &scene.geometry, &recipe);

    OverlayInstanceKey_UI lastReportedKey;
    int selectionChangeCount = 0;
    canvas.SetSelectionChangedCallback([&](const OverlayInstanceKey_UI& key) {
        lastReportedKey = key; ++selectionChangeCount;
    });

    // Item 3 — canvas click -> manual marker: a synthetic click at the manual marker's own screen
    // position (no procedural hit).
    const PreviewComposite::PreviewPixelPoint manualPixel = composite.WorldToPreviewPixel(0.5f, 0.5f);
    const RegionLocalPoint manualScreenPoint =
        canvas.View().ProjectPreviewPixelToRegionLocal(manualPixel.pixelX, manualPixel.pixelY);
    canvas.ApplyClick(manualScreenPoint.regionLocalX, manualScreenPoint.regionLocalY);
    check(canvas.HasSelection(), "a canvas click at a manual marker's screen position selects it");
    check(canvas.SelectedEntityIdentifier() == 99u,
          "the selected instanceIdentifier is the manual marker's own id (99), a linear-hit-test result");
    check(selectionChangeCount == 1 && lastReportedKey.bManual && lastReportedKey.instanceIndex == 99
              && lastReportedKey.collection == PlacementCollectionKind_UI::Markers,
          "the reported key is {Markers, 99, true, bManual=true} — a manual selection, correctly tagged "
          "(what Application::WireCallbacks() reads to sync tabState.markers.selectedManualInstanceIdentifier)");

    // Item 5 — list click -> canvas: the shell-mediated landing point drives the SAME real selection.
    selectionChangeCount = 0;
    canvas.SelectManualMarkerByInstanceIdentifier(99);
    check(selectionChangeCount == 0,
          "re-selecting the SAME instance is a no-op — SetSelection's own equal-key short-circuit");
    canvas.SelectManualMarkerByInstanceIdentifier(123);
    check(canvas.HasSelection() && canvas.SelectedEntityIdentifier() == 123u,
          "SelectManualMarkerByInstanceIdentifier drives the canvas's own real selection, not a proxy");
    check(selectionChangeCount == 1 && lastReportedKey.bManual && lastReportedKey.instanceIndex == 123
              && lastReportedKey.collection == PlacementCollectionKind_UI::Markers,
          "the canvas's selectedInstanceKey updates to exactly {Markers, 123, true, true}, as item 5 specifies");

    // The binding edge case (§19.25): instanceIdentifier < 0 is never a legal manual selection target
    // — a negative id clears the selection instead of claiming a nonsensical manual key, mirroring
    // MarkersTabState::selectedManualInstanceIdentifier's own "-1 = nothing selected" sentinel.
    canvas.SelectManualMarkerByInstanceIdentifier(-1);
    check(!canvas.HasSelection(), "a negative instanceIdentifier clears the selection, never claims it");
}

// STEP132 (ARCH §19.27) — the procedural sibling of RunManualMarkerSelectionChecks' item-5 half: a
// Markers-tab PROCEDURAL instance-list click's own landing point
// (SelectProceduralMarkerInstanceByArrayPosition) routes through the SAME canonical SetSelection,
// `bManual=false` and the array position as the key — never a second, divergent selection path.
void RunProceduralMarkerListSelectionChecks() {
    MapCanvas canvas;
    OverlayInstanceKey_UI lastReportedKey;
    int selectionChangeCount = 0;
    canvas.SetSelectionChangedCallback([&](const OverlayInstanceKey_UI& key) {
        lastReportedKey = key; ++selectionChangeCount;
    });

    canvas.SelectProceduralMarkerInstanceByArrayPosition(7);
    check(canvas.HasSelection() && canvas.SelectedEntityIdentifier() == 7u,
          "SelectProceduralMarkerInstanceByArrayPosition drives the canvas's own real selection");
    check(selectionChangeCount == 1 && !lastReportedKey.bManual && lastReportedKey.instanceIndex == 7
              && lastReportedKey.collection == PlacementCollectionKind_UI::Markers,
          "the reported key is {Markers, 7, true, bManual=false} — a PROCEDURAL selection (array "
          "position), correctly tagged the OPPOSITE of the manual sibling's bManual=true");

    selectionChangeCount = 0;
    canvas.SelectProceduralMarkerInstanceByArrayPosition(7);
    check(selectionChangeCount == 0,
          "re-selecting the SAME array position is a no-op — SetSelection's own equal-key short-circuit");

    // A negative array position is never a legal selection target, mirroring the manual sibling's
    // own "-1 = nothing selected" sentinel handling.
    canvas.SelectProceduralMarkerInstanceByArrayPosition(-1);
    check(!canvas.HasSelection(), "a negative array position clears the selection, never claims it");
}

} // namespace Ui
} // namespace SanmapGen
