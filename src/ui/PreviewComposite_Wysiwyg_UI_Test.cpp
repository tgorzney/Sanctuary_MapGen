// PreviewComposite_Wysiwyg_UI_Test.cpp — acceptance test, part 2: the composite samples the
// BAKE and nothing else. Changing a sim input without re-baking must not move a single pixel,
// while changing a baked field must (ARCH §3.2, hit-list #4 — the shadow-sim deletion).
//   cl /std:c++17 /EHsc PreviewComposite_UI.cpp PreviewComposite_Prepare_UI.cpp
//      PreviewComposite_Cpu_UI.cpp GradientLut_UI.cpp PreviewComposite_Wysiwyg_UI_Test.cpp
// Cpu twin only, so no GL context is needed.
#include "PreviewComposite_TestScene_UI.h"

using namespace SanmapGen;

namespace {

void check(bool bCondition, const char* label) { Ui::CheckPreviewExpectation(bCondition, label); }

std::vector<std::uint32_t> CopyIdentifiers(const Data::EntityIdBuffer& buffer) {
    return std::vector<std::uint32_t>(buffer.Data(), buffer.Data() + buffer.CellCount());
}

// Every mutation below is an input to a PROC stage — the gate, the merge, the remap, the seed,
// and the sim-owned proportion field itself. The bake was NOT re-run, so a preview that samples
// the bake cannot react to any of them. A preview that re-derived slope, re-filtered rules or
// re-ran a sim would.
void MutateSimulationInputsWithoutRebaking(Ui::PreviewTestScene& scene) {
    scene.geometry.seed = 12345u;
    scene.strata[0].bSlopeGateEnabled = true;
    scene.strata[0].minimumSlopeDegrees = 45.0f;
    scene.strata[0].maximumSlopeDegrees = 50.0f;
    scene.strata[0].slopeGateStrength = 0.25f;
    scene.strata[0].bInvertSlopeGate = true;
    scene.strata[0].importedMaskMode = Params::ImportedMaskMode::StaticOverride;
    for (int channel = 0; channel < Params::kStratumColorChannelCount; ++channel) {
        scene.strata[0].maskRemapMinimum[channel] = 0.3f;  // the ONE remap lives in Mask (§7.2.5)
        scene.strata[0].maskRemapMaximum[channel] = 0.6f;
    }
    scene.fields.materialProportions[0].Fill(0.0f);        // physical, sim-owned: not a preview input
}

void TestSimulationInputsDoNotMoveThePreview() {
    Ui::PreviewTestScene scene;
    Ui::BuildPreviewTestScene(scene);
    Ui::PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.areas, scene.fields,
                                   scene.instances, scene.entityIdentifiers);
    Ui::ConfigurePreviewSettings(composite.Settings());
    composite.Compose();
    const std::vector<unsigned int> bakedImage = composite.CompositeTexels();
    const std::vector<std::uint32_t> bakedIdentifiers = CopyIdentifiers(scene.entityIdentifiers);

    MutateSimulationInputsWithoutRebaking(scene);
    composite.Compose();
    check(composite.CompositeTexels() == bakedImage,
          "changing sim inputs without re-baking does NOT change the composite");
    check(CopyIdentifiers(scene.entityIdentifiers) == bakedIdentifiers,
          "no marker is re-filtered by the preview: the entity ids are unchanged too");
}

