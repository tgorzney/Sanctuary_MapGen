// HeightmapTab_UI.h — the Heightmap tab: seed, map size, the terrain height band, global gravity,
// and the GeoLayer stack. Layer: UI. Accuracy class: Visual (it edits settings; it simulates
// nothing). TAB_REBUILD_PLAN "2 · Heightmap".
//
// It edits ONE recipe slice — `Params::Geometry` — and hosts the shared Layer Editor for the
// stack (`Params::LayerStack`). The tier of any edit is `Pipeline::PreviewDriver`'s derivation
// from the stage hashes, never a per-widget decision here.
//
// SCOPE NOTES (ARCH §8.4 — a coder never invents a missing field; reported, not invented):
//  1. "Scale Features to Map Size" has no settings home: no `Params::Geometry` flag and no layer
//     field scales frequency by map size in the v2 tree. The checkbox is NOT drawn — a control
//     bound to nothing is worse than a missing one.
//  2. GLOBAL GRAVITY is not a `Params::Geometry` field either. Gravity is per-stratum
//     (`Proc::ErosionLayerSettings::gravity`, reached through PIPELINE), and no
//     `bUseGlobalGravity` opt-in flag exists, so the tab's slider is caller-owned UI state that
//     BULK-WRITES that one field on every stratum (`ApplyGlobalGravityToErosion`). It is not a
//     rival second store — the per-layer slider in the Layer Editor edits the very same field —
//     and it is not serialized. A durable global-gravity setting needs its own work-order.
//     (Same standing as SystemTab_UI's asset-cache directory.)
//  3. `terrainMinHeight` IS now a real setting (Geometry_PARAMS.h, promoted by this work-order),
//     but no generation stage consumes it yet; see the note on the field itself.
#pragma once
#include "LayerEditor_UI.h"
#include "SliderScalar_UI.h"
#include "../params/Geometry_PARAMS.h"

namespace SanmapGen {
namespace Params { struct MapRecipe; }
namespace Pipeline { class GenerationAssembler; class PreviewDriver; }
namespace Ui {

// The map sizes the tab offers, and their labels in the same order (Combo_UI maps row -> value).
inline constexpr int kHeightmapMapSizeCount = 5;
inline constexpr int heightmapMapSizeValues[kHeightmapMapSizeCount] = { 256, 512, 1024, 2048, 4096 };
inline const char* const heightmapMapSizeLabels[kHeightmapMapSizeCount] = {
    "256", "512", "1024", "2048", "4096"
};

// The offered size a recipe currently sits on, or -1 for a size the dropdown does not list (a
// recipe hand-edited to 768 keeps its value and simply shows nothing picked — Constitution §6).
inline int HeightmapMapSizeIndexOf(int mapSize) {
    for (int sizeIndex = 0; sizeIndex < kHeightmapMapSizeCount; ++sizeIndex)
        if (heightmapMapSizeValues[sizeIndex] == mapSize) return sizeIndex;
    return -1;
}

inline int HeightmapMapSizeAtIndex(int sizeIndex) {
    if (sizeIndex < 0 || sizeIndex >= kHeightmapMapSizeCount) return 0;
    return heightmapMapSizeValues[sizeIndex];
}

// Caller-owned tab state: the limits (Constitution §8), one RealtimeToggle per control, and the
// integer mirrors the float-valued controls need.
struct HeightmapTabState {
    ScalarSliderRange seedRange{ 0.0f, 1048576.0f, 1.0f };
    ScalarSliderRange terrainMaxHeightRange{ 1.0f, 4096.0f, 0.0f };
    ScalarSliderRange terrainMinHeightRange{ -1024.0f, 4095.0f, 0.0f };
    ScalarSliderRange globalGravityRange{ 1.0f, 20.0f, 0.0f };

    RealtimeToggle seedToggle;
    RealtimeToggle terrainMaxHeightToggle;
    RealtimeToggle terrainMinHeightToggle;
    RealtimeToggle globalGravityToggle;

    SectionState mapSection;
    SectionState geoLayerSection;

    int   seedValue     = 0;       // int mirror of the unsigned seed
    int   mapSizeIndex  = 0;
    float globalGravity = 4.0f;    // SCOPE NOTE 2 — not serialized; bulk-written to the strata

    LayerEditorState layerEditor;
};

// Forces the height band legal: the floor stays below the ceiling by at least one game unit, so
// `Geometry::IsValid()` can never be false because of a slider.
inline void ClampTerrainHeightBand(Params::Geometry& geometry) {
    if (geometry.terrainMaxHeight < 1.0f) geometry.terrainMaxHeight = 1.0f;
    if (geometry.terrainMinHeight > geometry.terrainMaxHeight - 1.0f)
        geometry.terrainMinHeight = geometry.terrainMaxHeight - 1.0f;
}

// recipe -> widget mirrors. Run whenever no edit is pending.
inline void LoadHeightmapTabValues(const Params::Geometry& geometry, HeightmapTabState& state) {
    state.seedValue    = static_cast<int>(geometry.seed);
    state.mapSizeIndex = HeightmapMapSizeIndexOf(geometry.mapSize);
}

// widget mirrors -> recipe. Reports whether the recipe actually moved.
inline bool StoreHeightmapTabValues(const HeightmapTabState& state, Params::Geometry& geometry) {
    const int seedValue = state.seedValue > 0 ? state.seedValue : 0;
    const unsigned int seed = static_cast<unsigned int>(seedValue);
    const int mapSize = HeightmapMapSizeAtIndex(state.mapSizeIndex);
    const bool bMoved = seed != geometry.seed || (mapSize > 0 && mapSize != geometry.mapSize);
    geometry.seed = seed;
    if (mapSize > 0) geometry.mapSize = mapSize;
    ClampTerrainHeightBand(geometry);
    return bMoved;
}

void DrawHeightmapTab(Params::MapRecipe& recipe, HeightmapTabState& state,
                      Pipeline::GenerationAssembler* generationAssembler,
                      Pipeline::PreviewDriver* previewDriver);

} // namespace Ui
} // namespace SanmapGen
