// ApplicationShell_DirtyTier_UI_Test.cpp — M5-7 acceptance, part 2: the three interactions the
// milestone names, driven through the SHELL. Adjusting a layer regenerates and the view updates; a
// gradient tweak recolors with NO regeneration; a click on a marker selects that entity and the
// shell's own selection callback observes it.
// Each edit uses the same call the drawn tab makes — the tab's Store* function, then
// PreviewDriver::NotifyParametersChanged() — so the tier is derived from the stages' parameter
// hashes and nothing here declares one.
#include "ApplicationShell_TestSupport_UI.h"

namespace SanmapGen {
namespace Ui {
namespace {

// A layer control, driven through the tab the rebuilt shell actually mounts: the Heightmap panel
// hosts the batch-B Layer Editor, so the edit goes through THAT editor's mirrors. NoiseBlend hashes
// the layer's levels, so the whole pipeline re-runs and the image must move — "adjusting a layer
// regenerates and updates the view".
void RunLayerRegenerationChecks(Application& application) {
    const unsigned long long settledImage = CompositeImageChecksum(application);
    const int settledPipelineRuns = application.Driver().PipelineRunCount();
    LayerEditorState& editorState = application.TabState().heightmap.layerEditor;
    Params::Layer* const layer =
        SelectedLayerEditorLayer(application.Recipe().layerStack, editorState);
    Check(layer != nullptr, "the default recipe has a layer to adjust");
    if (layer == nullptr) return;

    LoadLayerEditorValues(*layer, editorState);
    editorState.levelsValues.inputShadows = editorState.levelsValues.inputShadows + 0.25f;
    Check(StoreLayerEditorValues(editorState, *layer), "the layer moved");

    Check(application.Driver().NotifyParametersChanged() == Pipeline::RefreshTier::MapUpdate,
          "a layer control is stage-owned, so it trips bNeedsMapUpdate");
    Check(application.Driver().OwningStageName() == "NoiseBlend", "the noise stage claims it");
    Check(application.ServiceDirtyTier() == Pipeline::RefreshTier::MapUpdate, "the shell regenerates");
    Check(application.Driver().PipelineRunCount() == settledPipelineRuns + 1, "exactly one run");
    Check(application.Driver().StagesThatRan().size() == 7, "a layer change re-runs everything");
    Check(CompositeImageChecksum(application) != settledImage, "and the view updated");
}

// A ramp lives in the composite's PRESENTATION settings, which no stage's hash can see, so the
// derivation must answer "recolor only" — the milestone's "a gradient tweak recolors without a
// full regen". The ramp editor's own pure mutator is the exact call the Preview panel makes.
void RunGradientRecolorChecks(Application& application) {
    const unsigned long long settledImage = CompositeImageChecksum(application);
    const int settledPipelineRuns = application.Driver().PipelineRunCount();
    const int settledGridBuilds   = application.Assembler().SpatialGridBuildCount();
    const int settledComposites   = application.Driver().PreviewCompositeCount();

    std::vector<Params::GradientRamp>& ramps = application.Composite().Settings().gradientRamps;
    Check(!ramps.empty(), "the shell configured at least one preview ramp");
    if (ramps.empty()) return;
    bool bRampMoved = false;
    for (int stopIndex = 0; stopIndex < static_cast<int>(ramps[0].stops.size()); ++stopIndex) {
        const float recoloredStop[kGradientStopChannelCount] = { 1.0f, 0.0f, 1.0f, 1.0f };
        bRampMoved = RecolorGradientStop(ramps[0], stopIndex, recoloredStop) || bRampMoved;
    }
    Check(bRampMoved, "the height ramp was recolored");

    Check(application.Driver().NotifyParametersChanged() == Pipeline::RefreshTier::PreviewRender,
          "a ramp moves no stage parameter, so it trips only bNeedsPreviewRender");
    Check(application.Driver().OwningStageName().empty(), "no stage claims a preview ramp");
    Check(application.ServiceDirtyTier() == Pipeline::RefreshTier::PreviewRender, "the shell recolors");
    Check(application.Driver().PipelineRunCount() == settledPipelineRuns, "with no pipeline run");
    Check(application.Driver().StagesThatRan().empty(), "and no stage ran");
    Check(application.Assembler().SpatialGridBuildCount() == settledGridBuilds,
          "so the marker index cannot have moved");
    Check(application.Driver().PreviewCompositeCount() == settledComposites + 1, "exactly one composite");
    Check(CompositeImageChecksum(application) != settledImage, "and the recolor reached the image");
}

// A click on a marker: the canvas resolves it against the id buffer the composite wrote, and the
// selection callback the SHELL injected records it. One buffer read, never a scan of the
// population (UI_FRAMEWORK_SPEC §4).
void RunCanvasSelectionChecks(Application& application) {
    const Data::PlacementInstances& markers = application.Assembler().Placements().markers;
    Check(markers.Count() > 0, "there is a marker to click");
    if (markers.Count() == 0) return;

    bool bSomeMarkerSelected = false;
    for (std::size_t markerIndex = 0; markerIndex < markers.Count() && !bSomeMarkerSelected; ++markerIndex) {
        float cursorX = 0.0f, cursorY = 0.0f;
        MarkerCursorPosition(application, markerIndex, cursorX, cursorY);
        const std::uint32_t selected = application.Canvas().ApplyClick(cursorX, cursorY);
        bSomeMarkerSelected = selected != Data::EntityIdBuffer::emptySentinel
                           && selected == static_cast<std::uint32_t>(markerIndex);
    }
    Check(bSomeMarkerSelected, "clicking a marker selects that marker");
    Check(application.Canvas().HasSelection(), "the canvas holds the selection");
    Check(application.LastSelectedEntityIdentifier() == application.Canvas().SelectedEntityIdentifier(),
          "and the shell's injected selection callback observed the same identifier");

    application.Canvas().ApplyClick(-8.0f, -8.0f);
    Check(!application.Canvas().HasSelection(), "a click outside the image selects nothing");
}

} // namespace

void RunShellDirtyTierChecks(Application& application) {
    RunLayerRegenerationChecks(application);
    RunGradientRecolorChecks(application);
    RunCanvasSelectionChecks(application);
}

} // namespace Ui
} // namespace SanmapGen
