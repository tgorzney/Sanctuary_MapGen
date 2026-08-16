// WaterTab_UI.h — the Water tab: the surface level band, the deep-water shading window, the
// preview ramp, and the shore/wind wave settings. Layer: UI. Accuracy class: Visual.
// TAB_REBUILD_PLAN "ENVIRONMENT / Water".
//
// It edits ONE recipe slice — `Params::Water` — and hosts the shared GradientEditor over the
// preview composite's water ramp (which the composite owns, not this tab). The tier of any edit
// is `Pipeline::PreviewDriver`'s derivation from the stage hashes, never a per-widget decision
// here: both range sliders go through the identical NotifyParametersChanged() call.
//
// SCOPE NOTES (ARCH §8.4 — a coder never invents a missing field; reported, not invented):
//  1. `Params::Water` carries ONE level (`waterLevelMaximum`). v1 had a level BAND
//     (WaterLevelMin/Max) and the plan asks for a RangeSlider over it, so the low handle is
//     caller-owned tab state (`waterLevelMinimum`) until a PARAMS work-order adds the field.
//  2. The seven shore/wind values and the wave-generator blueprint have no `Params::Water` home
//     either; they live on the tab state, are NOT serialized, and no stage reads them yet. Same
//     standing as HeightmapTab_UI's global gravity.
//  3. The water GRADIENT is `PreviewCompositeSettings::gradientRamps[...]` — presentation, not
//     recipe — so the caller passes the ramp in. With none bound the row says so rather than
//     editing a rival second copy of the ramp.
#pragma once
#include <string>
#include "GradientEditorWidget_UI.h"
#include "LabelledDialWidget_UI.h"
#include "RangeSliderWidget_UI.h"
#include "Section_UI.h"
#include "SliderScalar_UI.h"
#include "TextInput_UI.h"
#include "../params/Geometry_PARAMS.h"
#include "../params/Water_PARAMS.h"

namespace SanmapGen {
namespace Params { struct MapRecipe; }
namespace Pipeline { class PreviewDriver; }
namespace Ui {

// SCOPE NOTE 2 — the shore and wave settings, caller-owned until they have a PARAMS home.
struct WaterShoreWindSettings {
    float windSpeed             = 0.25f;
    float windDirection         = 160.0f;
    float shoreWavesRemap       = 0.5f;
    float shoreDepthOffset      = 8.0f;
    float shoreDepthStrength    = 0.7f;
    float shoreDistanceOffset   = 0.0f;
    float shoreDistanceStrength = 2.0f;
};

inline constexpr int kWaterShoreWindControlCount = 7;

// One row of the Shore & Wind section. The limits live in this table, never as literals at the
// draw site (Constitution §8), and the table is what the acceptance test reads.
struct WaterShoreWindControl {
    const char*                    label;
    float WaterShoreWindSettings::* value;
    ScalarSliderRange              range;
};

inline constexpr WaterShoreWindControl waterShoreWindControls[kWaterShoreWindControlCount] = {
    { "Wind Speed",              &WaterShoreWindSettings::windSpeed,             { 0.0f,   1.0f } },
    { "Wind Direction",          &WaterShoreWindSettings::windDirection,         { 0.0f, 360.0f } },
    { "Shore Waves Remap",       &WaterShoreWindSettings::shoreWavesRemap,       { 0.0f,   1.0f } },
    { "Shore Depth Offset",      &WaterShoreWindSettings::shoreDepthOffset,      {-10.0f, 10.0f } },
    { "Shore Depth Strength",    &WaterShoreWindSettings::shoreDepthStrength,    { 0.0f,   5.0f } },
    { "Shore Distance Offset",   &WaterShoreWindSettings::shoreDistanceOffset,   {-5.0f,   5.0f } },
    { "Shore Distance Strength", &WaterShoreWindSettings::shoreDistanceStrength, { 0.0f,   5.0f } },
};

struct WaterTabState {
    // Limits are settings, never literals at a use site (Constitution §8). The depth window is
    // expressed in game units, like the surface level it is measured down from.
    DialRange         waterLevelRange{ 0.0f, 1024.0f, 0.0f, 400.0f };
    // TAB_REBUILD_PLAN "ENVIRONMENT / Water" states this track as 0..50 game units; it is the
    // shading window measured DOWN from the surface, not a terrain height, so it does not track
    // the terrain band the way the level above it does.
    RangeSliderBounds deepWaterDepthBounds{ 0.0f, 50.0f, 0.01f };

