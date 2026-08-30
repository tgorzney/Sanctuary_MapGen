// PreviewComposite_MapAreas_UI_Test.cpp — ARCH §14.17 acceptance: Params::MapArea rectangles
// compositing as a real PreviewFieldLayer (`PreviewLayerKind::MapAreas`) — an empty list paints
// nothing (the degenerate sentinel), a single area colors every cell it covers, and overlapping
// areas resolve forward-iteration LAST-match-wins (the same Z rule §21.8's own body hit-test
// uses). Runs the Cpu twin only — no GL context needed (PreviewComposite_UI_Test.cpp's own
// established posture).
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

// STEP222 acceptance: a hidden area contributes no rectangle to the composited fill (observed the
// same way every other test in this file observes BuildMapAreaConfigurations — through the
// composited pixel, since `mapAreaRectangles`/`BuildMapAreaConfigurations` are both private to
// PreviewComposite and this ticket adds no new accessor). Re-showing it restores its color, and
// hiding every area in the list still composes cleanly (the existing degenerate-sentinel fallback
// already proven empty-list-safe by TestEmptyAreaListPaintsNothing) rather than crashing on a
// zero-length rectangle buffer.
void TestHiddenAreaPaintsNothing() {
    Ui::PreviewTestScene scene;
    BuildBareMapAreasScene(scene);
    Params::MapArea area;
    area.name = "Whole"; area.originX = 0.0f; area.originZ = 0.0f;
    area.width = 4.0f; area.length = 4.0f;
    scene.areas.push_back(area);
    Ui::PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.areas, scene.fields,
                                   scene.instances, scene.entityIdentifiers);
    ConfigureBareSettings(composite.Settings());
    Ui::AreaColorEntry color;
    color.name = "Whole"; color.color[0] = 1.0f; color.color[1] = 0.0f;
    color.color[2] = 0.0f; color.color[3] = 1.0f;
    composite.Settings().areaColors.push_back(color);
    Ui::AreaVisibilityEntry hidden;
    hidden.name = "Whole"; hidden.bVisible = false;
    composite.Settings().areaVisibility.push_back(hidden);
    composite.Compose();
    const unsigned int hiddenTexel = composite.CompositeTexels()[0];
    check(ChannelNear(hiddenTexel, 0, 0.0f) && ChannelNear(hiddenTexel, 1, 0.0f) && ChannelNear(hiddenTexel, 2, 0.0f),
          "STEP222: a hidden area contributes NO rectangle — the clear color survives untouched");

    // Re-showing it restores exactly the same full-coverage color TestSingleAreaColorsCoveredCells
    // already proves for a visible area — the same table, the same area, now flipped back.
    composite.Settings().areaVisibility[0].bVisible = true;
    composite.Compose();
    const unsigned int shownTexel = composite.CompositeTexels()[0];
    check(ChannelNear(shownTexel, 0, 1.0f) && ChannelNear(shownTexel, 1, 0.0f) && ChannelNear(shownTexel, 2, 0.0f),
          "STEP222: re-showing a hidden area restores its own resolved color, full opacity");
}

void TestAllAreasHiddenComposesCleanly() {
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
    Ui::AreaVisibilityEntry firstHidden;  firstHidden.name  = "First";  firstHidden.bVisible  = false;
    Ui::AreaVisibilityEntry secondHidden; secondHidden.name = "Second"; secondHidden.bVisible = false;
    composite.Settings().areaVisibility.push_back(firstHidden);
    composite.Settings().areaVisibility.push_back(secondHidden);
    composite.Compose();   // must not crash on a zero-length rectangle buffer (the sentinel fallback)
    const unsigned int texel = composite.CompositeTexels()[0];
    check(ChannelNear(texel, 0, 0.0f) && ChannelNear(texel, 1, 0.0f) && ChannelNear(texel, 2, 0.0f),
          "STEP222: hiding every area in the list still composes cleanly to the clear color, exactly "
          "like an empty area list — never a zero-length rectangle buffer");
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

} // namespace

int main() {
    TestEmptyAreaListPaintsNothing();
    TestSingleAreaColorsCoveredCells();
    TestHiddenAreaPaintsNothing();
    TestAllAreasHiddenComposesCleanly();
    TestOverlapLastMatchWins();
    if (Ui::previewTestFailureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", Ui::previewTestFailureCount);
    return 1;
}
