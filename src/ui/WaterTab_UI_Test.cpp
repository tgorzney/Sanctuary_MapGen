// WaterTab_UI_Test.cpp — tab-rebuild C3 acceptance: the Water tab edits its Params::Water slice
// (enable, the level band, the deep-water window), states its shore/wind limits in one shared
// table, and carries the wave blueprint. Pure checks driven with synthetic pointer and keyboard
// input — no imgui frame, no window, no GL context.
#include "WaterTab_UI.h"
#include "Checkbox_UI.h"
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

RangeSliderPointerInput GrabHandle(RangeSliderHandle handle, float pointerValue) {
    RangeSliderPointerInput input;
    input.grabbedHandle = handle;
    input.pointerValue  = pointerValue;
    return input;
}

// The level band's track follows the terrain height band, so the water slider can always reach
// every height the terrain can occupy and never further.
void RunLevelBandChecks() {
    Params::MapRecipe recipe;
    WaterTabState state;
    recipe.geometry.terrainMinHeight = 10.0f;
    recipe.geometry.terrainMaxHeight = 300.0f;
    ApplyTerrainHeightBandToWaterLevel(recipe.geometry, state);
    Check(state.waterLevelRange.minimumValue == 10.0f && state.waterLevelRange.maximumValue == 300.0f,
          "the level track spans exactly the terrain height band");

    const RangeSliderBounds bounds = WaterLevelBounds(state);
    Check(bounds.lowerLimit == 10.0f && bounds.upperLimit == 300.0f,
          "the dual-handle track is derived from that same band - one statement of the limits");

    recipe.water.waterLevelMaximum = 120.0f;
    LoadWaterTabValues(recipe.water, state);
    Check(state.waterLevelValues.maximumValue == 120.0f, "the level mirror loaded the surface");
    Check(state.waterLevelValues.minimumValue == state.waterLevelMinimum,
          "and the low handle loaded the caller-owned floor (SCOPE NOTE 1)");

    StepRangeSliderInteraction(state.waterLevelToggle, state.waterLevelValues, bounds,
                               GrabHandle(RangeSliderHandle::Maximum, 200.0f));
    Check(StoreWaterLevelValues(state, recipe.water), "a moved level reports the recipe moved");
    Check(recipe.water.waterLevelMaximum == 200.0f, "and the surface reached the recipe");
    Check(!StoreWaterLevelValues(state, recipe.water), "an unmoved store reports nothing");
}

// The depth window is the tab's other paired setting, and its store must stay independent of the
// level store: a stale level mirror may never write back over a level moved elsewhere.
void RunDepthWindowChecks() {
    Params::MapRecipe recipe;
    WaterTabState state;
    Check(state.deepWaterDepthBounds.lowerLimit == 0.0f && state.deepWaterDepthBounds.upperLimit == 50.0f,
          "the depth window keeps the 0..50 track the plan states");
    LoadWaterTabValues(recipe.water, state);
    Check(!StoreWaterTabValues(state, recipe.water), "an untouched round trip moves nothing");

    StepRangeSliderInteraction(state.deepWaterDepthToggle, state.deepWaterDepthValues,
                               state.deepWaterDepthBounds,
                               GrabHandle(RangeSliderHandle::Maximum, 40.0f));
    Check(StoreWaterTabValues(state, recipe.water), "the depth window store reports a move");
    Check(recipe.water.deepWaterDepthMaximum == 40.0f, "the deep-water depth reached the recipe");
    Check(recipe.water.deepWaterDepthMinimum <= recipe.water.deepWaterDepthMaximum,
          "the window stayed ordered");

    recipe.water.waterLevelMaximum = 75.0f;              // moved by some other control
    StoreWaterTabValues(state, recipe.water);            // depth store only
    Check(recipe.water.waterLevelMaximum == 75.0f,
          "the depth store never touches the level - the two stores are independent");
}

// Enabling water is a boolean: the click IS the release, so it commits on the same frame.
void RunEnableChecks() {
    Params::MapRecipe recipe;
    Check(!recipe.water.bEnabled, "water opens disabled, as the recipe defaults it");
    const WidgetChange change = StepCheckboxInteraction(recipe.water.bEnabled, true);
    Check(change.bValueChanged && change.bCommitted && recipe.water.bEnabled,
          "a click enables water and commits on the same frame");
}

// The Shore & Wind section is table-driven, so the section's order and its limits have exactly
// one statement and the defaults are guaranteed reachable by their own sliders.
void RunShoreWindTableChecks() {
    WaterTabState state;
    Check(kWaterShoreWindControlCount == 7, "all seven v1 shore/wind settings are present");
    for (int controlIndex = 0; controlIndex < kWaterShoreWindControlCount; ++controlIndex) {
        const WaterShoreWindControl& control = waterShoreWindControls[controlIndex];
        Check(control.label != nullptr && control.value != nullptr, "every row names a value");
        Check(control.range.minimumValue < control.range.maximumValue, "every row has a real range");
        const float value = state.shoreWind.*control.value;
        Check(value == ClampScalarSliderValue(value, control.range),
              "and its default already sits inside its own slider");
    }
    Check(waterShoreWindControls[0].value == &WaterShoreWindSettings::windSpeed,
          "wind speed leads the section, as in v1");
    Check(waterShoreWindControls[3].range.minimumValue == -10.0f,
          "the shore depth offset keeps its signed v1 range");
}

// The blueprint is a path, so it takes the wider cap rather than the 64-character name default,
// and it settles to a trimmed value when the edit ends.
void RunWaveBlueprintChecks() {
    WaterTabState state;
    TextInputRules rules;
    rules.maximumLength = 200;
    TextInputSignal signal;
    signal.bTextEditedThisFrame = true;
    StepTextInputInteraction(state.waveGeneratorBlueprint, "  /env/waves/default.bp  ", rules, signal);
    Check(state.waveGeneratorBlueprint == "  /env/waves/default.bp  ",
          "typing keeps the spaces the user is still typing");

    signal.bTextEditedThisFrame   = false;
    signal.bEditFinishedThisFrame = true;
    const WidgetChange change = StepTextInputInteraction(state.waveGeneratorBlueprint, "", rules, signal);
    Check(change.bCommitted && state.waveGeneratorBlueprint == "/env/waves/default.bp",
          "leaving the field trims and commits once");
}

} // namespace

int main() {
    RunLevelBandChecks();
    RunDepthWindowChecks();
    RunEnableChecks();
    RunShoreWindTableChecks();
    RunWaveBlueprintChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
