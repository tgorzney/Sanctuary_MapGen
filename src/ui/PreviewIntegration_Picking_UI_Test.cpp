// PreviewIntegration_Picking_UI_Test.cpp — the other two halves of the M4-5 acceptance test:
// a click resolves the right entity against the grid THE PIPELINE built (both picking paths must
// agree on the same instance index), and the preview matches the bake pixel for pixel — every
// pixel no mark covers is exactly the ramp applied to the baked heightfield sample, so nothing
// in the image can have come from a re-derived or re-simulated quantity (ARCH §3.2).
#include "PreviewIntegration_TestScene_UI.h"
#include "Picking_UI.h"
#include "GradientLut_UI.h"
#include <cstdio>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

void check(bool bCondition, const char* label) { CheckPreviewExpectation(bCondition, label); }

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
