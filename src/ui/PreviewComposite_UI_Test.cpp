// PreviewComposite_UI_Test.cpp — acceptance test, part 1: the composited colors of a spot cell,
// the entity-id buffer, the pass ordering, the resolution tweakable and the slope layer (M5-0c).
//   cl /std:c++17 /EHsc PreviewComposite_UI.cpp PreviewComposite_Prepare_UI.cpp
//      PreviewComposite_Cpu_UI.cpp GradientLut_UI.cpp PreviewComposite_UI_Test.cpp
// Runs the Cpu twin, so it needs no GL context; the Gpu twin's parity against these same numbers
// is PreviewComposite_Gpu_UI_Test.cpp, which does need one.
#include "PreviewComposite_TestScene_UI.h"
#include <cstring>

using namespace SanmapGen;

namespace {

using Ui::ChannelByte;
using Ui::ChannelNear;
using Ui::QuantizeChannel;
void check(bool bCondition, const char* label) { Ui::CheckPreviewExpectation(bCondition, label); }

// The expected spot-cell color, re-derived from the scene's numbers alone: the height ramp at
// 0.25, a half-weight splat of the stratum tint, then the constant flow ramp added at its own
// 0.4 alpha — per layer, including the byte round-trip between passes.
float ExpectedSpotChannel(float stratumTint, float flowSource) {
    const float heightGrey = QuantizeChannel(0.25f);
    const float splatted = QuantizeChannel(heightGrey + (stratumTint - heightGrey) * 0.5f);
    return QuantizeChannel(splatted + ((splatted + flowSource) - splatted) * 0.4f);
}

void TestSpotCellColors() {
    Ui::PreviewTestScene scene;
    Ui::BuildPreviewTestScene(scene);
    Ui::PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.fields,
                                   scene.instances, scene.entityIdentifiers);
    Ui::ConfigurePreviewSettings(composite.Settings());
    composite.Compose();

    check(!composite.LastRunUsedGpu(), "no resource manager -> the Cpu twin ran");
    check(composite.Resolution() == 4 && composite.CompositeTexels().size() == 16u,
          "the composite is previewResolution squared");
    // Pixel (0,0) is outside the entity mark, so it is pure field colorization.
    const unsigned int spotTexel = composite.CompositeTexels()[0];
    check(ChannelNear(spotTexel, 0, ExpectedSpotChannel(0.8f, 0.0f)), "spot cell red channel");
    check(ChannelNear(spotTexel, 1, ExpectedSpotChannel(0.2f, 0.0f)), "spot cell green channel");
    check(ChannelNear(spotTexel, 2, ExpectedSpotChannel(0.1f, 1.0f)), "spot cell blue channel");
    check(ChannelByte(spotTexel, 3) == 255, "the composite image stays opaque");
    check(composite.CompositeTexels()[3] == spotTexel && composite.CompositeTexels()[12] == spotTexel,
          "uniform baked fields composite uniformly");   // every unmarked pixel matches the spot
}

// The 4x4 preview maps world (2, 2) to pixel centre (1.5, 1.5); a 0.9-pixel mark covers four.
void TestEntityIdentifierBuffer() {
    Ui::PreviewTestScene scene;
    Ui::BuildPreviewTestScene(scene);
    Ui::PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.fields,
                                   scene.instances, scene.entityIdentifiers);
    Ui::ConfigurePreviewSettings(composite.Settings());
    composite.Compose();
    check(scene.entityIdentifiers.Width() == 4 && scene.entityIdentifiers.Height() == 4,
          "the entity-id buffer is resized to the preview");
    for (int pixelY = 0; pixelY < 4; ++pixelY)
        for (int pixelX = 0; pixelX < 4; ++pixelX) {
            const bool bCovered = pixelX >= 1 && pixelX <= 2 && pixelY >= 1 && pixelY <= 2;
            const std::uint32_t identifier = scene.entityIdentifiers.Get(pixelX, pixelY);
            check(identifier == (bCovered ? 0u : Data::EntityIdBuffer::emptySentinel),
                  bCovered ? "entity pixels carry the instance index"
                           : "empty space carries emptySentinel");
        }
    // The overlay pass painted the same four pixels it wrote ids into.
    const unsigned int markedTexel = composite.CompositeTexels()[1 * 4 + 1];
    check(ChannelByte(markedTexel, 0) == 255 && ChannelByte(markedTexel, 2) == 0,
          "the entity mark color is composited over the terrain");
    check(markedTexel != composite.CompositeTexels()[0], "unmarked pixels are untouched by the mark");
}

// Pass ordering: clear -> one per enabled field layer -> overlay -> entity id.
void TestPassOrderingAndClear() {
    Ui::PreviewTestScene scene;
    Ui::BuildPreviewTestScene(scene);
    Ui::PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.fields,
                                   scene.instances, scene.entityIdentifiers);
    composite.Settings().previewResolution = 4;
    composite.Settings().bEntitiesEnabled = false;
    composite.Settings().clearColor[1] = 0.5f;                 // a clear color nothing overwrites
    composite.Compose();
    check(composite.ExecutedPassCount() == 3, "no layers -> clear + overlay + entity id");
    for (unsigned int texel : composite.CompositeTexels())
        check(ChannelByte(texel, 1) == 128 && ChannelByte(texel, 0) == 0,
              "with no layers the image is the clear color");
    check(scene.entityIdentifiers.Get(1, 1) == Data::EntityIdBuffer::emptySentinel,
          "entities disabled -> no id is written");
    Ui::ConfigurePreviewSettings(composite.Settings());
    composite.Settings().fieldLayers[1].bEnabled = false;       // disabled layers cost no pass
    composite.Compose();
    check(composite.ExecutedPassCount() == 5, "clear + two enabled layers + overlay + entity id");
}

