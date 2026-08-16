// HeightmapTab_UI.cpp — the imgui composition of the Heightmap tab. Layer: UI.
// Two sections: "Map" (seed, size, the height band, global gravity) and "GeoLayers", which hosts
// the shared Layer Editor. Every control is a batch-A shared widget; the only raw imgui here is
// the label vocabulary.
#include "HeightmapTab_UI.h"
#include "Combo_UI.h"
#include "LayerEditor_Erosion_UI.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../pipeline/PreviewDriver_PIPELINE.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

void NotifyChange(bool bCommitted, Pipeline::PreviewDriver* previewDriver) {
    if (bCommitted && previewDriver != nullptr) previewDriver->NotifyParametersChanged();
}

// The seed and the map size are INTEGER settings edited through mirrors; the two heights are
// floats the geometry stores directly.
void DrawMapSettings(Params::Geometry& geometry, HeightmapTabState& state,
                     Pipeline::PreviewDriver* previewDriver) {
    if (!state.seedToggle.IsCommitDeferred()) LoadHeightmapTabValues(geometry, state);

    WidgetChange change = DrawSliderScalarInteger("Seed", state.seedValue, state.seedRange,
                                                  state.seedToggle, WidgetStyle(), "%d");
    if (change.bValueChanged) StoreHeightmapTabValues(state, geometry);
    NotifyChange(change.bCommitted, previewDriver);

    ComboOptions mapSizeOptions;
    mapSizeOptions.labels     = heightmapMapSizeLabels;
    mapSizeOptions.count      = kHeightmapMapSizeCount;
    mapSizeOptions.emptyLabel = "<custom>";
    change = DrawCombo("Map Size", state.mapSizeIndex, mapSizeOptions);
    if (change.bValueChanged) StoreHeightmapTabValues(state, geometry);
    NotifyChange(change.bCommitted, previewDriver);

    change = DrawSliderScalar("Terrain Max Height", geometry.terrainMaxHeight,
                              state.terrainMaxHeightRange, state.terrainMaxHeightToggle,
                              WidgetStyle(), "%.1f");
    if (change.bValueChanged) ClampTerrainHeightBand(geometry);
    NotifyChange(change.bCommitted, previewDriver);

    change = DrawSliderScalar("Terrain Min Height", geometry.terrainMinHeight,
                              state.terrainMinHeightRange, state.terrainMinHeightToggle,
                              WidgetStyle(), "%.1f");
    if (change.bValueChanged) ClampTerrainHeightBand(geometry);
    NotifyChange(change.bCommitted, previewDriver);
}

// Global Gravity: a bulk write onto every stratum's erosion gravity (HeightmapTab_UI.h SCOPE
// NOTE 2). With no pipeline bound there is nothing to write, so the row says so rather than
// pretending to hold a value.
void DrawGlobalGravity(HeightmapTabState& state, Pipeline::GenerationAssembler* generationAssembler,
                       Pipeline::PreviewDriver* previewDriver) {
    const WidgetChange change = DrawSliderScalar("Global Gravity", state.globalGravity,
                                                 state.globalGravityRange, state.globalGravityToggle,
                                                 WidgetStyle(), "%.2f");
    if (generationAssembler == nullptr) {
        ImGui::TextUnformatted("No pipeline bound - gravity is not applied.");
        return;
    }
    if (!change.bValueChanged) return;
    const bool bMoved = ApplyGlobalGravityToErosion(state.globalGravity, *generationAssembler);
    NotifyChange(bMoved && change.bCommitted, previewDriver);
}

} // namespace

void DrawHeightmapTab(Params::MapRecipe& recipe, HeightmapTabState& state,
                      Pipeline::GenerationAssembler* generationAssembler,
                      Pipeline::PreviewDriver* previewDriver) {
    ImGui::PushID("heightmapTab");
    if (DrawSectionBegin("Map", state.mapSection)) {
        DrawMapSettings(recipe.geometry, state, previewDriver);
        DrawGlobalGravity(state, generationAssembler, previewDriver);
        DrawSectionEnd();
    }
    if (DrawSectionBegin("GeoLayers", state.geoLayerSection)) {
        DrawLayerEditor(recipe.layerStack, state.layerEditor, generationAssembler, previewDriver);
        DrawSectionEnd();
    }
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
