// PreviewIntegration_Picking_UI_Test.cpp — the other two halves of the M4-5 acceptance test:
// a click resolves the right entity against the grid THE PIPELINE built (both picking paths must
// agree on the same instance index), and the preview matches the bake pixel for pixel — every
// pixel no mark covers is exactly the ramp applied to the baked heightfield sample, so nothing
// in the image can have come from a re-derived or re-simulated quantity (ARCH §3.2).
// RunCanvasPickingChecks (STEP48) is the one addition this migration needs: it drives the actual
// `MapCanvas::ApplyClick` path — STEP47's inverse projection composed with `PickMarker` against
// `Data::SpatialGrid` — instead of exercising the two picking primitives independently, and proves
// the specific regression the migration exists to fix: a pick that does not drift with zoom.
#include "PreviewIntegration_TestScene_UI.h"
#include "MapCanvas_UI.h"
#include "Picking_UI.h"
#include "GradientLut_UI.h"
#include <cmath>
#include <cstdio>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

void check(bool bCondition, const char* label) { CheckPreviewExpectation(bCondition, label); }

// The exact inverse of the canvas's own click resolution: world -> preview pixel (the composite's
// own mapping) -> region-local point (MapCanvasView's inverse of ResolvePreviewPixel), so a test
// can synthesize "the cursor position that lands on this world point" at the view's CURRENT zoom.
RegionLocalPoint WorldToRegionLocal(const PreviewComposite& composite, const MapCanvasView& view,
                                    float worldX, float worldZ) {
    const PreviewComposite::PreviewPixelPoint pixel = composite.WorldToPreviewPixel(worldX, worldZ);
    return view.ProjectPreviewPixelToRegionLocal(pixel.pixelX, pixel.pixelY);
}

// A world position at least `clearance` units from every marker — the "clicked empty ground" case.
bool FindEmptyWorldPosition(const Data::PlacementInstances& markers, float mapWorldSize,
                            float clearance, float& worldX, float& worldZ) {
    for (float probeZ = 1.0f; probeZ < mapWorldSize; probeZ += 1.0f)
        for (float probeX = 1.0f; probeX < mapWorldSize; probeX += 1.0f) {
            bool bClear = true;
            for (std::size_t marker = 0; marker < markers.Count() && bClear; ++marker) {
                const float offsetX = markers.positionX[marker] - probeX;
                const float offsetZ = markers.positionZ[marker] - probeZ;
                bClear = offsetX * offsetX + offsetZ * offsetZ > clearance * clearance;
            }
            if (bClear) { worldX = probeX; worldZ = probeZ; return true; }
        }
    return false;
}

// PickMarker deliberately tests only the ONE chunk the click's world point falls in (ARCH §8.3),
// so a marker sitting near its chunk's boundary can legitimately miss a click that lands a
// fraction of a world unit into the neighbor chunk — the world<->preview-pixel round trip has up
// to ~1 preview pixel of reconstruction error (PreviewComposite_Prepare_UI.cpp's own contract
// note). Pick the resolved marker with the most clearance from its chunk's edges, so this test
// exercises the pick itself rather than that unrelated, pre-existing chunking edge case.
std::size_t MarkerAwayFromChunkBoundary(const Data::PlacementInstances& markers,
                                        const Data::SpatialGrid& grid) {
    const float chunkSize = grid.MapWorldSize() / static_cast<float>(grid.ChunkResolution());
    std::size_t best = 0;
    float bestClearance = -1.0f;
    for (std::size_t marker = 0; marker < markers.Count(); ++marker) {
        const float localX = std::fmod(markers.positionX[marker], chunkSize);
        const float localZ = std::fmod(markers.positionZ[marker], chunkSize);
        const float clearanceX = localX < chunkSize - localX ? localX : chunkSize - localX;
        const float clearanceZ = localZ < chunkSize - localZ ? localZ : chunkSize - localZ;
        const float clearance = clearanceX < clearanceZ ? clearanceX : clearanceZ;
        if (clearance > bestClearance) { bestClearance = clearance; best = marker; }
    }
    return best;
}

bool IsPixelUnderAnyMark(const PreviewIntegrationScene& scene, int pixelX, int pixelY) {
    const Data::PlacementInstances& markers = scene.assembler.Placements().markers;
    const float exclusion = scene.composite.Settings().entityMarkRadiusPixels + 2.0f;
    for (std::size_t marker = 0; marker < markers.Count(); ++marker) {
        int markPixelX = 0, markPixelY = 0;
        MarkerPixel(scene, marker, markPixelX, markPixelY);
        const float offsetX = static_cast<float>(markPixelX - pixelX);
        const float offsetY = static_cast<float>(markPixelY - pixelY);
        if (offsetX * offsetX + offsetY * offsetY <= exclusion * exclusion) return true;
    }
    return false;
}

} // namespace

