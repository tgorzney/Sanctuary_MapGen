// PreviewComposite_UI_Test.cpp — acceptance test, part 1: the composited colors of a spot cell,
// the entity-id buffer, the pass ordering and the resolution tweakable.
//   cl /std:c++17 /EHsc PreviewComposite_UI.cpp PreviewComposite_Prepare_UI.cpp
//      PreviewComposite_Cpu_UI.cpp GradientLut_UI.cpp PreviewComposite_UI_Test.cpp
// Runs the composite's Cpu twin, so it needs no GL context; the Gpu twin's parity against these
// same numbers is PreviewComposite_Gpu_UI_Test.cpp, which does need one.
#include "PreviewComposite_TestScene_UI.h"

using namespace SanmapGen;

namespace {

using Ui::ChannelByte;
using Ui::ChannelNear;
using Ui::QuantizeChannel;
void check(bool bCondition, const char* label) { Ui::CheckPreviewExpectation(bCondition, label); }

// The expected spot-cell color, re-derived here from the scene's numbers alone: the height ramp
// at 0.25, then a half-weight splat of the stratum tint, then the constant flow ramp added at
// its own 0.4 alpha. Written out per layer, including the byte round-trip between passes.
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
    // The fields are uniform, so every pixel the entity does not cover matches the spot cell.
    check(composite.CompositeTexels()[3] == spotTexel && composite.CompositeTexels()[12] == spotTexel,
          "uniform baked fields composite uniformly");
}

// The 4x4 preview maps the instance at world (2, 2) to pixel centre (1.5, 1.5); a 0.9-pixel mark
// covers exactly the four middle pixels.
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
    check(composite.Resolution() == 16 && composite.CompositeTexels().size() == 256u,
          "the preview resolution is a tweakable, not a fixed full-resolution pass");
    check(scene.entityIdentifiers.CellCount() == 256u, "the id buffer follows the resolution");
    composite.Settings().previewResolution = 0;                 // nonsense clamps, never divides by 0
    composite.Compose();
    check(composite.Resolution() == Ui::kMinimumPreviewResolution, "a nonsense resolution clamps");
}

} // namespace

int main() {
    TestSpotCellColors();
    TestEntityIdentifierBuffer();
    TestPassOrderingAndClear();
    TestResolutionIsTweakable();
    if (Ui::previewTestFailureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", Ui::previewTestFailureCount);
    return 1;
}