void TestResolutionIsTweakable() {
    Ui::PreviewTestScene scene;
    Ui::BuildPreviewTestScene(scene);
    Ui::PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.fields,
                                   scene.instances, scene.entityIdentifiers);
    Ui::ConfigurePreviewSettings(composite.Settings());
    composite.Settings().previewResolution = 16;                // escalate on idle (ARCH §4.4)
    composite.Compose();
    check(composite.Resolution() == 16 && composite.CompositeTexels().size() == 256u
          && scene.entityIdentifiers.CellCount() == 256u,
          "the preview resolution is a tweakable and the id buffer follows it");
    composite.Settings().previewResolution = 0;                 // nonsense clamps, never divides by 0
    composite.Compose();
    check(composite.Resolution() == Ui::kMinimumPreviewResolution, "a nonsense resolution clamps");
}

// M5-0c: a slope layer colorizes the BAKED slope through the M4-2 LUT. Slope 1.5 over a [0,2]
// domain is 0.75 of the black->white ramp; re-baking must move the pixel, and the composite must
// leave the field it samples byte-identical (Mask is the only writer, §3.4.1).
void TestSlopeLayerSamplesTheBakedField() {
    Ui::PreviewTestScene scene;
    Ui::BuildPreviewTestScene(scene);
    scene.fields.slope.Fill(1.5f);                       // gradient magnitude, the pinned unit
    const Data::FloatField bakedSlope = scene.fields.slope;   // Mask's output, read-only here
    Ui::PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.fields,
                                   scene.instances, scene.entityIdentifiers);
    composite.Settings().previewResolution = 4;
    composite.Settings().bEntitiesEnabled = false;
    composite.Settings().gradientRamps.push_back(Ui::MakeBlackToWhiteRamp());
    composite.Settings().fieldLayers.push_back(
        Ui::MakeLayer(Ui::PreviewLayerKind::Slope, Ui::PreviewBlendMode::Replace, 0, 0.0f, 2.0f));
    composite.Compose();
    check(ChannelNear(composite.CompositeTexels()[0], 0, 0.75f)
          && ChannelNear(composite.CompositeTexels()[0], 2, 0.75f),
          "the slope layer colorizes the baked slope through the LUT");
    check(std::memcmp(scene.fields.slope.Data(), bakedSlope.Data(),
                      bakedSlope.CellCount() * sizeof(float)) == 0,
          "the composite leaves the slope field it samples byte-identical");
    scene.fields.slope.Fill(0.5f);                       // a different bake -> a different pixel
    composite.Compose();
    check(ChannelNear(composite.CompositeTexels()[0], 0, 0.25f), "the layer samples the live bake");
}

// STEP47: WorldToPreviewPixel / PreviewPixelToWorld are exact inverses once baked
// (PixelsPerPreviewCell() > 0), and PreviewPixelToWorld degrades to (0,0) rather than dividing by
// zero before the first bake — the same discipline ResolvePreviewPixel's bInsideImage requires.
void TestWorldPreviewPixelRoundTrip() {
    Ui::PreviewTestScene scene;
    Ui::BuildPreviewTestScene(scene);
    Ui::PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.fields,
                                   scene.instances, scene.entityIdentifiers);
    check(composite.PixelsPerPreviewCell() == 0.0f,
          "an un-composed composite has not baked a cell scale yet");
    const Ui::PreviewComposite::PreviewWorldPoint unbaked = composite.PreviewPixelToWorld(3.0f, 5.0f);
    check(unbaked.worldX == 0.0f && unbaked.worldZ == 0.0f,
          "picking on an unbaked composite answers (0,0) instead of dividing by zero");

    Ui::ConfigurePreviewSettings(composite.Settings());
    composite.Compose();
    check(composite.PixelsPerPreviewCell() > 0.0f, "a composed composite has a positive cell scale");
    const Ui::PreviewComposite::PreviewPixelPoint pixel = composite.WorldToPreviewPixel(2.0f, 3.0f);
    const Ui::PreviewComposite::PreviewWorldPoint world =
        composite.PreviewPixelToWorld(pixel.pixelX, pixel.pixelY);
    const float differenceX = world.worldX - 2.0f;
    const float differenceZ = world.worldZ - 3.0f;
    check((differenceX < 0.001f && differenceX > -0.001f)
       && (differenceZ < 0.001f && differenceZ > -0.001f),
          "world -> preview pixel -> world is an exact round trip once baked");
}

} // namespace

int main() {
    TestSpotCellColors();
    TestEntityIdentifierBuffer();
    TestPassOrderingAndClear();
    TestResolutionIsTweakable();
    TestSlopeLayerSamplesTheBakedField();
    TestWorldPreviewPixelRoundTrip();
    if (Ui::previewTestFailureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", Ui::previewTestFailureCount);
    return 1;
}