void RunPickingChecks(PreviewIntegrationScene& scene) {
    const Data::SpatialGrid& grid = scene.assembler.MarkerSpatialGrid();
    const Data::PlacementInstances& markers = scene.assembler.Placements().markers;
    check(markers.Count() > 0, "the pipeline placed markers to pick");
    check(grid.EntryCount() == static_cast<std::int32_t>(markers.Count()),
          "every resolved marker is indexed by the grid PIPELINE built");
    check(grid.MapWorldSize() > 0.0f, "the grid was configured with the map's world extent");

    bool bEveryMarkerResolved = true, bEveryPickIsLocal = true, bIdentifiersAgree = true;
    for (std::size_t marker = 0; marker < markers.Count(); ++marker) {
        std::int32_t visitedEntryCount = 0;
        const std::int32_t picked = PickMarker(grid, markers, markers.positionX[marker],
                                               markers.positionZ[marker], 0.5f, &visitedEntryCount);
        if (picked < 0 || markers.positionX[static_cast<std::size_t>(picked)] != markers.positionX[marker]
            || markers.positionZ[static_cast<std::size_t>(picked)] != markers.positionZ[marker])
            bEveryMarkerResolved = false;
        if (visitedEntryCount > static_cast<std::int32_t>(markers.Count())) bEveryPickIsLocal = false;

        // The other O(1) path: the id the composite wrote under that marker's mark.
        int pixelX = 0, pixelY = 0;
        MarkerPixel(scene, marker, pixelX, pixelY);
        if (PickEntity(scene.entityIdentifiers, pixelX, pixelY)
            != static_cast<std::uint32_t>(picked)) bIdentifiersAgree = false;
    }
    check(bEveryMarkerResolved, "a click on a marker resolves that marker through the grid");
    check(bEveryPickIsLocal, "a pick tests one chunk's bucket, never the whole population");
    check(bIdentifiersAgree,
          "the entity id the composite wrote and the grid pick name the same instance");

    float emptyWorldX = 0.0f, emptyWorldZ = 0.0f;
    check(FindEmptyWorldPosition(markers, grid.MapWorldSize(), 2.0f, emptyWorldX, emptyWorldZ),
          "the map has ground no marker occupies");
    check(PickMarker(grid, markers, emptyWorldX, emptyWorldZ, 0.5f) == kNoMarkerPicked,
          "a click on empty ground picks nothing");
}