// The other half of the same claim: the composite must track the baked fields exactly.
void TestBakedFieldsDoMoveThePreview() {
    Ui::PreviewTestScene scene;
    Ui::BuildPreviewTestScene(scene);
    Ui::PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.areas, scene.fields,
                                   scene.instances, scene.entityIdentifiers);
    Ui::ConfigurePreviewSettings(composite.Settings());
    composite.Compose();
    const std::vector<unsigned int> bakedImage = composite.CompositeTexels();

    scene.fields.surfaceStratumWeights[0].Fill(0.9f);      // Mask's output: the VISIBLE weight
    composite.Compose();
    const std::vector<unsigned int> afterWeights = composite.CompositeTexels();
    check(afterWeights != bakedImage, "a re-baked surface weight changes the composite");

    scene.fields.heightfield.Fill(0.75f);
    composite.Compose();
    check(composite.CompositeTexels() != afterWeights, "a re-baked heightfield changes the composite");

    // Point the flow layer at the varying ramp so its own domain mapping is under test.
    composite.Settings().fieldLayers[2].gradientRampIndex = 0;
    composite.Compose();
    const std::vector<unsigned int> beforeFlow = composite.CompositeTexels();
    scene.fields.flow.Fill(0.0f);                          // the flow layer's domain is 0..2
    composite.Compose();
    check(composite.CompositeTexels() != beforeFlow, "a re-baked flow field changes the composite");
}

// Water colorizes the BAKED height against the settings water level; no water simulation exists
// here, and none is wanted.
void TestWaterReadsTheBakedHeight() {
    Ui::PreviewTestScene scene;
    Ui::BuildPreviewTestScene(scene);
    Ui::PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.areas, scene.fields,
                                   scene.instances, scene.entityIdentifiers);
    Ui::ConfigurePreviewSettings(composite.Settings());
    composite.Settings().gradientRamps.push_back(Ui::MakeConstantRamp(0.0f, 0.2f, 1.0f, 1.0f));
    composite.Settings().fieldLayers.push_back(
        Ui::MakeLayer(Ui::PreviewLayerKind::Water, Ui::PreviewBlendMode::AlphaBlend, 2, 0.0f, 1.0f));
    composite.Compose();
    const std::vector<unsigned int> dryImage = composite.CompositeTexels();

    scene.water.bEnabled = true;                           // terrain at 0.25 * 100 = 25 game units
    scene.water.waterLevelMaximum = 50.0f;
    scene.water.deepWaterDepthMaximum = 40.0f;
    composite.Compose();
    check(composite.CompositeTexels() != dryImage, "water below the surface level colorizes");

    scene.fields.heightfield.Fill(0.9f);                   // 90 game units: above the water line
    composite.Compose();
    const std::vector<unsigned int> raisedImage = composite.CompositeTexels();
    scene.water.bEnabled = false;
    composite.Compose();
    check(composite.CompositeTexels() == raisedImage,
          "terrain baked above the water level takes no water color at all");
}

// The composite draws the RESOLVED instances; it never decides which ones qualify.
void TestEntitiesAreDrawnNotFiltered() {
    Ui::PreviewTestScene scene;
    Ui::BuildPreviewTestScene(scene);
    Ui::PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.areas, scene.fields,
                                   scene.instances, scene.entityIdentifiers);
    Ui::ConfigurePreviewSettings(composite.Settings());
    composite.Compose();
    check(scene.entityIdentifiers.Get(1, 1) == 0u, "the resolved instance is drawn");

    Data::PlacementInstance second;                        // a second resolved instance, top-left
    second.positionX = 0.0f;
    second.positionZ = 0.0f;
    scene.instances.Append(second);
    composite.Compose();
    check(scene.entityIdentifiers.Get(0, 0) == 1u, "a newly resolved instance is drawn with its index");
    check(scene.entityIdentifiers.Get(1, 1) == 0u, "the first instance keeps its own index");
    check(scene.entityIdentifiers.Get(3, 3) == Data::EntityIdBuffer::emptySentinel,
          "uncovered pixels stay empty");
}

} // namespace

int main() {
    TestSimulationInputsDoNotMoveThePreview();
    TestBakedFieldsDoMoveThePreview();
    TestWaterReadsTheBakedHeight();
    TestEntitiesAreDrawnNotFiltered();
    if (Ui::previewTestFailureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", Ui::previewTestFailureCount);
    return 1;
}
