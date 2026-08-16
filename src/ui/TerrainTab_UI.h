// TerrainTab_UI.h — the map-geometry tab: size, seed, height extent, world scale.
// Layer: UI. Accuracy class: Visual (it edits settings; it derives no simulated quantity).
// Edits exactly ONE slice of the recipe — `Params::Geometry` — and nothing else (ARCH §3.2:
// UI sets params and trips dirty flags, it never simulates and never reaches into PROC).
//
// THE TIER IS NOT DECIDED HERE. A committed edit calls `Pipeline::PreviewDriver::
// NotifyParametersChanged()`, which asks each stage's own ComputeParameterHash which stage (if
// any) owns the field that moved, and derives bNeedsMapUpdate vs bNeedsPreviewRender from that
// (UI_FRAMEWORK_SPEC "ideally derive it from the dependency DAG, not by hand"). No tab in this
// library contains a per-widget flag mapping, and none may.
//
// THE SPLIT (WidgetHelpers_UI.h): everything in this header is pure and headless-testable —
// the state the caller owns and the two functions that move values between the widgets and the
// recipe. The imgui composition lives in TerrainTab_UI.cpp.
#pragma once
#include "LabelledDialWidget_UI.h"
#include "../params/Geometry_PARAMS.h"

namespace SanmapGen {
namespace Params { struct MapRecipe; }
namespace Pipeline { class PreviewDriver; }
namespace Ui {

// Caller-owned tab state: one RealtimeToggle per control, the control limits (Constitution §8 —
// limits are settings, never literals at a use site), and float mirrors for the two INTEGER
// settings, because every control in the library edits a float.
struct TerrainTabState {
    DialRange mapSizeRange{ 64.0f, 4096.0f, 1.0f, 800.0f };
    DialRange seedRange{ 0.0f, 1048576.0f, 1.0f, 800.0f };
    DialRange terrainMaxHeightRange{ 1.0f, 1024.0f, 0.0f, 400.0f };
    DialRange worldUnitsPerCellRange{ 0.0625f, 32.0f, 0.0f, 400.0f };

    RealtimeToggle mapSizeToggle;
    RealtimeToggle seedToggle;
    RealtimeToggle terrainMaxHeightToggle;
    RealtimeToggle worldUnitsPerCellToggle;

    float mapSizeValue = 256.0f;
    float seedValue    = 0.0f;
};

// recipe -> widget mirrors. Run whenever no edit is pending, so a recipe loaded from disk or
// moved by another tab is picked up without the caller remembering to refresh anything.
inline void LoadTerrainTabValues(const Params::Geometry& geometry, TerrainTabState& state) {
    state.mapSizeValue = static_cast<float>(geometry.mapSize);
    state.seedValue    = static_cast<float>(geometry.seed);
}

// widget mirrors -> recipe. Rounds to nearest rather than truncating (a dial lands on 255.9999
// as readily as on 256), and reports whether the recipe actually moved.
inline bool StoreTerrainTabValues(const TerrainTabState& state, Params::Geometry& geometry) {
    const int mapSize = static_cast<int>(ClampDialValue(state.mapSizeValue, state.mapSizeRange) + 0.5f);
    const unsigned int seed =
        static_cast<unsigned int>(ClampDialValue(state.seedValue, state.seedRange) + 0.5f);
    const bool bMoved = mapSize != geometry.mapSize || seed != geometry.seed;
    geometry.mapSize = mapSize;
    geometry.seed    = seed;
    return bMoved;
}

// Draws the tab. `previewDriver` may be null (a tab drawn with no pipeline behind it still edits
// the recipe) — every call through it is guarded.
void DrawTerrainTab(Params::MapRecipe& recipe, TerrainTabState& state,
                    Pipeline::PreviewDriver* previewDriver);

} // namespace Ui
} // namespace SanmapGen