// STEP48: the canvas itself — not the two primitives in isolation — resolves a click the same
// way regardless of zoom (screen-space radius, world-space math), which is the whole point of
// migrating off the baked, texel-space id buffer (ARCH_14_PreviewOverlayLayering.md §14).
void RunCanvasPickingChecks(PreviewIntegrationScene& scene) {
    const Data::PlacementInstances& markers = scene.assembler.Placements().markers;
    const Data::SpatialGrid& grid = scene.assembler.MarkerSpatialGrid();
    check(markers.Count() > 0, "there is a marker to click through the canvas");
    if (markers.Count() == 0) return;

    constexpr float canvasRegionSidePixels  = 256.0f;
    // A generous radius for the zoom-consistency check below: this scene's preview is
    // deliberately tiny (64 pixels over a 64-unit world — one pixel per world unit, chosen so
    // RunBakeMatchChecks can compare pixels exactly), which makes the world<->preview-pixel round
    // trip's own quantization (up to ~1 preview pixel, PreviewComposite_Prepare_UI.cpp) large
    // relative to a screen-realistic pick radius once zoomed in. A real preview (default 512)
    // makes that quantization negligible; this constant only compensates for the test scene's
    // coarseness, not a change to the picking math itself.
    constexpr float zoomConsistencyPickRadiusScreenPixels = 32.0f;
    // The pick radius for the "just outside" checks below — an ordinary, screen-realistic value
    // (matches ApplicationSettings::markerIconRadiusPixels' default).
    constexpr float outsideRadiusPickRadiusScreenPixels = 8.0f;

    MapCanvas canvas;
    canvas.View().SetPreviewResolution(scene.composite.Resolution());
    canvas.View().SetRegionSide(canvasRegionSidePixels);
    canvas.SetPreviewComposite(&scene.composite);
    canvas.SetMarkerPickingSource(&markers, &grid);

    // --- The same marker is picked at zoom 1 and after zooming in — no texel-space drift.
    canvas.SetMarkerPickRadiusScreenPixels(zoomConsistencyPickRadiusScreenPixels);
    const std::size_t sampledMarker = MarkerAwayFromChunkBoundary(markers, grid);
    const RegionLocalPoint atZoomOne = WorldToRegionLocal(scene.composite, canvas.View(),
        markers.positionX[sampledMarker], markers.positionZ[sampledMarker]);
    const std::uint32_t selectedAtZoomOne = canvas.ApplyClick(atZoomOne.regionLocalX, atZoomOne.regionLocalY);
    check(selectedAtZoomOne != Data::EntityIdBuffer::emptySentinel,
          "clicking a marker's exact world position selects it at zoom 1");

    canvas.View().ZoomAtRegionPoint(atZoomOne.regionLocalX, atZoomOne.regionLocalY, 4.0f);
    check(canvas.View().ZoomScale() > 1.0f, "the view actually zoomed in");
    const RegionLocalPoint atZoomedIn = WorldToRegionLocal(scene.composite, canvas.View(),
        markers.positionX[sampledMarker], markers.positionZ[sampledMarker]);
    const std::uint32_t selectedZoomedIn = canvas.ApplyClick(atZoomedIn.regionLocalX, atZoomedIn.regionLocalY);
    check(selectedZoomedIn == selectedAtZoomOne,
          "the same marker is picked after zooming in — the click math does not scale with zoom");

    // --- A click just outside every marker's screen-space pick radius selects nothing, at more
    // than one zoom level (proves the screen-space -> world-space radius conversion, not just the
    // world-space distance test, is correct).
    canvas.SetMarkerPickRadiusScreenPixels(outsideRadiusPickRadiusScreenPixels);
    canvas.View().SetPreviewResolution(scene.composite.Resolution());   // back to zoom 1
    for (float zoomStepScale : { 1.0f, 4.0f }) {
        if (zoomStepScale != 1.0f)
            canvas.View().ZoomAtRegionPoint(canvasRegionSidePixels * 0.5f, canvasRegionSidePixels * 0.5f,
                                            zoomStepScale);
        const float pickRadiusWorldUnits = outsideRadiusPickRadiusScreenPixels
            * canvas.View().PreviewPixelsPerRegionPixel()
            * scene.composite.Settings().worldUnitsPerCell / scene.composite.PixelsPerPreviewCell();
        float emptyWorldX = 0.0f, emptyWorldZ = 0.0f;
        check(FindEmptyWorldPosition(markers, grid.MapWorldSize(), pickRadiusWorldUnits + 2.0f,
                                     emptyWorldX, emptyWorldZ),
              "the map has ground outside every marker's screen-space pick radius");
        const RegionLocalPoint emptyPoint = WorldToRegionLocal(scene.composite, canvas.View(),
                                                                emptyWorldX, emptyWorldZ);
        const std::uint32_t selectedEmpty = canvas.ApplyClick(emptyPoint.regionLocalX, emptyPoint.regionLocalY);
        check(selectedEmpty == Data::EntityIdBuffer::emptySentinel,
              "a click just outside every marker's screen-space pick radius selects nothing");
    }
}

// Preview == bake: with one Replace-blended height ramp, an uncovered pixel must equal the ramp
// applied to the bilinear sample of the BAKED heightfield at that pixel's cell position.
void RunBakeMatchChecks(PreviewIntegrationScene& scene) {
    const PreviewCompositeSettings& settings = scene.composite.Settings();
    const std::vector<float> lookupTable = BakeGradientLut(settings.gradientRamps[heightRampIndex]);
    const int lookupEntryCount = static_cast<int>(lookupTable.size())
                               / static_cast<int>(kLookupChannelCount);
    const Data::FloatField& heightfield = scene.assembler.Fields().heightfield;
    const int resolution = scene.composite.Resolution();
    const float cellsPerPixel = 1.0f / PreviewPixelsPerCell(scene);

    int comparedPixelCount = 0, mismatchCount = 0;
    for (int pixelY = 0; pixelY < resolution; ++pixelY)
        for (int pixelX = 0; pixelX < resolution; ++pixelX) {
            if (IsPixelUnderAnyMark(scene, pixelX, pixelY)) continue;
            const float sampleX = (static_cast<float>(pixelX) + 0.5f) * cellsPerPixel;
            const float sampleY = (static_cast<float>(pixelY) + 0.5f) * cellsPerPixel;
            const PreviewColor expected = SampleGradientLookupTable(
                lookupTable.data(), lookupEntryCount,
                ClampUnit(heightfield.SampleBilinear(sampleX, sampleY)));
            const unsigned int texel =
                scene.composite.CompositeTexels()[static_cast<std::size_t>(pixelY) * resolution + pixelX];
            ++comparedPixelCount;
            if (!ChannelNear(texel, 0, expected.red) || !ChannelNear(texel, 1, expected.green)
                || !ChannelNear(texel, 2, expected.blue)) ++mismatchCount;
        }
    check(comparedPixelCount > resolution, "enough uncovered pixels to compare");
    check(mismatchCount == 0, "every uncovered preview pixel is the ramp applied to the baked height");
    std::printf("preview/bake pixels compared=%d mismatched=%d\n", comparedPixelCount, mismatchCount);
}
