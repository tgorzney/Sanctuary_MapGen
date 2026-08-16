// TerrainTab_UI.cpp — the imgui composition of the geometry tab. Layer: UI.
// Composes shared library controls ONLY (UI_FRAMEWORK_SPEC "Universal widget library": new tabs
// compose these; they do not hand-roll imgui) — there is no ImGui::SliderFloat/DragFloat/VSlider
// anywhere in this file, and there is none in any other *Tab_UI.cpp either.
#include "TerrainTab_UI.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../pipeline/PreviewDriver_PIPELINE.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// The ONE thing a tab does with a commit. Which tier it turns into is the driver's derivation
// from the stage parameter hashes, never this call site's decision.
void NotifyOnCommit(const WidgetChange& change, Pipeline::PreviewDriver* previewDriver) {
    if (change.bCommitted && previewDriver != nullptr) previewDriver->NotifyParametersChanged();
}

} // namespace

void DrawTerrainTab(Params::MapRecipe& recipe, TerrainTabState& state,
                    Pipeline::PreviewDriver* previewDriver) {
    Params::Geometry& geometry = recipe.geometry;
    ImGui::PushID("terrainTab");

    // The integer mirrors follow the recipe while nothing is mid-edit.
    if (!state.mapSizeToggle.IsCommitDeferred() && !state.seedToggle.IsCommitDeferred())
        LoadTerrainTabValues(geometry, state);

    WidgetChange change = DrawLabelledDial("Map Size (cells)", state.mapSizeValue,
                                           state.mapSizeRange, state.mapSizeToggle,
                                           WidgetStyle(), "%.0f");
    if (change.bValueChanged) StoreTerrainTabValues(state, geometry);
    NotifyOnCommit(change, previewDriver);

    change = DrawLabelledDial("Seed", state.seedValue, state.seedRange, state.seedToggle,
                              WidgetStyle(), "%.0f");
    if (change.bValueChanged) StoreTerrainTabValues(state, geometry);
    NotifyOnCommit(change, previewDriver);

    // These two are floats in the recipe, so the dial edits them in place — no mirror.
    change = DrawLabelledDial("Terrain Max Height (game units)", geometry.terrainMaxHeight,
                              state.terrainMaxHeightRange, state.terrainMaxHeightToggle,
                              WidgetStyle(), "%.1f");
    NotifyOnCommit(change, previewDriver);

    change = DrawLabelledDial("World Units Per Cell", geometry.worldUnitsPerCell,
                              state.worldUnitsPerCellRange, state.worldUnitsPerCellToggle,
                              WidgetStyle(), "%.4f");
    NotifyOnCommit(change, previewDriver);

    ImGui::Separator();
    ImGui::Text("Heightfield %d x %d vertices | extent %.1f game units",
                geometry.VertexSize(), geometry.VertexSize(),
                static_cast<float>(geometry.mapSize) * geometry.worldUnitsPerCell);
    if (!geometry.IsValid()) ImGui::TextUnformatted("Geometry is invalid: nothing will generate.");
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
