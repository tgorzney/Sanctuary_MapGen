// HeightmapTab_UI_Test.cpp — tab-rebuild B acceptance: the Heightmap tab edits its Geometry slice
// (seed, map size, the terrain height band) and hosts one Layer Editor. Pure checks driven with
// synthetic values — no imgui frame, no window, no GL context.
#include "HeightmapTab_UI.h"
#include "../params/MapRecipe_PARAMS.h"
#include <cstdio>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

void RunMapSizeChecks() {
    Check(HeightmapMapSizeIndexOf(1024) == 2, "the offered sizes map onto their dropdown rows");
    Check(HeightmapMapSizeIndexOf(768) == -1, "a size the dropdown does not list shows nothing picked");
    Check(HeightmapMapSizeAtIndex(4) == 4096, "the last row is 4096");
    Check(HeightmapMapSizeAtIndex(-1) == 0 && HeightmapMapSizeAtIndex(9) == 0,
          "an out-of-range row answers 'no size', never a neighbour");
}

void RunGeometryMirrorChecks() {
    Params::MapRecipe recipe;
    HeightmapTabState state;
    recipe.geometry.seed    = 4242u;
    recipe.geometry.mapSize = 512;
    LoadHeightmapTabValues(recipe.geometry, state);
    Check(state.seedValue == 4242, "the seed mirror loaded");
    Check(state.mapSizeIndex == 1, "the map size mirror loaded");
    Check(!StoreHeightmapTabValues(state, recipe.geometry), "an untouched round trip moves nothing");

    state.seedValue = 99;
    Check(StoreHeightmapTabValues(state, recipe.geometry), "a moved seed reports the recipe moved");
    Check(recipe.geometry.seed == 99u, "and reaches the geometry");

    state.mapSizeIndex = 4;
    StoreHeightmapTabValues(state, recipe.geometry);
    Check(recipe.geometry.mapSize == 4096, "the picked map size reaches the geometry");

    // A recipe on a size the dropdown does not list must survive an unrelated edit.
    recipe.geometry.mapSize = 768;
    LoadHeightmapTabValues(recipe.geometry, state);
    state.seedValue = 7;
    StoreHeightmapTabValues(state, recipe.geometry);
    Check(recipe.geometry.mapSize == 768, "an unlisted map size is preserved, never snapped");

    state.seedValue = -5;                                  // a mirror driven below zero
    StoreHeightmapTabValues(state, recipe.geometry);
    Check(recipe.geometry.seed == 0u, "a negative seed mirror lands on zero, not on 4 billion");
}

// terrainMinHeight is the field this work-order promoted (Geometry_PARAMS.h). The tab is the only
// place it can be moved, so the tab is where the band invariant is enforced.
void RunHeightBandChecks() {
    Params::Geometry geometry;
    Check(geometry.terrainMinHeight == 0.0f, "the promoted floor defaults to zero");
    Check(geometry.TerrainHeightSpan() == 128.0f, "the default band is the default ceiling");

    geometry.terrainMinHeight = 40.0f;
    ClampTerrainHeightBand(geometry);
    Check(geometry.terrainMinHeight == 40.0f && geometry.IsValid(),
          "a floor under the ceiling is left alone and stays valid");
    Check(geometry.TerrainHeightSpan() == 88.0f, "and the band narrows by exactly the floor");

    geometry.terrainMinHeight = 500.0f;                    // driven above the ceiling
    ClampTerrainHeightBand(geometry);
    Check(geometry.terrainMinHeight == geometry.terrainMaxHeight - 1.0f,
          "a floor pushed past the ceiling is held one unit below it");
    Check(geometry.IsValid(), "so a slider can never make the geometry invalid");

    geometry.terrainMaxHeight = 0.0f;                      // driven below its own limit
    ClampTerrainHeightBand(geometry);
    Check(geometry.terrainMaxHeight == 1.0f && geometry.IsValid(),
          "and a collapsed ceiling is raised back to its limit");
}

// The tab HOSTS the Layer Editor rather than re-implementing a stack editor.
void RunHostedLayerEditorChecks() {
    Params::MapRecipe recipe;
    HeightmapTabState state;
    recipe.layerStack.geoLayers.resize(1);
    recipe.layerStack.geoLayers[0].layers.resize(2);
    state.layerEditor.selectedGeoLayerIndex = 0;
    state.layerEditor.selectedLayerIndex    = 1;
    Check(SelectedLayerEditorLayer(recipe.layerStack, state.layerEditor)
          == &recipe.layerStack.geoLayers[0].layers[1],
          "the tab's editor state selects into the recipe's own stack");
    Check(!state.layerEditor.advancedConstantsSection.bOpen,
          "the hosted editor keeps Advanced (constants) collapsed");
    Check(state.globalGravity >= state.globalGravityRange.minimumValue
          && state.globalGravity <= state.globalGravityRange.maximumValue,
          "the global gravity default sits inside its own slider");
}

} // namespace

int main() {
    RunMapSizeChecks();
    RunGeometryMirrorChecks();
    RunHeightBandChecks();
    RunHostedLayerEditorChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
