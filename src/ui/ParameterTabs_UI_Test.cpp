// ParameterTabs_UI_Test.cpp — M5-6 acceptance: every tab edits its PARAMS slice and the change
// reaches the MapRecipe. Each check drives the widgets' PURE interaction functions with a
// synthetic press/drag/release (StepDialInteraction, StepRangeSliderInteraction,
// ApplyDraggableListSignal), so no imgui frame, window or GL context is needed — the same split
// every M5-1/2/3 widget test relies on.
// This file owns main() and the Terrain + System checks; the other tabs live in
// ParameterTabs_Layers_UI_Test.cpp and ParameterTabs_Rules_UI_Test.cpp, and the two-tier dirty
// contract in ParameterTabs_DirtyTier_UI_Test.cpp (ARCH §1.5 — one binary, four translation units).
#include "TerrainTab_UI.h"
#include "SystemTab_UI.h"
#include "ParameterTabs_TestSupport_UI.h"
#include "../pipeline/GenerationAssembler_TestScene_PIPELINE.h"

using namespace SanmapGen;
using namespace SanmapGen::Ui;

void RunLayersTabChecks(Params::MapRecipe& recipe);
void RunWaterTabChecks(Params::MapRecipe& recipe);
void RunMarkersTabChecks(Params::MapRecipe& recipe);
void RunPropsTabChecks(Params::MapRecipe& recipe);
void RunTabDirtyTierChecks();

namespace {

// The seed and the map size are INTEGER settings edited through float mirrors, so this also
// proves the mirror -> recipe store, not just the dial arithmetic.
void RunTerrainTabChecks(Params::MapRecipe& recipe) {
    TerrainTabState state;
    LoadTerrainTabValues(recipe.geometry, state);
    Check(state.seedValue == static_cast<float>(recipe.geometry.seed), "the mirror loaded the seed");

    const unsigned int settledSeed = recipe.geometry.seed;
    WidgetChange change = StepDialInteraction(state.seedToggle, state.seedValue, state.seedRange,
                                              DialDrag(-50.0f));
    Check(change.bValueChanged && !change.bCommitted,
          "RT off: the seed dial moves live and defers the commit to release");
    Check(StoreTerrainTabValues(state, recipe.geometry), "the store reports the recipe moved");
    Check(recipe.geometry.seed > settledSeed, "the seed reached the recipe");
    change = StepDialInteraction(state.seedToggle, state.seedValue, state.seedRange, DialRelease());
    Check(change.bCommitted, "the commit lands on release");

    const int settledMapSize = recipe.geometry.mapSize;
    StepDialInteraction(state.mapSizeToggle, state.mapSizeValue, state.mapSizeRange, DialDrag(-40.0f));
    StoreTerrainTabValues(state, recipe.geometry);
    Check(recipe.geometry.mapSize > settledMapSize, "the map size reached the recipe");
    Check(recipe.geometry.IsValid(), "and the geometry stayed valid");

    // terrainMaxHeight is a float in the recipe, so the dial edits it in place — no mirror.
    const float settledHeight = recipe.geometry.terrainMaxHeight;
    change = StepDialInteraction(state.terrainMaxHeightToggle, recipe.geometry.terrainMaxHeight,
                                 state.terrainMaxHeightRange, DialDrag(-30.0f));
    Check(change.bValueChanged && recipe.geometry.terrainMaxHeight > settledHeight,
          "the height dial writes the recipe field directly");
    Check(recipe.geometry.terrainMaxHeight <= state.terrainMaxHeightRange.maximumValue,
          "and it stays inside the tab's declared limits");
}

// The one tab that edits NO recipe field: execution settings are not reproducible-recipe content.
void RunSystemTabChecks(const Params::MapRecipe& recipe) {
    SystemTabState state;
    Sys::DispatchPolicy dispatchPolicy;
    Check(!ApplySystemTabSettings(state, dispatchPolicy), "an untouched tab moves no policy");

    state.bDeterministic = true;
    Check(ApplySystemTabSettings(state, dispatchPolicy), "the determinism toggle moves the policy");
    Check(dispatchPolicy.bDeterministic, "and lands on Sys::DispatchPolicy, the existing home");

    // The point of the toggle, checked through the SYS resolver rather than asserted by comment:
    // an Exact-class stage is forced onto the Cpu however the rest of the policy is set.
    dispatchPolicy.outputAccuracy = Sys::AccuracyClass::Exact;
    dispatchPolicy.outputBackend  = Sys::ComputeBackend::Gpu;
    Check(Sys::ResolveBackend(dispatchPolicy, Sys::GenerationContext::Output,
                              Sys::ComputeBackend::Gpu, Sys::DataResidency::OnGpu)
              == Sys::ComputeBackend::Cpu,
          "deterministic forces the Cpu path for an Exact stage");

    state.globalBackend = Sys::ComputeBackend::Cpu;
    Check(state.globalBackend == Sys::ComputeBackend::Cpu && state.assetCacheDirectory[0] == '\0',
          "backend and cache directory are tab state, not recipe state");
    Check(recipe.IsValid(), "the system tab left the recipe alone");
}

} // namespace

int main() {
    Params::MapRecipe recipe = AssemblerTest::MakeRecipe(4242u);
    RunTerrainTabChecks(recipe);
    RunLayersTabChecks(recipe);
    RunMarkersTabChecks(recipe);
    RunPropsTabChecks(recipe);
    RunWaterTabChecks(recipe);
    RunSystemTabChecks(recipe);
    RunTabDirtyTierChecks();

    if (previewTestFailureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", previewTestFailureCount);
    return 1;
}
