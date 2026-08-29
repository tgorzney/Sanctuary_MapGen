// PreviewComposite_MapAreas_UI_Test.cpp — ARCH §14.17 acceptance: Params::MapArea rectangles
// compositing as a real PreviewFieldLayer (`PreviewLayerKind::MapAreas`) — an empty list paints
// nothing (the degenerate sentinel), a single area colors every cell it covers, overlapping areas
// resolve forward-iteration LAST-match-wins (the same Z rule §21.8's own body hit-test uses), and
// (§14.17 item 11) the drag-suppressed index is entirely absent from the composited input. Runs the
// Cpu twin only — no GL context needed (PreviewComposite_UI_Test.cpp's own established posture).
#include "PreviewComposite_TestScene_UI.h"

using namespace SanmapGen;

namespace {

using Ui::ChannelNear;
void check(bool bCondition, const char* label) { Ui::CheckPreviewExpectation(bCondition, label); }

// A fresh scene with entities cleared, so the ONE MapAreas layer this file adds is the only thing
// that can paint a pixel — no entity-mark noise, no default ramps/layers from ConfigurePreviewSettings.
void BuildBareMapAreasScene(Ui::PreviewTestScene& scene) {
    Ui::BuildPreviewTestScene(scene);
    scene.instances.Clear();
}

void ConfigureBareSettings(Ui::PreviewCompositeSettings& settings) {
    settings.previewResolution = 4;
    settings.bEntitiesEnabled  = false;
    settings.fieldLayers.push_back(
        Ui::MakeLayer(Ui::PreviewLayerKind::MapAreas, Ui::PreviewBlendMode::AlphaBlend, -1, 0.0f, 1.0f));
}

void TestEmptyAreaListPaintsNothing() {
    Ui::PreviewTestScene scene;
    BuildBareMapAreasScene(scene);
    Ui::PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.areas, scene.fields,
                                   scene.instances, scene.entityIdentifiers);
    ConfigureBareSettings(composite.Settings());
    composite.Compose();
    const unsigned int texel = composite.CompositeTexels()[0];
    check(ChannelNear(texel, 0, 0.0f) && ChannelNear(texel, 1, 0.0f) && ChannelNear(texel, 2, 0.0f),
          "an empty area list paints nothing — the clear color survives untouched");
}

void TestSingleAreaColorsCoveredCells() {
    Ui::PreviewTestScene scene;
    BuildBareMapAreasScene(scene);
    Params::MapArea area;
    area.name = "Whole"; area.originX = 0.0f; area.originZ = 0.0f;
    area.width = 4.0f; area.length = 4.0f;   // the whole 4x4 map: world == cell space, worldUnitsPerCell 1.0
    scene.areas.push_back(area);
    Ui::PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.areas, scene.fields,
                                   scene.instances, scene.entityIdentifiers);
    ConfigureBareSettings(composite.Settings());
    Ui::AreaColorEntry color;
    color.name = "Whole"; color.color[0] = 1.0f; color.color[1] = 0.0f;
    color.color[2] = 0.0f; color.color[3] = 1.0f;
    composite.Settings().areaColors.push_back(color);
    composite.Compose();
    const unsigned int texel = composite.CompositeTexels()[0];
    check(ChannelNear(texel, 0, 1.0f) && ChannelNear(texel, 1, 0.0f) && ChannelNear(texel, 2, 0.0f),
          "a full-coverage area colors every cell with its own resolved color, full opacity");
}

void TestOverlapLastMatchWins() {
    Ui::PreviewTestScene scene;
    BuildBareMapAreasScene(scene);
    Params::MapArea first;  first.name = "First";  first.width = 4.0f; first.length = 4.0f;
    Params::MapArea second; second.name = "Second"; second.width = 4.0f; second.length = 4.0f;
    scene.areas.push_back(first);
    scene.areas.push_back(second);
    Ui::PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.areas, scene.fields,
                                   scene.instances, scene.entityIdentifiers);
    ConfigureBareSettings(composite.Settings());
    Ui::AreaColorEntry firstColor;
    firstColor.name = "First"; firstColor.color[0] = 1.0f; firstColor.color[1] = 0.0f;
    firstColor.color[2] = 0.0f; firstColor.color[3] = 1.0f;
    Ui::AreaColorEntry secondColor;
    secondColor.name = "Second"; secondColor.color[0] = 0.0f; secondColor.color[1] = 1.0f;
    secondColor.color[2] = 0.0f; secondColor.color[3] = 1.0f;
    composite.Settings().areaColors.push_back(firstColor);
    composite.Settings().areaColors.push_back(secondColor);
    composite.Compose();
    const unsigned int texel = composite.CompositeTexels()[0];
    check(ChannelNear(texel, 0, 0.0f) && ChannelNear(texel, 1, 1.0f) && ChannelNear(texel, 2, 0.0f),
          "two fully-overlapping areas resolve to the LAST one in the vector, forward iteration");
}

// ARCH §14.17 item 11 (Piece C) — the drag-suppressed index is entirely absent from this frame's
// composited input, so the one area being dragged never double-paints against its own live canvas
// fill (MapCanvas_AreaDraw_UI.cpp's own suppressed-only fill).
void TestSuppressedIndexOmitsRectangle() {
    Ui::PreviewTestScene scene;
    BuildBareMapAreasScene(scene);
    Params::MapArea area;
    area.name = "Whole"; area.width = 4.0f; area.length = 4.0f;
    scene.areas.push_back(area);
    Ui::PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.areas, scene.fields,
                                   scene.instances, scene.entityIdentifiers);
    ConfigureBareSettings(composite.Settings());
    Ui::AreaColorEntry color;
    color.name = "Whole"; color.color[0] = 1.0f; color.color[1] = 0.0f;
    color.color[2] = 0.0f; color.color[3] = 1.0f;
    composite.Settings().areaColors.push_back(color);

    composite.Settings().mapAreaSuppressedIndex = 0;
    composite.Compose();
    const unsigned int suppressedTexel = composite.CompositeTexels()[0];
    check(ChannelNear(suppressedTexel, 0, 0.0f) && ChannelNear(suppressedTexel, 1, 0.0f)
       && ChannelNear(suppressedTexel, 2, 0.0f),
          "the suppressed index's rectangle is entirely absent from the composited frame, matching "
          "the immediate-mode canvas pass drawing that ONE area meanwhile");

    // An out-of-range suppressed value suppresses nothing — the correct degradation §14.17 item 11
    // names for a list reorder racing a gesture.
    composite.Settings().mapAreaSuppressedIndex = 5;
    composite.Compose();
    const unsigned int unsuppressedTexel = composite.CompositeTexels()[0];
    check(ChannelNear(unsuppressedTexel, 0, 1.0f) && ChannelNear(unsuppressedTexel, 1, 0.0f)
       && ChannelNear(unsuppressedTexel, 2, 0.0f),
          "an out-of-range suppressed index suppresses nothing");
}

} // namespace

int main() {
    TestEmptyAreaListPaintsNothing();
    TestSingleAreaColorsCoveredCells();
    TestOverlapLastMatchWins();
    TestSuppressedIndexOmitsRectangle();
    if (Ui::previewTestFailureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", Ui::previewTestFailureCount);
    return 1;
}