    RealtimeToggle    waterLevelToggle;
    RealtimeToggle    deepWaterDepthToggle;
    RangeSliderValues deepWaterDepthValues;   // Water::deepWaterDepthMinimum/Maximum mirror
    RangeSliderValues waterLevelValues;       // (waterLevelMinimum, Water::waterLevelMaximum)
    float             waterLevelMinimum = 0.0f;   // SCOPE NOTE 1 — not serialized

    WaterShoreWindSettings shoreWind;                                    // SCOPE NOTE 2
    RealtimeToggle         shoreWindToggles[kWaterShoreWindControlCount];
    std::string            waveGeneratorBlueprint;

    SectionState        waterLevelSection;
    SectionState        shoreWindSection;
    GradientEditorState waterGradientEditor;
};

// The level band's track: the terrain height band it is measured in (TAB_REBUILD_PLAN — "terrain
// min..max"), so raising the terrain ceiling widens the water slider with it.
inline void ApplyTerrainHeightBandToWaterLevel(const Params::Geometry& geometry, WaterTabState& state) {
    state.waterLevelRange.minimumValue = geometry.terrainMinHeight;
    state.waterLevelRange.maximumValue = geometry.terrainMaxHeight;
}

// The dual-handle track derived from that band, so the dial limits and the range-slider limits
// can never disagree — there is exactly one statement of the water level's limits.
inline RangeSliderBounds WaterLevelBounds(const WaterTabState& state) {
    RangeSliderBounds bounds;
    bounds.lowerLimit        = state.waterLevelRange.minimumValue;
    bounds.upperLimit        = state.waterLevelRange.maximumValue;
    bounds.minimumSeparation = 0.0f;
    return bounds;
}

// water -> widget mirrors (the paired min/max the two range sliders edit).
inline void LoadWaterTabValues(const Params::Water& water, WaterTabState& state) {
    state.deepWaterDepthValues.minimumValue = water.deepWaterDepthMinimum;
    state.deepWaterDepthValues.maximumValue = water.deepWaterDepthMaximum;
    state.waterLevelValues.minimumValue     = state.waterLevelMinimum;
    state.waterLevelValues.maximumValue     = water.waterLevelMaximum;
}

// widget mirrors -> water, for the depth window. Reports whether the recipe actually moved.
inline bool StoreWaterTabValues(const WaterTabState& state, Params::Water& water) {
    const bool bMoved = state.deepWaterDepthValues.minimumValue != water.deepWaterDepthMinimum
                     || state.deepWaterDepthValues.maximumValue != water.deepWaterDepthMaximum;
    water.deepWaterDepthMinimum = state.deepWaterDepthValues.minimumValue;
    water.deepWaterDepthMaximum = state.deepWaterDepthValues.maximumValue;
    return bMoved;
}

// widget mirrors -> water, for the level band. Kept apart from the depth store so a stale level
// mirror can never write back over a level the user just moved on another control.
inline bool StoreWaterLevelValues(WaterTabState& state, Params::Water& water) {
    const bool bMoved = state.waterLevelValues.maximumValue != water.waterLevelMaximum
                     || state.waterLevelValues.minimumValue != state.waterLevelMinimum;
    water.waterLevelMaximum = state.waterLevelValues.maximumValue;
    state.waterLevelMinimum = state.waterLevelValues.minimumValue;
    return bMoved;
}

// `previewDriver` and `waterGradientRamp` may both be null (a tab drawn with no pipeline and no
// composite behind it still edits the recipe) — every use of either is guarded.
void DrawWaterTab(Params::MapRecipe& recipe, WaterTabState& state,
                  Pipeline::PreviewDriver* previewDriver,
                  Params::GradientRamp* waterGradientRamp = nullptr);

} // namespace Ui
} // namespace SanmapGen
