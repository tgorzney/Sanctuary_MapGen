// ApplicationShell_UI_Test.cpp — M5-7 acceptance, part 1: a freshly constructed Ui::Application
// owes a map update, and servicing it once generates the whole pipeline, composites exactly once,
// builds the marker index and binds the result to the canvas. Every call below goes through the
// SHELL's own wiring — its default recipe, its injected composite callback, its canvas binding —
// so what passes is the assembly, not a replica of it. Cpu twin throughout: no window, no GL.
#include "ApplicationShell_TestSupport_UI.h"
#include <cstdio>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

void RunFirstGenerationChecks(Application& application) {
    Check(application.Driver().NeedsMapUpdate(), "a fresh shell owes a map update");
    Check(!application.Driver().NeedsPreviewRender(), "and owes no preview render");
    Check(application.Assembler().SpatialGridBuildCount() == 0, "nothing is built before the first service");

    Check(application.ServiceDirtyTier() == Pipeline::RefreshTier::MapUpdate,
          "the first serviced tier generates the map");
    Check(application.Composite().Resolution() == shellTestPreviewResolution,
          "the shell's composite ran at the configured resolution");
    Check(application.Driver().StagesThatRan().size() == 7, "the first service ran every stage");
    Check(application.Driver().PreviewCompositeCount() == 1, "a map update composites exactly once");
    Check(application.Assembler().SpatialGridBuildCount() == 1,
          "PIPELINE built the marker index inside the Placement stage");
    Check(application.ServiceDirtyTier() == Pipeline::RefreshTier::Nothing,
          "an idle service does nothing");
    Check(application.Driver().PreviewCompositeCount() == 1, "and does not recomposite");
}

// The shell's default recipe must produce terrain a user can SEE — a full image with real relief,
// not a flat or empty one. This is the milestone's "generating from a default MapRecipe produces
// visible terrain", asserted rather than eyeballed.
void RunVisibleTerrainChecks(Application& application) {
    const std::size_t expectedTexelCount = static_cast<std::size_t>(shellTestPreviewResolution)
                                         * shellTestPreviewResolution;
    Check(application.Composite().CompositeTexels().size() == expectedTexelCount,
          "the composite produced a full preview image");

    const Data::FloatField& heightfield = application.Assembler().Fields().heightfield;
    float lowestHeight = 1.0f, highestHeight = 0.0f;
    for (std::size_t cell = 0; cell < heightfield.CellCount(); ++cell) {
        const float height = heightfield.Data()[cell];
        if (height < lowestHeight)  lowestHeight = height;
        if (height > highestHeight) highestHeight = height;
    }
    Check(highestHeight - lowestHeight > 0.05f, "the default recipe generated real relief");

    unsigned int firstTexel = 0;
    bool bImageHasContrast = false;
    if (!application.Composite().CompositeTexels().empty())
        firstTexel = application.Composite().CompositeTexels()[0];
    for (unsigned int texel : application.Composite().CompositeTexels())
        if (texel != firstTexel) { bImageHasContrast = true; break; }
    Check(bImageHasContrast, "and the preview image shows it, rather than one flat colour");
    Check(application.Assembler().Placements().markers.Count() > 0,
          "the default recipe placed markers for the canvas to select");
}

// The canvas is bound by the shell, not by the test: the id buffer at construction, the preview
// resolution on every serviced tier. Without both, a click could not resolve anything.
void RunCanvasBindingChecks(Application& application) {
    Check(application.Canvas().View().PreviewResolution() == shellTestPreviewResolution,
          "the shell pointed the canvas at the composite's resolution");
    Check(application.EntityIdentifiers().Width() == shellTestPreviewResolution,
          "the composite sized the entity-id buffer the canvas picks against");
}

} // namespace

int main() {
    Application application;
    PrepareShellForTest(application);

    RunFirstGenerationChecks(application);
    RunVisibleTerrainChecks(application);
    RunCanvasBindingChecks(application);
    RunShellDirtyTierChecks(application);
    RunShellIconBridgeChecks();

    if (previewTestFailureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", previewTestFailureCount);
    return 1;
}
