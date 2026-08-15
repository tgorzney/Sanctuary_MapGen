// PreviewIntegration_TestScene_UI.h — test-only scaffolding for the M4-5 end-to-end acceptance
// test: one real GenerationAssembler (the seven PROC stages on the M3-8 recipe), one real
// PreviewComposite over the fields that pipeline bakes, and the PreviewDriver that decides which
// of the two runs. Not part of the layer graph; nothing outside a *_Test.cpp includes it.
// It stands in for the M5 UI caller — binding the composite to the driver's callback is the UI's
// job, which is why the acceptance test lives in `ui/` and `pipeline/` stays free of _UI headers.
#pragma once
#include "PreviewComposite_UI.h"
#include "PreviewComposite_TestScene_UI.h"          // CheckPreviewExpectation, MakeLayer, ramps
#include "../pipeline/GenerationAssembler_TestScene_PIPELINE.h"
#include "../pipeline/PreviewDriver_PIPELINE.h"

namespace SanmapGen {
namespace Ui {

constexpr int previewIntegrationResolution = 64;   // == AssemblerTest::mapSize: one pixel per cell
constexpr int heightRampIndex              = 0;

struct PreviewIntegrationScene {
    Params::MapRecipe             recipe;
    Pipeline::GenerationAssembler assembler;
    Data::EntityIdBuffer          entityIdentifiers;
    PreviewComposite              composite;
    Pipeline::PreviewDriver       driver;

    PreviewIntegrationScene()
        : recipe(AssemblerTest::MakeRecipe(4242u)),
          assembler(recipe),
          composite(recipe.geometry, recipe.water, recipe.strata, assembler.Fields(),
                    assembler.Placements().markers, entityIdentifiers),
          driver(assembler) {
        AssemblerTest::ConfigureStages(assembler);
        ConfigureCompositeSettings();
        driver.SetPreviewCompositeCallback([this] { composite.Compose(); });
    }

    // ONE colorized layer (height through a black->white ramp, Replace) so every pixel the marks
    // do not cover is exactly the ramp applied to the baked height — that is what makes
    // "the preview matches the bake" an exact assertion rather than an eyeball.
    // worldUnitsPerCell comes from PIPELINE: it is the recipe's map geometry (M5-0a), the same
    // value Placement emitted its positions with.
    void ConfigureCompositeSettings() {
        PreviewCompositeSettings& settings = composite.Settings();
        settings.previewResolution = previewIntegrationResolution;
        settings.worldUnitsPerCell = assembler.WorldUnitsPerCell();
        settings.gradientRamps.push_back(MakeBlackToWhiteRamp());
        settings.fieldLayers.push_back(MakeLayer(PreviewLayerKind::HeightRamp,
                                                 PreviewBlendMode::Replace, heightRampIndex,
                                                 0.0f, 1.0f));
        settings.entityMarkRadiusPixels = 1.5f;
    }
};

// Preview pixels per heightfield cell — the exact factor PreviewComposite::BuildEntityPoints uses.
inline float PreviewPixelsPerCell(const PreviewIntegrationScene& scene) {
    return static_cast<float>(scene.composite.Resolution())
         / static_cast<float>(scene.assembler.Fields().VertexSize() - 1);
}

// Where one resolved instance lands in the image, rounded to the pixel its mark centres on.
inline void MarkerPixel(const PreviewIntegrationScene& scene, std::size_t markerIndex,
                        int& pixelX, int& pixelY) {
    const Data::PlacementInstances& markers = scene.assembler.Placements().markers;
    const float cellsPerWorldUnit = ReciprocalOrZero(scene.composite.Settings().worldUnitsPerCell);
    const float pixelsPerCell = PreviewPixelsPerCell(scene);
    pixelX = static_cast<int>(markers.positionX[markerIndex] * cellsPerWorldUnit * pixelsPerCell);
    pixelY = static_cast<int>(markers.positionZ[markerIndex] * cellsPerWorldUnit * pixelsPerCell);
}

inline unsigned long long CompositeChecksum(const std::vector<unsigned int>& texels) {
    unsigned long long checksum = 1469598103934665603ull;
    for (unsigned int texel : texels) checksum = (checksum ^ texel) * 1099511628211ull;
    return checksum;
}

} // namespace Ui
} // namespace SanmapGen
