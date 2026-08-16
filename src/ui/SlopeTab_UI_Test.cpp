// SlopeTab_UI_Test.cpp — WO C1 acceptance for the Slope overlay tab: the shared overlay lookups,
// the degrees<->gradient unit conversion, and the domain mirror. All pure; no imgui frame, no
// window, no GL context.
// NOT YET REGISTERED IN CMake — WO C1 does not own CMakeLists.txt.
#include "SlopeTab_UI.h"
#include <cstdio>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

bool NearlyEqual(float left, float right, float tolerance) {
    const float difference = left - right;
    return (difference < 0.0f ? -difference : difference) <= tolerance;
}

PreviewCompositeSettings SettingsWithSlopeLayer() {
    PreviewCompositeSettings settings;
    PreviewFieldLayer heightLayer;
    heightLayer.kind = PreviewLayerKind::HeightRamp;
    PreviewFieldLayer slopeLayer;
    slopeLayer.kind = PreviewLayerKind::Slope;
    slopeLayer.gradientRampIndex = 0;
    settings.fieldLayers.push_back(heightLayer);
    settings.fieldLayers.push_back(slopeLayer);
    settings.gradientRamps.push_back(Params::GradientRamp());
    return settings;
}

void RunOverlayLookupChecks() {
    PreviewCompositeSettings settings = SettingsWithSlopeLayer();
    PreviewFieldLayer* const slopeLayer =
        PreviewFieldLayerOfKind(settings, PreviewLayerKind::Slope);
    Check(slopeLayer != nullptr && slopeLayer->kind == PreviewLayerKind::Slope,
          "the lookup finds the layer of the asked-for kind, not the first layer");
    Check(PreviewFieldLayerOfKind(settings, PreviewLayerKind::Water) == nullptr,
          "a kind the composite does not carry answers null, never a wrong layer");
    Check(PreviewRampOfFieldLayer(settings, *slopeLayer) == &settings.gradientRamps[0],
          "the ramp lookup resolves the layer's index");

    Check(IsPreviewOverlayShown(settings, PreviewLayerKind::Slope), "the overlay starts shown");
    slopeLayer->bEnabled = false;
    Check(!IsPreviewOverlayShown(settings, PreviewLayerKind::Slope), "and the tick reports through");
    Check(!IsPreviewOverlayShown(settings, PreviewLayerKind::Water),
          "a missing layer is not 'shown'");

    slopeLayer->gradientRampIndex = 7;                 // a ramp list that shrank under the recipe
    Check(PreviewRampOfFieldLayer(settings, *slopeLayer) == nullptr,
          "a dangling ramp index resolves to null rather than reading off the end");
    slopeLayer->gradientRampIndex = -1;
    Check(PreviewRampOfFieldLayer(settings, *slopeLayer) == nullptr, "so does 'no ramp'");
}

// The tab edits DEGREES; the baked field is gradient magnitude (rise/run). Constitution §6: the
// vertical asymptote of tan() must be fenced, not hit.
void RunUnitConversionChecks() {
    Check(SlopeGradientFromDegrees(0.0f) == 0.0f, "flat ground is zero gradient");
    Check(NearlyEqual(SlopeGradientFromDegrees(45.0f), 1.0f, 1.0e-5f),
          "45 degrees is a gradient of one");
    Check(SlopeGradientFromDegrees(89.0f) > 50.0f, "the steepest offered angle is very steep");
    Check(SlopeGradientFromDegrees(90.0f) == SlopeGradientFromDegrees(kSlopeDegreeCeiling),
          "vertical is clamped to the ceiling rather than driven through tan()'s asymptote");
    Check(SlopeGradientFromDegrees(-30.0f) == 0.0f, "a negative angle clamps to flat");

    Check(NearlyEqual(SlopeDegreesFromGradient(1.0f), 45.0f, 1.0e-3f), "and the inverse agrees");
    Check(SlopeDegreesFromGradient(-5.0f) == 0.0f, "a negative gradient reads as flat");
    for (float degrees = 0.0f; degrees <= 85.0f; degrees += 5.0f)
        Check(NearlyEqual(SlopeDegreesFromGradient(SlopeGradientFromDegrees(degrees)), degrees, 1.0e-2f),
              "degrees round-trip through the field's unit");
}

void RunDomainMirrorChecks() {
    PreviewCompositeSettings settings = SettingsWithSlopeLayer();
    PreviewFieldLayer& layer = *PreviewFieldLayerOfKind(settings, PreviewLayerKind::Slope);
    SlopeTabState state;

    // The tab's default band (0-45 degrees) IS the layer's default 0..1 gradient domain, so the
    // first store is a genuine no-op — which is the contract: a store that moves nothing reports
    // nothing, and so cannot trip a preview refresh on a frame the user did not touch anything.
    Check(!StoreSlopeTabValues(state, layer), "an unchanged store reports nothing moved");
    Check(NearlyEqual(layer.domainMinimum, 0.0f, 1.0e-6f)
          && NearlyEqual(layer.domainMaximum, 1.0f, 1.0e-5f),
          "and 0-45 degrees is exactly the 0..1 gradient domain");

    state.maximumDegrees = 60.0f;
    Check(StoreSlopeTabValues(state, layer), "a moved bound reports the layer moved");
    Check(NearlyEqual(layer.domainMaximum, 1.7320508f, 1.0e-4f),
          "60 degrees reaches the layer as a gradient of sqrt(3)");

    LoadSlopeTabValues(layer, state);
    Check(NearlyEqual(state.maximumDegrees, 60.0f, 1.0e-2f), "the mirror reads the domain back");

    // An inverted pair would paint the ramp backwards, so the store orders it.
    state.minimumDegrees = 70.0f;
    state.maximumDegrees = 20.0f;
    StoreSlopeTabValues(state, layer);
    Check(layer.domainMinimum < layer.domainMaximum,
          "a crossed pair is ordered on the way into the layer");
    Check(NearlyEqual(SlopeDegreesFromGradient(layer.domainMinimum), 20.0f, 1.0e-2f),
          "keeping both values the user set");
}

} // namespace

int main() {
    RunOverlayLookupChecks();
    RunUnitConversionChecks();
    RunDomainMirrorChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
