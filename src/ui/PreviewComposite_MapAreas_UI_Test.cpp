// PreviewComposite_MapAreas_UI_Test.cpp — ARCH §14.17/§14.19 acceptance: Params::MapArea
// rectangles compositing as a real PreviewFieldLayer (`PreviewLayerKind::MapAreas`) — an empty
// list paints nothing (the degenerate sentinel), a single area colors every cell it covers, and
// overlapping areas resolve forward-iteration FIRST-match-wins, early exit (§14.19 — supersedes
// §14.17's own "last match wins": ascending array index is now Z-descending, index 0 = top, the
// same Z rule §21.8's own body hit-test uses). Runs the Cpu twin only — no GL context needed
// (PreviewComposite_UI_Test.cpp's own established posture).
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

// ARCH §14.19 — supersedes the old TestOverlapLastMatchWins: forward iteration, FIRST match wins,
// early exit. Two same-size, fully-overlapping areas now resolve to the FIRST one in the vector.
void TestOverlapFirstMatchWins() {
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
    check(ChannelNear(texel, 0, 1.0f) && ChannelNear(texel, 1, 0.0f) && ChannelNear(texel, 2, 0.0f),
          "two fully-overlapping areas resolve to the FIRST one in the vector, forward iteration, "
          "early exit (ARCH §14.19)");
}

// STEP227 acceptance — proves the rule is genuinely about ARRAY POSITION, not size or any other
// tiebreak: a SMALL area at index 0 and a LARGE area at index 1, fully overlapping. The SMALL
// area's color must win (it is first in the vector), even though it is the smaller rectangle.
// Reversing the array order flips which color wins, with the exact same two rectangles/colors —
// isolating position, not size, as the deciding factor.
void TestOverlapArrayPositionDecidesRegardlessOfSize() {
    {
        Ui::PreviewTestScene scene;
        BuildBareMapAreasScene(scene);
        Params::MapArea small; small.name = "Small"; small.width = 1.0f; small.length = 1.0f;
        Params::MapArea large; large.name = "Large"; large.width = 4.0f; large.length = 4.0f;
        scene.areas.push_back(small);   // index 0
        scene.areas.push_back(large);   // index 1
        Ui::PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.areas, scene.fields,
                                       scene.instances, scene.entityIdentifiers);
        ConfigureBareSettings(composite.Settings());
        Ui::AreaColorEntry smallColor;
        smallColor.name = "Small"; smallColor.color[0] = 1.0f; smallColor.color[1] = 0.0f;
        smallColor.color[2] = 0.0f; smallColor.color[3] = 1.0f;
        Ui::AreaColorEntry largeColor;
        largeColor.name = "Large"; largeColor.color[0] = 0.0f; largeColor.color[1] = 1.0f;
        largeColor.color[2] = 0.0f; largeColor.color[3] = 1.0f;
        composite.Settings().areaColors.push_back(smallColor);
        composite.Settings().areaColors.push_back(largeColor);
        composite.Compose();
        const unsigned int texel = composite.CompositeTexels()[0];
        check(ChannelNear(texel, 0, 1.0f) && ChannelNear(texel, 1, 0.0f) && ChannelNear(texel, 2, 0.0f),
              "recipe.areas = [small, large]: a pixel inside both samples the SMALL area's color, "
              "the one at index 0 (ARCH §14.19's inverted Z rule)");
    }
    {
        Ui::PreviewTestScene scene;
        BuildBareMapAreasScene(scene);
        Params::MapArea large; large.name = "Large"; large.width = 4.0f; large.length = 4.0f;
        Params::MapArea small; small.name = "Small"; small.width = 1.0f; small.length = 1.0f;
        scene.areas.push_back(large);   // index 0
        scene.areas.push_back(small);   // index 1
        Ui::PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.areas, scene.fields,
                                       scene.instances, scene.entityIdentifiers);
        ConfigureBareSettings(composite.Settings());
        Ui::AreaColorEntry largeColor;
        largeColor.name = "Large"; largeColor.color[0] = 0.0f; largeColor.color[1] = 1.0f;
        largeColor.color[2] = 0.0f; largeColor.color[3] = 1.0f;
        Ui::AreaColorEntry smallColor;
        smallColor.name = "Small"; smallColor.color[0] = 1.0f; smallColor.color[1] = 0.0f;
        smallColor.color[2] = 0.0f; smallColor.color[3] = 1.0f;
        composite.Settings().areaColors.push_back(largeColor);
        composite.Settings().areaColors.push_back(smallColor);
        composite.Compose();
        const unsigned int texel = composite.CompositeTexels()[0];
        check(ChannelNear(texel, 0, 0.0f) && ChannelNear(texel, 1, 1.0f) && ChannelNear(texel, 2, 0.0f),
              "reversing the array to [large, small] samples the LARGE area's color at the SAME "
              "pixel: the rule is about array position, not size or any other tiebreak");
    }
}

} // namespace

int main() {
    TestEmptyAreaListPaintsNothing();
    TestSingleAreaColorsCoveredCells();
    TestHiddenAreaPaintsNothing();
    TestAllAreasHiddenComposesCleanly();
    TestOverlapFirstMatchWins();
    TestOverlapArrayPositionDecidesRegardlessOfSize();
    if (Ui::previewTestFailureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", Ui::previewTestFailureCount);
    return 1;
}
